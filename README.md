# RPG
## A high performance python library for displaying drifting gratings on the
Raspberry Pi 3.

## Download
Move to your home directory, and clone the repository

```
    $ cd ~
    $ git clone https://github.com/bill-connelly/rpg
```
    
## Configure
Before installation you need to set the degrees of visual angle your monitor convers. This is a simple edit of a single line of code.

```
    $ cd rpg
    $ nano rpg/_rpigratings.c
```
        
On line 28 the code will say `#define DEGREES_SUBTENDED 80` change the number `80` to however many degrees your monitor covers. Then press `cntrl-x` to exit `y` to save, and press enter to accept file name.

## Install

Confirm you are in the rpg directory (i.e. the directory with setup.py) and install
```
    $ cd ~/rpg
    $ pip3 install .
```
## Run

RPG is designed to run in response to 3.3 volt triggers from other hardware, or in a free running mode, where it will provide a 3.3V output when it displays each frame. There are several examples scripts in the examples folder. But briefly, to confirm that RPG has been install successfully, and to show it's functionality, the following lines of code can be run. 

First open python

```
    $ python3
```
Then import the module
```    
    >>> import rgp
```
Now we need to write some drifting gratings to disk. These can either be fullscreen, in a circular mask or as gabor patches. In this example, we will create a full screen gratings. First we need to create a dictionary to store the parameters we intend to use.
```
    >>> options = {"duration": 2, "angle": 45, "spac_freq": 0.2, "temp_freq": 1}
```

Now we will write the grating to disk, aka, *build* the grating. This grating will have the default resolution of 1280x720, but many other resolutions are possible. To build a grating like this, the directory it is being written to must already exist, so we will write it into the home directory.
```
    >>> rpg.build_grating("~/first_grating.dat", options)
```
You will receive some feedback. Despite requesting a temporal frequency of 1 cycle per second (the last option in the dictionary), you may be told that the temporal frequency of the file not quite what was requested. This change was performed in order to optimize file size and memory usuage and because the wavefront must proceed at an integer number of pixels per frame.

Now we will create a Screen object. This object contains the methods to load and display gratings, and represents video drivers of the raspberry pi and some display options. We will use the default resolution of 1280x720. The resolution of the Screen object must match the resolutions of the gratings being displayed.
```
    >>> myscreen = rpg.Screen()
```
Now we will load the the grating built earlier. This command returns a grating object.
```
    >>> grating = myscreen.load_grating("~/first_grating.dat")    


load the grating into memory to allow rapid display
```
    >>> 
  

    >>> #Create a grating file called "filename" with
    >>> #spacial and temporal freqs of 0.2 and 1 respectively
    >>> rpg.build_grating("filename",0.2,1,angle=30,resolution=())
    >>> #Create a Screen instance
    >>> root = rpg.Screen()
    >>> #load our "filename" grating into memory
    >>> root.load_grating("filename")
    >>> #display the grating
    >>> root.display_grating(root, grating)
    >>> #unload the grating to free up memory
    >>> root.unload_grating()
    >>> #finally, restore the display to previous settings
    >>> root.close()



Tested on Raspian GNU/Linux 8, Python *3.4.2*.

## Troubleshooting

!STATIONARY GRATINGS!
    Low resolution combined with low propogation speeds (low temporal frequency or
    high spacial frequency) and high FPS may result in unmoving gratings, because the
    per-frame propogation speed is rounded to the nearest integer number of pixels
    (such that propogation is even each frame), and this speed may be rounded down to zero.

 !MEMORY MANAGEMENT!
    In order to prevent dynamic handling of animations such that high frame rates
    can be maintained, the size of the grating files in memory is high. Higher resolutions
    will still be displayed at 60FPS, but may take several seconds to load into memory.
