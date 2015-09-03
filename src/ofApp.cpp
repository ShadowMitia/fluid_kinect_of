#include "ofApp.h"


//--------------------------------------------------------------
void ofApp::setup() {
    ofEnableAlphaBlending();
	//ofSetLogLevel(OF_LOG_VERBOSE);

    // call setup methods
    setupKinect();
    setupOpenCv();
    setupSmokeFluid();

    // set framerate to 60 fps
	ofSetFrameRate(60);

    // load shader to blackout the hand
	blackHandShader.load("", "blackHandShader.frag");

    // debug HUD boolean
	showDebugVideo = false;
}

//--------------------------------------------------------------
void ofApp::update() {

	ofBackground(100, 100, 100);

	kinect.update();
	fluid.update();

    updateCvImages();
    contourFinder.findContours(grayImage, 100, (kinect.width*kinect.height)/2, 5, false, true);
    collectContours();

	// show framerate in titlebar
	ofSetWindowTitle(ofToString(ofGetFrameRate()));
}

//--------------------------------------------------------------
void ofApp::draw() {

    // draw the smoke
    fluid.draw();

    // draw the contours, and make the smoke see it as an obstacle


    for (int i = 0; i < contourFinder.blobs.size(); i++){
        fluid.begin();
        polyContour[i].draw();
        fluid.end();
    }

    for (int i = 0; i < contourFinder.blobs.size(); i++){
        blackHandShader.begin();
        polyContour[i].draw();
        blackHandShader.end();
    }

    // show debug HUD
    if (showDebugVideo){
        drawDebug();
    }
}


//--------------------------------------------------------------

void ofApp::exit() {
	kinect.setCameraTiltAngle(0); // zero the tilt angle on exit
	kinect.close(); // close the kinect
}

//--------------------------------------------------------------
void ofApp::keyPressed (int key) {
    switch (key) {
        case '>':
		case '.':
		    if (showDebugVideo){
                farThreshold ++;
                if (farThreshold > 255) farThreshold = 255;
		    }
			break;

		case '<':
		case ',':
		    if (showDebugVideo){
                farThreshold --;
                if (farThreshold < 0) farThreshold = 0;
		    }
			break;

		case '+':
		case '=':
		    if (showDebugVideo){
                nearThreshold ++;
                if (nearThreshold > 255) nearThreshold = 255;
		    }
			break;

		case '-':
		    if (showDebugVideo){
                nearThreshold --;
                if (nearThreshold < 0) nearThreshold = 0;
		    }
			break;
        case ' ':
            showDebugVideo = !showDebugVideo;
            break;
    }
}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button)
{}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button)
{}

//-------------------------------contourFinder.findContours(grayImage, 100, (kinect.width*kinect.height), 5, false, true);-------------------------------
void ofApp::windowResized(int w, int h)
{}

void ofApp::setupKinect() {

    // calibrates the depth image with the rgb image
    kinect.setRegistration(true);

    // init kinect and open the first one found
	kinect.init();
	kinect.open();

    // make sure the kinect is horizontal
    kinect.setCameraTiltAngle(0);

    // values for near and far threshold
    // these values the points which are interesting
    nearThreshold = 241;
	farThreshold = 230;
}

void ofApp::setupOpenCv() {
    // allocates the rgb image holder for the kinect feed
	colorImg.allocate(kinect.width, kinect.height);
	// allocate the grayscale image holder for the kinect feed
	grayImage.allocate(kinect.width, kinect.height);
	// these are used to make the grawscale with the points we want
	grayThreshNear.allocate(kinect.width, kinect.height);
	grayThreshFar.allocate(kinect.width, kinect.height);
}

void ofApp::setupSmokeFluid() {

    // allocate the size need to show the smoke
    fluid.allocate(WIDTH, HEIGHT, 0.5);

    // some values to get some nice smoke
    fluid.dissipation = 0.99;
    fluid.velocityDissipation = 0.99;

    // We don't want any default gravity values
    fluid.setGravity(ofVec2f(0.0,0.0));

    // list of all the points of origin of the different smoke points
    std::vector<ofPoint> origins;
    origins.push_back(ofPoint(WIDTH / 2, 0));
    //origins.push_back(ofPoint(WIDTH / 4, 0));
    //origins.push_back(ofPoint(50, 0));
    //origins.push_back(ofPoint(WIDTH - 50, 0));
    //origins.push_back(ofPoint(WIDTH - WIDTH / 4, 0));

    // for each point we define a color
    std::vector<ofFloatColor> smokeColors;
    smokeColors.push_back(ofFloatColor(0.5,0.1,0.0)); // orange
    //smokeColors.push_back(ofFloatColor(1.0,0.0,0.0)); // red
    //smokeColors.push_back(ofFloatColor(0.0,1.0,0.0)); // green
    //smokeColors.push_back(ofFloatColor(0.0,0.0,1.0)); // blue
    //smokeColors.push_back(ofFloatColor(0.1,0.0,0.5)); // purple

    // initalise every smoke point with a radius of 10.f and a y vel of 2
    for (int i = 0; i < origins.size(); i++){
        fluid.addConstantForce(origins[i], ofPoint(0,2), smokeColors[i], 10.f);
    }
}

void ofApp::drawDebug() {
    // show rgb feed
    grayImage.draw(0, 0, 400, 300);
    // show contours found bw contourFinder
    contourFinder.draw(10, 320, 400, 300);
    // Show debug info
    stringstream ss;
    ss << "Far threshold " << farThreshold << " \nNear Threshold " << nearThreshold << " \nKinect width " << kinect.width << " \nKinect height " << kinect.height;
    ofDrawBitmapString(ss.str(), 0, HEIGHT - 100);
}

void ofApp::updateCvImages() {
	if (kinect.isFrameNew()){
        grayImage.setFromPixels(kinect.getDepthPixels(), kinect.width, kinect.height);

		// load grayscale depth image from the kinect source
		grayImage.setFromPixels(kinect.getDepthPixels(), kinect.width, kinect.height);

        grayThreshNear = grayImage;
        grayThreshFar = grayImage;
        grayThreshNear.threshold(nearThreshold, true);
        grayThreshFar.threshold(farThreshold);
        cvAnd(grayThreshNear.getCvImage(), grayThreshFar.getCvImage(), grayImage.getCvImage(), NULL);

        colorImg.setFromPixels(kinect.getPixels(), kinect.width, kinect.height);
	}
}

void ofApp::collectContours(){
    if (contourFinder.nBlobs > 0) {
        polyContour.clear();
        polyContour.resize(contourFinder.nBlobs);
        for (int i = 0; i < contourFinder.blobs.size(); i++){
            polyContour[i].addVertices(contourFinder.blobs[i].pts);
            polyContour[i].setClosed(true);
        }

    }
}
