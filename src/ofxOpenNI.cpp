/*
 * ofxOpenNI.cpp
 *
 *  Created on: 11/10/2011
 *      Author: arturo
 */


#include "ofxOpenNI.h"

#include <XnLog.h>

#include "ofxOpenNIUtils.h"

#include "ofLog.h"


string ofxOpenNI::LOG_NAME = "ofxOpenNI";

using namespace xn;

//--------------------------------------------------------------
ofxOpenNI::ofxOpenNI()
{
	g_bUseDepth   = true;
	g_bUseImage   = true;
	g_bUseIR      = true;
  g_bUseUser    = true;
  g_bUseHands   = true;
  g_bUseGesture = true;
	g_bUseAudio   = true;
	g_bUsePlayer  = true;

  g_bHasDepth   = false;
	g_bHasImage   = false;
	g_bHasIR      = false;
  g_bHasHands   = false;
  g_bHasGesture = false;
  g_bHasUser    = false;
	g_bHasAudio   = false;
	g_bHasPlayer  = false;

	g_pPrimary = NULL;

	useTexture = true;
	bGeneratePCColors = false;
	bGeneratePCTexCoords = false;
}

//--------------------------------------------------------------
void
ofxOpenNI::initConstants()
{
	// Primary Streams
	int nIndex = 0;

	g_PrimaryStream.pValues[nIndex++] = "Any";
	g_PrimaryStream.pValues[nIndex++] = xnProductionNodeTypeToString(XN_NODE_TYPE_DEPTH);
	g_PrimaryStream.pValues[nIndex++] = xnProductionNodeTypeToString(XN_NODE_TYPE_IMAGE);
	g_PrimaryStream.pValues[nIndex++] = xnProductionNodeTypeToString(XN_NODE_TYPE_IR);
	g_PrimaryStream.pValues[nIndex++] = xnProductionNodeTypeToString(XN_NODE_TYPE_AUDIO);

	g_PrimaryStream.nValuesCount = nIndex;

	// Registration
	nIndex = 0;

	g_Registration.pValues[nIndex++] = FALSE;
	g_Registration.pValueToName[FALSE] = "Off";

	g_Registration.pValues[nIndex++] = TRUE;
	g_Registration.pValueToName[TRUE] = "Depth -> Image";

	g_Registration.nValuesCount = nIndex;

	// Resolutions
	nIndex = 0;

	g_Resolution.pValues[nIndex++] = XN_RES_QVGA;
	g_Resolution.pValueToName[XN_RES_QVGA] = Resolution(XN_RES_QVGA).GetName();

	g_Resolution.pValues[nIndex++] = XN_RES_VGA;
	g_Resolution.pValueToName[XN_RES_VGA] = Resolution(XN_RES_VGA).GetName();

	g_Resolution.pValues[nIndex++] = XN_RES_SXGA;
	g_Resolution.pValueToName[XN_RES_SXGA] = Resolution(XN_RES_SXGA).GetName();

	g_Resolution.pValues[nIndex++] = XN_RES_UXGA;
	g_Resolution.pValueToName[XN_RES_UXGA] = Resolution(XN_RES_UXGA).GetName();

	g_Resolution.nValuesCount = nIndex;

}

//--------------------------------------------------------------
void
ofxOpenNI::allocateDepthBuffers()
{
	if (g_bHasDepth && g_bUseDepth)
  {
    max_depth       = g_Depth.GetDeviceMaxDepth();
    depthPixels[0].allocate(getWidth(),getHeight(),OF_PIXELS_MONO);
    depthPixels[1].allocate(getWidth(),getHeight(),OF_PIXELS_MONO);
    currentDepthPixels = &depthPixels[0];
    backDepthPixels = &depthPixels[1];
    if (useTexture)
      depthTexture.allocate(getWidth(),getHeight(),GL_LUMINANCE16);
  }
}

//--------------------------------------------------------------
void
ofxOpenNI::allocateRGBBuffers()
{
	if (g_bHasImage && g_bUseImage)
  {
		rgbPixels[0].allocate(getWidth(),getHeight(),OF_IMAGE_COLOR);
		rgbPixels[1].allocate(getWidth(),getHeight(),OF_IMAGE_COLOR);
		currentRGBPixels = &rgbPixels[0];
		backRGBPixels = &rgbPixels[1];
		if (useTexture)
      rgbTexture.allocate(getWidth(),getHeight(),GL_RGB);
	}
}

//--------------------------------------------------------------
void
ofxOpenNI::allocateIRBuffers()
{
	if (g_bHasIR && g_bUseIR)
  {
		irPixels[0].allocate(getWidth(),getHeight(),OF_IMAGE_GRAYSCALE);
		irPixels[1].allocate(getWidth(),getHeight(),OF_IMAGE_GRAYSCALE);
		currentIRPixels = &irPixels[0];
		backIRPixels = &irPixels[1];
		if (useTexture)
      irTexture.allocate(getWidth(),getHeight(),GL_LUMINANCE8);
	}
}

//--------------------------------------------------------------
void
ofxOpenNI::allocateUserMaskBuffers()
{
	if (g_bHasUser && g_bUseUser)
  {
    userMaskPixels[0].allocate(getWidth(),getHeight(),OF_PIXELS_MONO);
    userMaskPixels[1].allocate(getWidth(),getHeight(),OF_PIXELS_MONO);
    currentUserMaskPixels = &userMaskPixels[0];
    backUserMaskPixels = &userMaskPixels[1];
    if (useTexture)
      userMaskTexture.allocate(getWidth(),getHeight(),GL_LUMINANCE16);
  }
}

//--------------------------------------------------------------
void XN_CALLBACK_TYPE
ofxOpenNI::onErrorStateChanged(XnStatus errorState, void* pCookie)
{

	if (errorState != XN_STATUS_OK)
  {
		ofLogError(LOG_NAME) << xnGetStatusString(errorState);
		//setErrorState(xnGetStatusString(errorState));
	}else{
		//setErrorState(NULL);
	}
}

//--------------------------------------------------------------
void
ofxOpenNI::openCommon()
{
	XnStatus nRetVal = XN_STATUS_OK;

	g_bHasDepth   = false;
	g_bHasImage   = false;
	g_bHasIR      = false;
  g_bHasUser    = false;
  g_bHasHands   = false;
  g_bHasGesture = false;
	g_bHasAudio   = false;
	g_bHasPlayer  = false;
	
	NodeInfoList list;
	nRetVal = g_Context.EnumerateExistingNodes(list);
	if (nRetVal == XN_STATUS_OK)
	{
		for (NodeInfoList::Iterator it = list.Begin(); it != list.End(); ++it)
		{
			switch ((*it).GetDescription().Type)
			{
        case XN_NODE_TYPE_DEVICE:
          ofLogVerbose(LOG_NAME) << "Creating device";
          (*it).GetInstance(g_Device);
          break;
        case XN_NODE_TYPE_DEPTH:
          ofLogVerbose(LOG_NAME) << "Creating depth generator";
          g_bHasDepth = true;
          (*it).GetInstance(g_Depth);
          break;
        case XN_NODE_TYPE_IMAGE:
          ofLogVerbose(LOG_NAME) << "Creating image generator";
          g_bHasImage = true;
          (*it).GetInstance(g_Image);
          break;
        case XN_NODE_TYPE_IR:
          ofLogVerbose(LOG_NAME) << "Creating ir generator";
          g_bHasIR = true;
          (*it).GetInstance(g_IR);
          break;
        case XN_NODE_TYPE_USER:
          ofLogVerbose(LOG_NAME) << "Creating user generator";
          g_bHasUser = true;
          (*it).GetInstance(g_User);
          break;
        case XN_NODE_TYPE_HANDS:
          ofLogVerbose(LOG_NAME) << "Creating hands generator";
          g_bHasHands = true;
          (*it).GetInstance(g_Hands);
          break;
        case XN_NODE_TYPE_GESTURE:
          ofLogVerbose(LOG_NAME) << "Creating gesture generator";
          g_bHasGesture = true;
          (*it).GetInstance(g_Gesture);
          break;
        case XN_NODE_TYPE_AUDIO:
          ofLogVerbose(LOG_NAME) << "Creating audio generator";
          g_bHasAudio = true;
          (*it).GetInstance(g_Audio);
          break;
        case XN_NODE_TYPE_PLAYER:
          ofLogVerbose(LOG_NAME) << "Creating player";
          g_bHasPlayer = true;
          (*it).GetInstance(g_Player);
          break;
			}
		}
	}

	XnCallbackHandle hDummy;
	g_Context.RegisterToErrorStateChange(onErrorStateChanged, this, hDummy);

	initConstants();
	allocateDepthBuffers();
	allocateRGBBuffers();
	allocateIRBuffers();
	allocateUserMaskBuffers();

	pointCloud.setMode(OF_PRIMITIVE_POINTS);
	pointCloud.getVertices().resize(getWidth()*getHeight());

	int i=0;
	for (int y=0;y<getHeight();y++)
  {
		for (int x=0;x<getWidth();x++)
    {
			pointCloud.getVertices()[i].set(float(x)/getWidth(),float(y)/getHeight(),0);
			i++;
		}
	}

	isPointCloudValid = false;

	readFrame();
}

//--------------------------------------------------------------
void
ofxOpenNI::addLicense(string sVendor, string sKey)
{
	XnLicense license = {0};
	XnStatus status = XN_STATUS_OK;

	status = xnOSStrNCopy(license.strVendor, sVendor.c_str(),sVendor.size(), sizeof(license.strVendor));
	if (status != XN_STATUS_OK)
  {
		ofLogError(LOG_NAME) << "ofxOpenNIContext error creating license (vendor)";
		return;
	}

	status = xnOSStrNCopy(license.strKey, sKey.c_str(), sKey.size(), sizeof(license.strKey));
	if (status != XN_STATUS_OK)
  {
		ofLogError(LOG_NAME) << "ofxOpenNIContext error creating license (key)";
		return;
	}

	status = g_Context.AddLicense(license);
	SHOW_RC(status, "AddLicense");

	xnPrintRegisteredLicenses();

}

//--------------------------------------------------------------
bool
ofxOpenNI::setupFromXML(string xml, bool _threaded)
{
	threaded = _threaded;
	XnStatus nRetVal = XN_STATUS_OK;
	EnumerationErrors errors;

	nRetVal = g_Context.InitFromXmlFile(ofToDataPath(xml).c_str(), g_scriptNode, &errors);
	SHOW_RC(nRetVal, "setup from XML");

	if (nRetVal!=XN_STATUS_OK)
    return false;

	openCommon();

	if (threaded)
    startThread(true,false);

	return true;
}

//--------------------------------------------------------------
bool
ofxOpenNI::setupFromRecording(string recording, bool _threaded)
{
	threaded = _threaded;
	xnLogInitFromXmlFile(ofToDataPath("openni/config/ofxopenni_config.xml").c_str());

	XnStatus nRetVal = g_Context.Init();
	if (nRetVal!=XN_STATUS_OK) return false;

	addLicense("PrimeSense", "0KOIk2JeIBYClPWVnMoRKn5cdY4=");

	nRetVal = g_Context.OpenFileRecording(ofToDataPath(recording).c_str(), g_Player);
	SHOW_RC(nRetVal, "setup from recording");

	if (nRetVal!=XN_STATUS_OK)
    return false;

	openCommon();

	if (threaded)
    startThread(true,false);

	return true;
}

//--------------------------------------------------------------
void
ofxOpenNI::readFrame()
{
	XnStatus rc = XN_STATUS_OK;

	if (g_pPrimary != NULL)
		rc = g_Context.WaitOneUpdateAll(*g_pPrimary);
	else
		rc = g_Context.WaitAnyUpdateAll();

	if (rc != XN_STATUS_OK)
		ofLogError(LOG_NAME) << "Error:" << xnGetStatusString(rc);

	if (g_Depth.IsValid())
		g_Depth.GetMetaData(g_DepthMD);

	if (g_Image.IsValid())
		g_Image.GetMetaData(g_ImageMD);

	if (g_IR.IsValid())
		g_IR.GetMetaData(g_irMD);

	if (g_Audio.IsValid())
		g_Audio.GetMetaData(g_AudioMD);
  
	if (g_bHasDepth && g_bUseDepth)
		generateDepthPixels();

	if (g_bHasImage && g_bUseImage)
		generateImagePixels();

  if (g_bHasIR && g_bUseIR)
		generateIRPixels();

  if (g_bHasUser && g_bUseUser)
		generateUserMaskPixels();

	if (threaded)
    lock();

	if (g_bHasDepth && g_bUseDepth)
    swap(backDepthPixels,currentDepthPixels);

	if (g_bHasImage && g_bUseImage)
		swap(backRGBPixels,currentRGBPixels);

	if (g_bHasIR && g_bUseIR)
		swap(backIRPixels,currentIRPixels);
	
  if (g_bHasUser && g_bUseUser)
		swap(backUserMaskPixels,currentUserMaskPixels);

	bNewPixels = true;
	if (threaded)
    unlock();
}

//--------------------------------------------------------------
void
ofxOpenNI::threadedFunction()
{
	while(isThreadRunning())
		readFrame();
}

//--------------------------------------------------------------
bool
ofxOpenNI::isNewFrame()
{
	return bNewFrame;
}

//--------------------------------------------------------------
void
ofxOpenNI::update()
{
	if (!threaded)
		readFrame();
	else
		lock();

	if (bNewPixels)
  {
		if (g_bHasDepth && g_bUseDepth && useTexture)
      depthTexture.loadData(*currentDepthPixels);

		if (g_bHasImage && g_bUseImage && useTexture)
			rgbTexture.loadData(*currentRGBPixels);

		if (g_bHasIR && g_bUseIR && useTexture)
			irTexture.loadData(*currentIRPixels);

		if (g_bHasUser && g_bUseUser && useTexture)
			userMaskTexture.loadData(*currentUserMaskPixels);

		bNewPixels = false;
		bNewFrame = true;
		isPointCloudValid = false;
	}

	if (threaded)
		unlock();
}

//--------------------------------------------------------------
bool ofxOpenNI::toggleCalibratedRGBDepth(){

	// TODO: make work with IR generator
	if (!g_Image.IsValid())
  {
		printf("No Image generator found: cannot register viewport");
		return false;
	}

	// Toggle registering view point to image map
	if (g_Depth.IsCapabilitySupported(XN_CAPABILITY_ALTERNATIVE_VIEW_POINT))
	{
		if (g_Depth.GetAlternativeViewPointCap().IsViewPointAs(g_Image))
			disableCalibratedRGBDepth();
		else
			enableCalibratedRGBDepth();

	}
  else
    return false;

	return true;
}

//--------------------------------------------------------------
bool
ofxOpenNI::enableCalibratedRGBDepth()
{
	if (!g_Image.IsValid())
  {
		ofLogError(LOG_NAME) << ("No Image generator found: cannot register viewport");
		return false;
	}

	// Register view point to image map
	if (g_Depth.IsCapabilitySupported(XN_CAPABILITY_ALTERNATIVE_VIEW_POINT))
  {
		XnStatus result = g_Depth.GetAlternativeViewPointCap().SetViewPoint(g_Image);
		SHOW_RC(result, "Register viewport");
		if (result!=XN_STATUS_OK)
			return false;
  }
	else {
		ofLogError(LOG_NAME) << ("Can't enable calibrated RGB depth, alternative viewport capability not supported");
		return false;
	}

	return true;
}

//--------------------------------------------------------------
bool
ofxOpenNI::disableCalibratedRGBDepth()
{
	// Unregister view point from (image) any map
	if (g_Depth.IsCapabilitySupported(XN_CAPABILITY_ALTERNATIVE_VIEW_POINT))
  {
		XnStatus result = g_Depth.GetAlternativeViewPointCap().ResetViewPoint();
		SHOW_RC(result, "Unregister viewport");
		if (result!=XN_STATUS_OK)
      return false;
  }
	else
		return false;

	return true;
}

//--------------------------------------------------------------
void
ofxOpenNI::generateDepthPixels()
{
	// get the pixels
	const XnDepthPixel* depth = g_DepthMD.Data();
	XN_ASSERT(depth);
  
	if (g_DepthMD.FrameID() == 0)
    return;
  
	// copy raw values
  backDepthPixels->setFromPixels(depth, getWidth(), getHeight(), OF_IMAGE_GRAYSCALE);	
}

//--------------------------------------------------------------
void
ofxOpenNI::generateImagePixels()
{
	xn::ImageMetaData imd;
	g_Image.GetMetaData(imd);
	const XnUInt8* pImage = imd.Data();

	backRGBPixels->setFromPixels(pImage, imd.XRes(),imd.YRes(),OF_IMAGE_COLOR);
}

//--------------------------------------------------------------
void
ofxOpenNI::generateIRPixels()
{
	xn::IRMetaData imd;
	g_IR.GetMetaData(imd);
	const XnIRPixel* pImage = imd.Data();

  for (unsigned int y=0; y<imd.YRes(); ++y)
    for (unsigned int x=0; x<imd.XRes(); ++x)
      backIRPixels->setColor(x, y, pImage[y*imd.XRes()+x]);
}

//--------------------------------------------------------------
void
ofxOpenNI::generateUserMaskPixels()
{
  xn::SceneMetaData smd;
  if (g_User.GetUserPixels(0, smd) == XN_STATUS_OK)
  {
    const XnUInt16* pImage = smd.Data();

    backUserMaskPixels->setFromPixels(pImage, smd.XRes(),smd.YRes(),OF_IMAGE_GRAYSCALE);
  }
}

//--------------------------------------------------------------
void
ofxOpenNI::draw(int x, int y)
{ depthTexture.draw(x,y); }

//--------------------------------------------------------------
void
ofxOpenNI::drawRGB(int x, int y)
{ rgbTexture.draw(x,y); }

//--------------------------------------------------------------
void
ofxOpenNI::drawIR(int x, int y)
{ irTexture.draw(x,y); }

//--------------------------------------------------------------
void
ofxOpenNI::drawUserMask(int x, int y)
{ userMaskTexture.draw(x,y); }

//--------------------------------------------------------------
xn::Context&
ofxOpenNI::getXnContext()
{ return g_Context; }

//--------------------------------------------------------------
xn::Device&
ofxOpenNI::getDevice()
{ return g_Device; }

//--------------------------------------------------------------
xn::DepthGenerator&
ofxOpenNI::getDepthGenerator()
{ return g_Depth; }

//--------------------------------------------------------------
xn::ImageGenerator&
ofxOpenNI::getImageGenerator()
{ return g_Image; }

//--------------------------------------------------------------
xn::IRGenerator&
ofxOpenNI::getIRGenerator()
{ return g_IR; }

//--------------------------------------------------------------
xn::UserGenerator&
ofxOpenNI::getUserGenerator()
{ return g_User; }

//--------------------------------------------------------------
xn::HandsGenerator&
ofxOpenNI::getHandsGenerator()
{ return g_Hands; }

//--------------------------------------------------------------
xn::GestureGenerator&
ofxOpenNI::getGestureGenerator()
{ return g_Gesture; }

//--------------------------------------------------------------
xn::AudioGenerator&
ofxOpenNI::getAudioGenerator()
{ return g_Audio; }

//--------------------------------------------------------------
xn::Player&
ofxOpenNI::getPlayer()
{ return g_Player; }

//--------------------------------------------------------------
xn::DepthMetaData&
ofxOpenNI::getDepthMetaData()
{ return g_DepthMD; }

//--------------------------------------------------------------
xn::ImageMetaData&
ofxOpenNI::getImageMetaData()
{	return g_ImageMD; }

//--------------------------------------------------------------
xn::IRMetaData&
ofxOpenNI::getIRMetaData()
{	return g_irMD; }

//--------------------------------------------------------------
xn::AudioMetaData&
ofxOpenNI::getAudioMetaData()
{	return g_AudioMD; }

//--------------------------------------------------------------
ofShortPixels&
ofxOpenNI::getDepthPixels()
{
	Poco::ScopedLock<ofMutex> lock(mutex);	
	return *currentDepthPixels;
}

//--------------------------------------------------------------
ofPixels&
ofxOpenNI::getRGBPixels()
{
	Poco::ScopedLock<ofMutex> lock(mutex);
	return *currentRGBPixels;
}

//--------------------------------------------------------------
//ofShortPixels&
ofPixels&
ofxOpenNI::getIRPixels()
{
	Poco::ScopedLock<ofMutex> lock(mutex);
	return *currentIRPixels;
}

//--------------------------------------------------------------
ofShortPixels&
ofxOpenNI::getUserMaskPixels()
{
	Poco::ScopedLock<ofMutex> lock(mutex);
	return *currentUserMaskPixels;
}

//--------------------------------------------------------------
ofTexture&
ofxOpenNI::getDepthTextureReference()
{ return depthTexture; }

//--------------------------------------------------------------
ofTexture&
ofxOpenNI::getRGBTextureReference()
{ return rgbTexture; }

//--------------------------------------------------------------
ofTexture& ofxOpenNI::getIRTextureReference()
{ return irTexture; }

//--------------------------------------------------------------
ofTexture& ofxOpenNI::getUserMaskTextureReference()
{ return userMaskTexture; }

//--------------------------------------------------------------
void ofxOpenNI::setGeneratePCColors(bool generateColors)
{
	bGeneratePCColors = generateColors;
	if (bGeneratePCColors)
		pointCloud.getColors().resize(getWidth()*getHeight());
	else
		pointCloud.getColors().clear();
}

//--------------------------------------------------------------
void
ofxOpenNI::setGeneratePCTexCoords(bool generateTexCoords)
{
	bGeneratePCTexCoords = generateTexCoords;
	if (bGeneratePCTexCoords)
  {
		pointCloud.getTexCoords().resize(getWidth()*getHeight());
		int i=0;
		for (int y=0;y<getHeight();y++)
    {
			for (int x=0;x<getWidth();x++)
      {
				pointCloud.getTexCoords()[i].set(x,y);
				i++;
			}
		}
	}
  else
		pointCloud.getTexCoords().clear();
}

//--------------------------------------------------------------
ofMesh&
ofxOpenNI::getPointCloud()
{
	if (!isPointCloudValid)
  {
		mutex.lock();
		const XnDepthPixel * depth = g_DepthMD.Data();
		for (XnUInt16 y = g_DepthMD.YOffset(); y < g_DepthMD.YRes() + g_DepthMD.YOffset(); y++)
    {
			ofVec3f * pcDepth = &pointCloud.getVertices()[0] + y * g_DepthMD.XRes() + g_DepthMD.XOffset();
			for (XnUInt16 x = 0; x < g_DepthMD.XRes(); x++, depth++, pcDepth++)
				pcDepth->z = (*depth)/max_depth;
		}
		if (g_bHasImage && g_bUseImage && bGeneratePCColors)
    {
			unsigned char * rgbColorPtr = currentRGBPixels->getPixels();
			for (int i=0;i<(int)pointCloud.getColors().size();i++)
      {
				pointCloud.getColors()[i] = ofColor(*rgbColorPtr, *(rgbColorPtr+1), *(rgbColorPtr+2));
				rgbColorPtr+=3;
			}
		}
		mutex.unlock();
		isPointCloudValid = true;
	}
	return pointCloud;
}

//--------------------------------------------------------------
float
ofxOpenNI::getWidth()
{
  float width = 0;
	if (g_bHasDepth)
		width = g_DepthMD.XRes();
	else if (g_bHasImage)
		width = g_ImageMD.XRes();
	else if (g_bHasIR)
		width = g_irMD.XRes();
	else
		ofLogWarning(LOG_NAME) << "getWidth() : We haven't yet initialised any generators, so this value returned is returned as 0";

  if (width == 0)
    width = 640;
  return width;
}

//--------------------------------------------------------------
float
ofxOpenNI::getHeight()
{
  float height = 0;
	if (g_bHasDepth)
		height = g_DepthMD.YRes();
	else if (g_bHasImage)
		height = g_ImageMD.YRes();
	else if (g_bHasIR)
		height = g_irMD.YRes();
	else
		ofLogWarning(LOG_NAME) << "getHeight() : We haven't yet initialised any generators, so this value returned is returned as 0";

  if (height == 0)
    height = 480;
  return height;
}

//--------------------------------------------------------------
ofPoint
ofxOpenNI::worldToProjective(const ofPoint& p)
{
	XnVector3D world = toXn(p);
	return worldToProjective(world);
}

//--------------------------------------------------------------
ofPoint
ofxOpenNI::worldToProjective(const XnVector3D& p)
{
	XnVector3D proj;
	g_Depth.ConvertRealWorldToProjective(1, &p, &proj);
	return toOf(proj);
}

//--------------------------------------------------------------
ofPoint
ofxOpenNI::projectiveToWorld(const ofPoint& p)
{
	XnVector3D proj = toXn(p);
	return projectiveToWorld(proj);
}

//--------------------------------------------------------------
ofPoint
ofxOpenNI::projectiveToWorld(const XnVector3D& p)
{
	XnVector3D world;
	g_Depth.ConvertProjectiveToRealWorld(1, &p, &world);
	return toOf(world);
}

//--------------------------------------------------------------
ofPoint
ofxOpenNI::cameraToWorld(const ofVec2f& c)
{
	vector<ofVec2f> vc(1, c);
	vector<ofVec3f> vw(1);
	
	cameraToWorld(vc, vw);
	
	return vw[0];
}

//--------------------------------------------------------------
void
ofxOpenNI::cameraToWorld(const vector<ofVec2f>& c, vector<ofVec3f>& w)
{
	const int nPoints = c.size();
	w.resize(nPoints);
	
	vector<XnPoint3D> projective(nPoints);
	XnPoint3D *out = &projective[0];
	
	if (threaded) lock();
	const XnDepthPixel* d = currentDepthPixels->getPixels();
	unsigned int pixel;
	for (int i=0; i<nPoints; ++i)
  {
		pixel  = (int)c[i].x + (int)c[i].y * getWidth();
		if (pixel >= getWidth()*getHeight())
			continue;
		
		projective[i].X = c[i].x;
		projective[i].Y = c[i].y;
		projective[i].Z = float(d[pixel]) / 1000.0f;
	}

	if (threaded)
    unlock();
	
	g_Depth.ConvertProjectiveToRealWorld(nPoints, &projective[0], (XnPoint3D*)&w[0]);	
}

//--------------------------------------------------------------
void
ofxOpenNI::useDepth(bool bUseDepth)
{
  g_bUseDepth = bUseDepth;
}

//--------------------------------------------------------------
void
ofxOpenNI::useImage(bool bUseImage)
{  
  g_bUseImage = bUseImage;
}

//--------------------------------------------------------------
void
ofxOpenNI::useIR(bool bUseIR)
{  
  g_bUseIR = bUseIR;
}

//--------------------------------------------------------------
void
ofxOpenNI::useUser(bool bUseUser)
{  
  g_bUseUser = bUseUser;
}

//--------------------------------------------------------------
void
ofxOpenNI::useHands(bool bUseHands)
{  
  g_bUseHands = bUseHands;
}

//--------------------------------------------------------------
void
ofxOpenNI::useGesture(bool bUseGesture)
{  
  g_bUseGesture = bUseGesture;
}

//--------------------------------------------------------------
void
ofxOpenNI::useAudio(bool bUseAudio)
{
  g_bUseAudio = bUseAudio;
}

//--------------------------------------------------------------
void
ofxOpenNI::usePlayer(bool bUsePlayer)
{
  g_bUsePlayer = bUsePlayer;
}

//--------------------------------------------------------------
bool
ofxOpenNI::usingDepth()
{
  return (g_bHasDepth && g_bUseDepth);
}

//--------------------------------------------------------------
bool
ofxOpenNI::usingImage()
{
  return (g_bHasImage && g_bUseImage);
}

//--------------------------------------------------------------
bool
ofxOpenNI::usingIR()
{
  return (g_bHasIR && g_bUseIR);
}

//--------------------------------------------------------------
bool
ofxOpenNI::usingUser()
{
  return (g_bHasUser && g_bUseUser);
}

//--------------------------------------------------------------
bool
ofxOpenNI::usingHands()
{
  return (g_bHasHands && g_bUseHands);
}

//--------------------------------------------------------------
bool
ofxOpenNI::usingGesture()
{
  return (g_bHasGesture && g_bUseGesture);
}

//--------------------------------------------------------------
bool
ofxOpenNI::usingAudio()
{
  return (g_bHasAudio && g_bUseAudio);
}

//--------------------------------------------------------------
bool
ofxOpenNI::usingPlayer()
{
  return (g_bHasPlayer && g_bUsePlayer);
}
