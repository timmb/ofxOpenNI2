#pragma once

#include <XnCppWrapper.h>
#include <XnTypes.h>
#include "ofConstants.h"
#include "ofPixels.h"
#include "ofTexture.h"
#include "ofThread.h"

class ofxOpenNI: public ofThread{
public:
	ofxOpenNI();

	bool setupFromXML(string xml, bool threaded=true);
	bool setupFromRecording(string recording, bool threaded=true);

	bool isNewFrame();
	void update();
	void draw(int x, int y);
	void drawRGB(int x, int y);
	void drawIR(int x, int y);
	void drawUserMask(int x, int y);

	void setUseTexture(bool useTexture);

	ofShortPixels&  getDepthPixels();
	ofPixels&       getRGBPixels();
//	ofShortPixels& getIRPixels();
	ofPixels&       getIRPixels();
	ofShortPixels&  getUserMaskPixels();
	
	ofTexture&      getDepthTextureReference();
	ofTexture&      getRGBTextureReference();
	ofTexture&      getIRTextureReference();
	ofTexture&      getUserMaskTextureReference();

	ofMesh& getPointCloud();
	void setGeneratePCColors(bool generateColors);
	void setGeneratePCTexCoords(bool generateTexCoords);

	float getWidth();
	float getHeight();

	bool toggleCalibratedRGBDepth();
	bool enableCalibratedRGBDepth();
	bool disableCalibratedRGBDepth();

	ofPoint worldToProjective(const ofPoint& p);
	ofPoint worldToProjective(const XnVector3D& p);

	ofPoint projectiveToWorld(const ofPoint& p);
	ofPoint projectiveToWorld(const XnVector3D& p);

	ofPoint cameraToWorld(const ofVec2f& c);
	void cameraToWorld(const vector<ofVec2f>& c, vector<ofVec3f>& w);

	void addLicense(string sVendor, string sKey);

	xn::Context&          getXnContext();
	xn::Device&           getDevice();
	xn::DepthGenerator&   getDepthGenerator();
	xn::ImageGenerator&   getImageGenerator();
	xn::IRGenerator&      getIRGenerator();
	xn::UserGenerator&    getUserGenerator();
	xn::HandsGenerator&   getHandsGenerator();
	xn::GestureGenerator& getGestureGenerator();
	xn::AudioGenerator&   getAudioGenerator();
	xn::Player&           getPlayer();

	xn::DepthMetaData&    getDepthMetaData();
	xn::ImageMetaData&    getImageMetaData();
	xn::IRMetaData&       getIRMetaData();
	xn::AudioMetaData&    getAudioMetaData();

  void useDepth         (bool bUseDepth);
	void useImage         (bool bUseImage);
	void useIR            (bool bUseIR);
  void useUser          (bool bUseUser);
  void useHands         (bool bUseHands);
  void useGesture       (bool bUseGesture);
	void useAudio         (bool bUseAudio);
	void usePlayer        (bool bUsePlayer);

  bool usingDepth();
	bool usingImage();
	bool usingIR();
  bool usingHands();
  bool usingGesture();
  bool usingUser();
	bool usingAudio();
	bool usingPlayer();

	static string LOG_NAME;

protected:
	void threadedFunction();

private:
	void openCommon();
	void initConstants();
	void readFrame();
	void generateDepthPixels();
	void generateImagePixels();
	void generateIRPixels();
	void generateUserMaskPixels();
	void allocateDepthBuffers();
	void allocateRGBBuffers();
	void allocateIRBuffers();
	void allocateUserMaskBuffers();

	static void XN_CALLBACK_TYPE onErrorStateChanged(XnStatus errorState, void* pCookie);

	xn::Context g_Context;
	xn::ScriptNode g_scriptNode;

	struct DeviceParameter{
		int nValuesCount;
		unsigned int pValues[20];
		string pValueToName[20];
	};

	struct NodeCodec{
		int nValuesCount;
		XnCodecID pValues[20];
		string pIndexToName[20];
	};

	struct DeviceStringProperty{
		int nValuesCount;
		string pValues[20];
	};

	DeviceStringProperty g_PrimaryStream;
	DeviceParameter g_Registration;
	DeviceParameter g_Resolution;

	bool g_bHasDepth;
	bool g_bHasImage;
	bool g_bHasIR;
  bool g_bHasUser;
  bool g_bHasHands;
  bool g_bHasGesture;
	bool g_bHasAudio;
	bool g_bHasPlayer;

  bool g_bUseDepth;
	bool g_bUseImage;
	bool g_bUseIR;
  bool g_bUseUser;
  bool g_bUseHands;
  bool g_bUseGesture;
	bool g_bUseAudio;
	bool g_bUsePlayer;

	xn::Device            g_Device;
	xn::DepthGenerator    g_Depth;
	xn::ImageGenerator    g_Image;
	xn::IRGenerator       g_IR;
  xn::UserGenerator     g_User;
	xn::HandsGenerator    g_Hands;
	xn::GestureGenerator  g_Gesture;
	xn::AudioGenerator    g_Audio;
	xn::Player            g_Player;

	xn::MockDepthGenerator mockDepth;

	xn::DepthMetaData     g_DepthMD;
	xn::ImageMetaData     g_ImageMD;
	xn::IRMetaData        g_irMD;
	xn::AudioMetaData     g_AudioMD;

	xn::ProductionNode*   g_pPrimary;


	bool useTexture;
	bool bNewPixels;
	bool bNewFrame;
	bool threaded;

	// depth
	ofTexture depthTexture;
	ofShortPixels depthPixels[2];
	ofShortPixels * backDepthPixels, * currentDepthPixels;
	float	max_depth;

	// rgb
	ofTexture rgbTexture;
	ofPixels rgbPixels[2];
	ofPixels * backRGBPixels, * currentRGBPixels;

  // ir
	ofTexture irTexture;
//	ofShortPixels irPixels[2];
//	ofShortPixels * backIRPixels, * currentIRPixels;
	ofPixels irPixels[2];
	ofPixels * backIRPixels, * currentIRPixels;

  // user mask
	ofTexture userMaskTexture;
	ofShortPixels userMaskPixels[2];
	ofShortPixels * backUserMaskPixels, * currentUserMaskPixels;

	// point cloud
	ofMesh pointCloud;
	bool isPointCloudValid;
	bool bGeneratePCColors, bGeneratePCTexCoords;
};
