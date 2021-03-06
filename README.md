# OSC Video Player

The video playback is organized in slots. Each slot can load a specific video file.
and there is only one slot visible at any time. Playback is organized following the
paradigm of a stack: if slot 1 is playing and playback for slot 2 is
triggered, slot 2 starts playing and takes over the display. If slot 1 keeps playing
and slot 2 is stopped, the display returns to slot 1.

Every aspect of playback (start, stop, speed, position) can be controlled via 
OSC. Scripts can also be loaded via OSC. Alternatively a folder can be loaded
either via OSC or from the command line. In that case, clips in the folder must
be organized as shown below, where each clip has a prefix indicating the slot
where it should be loaded.

## Load all clips from a folder

```
media/
  001_cat.mp4
  002_dog.mp4
  004_brid.mp4
```

``` bash

OSC-Video-Player --folder $(realpath media)

```

This will load each clip to the specified slot (1, 2, 4 in this case)


## OSC Api

NB: this information might be out of date. To print the current OSC api, do `OSC-Video-Player --man`

```
OSC messages accepted:

/load slot:int path:str
    * Load a video at the given slot. The path must be absolute

/play slot:int [speed:float=1] [starttime:float=0] [paused:int=0] [stopWhenFinished:int=1]
    * Play the given slot with given speed, starting at starttime (secs)

      paused: if 1, the playback will be paused
      stopWhenFinished: if 1, playback will stop at the end, otherwise it
        pauses at the last frame

/stop [slot:int]
    * Stop playback. If no slot is given, the currently playing slot is stopped

/pause state:int
    * If state 1, pause playback, 0 resumes playback

/setspeed speed:float
    * Change the speed of the playing slot

/scrub pos:float [slot:int=current]
    * Set the relative position 0-1. Sets the given slot as the current slot
      In scrub mode, video is paused and must be driven externally

/scrubabs timepos:float [slot:int=current]
    * Set the absolute time and activates the given slot. In scrub mode, video
      is paused and must be driven externally

/setpos
     * Sets the relative (0-1) playing position of the current clip
       (does not pause the clip like /scrub)

/settime
     * Sets the absolute playing position of the current clip
       (does not pause the clip like /scrubabs)

/dump
    * Dump information about loaded clips

/quit
    * Quit this application


Keyboard Shortcuts
==================

F - Toggle Fullscreen
D - Dump Information about loaded clips
Q - Quit

```


## Command line

```

USAGE:

   bin/OSC-Video-Player  [-r <int>] [-o <string>] [-m] [-d] [-p <int>] [-f
                         <string>] [-n <int>] [--] [--version] [-h]


Where:

   -r <int>,  --framerate <int>
     Frame Rate

   -o <string>,  --oscout <string>
     [host:]port (for ex. '127.0.0.1:9998', or simply '9998') to send
     information to

   -m,  --man
     Print manual and exit

   -d,  --debug
     debug mode

   -p <int>,  --port <int>
     OSC port to listen to

   -f <string>,  --folder <string>
     Load all videos from this folder

   -n <int>,  --numslots <int>
     Max. number of video slots

   --,  --ignore_rest
     Ignores the rest of the labeled arguments following this flag.

   --version
     Displays version information and exits.

   -h,  --help
     Displays usage information and exits.


   OSC Video Player
```

## Build

Given an installation of openFrameworks into folder `OF`, put this project into `OF/apps/myApps`

### Unix Make

```bash
make
```

### QtCreator

* edit OSC-Video-Player.qbs and set `of_root` to `OF` (give an absolute path)
* set the import path of ofApp to `OF/libs/openFramworksCompiled/project/qtcreator/ofApp.qbs`

