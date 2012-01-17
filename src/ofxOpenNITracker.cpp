/*
 * ofxOpenNITracker.cpp
 *
 *  Created on: 12/10/2011
 *      Author: arturo
 */

#include "ofxOpenNITracker.h"

#include "ofGraphics.h"

#include "ofxOpenNIUtils.h"
#include "ofxOpenNI.h"


string ofxOpenNITracker::LOG_NAME = "ofxOpenNITracker";

#define MAX_NUMBER_USERS 20

// CALLBACKS
// =============================================================================

//----------------------------------------
void XN_CALLBACK_TYPE ofxOpenNITracker::User_NewUser(xn::UserGenerator& rGenerator, XnUserID nID, void* pCookie){
	ofLogVerbose(LOG_NAME) << "New User" << nID;

	ofxOpenNITracker* tracker = static_cast<ofxOpenNITracker*>(pCookie);
  tracker->setUserState(nID, ofxOpenNIUser::Found);
  if(tracker->loadCalibrationData(nID))
    tracker->startTracking(nID);
	else if(tracker->needsPoseForCalibration())
		tracker->startPoseDetection(nID);
	else
    tracker->startTracking(nID);
//		tracker->requestCalibration(nID);
}

//----------------------------------------
void XN_CALLBACK_TYPE ofxOpenNITracker::User_LostUser(xn::UserGenerator& rGenerator, XnUserID nID, void* pCookie){
	ofLogVerbose(LOG_NAME) << "Lost user" << nID;

	ofxOpenNITracker* tracker = static_cast<ofxOpenNITracker*>(pCookie);
	rGenerator.GetSkeletonCap().Reset(nID);
  tracker->setUserState(nID, ofxOpenNIUser::Lost);
}

//----------------------------------------
void XN_CALLBACK_TYPE ofxOpenNITracker::UserPose_PoseDetected(xn::PoseDetectionCapability& rCapability, const XnChar* strPose, XnUserID nID, void* pCookie){
	ofxOpenNITracker* tracker = static_cast<ofxOpenNITracker*>(pCookie);
	ofLogVerbose(LOG_NAME) << "Pose" << strPose << "detected for user" << nID;
	tracker->requestCalibration(nID);
	tracker->stopPoseDetection(nID);
}


//----------------------------------------
void XN_CALLBACK_TYPE ofxOpenNITracker::UserCalibration_CalibrationStart(xn::SkeletonCapability& capability, XnUserID nID, void* pCookie){
	ofLogVerbose(LOG_NAME) << "Calibration started for user" << nID;
}


//----------------------------------------
void XN_CALLBACK_TYPE ofxOpenNITracker::UserCalibration_CalibrationEnd(xn::SkeletonCapability& rCapability, XnUserID nID, XnCalibrationStatus bSuccess, void* pCookie){
	ofxOpenNITracker* tracker = static_cast<ofxOpenNITracker*>(pCookie);
	if(bSuccess == XN_CALIBRATION_STATUS_OK) {
		ofLogVerbose(LOG_NAME) << "+++++++++++++++++++++++ Successfully tracked user:" << nID;
    tracker->saveCalibrationData(nID, false);
		tracker->startTracking(nID);
	} else {
		if(tracker->needsPoseForCalibration())
			tracker->startPoseDetection(nID);
		else
			tracker->requestCalibration(nID);
	}
}

//----------------------------------------
ofxOpenNITracker::ofxOpenNITracker(){
	openNI = NULL;
	smoothing_factor = 1;
	usePointClouds = false;
	useMaskPixels = false;
}

//----------------------------------------
bool ofxOpenNITracker::setup(ofxOpenNI & _openNI){
	openNI = &_openNI;

	if (!openNI->getDepthGenerator().IsValid()){
		ofLogError(LOG_NAME) << "no depth generator present";
		return false;
	}

	XnStatus result = XN_STATUS_OK;

	// get map_mode so we can setup width and height vars from depth gen size
	XnMapOutputMode map_mode;
	openNI->getDepthGenerator().GetMapOutputMode(map_mode);

	width = map_mode.nXRes;
	height = map_mode.nYRes;

	// set update mask pixels default to false
	//useMaskPixels = false;

	// setup mask pixels array TODO: clean this up on closing or dtor
	//including 0 as all users
	//for (int user = 0; user <= MAX_NUMBER_USERS; user++) {
	//	maskPixels[user] = new unsigned char[width * height];
	//}

	// set update cloud points default to false
	/*cloudPoints.resize(MAX_NUMBER_USERS);

	// setup cloud points array TODO: clean this up on closing or dtor
	// including 0 as all users
	for (int user = 0; user <= MAX_NUMBER_USERS; user++) {
		cloudPoints[user].getVertices().resize(width * height);
		cloudPoints[user].getColors().resize(width * height);
		cloudPoints[user].setMode(OF_PRIMITIVE_POINTS);
	}*/

	// if one doesn't exist then create user generator.
//	result = user_generator.Create(openNI->getXnContext());
//	SHOW_RC(result, "Create user generator");

	if (result != XN_STATUS_OK) return false;

  xn::UserGenerator& user_generator = openNI->getUserGenerator();
	// register user callbacks
	XnCallbackHandle user_cb_handle;
	user_generator.RegisterUserCallbacks(
		 User_NewUser
		,User_LostUser
		,this
		,user_cb_handle
	);

	XnCallbackHandle calibration_cb_handle;
	user_generator.GetSkeletonCap().RegisterToCalibrationStart(
		 UserCalibration_CalibrationStart
		,this
		,calibration_cb_handle
	);

	user_generator.GetSkeletonCap().RegisterToCalibrationComplete(
			UserCalibration_CalibrationEnd
			,this
			,calibration_cb_handle
	);

	// check if we need to pose for calibration
	if(user_generator.GetSkeletonCap().NeedPoseForCalibration()) {

		needs_pose = true;

		if(!user_generator.IsCapabilitySupported(XN_CAPABILITY_POSE_DETECTION)) {
			ofLogError(LOG_NAME) << "Pose required, but not supported";
			return false;
		}

		XnCallbackHandle user_pose_cb_handle;

		user_generator.GetPoseDetectionCap().RegisterToPoseDetected(
			 UserPose_PoseDetected
			,this
			,user_pose_cb_handle
		);

		user_generator.GetSkeletonCap().GetCalibrationPose(calibration_pose);

	}

	user_generator.GetSkeletonCap().SetSkeletonProfile(XN_SKEL_PROFILE_ALL);

	// needs this to allow skeleton tracking when using pre-recorded .oni or nodes init'd by code (as opposed to xml)
	// as otherwise the image/depth nodes play but are not generating callbacks
	//if (context->isUsingRecording()) {
	result = openNI->getXnContext().StartGeneratingAll();
	SHOW_RC(result, "StartGenerating");
	if (result != XN_STATUS_OK) return false;

	return true;
}

//----------------------------------------
void ofxOpenNITracker::setUseMaskPixels(bool b){
	useMaskPixels = b;
}

//----------------------------------------
void ofxOpenNITracker::setUsePointClouds(bool b){
	usePointClouds = b;
}

//----------------------------------------
void ofxOpenNITracker::setSmoothing(float smooth){
	if (smooth > 0.0f && smooth < 1.0f) {
		smoothing_factor = smooth;
		if (openNI->getUserGenerator().IsValid()) {
			openNI->getUserGenerator().GetSkeletonCap().SetSmoothing(smooth);
		}
	}
}

//----------------------------------------
float ofxOpenNITracker::getSmoothing(){
	return smoothing_factor;
}

//----------------------------------------
void ofxOpenNITracker::update(){
	vector<XnUserID> userIds(MAX_NUMBER_USERS);
  XnUInt16 nUsers = userIds.size();
  xn::UserGenerator& user_generator = openNI->getUserGenerator();
	user_generator.GetUsers(&userIds[0], nUsers);
  userIds.resize(nUsers);

  unsigned int stateHasMask = (ofxOpenNIUser::Found |
                               ofxOpenNIUser::NeedsPose |
                               ofxOpenNIUser::Calibrating |
                               ofxOpenNIUser::Tracking);

	for(int i = 0; i < nUsers; ++i) {
    unsigned int nID = userIds[i];
    ofxOpenNIUser& user = users[nID];

		if((user.state & stateHasMask)) {
			user.id = nID;
			XnPoint3D center;
			user_generator.GetCoM(nID, center);
			user.center = toOf(center);

			if (usePointClouds) updatePointClouds(user);
			if (useMaskPixels) updateUserPixels(user);
    }

		if(user.state == ofxOpenNIUser::Tracking) {
			for(int j=0;j<ofxOpenNIUser::NumLimbs;j++){
				XnSkeletonJointPosition a,b;
				user_generator.GetSkeletonCap().GetSkeletonJointPosition(nID, user.limbs[j].start_joint, a);
				user_generator.GetSkeletonCap().GetSkeletonJointPosition(nID, user.limbs[j].end_joint, b);
				user_generator.GetSkeletonCap().GetSkeletonJointOrientation(nID, user.limbs[j].start_joint, user.limbs[j].orientation);
				if(a.fConfidence < 0.3f || b.fConfidence < 0.3f) {
					user.limbs[j].found = false;
					continue;
				}

				user.limbs[j].found = true;
				user.limbs[j].begin = openNI->worldToProjective(a.position);
				user.limbs[j].end = openNI->worldToProjective(b.position);
				user.limbs[j].worldBegin = toOf(a.position);
				user.limbs[j].worldEnd = toOf(b.position);
			}
		}
	}

	map<XnUserID, ofxOpenNIUser>::iterator it;
	for(it=users.begin();it!=users.end();it++){
		if(std::find(userIds.begin(), userIds.end(), it->first)==userIds.end()){
			users.erase(it);
		}
	}
}

//----------------------------------------
void ofxOpenNITracker::updatePointClouds(ofxOpenNIUser & user) {

	const XnRGB24Pixel*		pColor;
	const XnDepthPixel*		pDepth = openNI->getDepthMetaData().Data();

	bool hasImageGenerator = openNI->getImageGenerator().IsValid();

	if (hasImageGenerator) {
		pColor = openNI->getImageMetaData().RGB24Data();
	}

	xn::SceneMetaData smd;
	unsigned short *userPix;

	if (openNI->getUserGenerator().GetUserPixels(user.id, smd) == XN_STATUS_OK) {
		userPix = (unsigned short*)smd.Data();
	}

	int step = 1;
	int nIndex = 0;

	user.pointCloud.getVertices().clear();
	user.pointCloud.getColors().clear();
	user.pointCloud.setMode(OF_PRIMITIVE_POINTS);

	for (int nY = 0; nY < height; nY += step) {
		for (int nX = 0; nX < width; nX += step, nIndex++) {
			if (userPix[nIndex] == user.id) {
				user.pointCloud.addVertex(ofPoint( nX,nY,pDepth[nIndex] ));
				ofColor color;
				if(hasImageGenerator){
					user.pointCloud.addColor(ofColor(pColor[nIndex].nRed,pColor[nIndex].nGreen,pColor[nIndex].nBlue));
				}else{
					user.pointCloud.addColor(ofFloatColor(1,1,1));
				}
			}
		}
	}
}

//----------------------------------------
void ofxOpenNITracker::updateUserPixels(ofxOpenNIUser & user){
	xn::SceneMetaData smd;
	unsigned short *userPix;

	if (openNI->getUserGenerator().GetUserPixels(user.id, smd) == XN_STATUS_OK) { //	GetUserPixels is supposed to take a user ID number,
		userPix = (unsigned short*)smd.Data();					//  but you get the same data no matter what you pass.
	}															//	userPix actually contains an array where each value
																//  corresponds to the user being tracked.
																//  Ie.,	if userPix[i] == 0 then it's not being tracked -> it's the background!
																//			if userPix[i] > 0  then the pixel belongs to the user who's value IS userPix[i]
																//  // (many thanks to ascorbin who's code made this apparent to me)

	user.maskPixels.allocate(width,height,OF_IMAGE_GRAYSCALE);

	for (int i =0 ; i < width * height; i++) {
		if (userPix[i] == user.id) {
			user.maskPixels[i] = 255;
		} else {
			user.maskPixels[i] = 0;
		}
	}
}

//----------------------------------------
void ofxOpenNITracker::draw(){
	ofPushStyle();
  // draw all the users
	map<XnUserID, ofxOpenNIUser>::iterator it;
	for(it=users.begin();it!=users.end();it++){
    it->second.draw();
  }
	ofPopStyle();
}

//----------------------------------------
void ofxOpenNITracker::startPoseDetection(XnUserID nID) {
	ofLogVerbose(LOG_NAME) << "Start pose detection for user" << nID;
	openNI->getUserGenerator().GetPoseDetectionCap().StartPoseDetection(calibration_pose, nID);
  setUserState(nID, ofxOpenNIUser::NeedsPose);
}


//----------------------------------------
void ofxOpenNITracker::stopPoseDetection(XnUserID nID) {
	openNI->getUserGenerator().GetPoseDetectionCap().StopPoseDetection(nID);
}


//----------------------------------------
void ofxOpenNITracker::requestCalibration(XnUserID nID) {
	ofLogVerbose(LOG_NAME) << "Calibration requested for user" << nID;
	openNI->getUserGenerator().GetSkeletonCap().RequestCalibration(nID, TRUE);
  setUserState(nID, ofxOpenNIUser::Calibrating);
}

//----------------------------------------
void ofxOpenNITracker::startTracking(XnUserID nID) {
	ofLogVerbose(LOG_NAME) << "Start tracking user" << nID;
	openNI->getUserGenerator().GetSkeletonCap().StartTracking(nID);
  setUserState(nID, ofxOpenNIUser::Tracking);
}

//----------------------------------------
bool ofxOpenNITracker::needsPoseForCalibration() {
  return false;
//	return needs_pose;
}

//----------------------------------------
unsigned int ofxOpenNITracker::getNumberOfUsers(unsigned int stateMask){
  unsigned int nUsers = 0;
	map<XnUserID, ofxOpenNIUser>::iterator it;
	for(it=users.begin();it!=users.end();it++){
    if (it->second.state & stateMask)
      nUsers++;
  }
	return nUsers;
}

//----------------------------------------
ofxOpenNIUser* ofxOpenNITracker::getUserByIndex(int nUserNum, unsigned int stateMask){
  unsigned int nValidUsers=0;
  map<XnUserID, ofxOpenNIUser>::iterator it;
	for(it=users.begin();it!=users.end();it++){
    if (it->second.state & stateMask)
      nValidUsers++;
    if ((nValidUsers-1)==nUserNum)
      return &it->second;
  }

	return NULL;
}

//----------------------------------------
ofxOpenNIUser* ofxOpenNITracker::getUserByID(int nID, unsigned int stateMask){
  if (users.find(nID) != users.end())
    if (users[nID].state & stateMask)
      return (&users[nID]);

  return NULL;
}

//----------------------------------------
float ofxOpenNITracker::getWidth(){
	return width;
}

//----------------------------------------
float ofxOpenNITracker::getHeight(){
	return height;
}

//----------------------------------------
bool ofxOpenNITracker::saveCalibrationData(unsigned int nID, bool overwrite){
  if (!overwrite && ofFile::doesFileExist("openni/calibration.bin"))
    return false;

  if (openNI->getUserGenerator().GetSkeletonCap().IsCalibrated(nID)){
//    std::string userFilePath = ofToDataPath("user_"+ofToString(nID)+".bin");
    std::string userFilePath = ofToDataPath("openni/calibration.bin");
    XnStatus nRetVal = openNI->getUserGenerator().GetSkeletonCap().SaveCalibrationDataToFile(nID, userFilePath.c_str());
    if (nRetVal != XN_STATUS_OK)
      std::cout << "Save calibration to file failed: " << xnGetStatusString(nRetVal) << std::endl;
    else
      std::cout << "Saved calibration to file" << std::endl;

    return (nRetVal != XN_STATUS_OK);
  }
  return false;
}

//----------------------------------------
bool ofxOpenNITracker::loadCalibrationData(unsigned int nID){
  if (ofFile::doesFileExist("openni/calibration.bin")) {
    std::cout << "Loaded calibration from file" << std::endl;
    openNI->getUserGenerator().GetSkeletonCap().LoadCalibrationDataFromFile(nID, ofToDataPath("openni/calibration.bin").c_str());
    return true;
  }
  return false;
}

//----------------------------------------
ofxOpenNIUser::TrackingState ofxOpenNITracker::getUserState(unsigned int nID)
{
  if (users.find(nID) != users.end())
    return users[nID].state;

  return ofxOpenNIUser::Invalid;
}

//----------------------------------------
void ofxOpenNITracker::setUserState(unsigned int nID, ofxOpenNIUser::TrackingState userState)
{
  if (users.find(nID) != users.end())
  {
    users[nID].state = userState;
    users[nID].stateChangedTimestamp = ofGetSystemTime();
  }
}
