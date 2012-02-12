/*
 * ofxOpenNIHandsTracker.cpp
 *
 *  Created on: 12/10/2011
 *      Author: arturo
 */

#include "ofxOpenNIHandsTracker.h"

#include "ofGraphics.h"

#include "ofxOpenNIUtils.h"
#include "ofxOpenNI.h"


string ofxOpenNIHandsTracker::LOG_NAME = "ofxOpenNIHandsTracker";

#define MAX_NUMBER_USERS 20

// CALLBACKS
// =============================================================

//--------------------------------------------------------------

void XN_CALLBACK_TYPE
ofxOpenNIHandsTracker::Hand_Create(xn::HandsGenerator& userGenerator, 
                                   XnUserID nID, 
                                   const XnPoint3D*	position, 
                                   XnFloat timestamp, 
                                   void* pCookie)
{
  ofLogVerbose(LOG_NAME) << "New Hand" << nID;
}

//--------------------------------------------------------------
void XN_CALLBACK_TYPE
ofxOpenNIHandsTracker::Hand_Update(xn::HandsGenerator& userGenerator, 
                                   XnUserID nID, 
                                   const XnPoint3D*	position, 
                                   XnFloat timestamp, 
                                   void* pCookie)
{
	ofLogVerbose(LOG_NAME) << "Update Hand" << nID;
	ofxOpenNIHandsTracker* tracker = static_cast<ofxOpenNIHandsTracker*>(pCookie);
}

//--------------------------------------------------------------
void XN_CALLBACK_TYPE
ofxOpenNIHandsTracker::Hand_Destroy(xn::HandsGenerator& userGenerator,
                                    XnUserID nID,
                                    XnFloat timestamp,
                                    void* pCookie)
{
	ofLogVerbose(LOG_NAME) << "Lost Hand" << nID;
	ofxOpenNIHandsTracker* tracker = static_cast<ofxOpenNIHandsTracker*>(pCookie);
  
}

//--------------------------------------------------------------
void XN_CALLBACK_TYPE
ofxOpenNIHandsTracker::Gesture_Recognized(xn::GestureGenerator& userGenerator,
                                          const XnChar* strGesture,
                                          const XnPoint3D* pIDPosition,
                                          const XnPoint3D* pEndPosition,
                                          void* pCookie)
{
	ofxOpenNIHandsTracker* tracker = static_cast<ofxOpenNIHandsTracker*>(pCookie);
  xn::GestureGenerator& gesture_generator = userGenerator;
  xn::HandsGenerator& hands_generator = tracker->openNI->getHandsGenerator();

  printf("Gesture recognized: %s\n", strGesture);
  userGenerator.RemoveGesture(strGesture);
  
  hands_generator.StartTracking(*pEndPosition);
}
                                         
//--------------------------------------------------------------
void XN_CALLBACK_TYPE
ofxOpenNIHandsTracker::Gesture_Process(xn::GestureGenerator& userGenerator,
                                       const XnChar* strGesture,
                                       const XnPoint3D* pPosition,
                                       XnFloat fProgress,
                                       void* pCookie)
{
	ofxOpenNIHandsTracker* tracker = static_cast<ofxOpenNIHandsTracker*>(pCookie);
  xn::GestureGenerator& gesture_generator = userGenerator;
  xn::HandsGenerator& hands_generator = tracker->openNI->getHandsGenerator();

  printf("Gesture process: %s\n", strGesture);
}
                                         
//--------------------------------------------------------------
ofxOpenNIHandsTracker::ofxOpenNIHandsTracker()
: openNI(NULL)
{}

//--------------------------------------------------------------
bool ofxOpenNIHandsTracker::setup(ofxOpenNI & _openNI)
{
  XnStatus result;

	openNI = &_openNI;
  xn::HandsGenerator& hands_generator = openNI->getHandsGenerator();
  xn::GestureGenerator& gesture_generator = openNI->getGestureGenerator();

	// register user callbacks
	XnCallbackHandle gesture_cb_handle;
	XnCallbackHandle hands_cb_handle;
  
  result = gesture_generator.RegisterGestureCallbacks(Gesture_Recognized,
                                                      Gesture_Process,
                                                      this,
                                                      gesture_cb_handle);
	SHOW_RC(result, "RegisterGestureCallbacks");

  result = hands_generator.RegisterHandCallbacks(Hand_Create,
                                                 Hand_Update,
                                                 Hand_Destroy,
                                                 this,
                                                 hands_cb_handle);
	SHOW_RC(result, "RegisterHandCallbacks");

	// needs this to allow skeleton tracking when using pre-recorded .oni or nodes init'd by code (as opposed to xml)
	// as otherwise the image/depth nodes play but are not generating callbacks
	result = openNI->getXnContext().StartGeneratingAll();
	SHOW_RC(result, "StartGenerating");

  if(gesture_generator.AddGesture("Click", NULL) != XN_STATUS_OK)
    std::cout << "Unable to add gesture, bailing." << std::endl;

  if(gesture_generator.AddGesture("Wave", NULL) != XN_STATUS_OK)
    std::cout << "Unable to add gesture, bailing." << std::endl;
  
	return (result == XN_STATUS_OK);
}

//--------------------------------------------------------------
void ofxOpenNIHandsTracker::update()
{
  /*
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
  */
}
