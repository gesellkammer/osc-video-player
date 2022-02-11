OSC Video Player

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

The clips can be played at any speed, stopped, paused, scrubbed, etc.

## OSC Api

NB: this information might be out of date. To print the current OSC api, do `OSC-Video-Player --man`

```
/load slot:int path:str
    * Load a video at the given slot

/play slot:int [speed:float=1] [starttime:float=0] [paused:int=0] [stopWhenFinished:int=1]
    * Play the given slot with given speed, starting at starttime (secs)

      paused: if 1, the playback will be paused
      stopWhenFinished: if 1, playback will stop at the end, otherwise it
        pauses at the last frame

/stop
    * Stop playback

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
```


## Command line

```

USAGE:

   OSC-Video-Player  [-o <string>] [-m] [-d] [-p <int>] [-f <string>]
                     [-n <int>] [--] [--version] [-h]

Where:

   -o <string>,  --oscout <string>
     OSC host:port to send information to

   -m,  --man
     Print manual and exit

   -d,  --debug
     debug mode

   -p <int>,  --port <int>
     OSC port to listen to (default: 30003)

   -f <string>,  --folder <string>
     Load all videos from this folder

   -n <int>,  --numslots <int>
     Max. number of video slots (default: 50)

   --,  --ignore_rest
     Ignores the rest of the labeled arguments following this flag.

   --version
     Displays version information and exits.

   -h,  --help
     Displays usage information and exits.

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

