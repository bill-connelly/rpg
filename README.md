## rpigratings - A High Performance Raspberry Pi Drifting Grating Generation 
                   and Display Module for Python 3, written in C.

## Build

    python3 setup.py build

## Run

    $ cd build/lib.linux-armv7l-3.4
    $ python3
    >>> import rpigratings as rg
    >>> #Create a grating file called "filename" with
    >>> #spacial and temporal freqs of 0.2 and 1 respectively
    >>> rg.draw_grating("filename",0.2,1,angle=30)
    >>> #Create a Screen instance
    >>> root = rg.Screen()
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