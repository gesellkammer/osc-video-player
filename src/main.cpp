#include "ofMain.h"
#include "ofApp.h"
#include "ofxTclap.h"
#include "ofLog.h"

using namespace std;

void parseOscAddress(const string &addr, string &host, ui32 &port) {
    size_t sep = addr.find(":");
    int port_;
    if(sep == string::npos) {
        // no host, must be a port
        host = "127.0.0.1";
        port_ = stoi(addr);

    } else {
        host = addr.substr(0, sep);
        port_ = stoi(addr.substr(sep+1, addr.size()));
    }
    if(port_ < 1024 || port_ >= 65535) {
        ofLogError() << "port out of range: " << port_ << endl;
        port = 0;
    } else {
        port = (ui32)port_;
    }
}

//========================================================================
int main(int argc, char **argv){
    int numslots;
    int port;
    bool debug;
    string loadFolderPath;
    bool printDocumentation;
    string outHost = "127.0.0.1";
    ui32 outPort = 0;
    int frameRate = 0;

    try {
        CmdLine cmd("OSC Video Player");
        ValueArg<int> numslotsArg("n", "numslots",
                                  "Max. number of video slots", false, 100, "int");
        cmd.add(numslotsArg);

        ValueArg<string> folder("f", "folder",
                                "Load all videos from this folder", false, "", "string");
        cmd.add(folder);

        ValueArg<int> portArg("p", "port",
                              "OSC port to listen to", false, 30003, "int");
        cmd.add(portArg);

        SwitchArg debugSwitch("d", "debug",
                              "debug mode", false);
        cmd.add(debugSwitch);

        SwitchArg manSwitch("m", "man",
                            "Print manual and exit", false);
        cmd.add(manSwitch);

        ValueArg<string> oscOutAddress("o", "oscout",
                                       "[host:]port (for ex. '127.0.0.1:9998', or simply '9998') "
                                       "to send information to",
                                       false, "", "string");
        cmd.add(oscOutAddress);


        ValueArg<int> frameRateArg("r", "framerate",
                                   "Frame Rate", false, 0, "int");

        cmd.add(frameRateArg);

        cmd.parse(argc, argv);
        numslots = numslotsArg.getValue();
        port = portArg.getValue();
        debug = debugSwitch.getValue();
        loadFolderPath = folder.getValue();
        printDocumentation = manSwitch.getValue();
        frameRate = frameRateArg.getValue();

        if(debug) {
            ofSetLogLevel(OF_LOG_VERBOSE);
        }


        if(!oscOutAddress.getValue().empty()) {
            parseOscAddress(oscOutAddress.getValue(), outHost, outPort);
        }

    }
    catch (ArgException &e) {
        // catch any exceptions
        ofLogError("main") << e.error() << " for arg " << e.argId();
    }

    ofLogNotice() << "debug: " << debug;
    ofLogNotice() << "numslots: " << numslots;
    ofLogNotice() << "OSC port: " << port;
    ofLogNotice() << "Out OSC: " << outHost << ":" << outPort;

    // start
    ofGLFWWindowSettings settings;
    settings.setGLVersion(3,3);
    ofCreateWindow(settings);
    // ofSetupOpenGL(1024,768, OF_WINDOW);			// <-------- setup the GL context
    // ofSetVerticalSync(true);
    if (frameRate > 0)
        ofSetFrameRate(frameRate);
    // this kicks off the running of my app
    // can be OF_WINDOW or OF_FULLSCREEN
    // pass in width and height too:
    auto app = new ofApp((size_t)numslots, port);

    if(outPort != 0) {
        app->oscOutHost = outHost;
        app->oscOutPort = outPort;

    }

    if(!loadFolderPath.empty()) {
        int ok = app->loadFolder(loadFolderPath);
        if(!ok) {
            cerr << "*ERROR* Could not load some clips from folder " << loadFolderPath << endl;
        }
    }

    if(printDocumentation) {
        app->printOscApi();
        cout << endl;
        app->printKeyboardShortcuts();
        exit(0);
    }

    ofRunApp(app);



}
