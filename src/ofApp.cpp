
#include "ofApp.h"

#define LOG  ofLogVerbose()
#define INFO ofLogNotice()
#define ERR  ofLogError()
#define WARN ofLogWarning()


//--------------------------------------------------------------
ofApp::ofApp(size_t numslots_, int port_) {
    numSlots = numslots_;
    oscPort = port_;
    oscOutPort = 0;
    oscOutHost = "";

    debugging = true;
    draw_x0 = 0;
    draw_y0 = 0;

    for (int i=0; i < numSlots; i++) {
        loaded.push_back(0);
        movs.push_back(ofVideoPlayer());
        speeds.push_back(1.f);
        paused.push_back(0);
        shouldStop.push_back(0);
        durations.push_back(0.f);
        drawclip.push_back(false);
        needsUpdate.push_back(false);
    }
}

void ofApp::setup(){
    // numSlots = NUMSLOTS;
    draw_height = ofGetWindowHeight();
    draw_width = ofGetWindowWidth();
    oscReceiver.setup(oscPort);
    if(oscOutPort != 0) {
        oscSender.setup(oscOutHost, oscOutPort);
    }
    ofBackground(0);
    printOscApi();
    printKeyboardShortcuts();
}

void ofApp::printOscApi() {
    cout << "OSC port: " << oscPort << "\n";
    cout << "Number of Slots: " << numSlots << "\n\n";
    cout << "OSC messages accepted:\n\n"
            "/load slot:int path:str\n"
            "    * Load a video at the given slot \n\n"
            "/play slot:int [speed:float=1] [starttime:float=0] [paused:int=0] [stopWhenFinished:int=1]\n"
            "    * Play the given slot with given speed, starting at starttime (secs)\n\n"
            "      paused: if 1, the playback will be paused\n"
            "      stopWhenFinished: if 1, playback will stop at the end, otherwise it\n"
            "        pauses at the last frame\n\n"
            "/stop [slot:int] \n"
            "    * Stop playback. If no slot is given, the currently playing slot is stopped\n\n"
            "/pause state:int\n"
            "    * If state 1, pause playback, 0 resumes playback\n\n"
            "/setspeed speed:float \n"
            "    * Change the speed of the playing slot\n\n"
            "/scrub pos:float [slot:int=current] \n"
            "    * Set the relative position 0-1. Sets the given slot as the current slot\n"
            "      In scrub mode, video is paused and must be driven externally\n\n"
            "/scrubabs timepos:float [slot:int=current] \n"
            "    * Set the absolute time and activates the given slot. In scrub mode, video\n"
            "      is paused and must be driven externally\n\n"
            "/setpos \n"
            "     * Sets the relative (0-1) playing position of the current clip\n"
            "       (does not pause the clip like /scrub)\n\n"
            "/settime \n"
            "     * Sets the absolute playing position of the current clip\n"
            "       (does not pause the clip like /scrubabs)\n\n"
            "/dump \n"
            "    * Dump information about loaded clips\n\n"
            "/quit \n"
            "    * Quit this application\n"
            ;
}

void ofApp::printKeyboardShortcuts() {
    cout << "\n"
            "Keyboard Shortcuts\n"
            "==================\n\n"
            "F - Toggle Fullscreen \n"
            "D - Dump Information about loaded clips \n"
            "Q - Quit \n";
}


//--------------------------------------------------------------
bool ofApp::loadMov(int slot, const string &path) {
    if(slot < 0 && slot >= numSlots) {
        ERR << "loadMov -- Slot out of range: " << slot << endl;
        return false;
    }
    auto idx = static_cast<size_t>(slot);
    if(loaded[idx] == 1) {
        LOG << "Slot already loaded. Closing old movie, idx: " << idx;
        movs[idx].close();
    }
    auto & mov = movs[idx];
    // mov.setPixelFormat(OF_PIXELS_RGB);
    mov.setPixelFormat(OF_PIXELS_NATIVE);
    mov.load(path);
    mov.setLoopState(OF_LOOP_NONE);
    mov.setPaused(true);
    mov.play();

    loaded[idx] = 1;
    float dur = mov.getDuration();
    if(dur < 0.001) {
        ERR << "loadMov -- Clip too short: " << path << ", dur: " << dur << endl;
        return false;
    }
    durations[idx] = dur;
    INFO << "Loaded slot " << slot << ": " << path << endl;
    if(this->oscOutPort != 0) {
        this->sendClipInfo(idx, this->oscOutHost, this->oscOutPort);
    }
    return true;
}

bool ofApp::playClip(size_t slot, float speed, float skiptime, bool startPaused,
                     bool stopWhenFinished, bool stopPrevious) {
    if(stopPrevious && !stack.empty()) {
        auto &currmov = movs[stack[stack.size()-1]];
        if(currmov.isPlaying()) {
            currmov.stop();
            stack.pop_back();
        }
    }
    // TODO: check if slot is already in stack
    speeds[slot] = speed;
    paused[slot] = startPaused;
    shouldStop[slot] = stopWhenFinished;
    auto & mov = movs[slot];
    mov.setSpeed(speed);
    mov.setPaused(startPaused);
    auto dur = mov.getDuration();
    if(dur <= 0.01) {
        ERR << "Clip too short, dur: " << dur << endl;
        return false;
    }
    float relpos = skiptime / dur;
    if(relpos < 0 || relpos >= 1.0f) {
        ERR << "skiptime " << skiptime << " out of bounds, clip duration: "
            << dur << ", rel. pos: " << relpos << endl
            << "/play message aborted" << endl;
        return false;
    }
    int frame = static_cast<int>(relpos * mov.getTotalNumFrames());
    mov.setFrame(frame);
    // setPosition is between 0-1
    // mov.setPosition(relpos);
    drawclip[slot] = true;
    calculateDrawCoords();
    stack.push_back(slot);
    mov.play();

    INFO << "/play - slot:" << slot
         << ", speed:" << speed
         << ", skiptime:" << skiptime
         << ", paused: " << startPaused
         << ", stopWhenFinished: " << stopWhenFinished
         << "rel. pos: " << relpos
         << endl;
    return true;
}

/*
size_t ofApp::currentSlot() {
    // returns the active slot, or numSlots + 1 to signal that there is not active slot
    if (stack.empty())
        return numSlots + 1;
    return stack[stack.size() -1];
}
*/

void ofApp::update(){
    ofxOscMessage msg;
    while(oscReceiver.hasWaitingMessages()) {
        oscReceiver.getNextMessage(msg);
        string addr = msg.getAddress();
        auto numargs = msg.getNumArgs();
        if (addr == "/scrub") {
            if(numargs < 1 || numargs > 2) {
                ERR << "/scrub expected 1 or 2 args, received " << numargs << endl;
                ERR << "    /scrub relpos:float [slot:int]";
                continue;
            }
            size_t currSlot = currentSlot();
            size_t slot = numargs == 2 ? msg.getArgAsInt32(1) : currSlot;
            if (slot >= numSlots) {
                ERR << "/stop: invalid slot, " << slot << ", num. slots: " << numSlots << endl;
                continue;
            }
            if (!loaded[slot]) {
                ERR << "/stop: slot " << slot << " is empty\n";
                continue;
            }
            float pos = msg.getArgAsFloat(0);
            auto &mov = movs[slot];
            if(slot != currSlot) {
                if(currSlot < numSlots)
                    stopMov(currSlot);
                mov.setPaused(true);
                mov.setSpeed(0);
                drawclip[slot] = true;
                stack.push_back(slot);
                calculateDrawCoords();
            }
            int totalFrames = mov.getTotalNumFrames();
            int frame = static_cast<int>(pos * totalFrames);
            if(frame >= totalFrames)
                frame = totalFrames - 1;
            mov.setFrame(frame);
        }
        else if(addr == "/scrubabs") {
            if(numargs < 1 || numargs > 2) {
                ERR << "/scrub expected 1 or 2 args, received " << numargs << endl;
                ERR << "    /scrub relpos:float [slot:int]";
                continue;
            }
            size_t currSlot = currentSlot();
            size_t slot = numargs == 2 ? msg.getArgAsInt32(1) : currSlot;
            if (slot >= numSlots) {
                ERR << "/stop: invalid slot, " << slot << ", num. slots: " << numSlots << endl;
                continue;
            }
            if (!loaded[slot]) {
                ERR << "/stop: slot " << slot << " is empty\n";
                continue;
            }
            float time = msg.getArgAsFloat(0);
            auto &mov = movs[slot];
            if(slot != currSlot) {
                if(currSlot < numSlots)
                    stopMov(currSlot);
                mov.setPaused(true);
                mov.setSpeed(0);
                drawclip[slot] = true;
                stack.push_back(slot);
                calculateDrawCoords();
            }
            int totalFrames = mov.getTotalNumFrames();
            int frame = static_cast<int>((time/durations[slot])*mov.getTotalNumFrames());
            if(frame >= totalFrames)
                frame = totalFrames - 1;
            mov.setFrame(frame);
            // mov.setPosition(time/durations[slot]);
        }
        else if(addr == "/play") {
            // args: slot, speed, skip(seconds), paused, stop_when_finished
            if(numargs == 0 || numargs > 5) {
                ERR << "/play expects 1-5 arguments, got " << numargs << endl
                    << "    /play slot:int, speed:float=1, skipsecods:float=0, "
                       "paused:int=0, stopWhenFinished:int=1\n";
                continue;
            }
            size_t slot = msg.getArgAsInt32(0);
            if(slot >= numSlots) {
                ERR << "/play: slot "<< slot << " out of range\n";
                continue;
            }
            if(!loaded[slot]) {
                ERR << "/play: slot "<< slot << " not loaded\n";
                continue;
            }
            float speed = numargs >= 2 ? msg.getArgAsFloat(1) : 1.0f;
            float skiptime = numargs >= 3 ? msg.getArgAsFloat(2) : 0.0f;
            int pausestatus = numargs >= 4 ? msg.getArgAsInt32(3) : 0;
            int stopWhenFinished = numargs >= 5 ? msg.getArgAsInt32(4) : 1;
            int stopPrevious = numargs >= 6 ? msg.getArgAsInt32(5) : 0;
            playClip(slot, speed, skiptime, pausestatus, stopWhenFinished, stopPrevious);
        }
        else if(addr == "/stop") {
            size_t slot = numargs == 1 ? msg.getArgAsInt32(0) : currentSlot();
            if (slot >= numSlots) {
                ERR << "/stop: invalid slot, " << slot << ", num. slots: " << numSlots << endl;
                continue;
            }
            if (!loaded[slot]) {
                ERR << "/stop: slot " << slot << " is empty\n";
                continue;
            }
            stopMov(slot);
        }
        else if(addr == "/setpos") {
            if(numargs != 1) {
                ERR << "/setpos expected 1 argument, received " << numargs << endl;
                continue;
            }
            if(stack.empty()) {
                ERR << "/setpos: no active slot\n";
                continue;
            }
            size_t slot = currentSlot();
            float pos = msg.getArgAsFloat(0);
            movs[slot].setPosition(pos);
        }
        else if(addr == "/settime") {
            if(numargs != 1) {
                ERR << "/setpos expected 1 argument, received " << numargs << endl;
                continue;
            }
            if(stack.empty()) {
                ERR << "/setpos: no active slot\n";
                continue;
            }
            size_t slot = currentSlot();
            float time = msg.getArgAsFloat(0);
            auto &mov = movs[slot];
            int totalFrames = mov.getTotalNumFrames();
            int frame = static_cast<int>((time/durations[slot])*mov.getTotalNumFrames());
            if(frame >= totalFrames)
                frame = totalFrames - 1;
            mov.setFrame(frame);
        }
        else if(addr == "/pause") {
            if(numargs != 1) {
                ERR << "/pause expects 1 arguments, got " << numargs << endl
                    << "    Syntax: /pause pauseStatus:int\n";
                continue;
            }
            size_t slot = currentSlot();
            if (slot >= numSlots) {
                ERR << "/pause: invalid slot " << slot << endl;
            } else if(!loaded[slot]) {
                ERR << "/pause: slot not loaded: " << slot << endl;
            } else {
                int status = msg.getArgAsInt32(0);
                movs[slot].setPaused(status);
            }

        }
        else if(addr == "/load") {
            if(numargs != 2) {
                ERR << "/load expects 2 arguments, got " << numargs << endl
                    << "    /load slot:int path:string\n";
                continue;
            }
            int slot = msg.getArgAsInt32(0);
            if(slot < 0 || slot >= numSlots) {
                ERR << "/load: slot "<<slot<<" out of range\n";
                continue;
            }

            string path = msg.getArgAsString(1);
            auto ok = loadMov(slot, path);
            if(!ok) {
                ERR << "Could not load movie " << path << endl;
            } else {
                INFO << "/load - slot:" << slot << ", path:" << path << "\n";
            }
        }
        else if(addr == "/loadfolder") {
            if(numargs != 1) {
                ERR << "/loadfolder expects 1 arguments, got " << numargs << endl
                    << "    Syntax: /loadfolder path:string\n";
                continue;
            }
            // the name pattern is XXX_descr.ext, where XXX is the slot number.
            // right now we dont do anything with descr
            string path = msg.getArgAsString(0);
            bool ok = loadFolder(path);
            if(!ok) {
                ERR << "/loadfolder could not load some of the samples \n";
            }
        }
        else if(addr == "/dump") {
            /*
            if(numargs != 0) {
                ERR << "/dump expects 0 arguments, got " << numargs << endl;
                continue;
            }
            */
            this->dumpClipsInfo();
        }
        else if(addr == "/setspeed") {
            if(numargs != 1) {
                ERR << "/setspeed expects 1 arguments, got " << numargs << endl;
                ERR << "Syntax: /setspeed speed:float  (change the speed of the playing clip)";
                continue;
            }
            size_t slot = currentSlot();
            if(slot >= numSlots) {
                ERR << "/setspeed: invalid slot " << slot << endl;
                continue;
            }
            if(!loaded[slot]) {
                ERR << "/setspeed: slot "<< slot <<" not loaded\n";
                continue;
            }
            float speed = msg.getArgAsFloat(0);
            INFO << "/setspeed slot: " << slot << ", value: " << speed << endl;
            movs[slot].setSpeed(speed);
            speeds[slot] = speed;
        }
        else {
            ERR << "Message not recognized: " << addr << endl;
        }
    } // finished with OSC

    for(const auto &slot: stack) {
        auto &mov = movs[slot];
        if(mov.getIsMovieDone()) {
            mov.setPaused(true);
            // mov.stop();
            if( shouldStop[slot] ) {
                drawclip[slot] = false;
                // mov.setPosition(0.f);
            }
        }
        else if(mov.isPlaying()) {
            mov.update();
        }
    }

    size_t slot = currentSlot();
    if(oscOutPort != 0 and slot < numSlots) {
        ofxOscMessage msg;
        auto &mov = movs[slot];
        auto dur = durations[slot];
        auto time = mov.getPosition() * dur;
        if(time >= 0 && (
                lastOscMsg.getNumArgs() != 3 ||
                lastOscMsg.getAddress() != "/play" ||
                lastOscMsg.getArgAsInt(0) != slot ||
                abs(lastOscMsg.getArgAsFloat(1) - time) > 1e-7f)) {
            msg.setAddress("/play");
            msg.addIntArg(slot);
            msg.addFloatArg(time);
            msg.addFloatArg(dur);
            oscSender.sendMessage(msg);
            lastOscMsg = msg;
        }
    }

}

void ofApp::calculateDrawCoords() {
    size_t currSlot = stack.empty() ? numSlots+1 : stack[stack.size()-1];
    if(currSlot >= numSlots || !loaded[currSlot]) {
        draw_x0 = 0;
        draw_y0 = 0;
        draw_height = ofGetWindowHeight();
        draw_width = ofGetWindowWidth();
        return;
    }
    auto mov = &(movs[currSlot]);
    int windowWidth = ofGetWindowWidth();
    int windowHeight = ofGetWindowHeight();
    float movWidth = mov->getWidth();
    float movHeight = mov->getHeight();
    float wr = windowWidth / movWidth;
    float hr = windowHeight / movHeight;
    float draw_h, draw_w;
    if (hr < wr) {
        draw_h = windowHeight;
        draw_w = draw_h * (movWidth / movHeight);
        draw_x0 = (windowWidth - static_cast<int>(draw_w)) / 2;
        draw_y0 = 0;
    } else {
        draw_w = windowWidth;
        draw_h = draw_w * (movHeight / movWidth);
        draw_x0 = 0;
        draw_y0 = (windowHeight - static_cast<int>(draw_h)) / 2;
    }
    draw_height = int(draw_h);
    draw_width = int(draw_w);
    INFO << "height: " << draw_height << ", width: " << draw_width;
}

bool ofApp::loadFolder(const string &path) {
    // the name pattern is XXX_descr.ext, where XXX is the slot number.
    // right now we dont do anything with descr
    // returns true if ok, false if errors
    ofDirectory dir(path);
    dir.allowExt("mp4");
    dir.allowExt("mkv");
    dir.allowExt("mpg");
    dir.allowExt("avi");
    dir.allowExt("ogv");
    dir.allowExt("m4v");
    dir.allowExt("mov");
    //populate the directory object
    dir.listDir();

    for(size_t i = 0; i < dir.size(); i++){
        string filename = dir.getName(i);
        LOG << "loadFolder: loading " << filename << endl;
        auto delim = filename.find("_");
        // dont accept extremely long names
        if (delim >= 100) {
            ERR << "/loadfolder: filename should have the format XXX_descr.ext\n"
                << "   filename: " << filename << endl;
            return false;
        } else {
            int slot = std::stoi(filename.substr(0, delim));
            if(slot < 0 || slot >= numSlots) {
                ERR << "Slot out of range: " << slot << ", num slots: " << numSlots << endl
                    << "    filename: " << filename << endl;
                continue;
            }
            size_t idx = static_cast<size_t>(slot);
            if(loaded[idx]) {
                INFO << "/loadfolder: loading a clip in an already used slot\n"
                     << "    Slot: " << slot << endl
                     << "    New clip: " << filename << endl
                     << "    Previous clip: " << movs[idx].getMoviePath() << endl;
            }
            LOG << "loading slot: " << slot << ", path: " << dir.getPath(i);
            auto ok = this->loadMov(slot, dir.getPath(i));
            if(!ok) {
                ERR << "Could not load clip num " << i << " path: " << dir.getPath(i) << endl;
                return false;
            }
        }
    }
    return true;
}

//--------------------------------------------------------------
void ofApp::draw() {
    if(!stack.empty()) {
        size_t currSlot = stack[stack.size() - 1];
        auto &mov = movs[currSlot];
        if(drawclip[currSlot] && (mov.isPaused() || mov.isPlaying())) {
            mov.draw(draw_x0, draw_y0, draw_width, draw_height);
        }
    }
}

void ofApp::dumpClipsInfo() {
    cout << "Loaded Clips: \n";
    for(size_t i=0; i<numSlots; i++) {
        if(!loaded[i])
            continue;

        cout << "  * slot:" << i
             << ", path: " << movs[i].getMoviePath()
             << ", dur:" << movs[i].getDuration()
             << endl;
    }
}


void ofApp::sendClipInfo(ui32 idx, const string &host, int port) {
    if(!loaded[idx]) {
        ERR << "Slot " << idx << " not loaded\n";
        return;
    }
    LOG << "sendClipInfo slot: " << idx << ", " << host << ":" << port;
    ofxOscMessage msg;
    msg.setAddress("/clipinfo");
    msg.addIntArg(idx);
    msg.addStringArg(movs[idx].getMoviePath());
    msg.addFloatArg(movs[idx].getDuration());
    oscSender.sendMessage(msg);
}

void ofApp::sendClipsInfo() {
    if(this->oscOutPort == 0) {
        ERR << "sendClipsInfo: out osc port not set \n";
        return;
    }
    for(int i=0; i<numSlots; i++) {
        if(!loaded[i])
            continue;
        sendClipInfo(i, this->oscOutHost, this->oscOutPort);
    }
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key) {
    switch(key) {
    case 'f':
        ofToggleFullscreen();
        break;
    case 'q':
        ofExit();
        break;
    case 'd':
        this->dumpClipsInfo();
        break;
    default:
        LOG << "Unknown key: " << key;
    }

}

//--------------------------------------------------------------
void ofApp::keyReleased(int key) {

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){
    calculateDrawCoords();
}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){

}
