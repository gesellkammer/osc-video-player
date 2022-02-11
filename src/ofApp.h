#pragma once

#include "ofMain.h"
#include "ofxOsc.h"

#define PORT 30003

typedef unsigned int ui32;
typedef unsigned long ui64;

typedef struct {
    bool loaded;
    float speed;
    bool draw;
    bool paused;
    bool shouldstop;
    float duration;
    bool needsupdate;
} ClipStatus;

class ofApp : public ofBaseApp{

public:
    ofApp(size_t numslots_, int port_);
    void setup();
    void update();
    void draw();

    void keyPressed(int key);
    void keyReleased(int key);
    void mouseMoved(int x, int y );
    void mouseDragged(int x, int y, int button);
    void mousePressed(int x, int y, int button);
    void mouseReleased(int x, int y, int button);
    void mouseEntered(int x, int y);
    void mouseExited(int x, int y);
    void windowResized(int w, int h);
    void dragEvent(ofDragInfo dragInfo);
    void gotMessage(ofMessage msg);

    void calculateDrawCoords();
    void printOscApi();
    void printKeyboardShortcuts();
    bool playClip(size_t slot, float speed=1.f, float offset=0.f, bool paused=false,
                  bool stopWhenFinished=true, bool stopPrevious=true);
    /*
     *
     * float speed = numargs >= 2 ? msg.getArgAsFloat(1) : 1.0f;
            float skiptime = numargs >= 3 ? msg.getArgAsFloat(2) : 0.0f;
            int pausestatus = numargs >= 4 ? msg.getArgAsInt32(3) : 0;
            int stopWhenFinished = numargs >= 5 ? msg.getArgAsInt32(4) : 1;
            int stopPrevious = numargs >= 6 ? msg.getArgAsInt32(5) : 0;
     */
    bool loadMov(int slot, string const &path);
    bool loadFolder(string const &path);
    void dumpClipsInfo();

    size_t currentSlot() {
        if (stack.empty())
            return numSlots + 1;
        return stack[stack.size() -1];
    }

    void stopMov(size_t slot) {
        if(stack.size() == 0)
            return;
        // movs[slot].stop();
        movs[slot].setPaused(true);
        drawclip[slot] = false;

        if(slot == stack[stack.size()-1])
            stack.pop_back();
        else {
            auto idx = find(stack.begin(), stack.end(), slot);
            if(idx != stack.end())
                stack.erase(idx);
        }
        if(oscOutPort != 0) {
            ofxOscMessage msg;
            msg.setAddress("/stop");
            msg.addIntArg(slot);
            oscSender.sendMessage(msg);
            lastOscMsg = msg;
        }
    }

    void sendClipInfo(ui32 idx, const string &host, int port);
    void sendClipsInfo();

    vector <ofVideoPlayer> movs;
    vector <size_t> stack;
    vector <ClipStatus> clipstatus;
    vector<bool> loaded;
    vector<float> speeds;
    vector<bool> drawclip;
    vector<bool> paused;
    vector<bool> shouldStop;
    vector<bool> needsUpdate;
    vector<float> durations;
    size_t numSlots;
    int oscPort;
    bool debugging;
    int draw_x0, draw_y0, draw_width, draw_height;
    ofxOscMessage lastOscMsg;

    ui32 oscOutPort;
    string oscOutHost;

    ofxOscReceiver oscReceiver;
    ofxOscSender oscSender;

};
