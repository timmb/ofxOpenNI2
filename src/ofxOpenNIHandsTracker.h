#pragma once

#include <map>
#include <set>
#include <XnTypes.h>
#include <XnCppWrapper.h>
#include "ofConstants.h"
#include "ofVec3f.h"

class ofxOpenNI;

class ofxOpenNIHandsTracker
{
public:
	ofxOpenNIHandsTracker();

	bool setup(ofxOpenNI & openNI);

	void update();
	void draw();

	static string LOG_NAME;
private:
  static void XN_CALLBACK_TYPE Gesture_Recognized(xn::GestureGenerator& userGenerator,
                                                  const XnChar* strGesture,
                                                  const XnPoint3D* pIDPosition,
                                                  const XnPoint3D* pEndPosition,
                                                  void* pCookie);
  static void XN_CALLBACK_TYPE Gesture_Process(xn::GestureGenerator& userGenerator,
                                               const XnChar* strGesture,
                                               const XnPoint3D* pPosition,
                                               XnFloat fProgress,
                                               void* pCookie);

	static void XN_CALLBACK_TYPE Hand_Create(xn::HandsGenerator& userGenerator, 
                                           XnUserID nID, 
                                           const XnPoint3D*	position, 
                                           XnFloat timestamp, 
                                           void* pCookie);
	static void XN_CALLBACK_TYPE Hand_Update(xn::HandsGenerator& userGenerator, 
                                           XnUserID nID, 
                                           const XnPoint3D*	position, 
                                           XnFloat timestamp, 
                                           void* pCookie);
	static void XN_CALLBACK_TYPE Hand_Destroy(xn::HandsGenerator& userGenerator,
                                            XnUserID nID,
                                            XnFloat timestamp,
                                            void* pCookie);
  
	ofxOpenNI* openNI;

	map<XnUserID, std::pair<ofVec3f, ofVec3f> > hands;
};
