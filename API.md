
# Table of Contents
## Functions
  - ### [rpg.build_grating()](#rpgbuild_gratingfilename-options)
  - ### [rpg.build_masked_grating()](#rpgbuild_masked_gratingfilename-options)
  - ### [rpg.build_gabor()](#rpgbuild_gaborfilename-options)
  - ### [rpg.build_list_of_gratings()](#rpgbuild_list_of_gratingsfunc_string-directory_path-options)
  - ### [rpg.convert_raw()](#rpgconvert_rawfilename-new_filename-n_frames-width-height-refreshes_per_frame)
## Classes
  - ### [rpg.Screen()](#rpgscreenresolution-background)
    * #### Methods
    * #### [load_grating()](#load_gratingfilename)
    *  #### [load_raw()](#load_rawfilename)
    *  #### [display_grating()](#display_gratinggrating-trigger_pin)
    *  #### [display_raw()](#display_rawraw-trigger_pin)
    *  #### [display_greyscale()](#display_greyscalecolor)
    *  #### [display_gratings_randomly()](#display_gratings_randomlydir_containing_gratings-intertrial_time-logfile_name)
    *  #### [display_raw_randomly()](#display_raw_randomlydir_containing_raws-intertrial_time-logfile_name)
    *  #### [display_rand_grating_on_pulse()](#display_rand_grating_on_pulsedir_containing_gratings-trigger_pin-logfile_name)
    *  #### [display_rand_raw_on_pulse()](#display_rand_raw_on_pulsedir_containing_raws-trigger_pin-logfile_name)
    *  #### [close()](#close)
    *  #### [\_print_log()](#_print_logfilename-file_type-file_displayed-perf)
    *  #### [\_randomize_list()](#_randomize_listself-list)

---

## rpg.build_grating(filename, options)

Create a raw animation file of a drifting grating. Saves file to hard disc. This file is then loaded with Screen.load_grating, and displayed with one of the Screen methods.

* Parameters:

  * filename (string) - The filename and path to file. ~/dir/filename will generate a file called filename in a directory dir in the users home directory. Can be specified as relative to home with \~ or absolute paths
  * options (dict) -  A dictionary with the required keys of, duration, angle, spac_freq, and temp_freq, which can be created as follows:

        options = {  
            "duration": 2,     #2 second long grating  
            "angle": 90,       #90 degree gratings  
            "spac_freq": 0.1,  #spatial frequency in cycles per degree  
            "temp_freq": 0.5,  #temporal frequency in cycles per second  
        }  
    
  * optional options are:
  
            "contrast": 1      #maximum contrast  
            "background": 127,   #  
            "resolution": (1280, 720)   #resolution of gratings. Must match Screen()  
            "waveform": rpg.SINE #rpg.SQUARE (square wave) or rpg.SINE (sine wave)
            "colormode": 16        #bits per pixel, must be 16 or 24  
* Returns:
  * None


## rpg.build_masked_grating(filename, options)

Create a raw animation file of a drifting grating with a circular mask. Saves file to hard disc. This file is then loaded with Screen.load_grating, and displayed with one of the Screen methods.

* Parameters:
  * filename (string) - The filename and path to file. ~/dir/filename will generate a file called filename in a directory dir in the users home directory.
  * options (dict) - A dictionary with the required keys of: duration, angle, spac_freq and temp_freq. The user will also want to set the keys of percent_diameter, percent_center_top, percent_center_left and percent_padding which can be created as follows:

        options = {  
          "duration": 2,     #2 second long grating  
          "angle": 90,       #90 degree gratings, 
          "spac_freq": 0.1,  #spatial frequency in cycles per degree  
          "temp_freq": 0.5,  #temporal frequency in cycles per second 
          "percent_diameter": 50,    #diameter of mask as percentage of screen width  
          "percent_center_left": 50, #horizontal center of mask as percentage of screen width  
          "percent_ceter_top": 50,   #vertical center of mask as percentage of screen height  
        }  
    
  * optional options are:  
  
        "percent_padding", 10      #specify whether the edge of mask blurs to background value over what distance  
        "contrast": 1,     #maximum contrast  
        "background": 127,   #  
        "resolution": (1280, 720)   #resolution of gratings. Must match Screen()  
        "waveform": rpg.SINE #rpg.SQUARE (square wave) or rpg.SINE (sine wave)
        "colormode": 16        #bits per pixel, must be 16 or 24  

* Returns:  
  * None


## rpg.build_gabor(filename, options):

Create a raw animation file of a drifting gabor patch. Saves file to hard disc. This file is then loaded with Screen.load_grating, and displayed with one of the Screen methods.

* Parameters:
  * filename (string) - The filename and path to file. ~/dir/filename will generate a file called filename in a directory dir in the users home directory.
  * options (dict) - A dictionary with the required keys of: duration, angle, spac_freq, and temp_freq. The user will also want to set the keys of sigma, percent_center_top, percent_center_left and percent_padding which can be created as follows:
  
        options = {  
          "duration": 2,     #2 second long grating  
          "angle": 90,       #90 degree gratings  
          "spac_freq": 0.1,  #spatial frequency in cycles per degree  
          "temp_freq": 0.5,  #temporal frequency in cycles per second  
          "percent_sigma": 10,       #'standard deviation' of the gaussian envelope of the gabor function as a percentage of screen width  
          "percent_center_left": 50, #horizontal center of mask as percentage of screen width  
          "percent_ceter_top": 50,   #vertical center of mask as percentage of screen height  
        }    
        
  * optional options are: 
  
        "contrast": 1,     #maximum contrast  
        "background": 127,   #shade of the background   
        "resolution": (1280, 720)   #resolution of gratings. Must match Screen()  
        "waveform": rpg.SINE #rpg.SQUARE is not allowed for gabor
        "colormode": 16        #bits per pixel, must be 16 or 24  

* Returns:  
    * None

## rpg.build_list_of_gratings(func_string, directory_path, options):

Builds a range of gratings varying over one property. One of the options supplied can be a list, and the function will iterate over that list building gratings matching each element of this list

* Parameters:
  * func_string (string) - String matching either "grating", "mask" or "gabor", to produce full screen gratings, gratings with a circular mask, or gabors, respectively  
  * directory_path (string) - An absolute or relative path to the directory where where the above files will be saved. Most likely, each set of gratings generated with this function will be saved in their own directory so can be displayed with the Screen.display_rand_grating_on_pulse()  
  * options: A dictionary containing options, see build_grating(), build_masked_grating() or build_gabor() for appropriate options, but note one of the options must be in the form of a list, e.g. `options["angle"] = [0, 30, 60, 90, 120, 150, 180, 210, 240, 270, 300, 330]` will create the typical 12 orientation set of stimuli

* Returns:
  * None

Files will be saved with names matching the element of the list they are generated from. e.g. if generated with options["angle"] = [0 45 90], then there will be three files generated with names "0", "45" and "90" in the directory specificied in  directory path.  

## rpg.convert_raw(filename, new_filename, n_frames, width, height, refreshes_per_frame)

Converts a raw video/image file saves as uint8: RGBRGBRGB... starting in the top left pixel and proceeding rowwise, into a form readily displayed by RPG.

* Parameters
  * filename (string) - The exact path to the raw file, either as relative or absolute e.g. "~/videos/raw1.raw".
  * new_filename (string) - The exact path of the converted file to be produced e.g. "~/raws/raw_converted.raw".
  * n_frames (int) - The number of frames in the raw video/image. Do not use to shorten movies. Only limits how long the movie is played. Not how many frames are converted.
  * width (int) - The width of the original file in pixels. Cannot be used to resize images/movie
  * height (int) - The height of the original file in pixels. Cannot be used to resize image/movie
  * refreshes_per_frame (int) - The number of monitor refreshes to display each frame for. For a movie to display at 30 frames per second, on a 60 Hz monitor, this would be 2. On a 75 Hz monitor, 25 frames per second would be acheived by setting this to 3. If a still image is displayed, if you require it displayed for X seconds, and your monitor refresh rate is R Hz, then this value should be set to X * R.
  * colormode (int) - THe number of bits per pixel, 16 or 24. Defaults to 16.

* Returns:
  * None
  
---

# rpg.Screen(resolution, background)

A class encapsulating the raspberry pi's framebuffer, with methods to display animations gratings and solid shades to the screen.  
 
ONLY ONE INSTANCE OF THIS OBJECT SHOULD EXIST AT ANY ONE TIME. Otherwise both objects will be attempting to manipulate the memory assosiated with the linux framebuffer. If a resolution change is desired first clean up the old instance of this class with the close() method and then create the new instance, or del the first instance. The resolution of this screen object does NOT need to match the actual resolution of the physical display; the linux framebuffer device is automatically scaled up to fit the physical display. The resolution of this object MUST match the resolution of any  animation files ; if the resolution of the animation is smaller pixels will simply be misaligned, but if the animation is larger then attempting to play it will cause a  segmentation fault.

* Parameters:
  * resolution (int tuple) - Defaults to (1280,720). a tuple of the desired width of the display  resolution as (width, height).  
  * background (int) - Defaults to 127. value between 0 and 255 for the background. This is the shade that will display between animations and will NOT change the background color of any animation while it plays. 
  * colormode (int) - THe number of bits per pixel, 16 or 24. Defaults to 16.  

* Returns:
  * Screen object
  
## Methods
### load_grating(filename)

Load a grating file called filename into  memory. Once loaded in this way, display_grating() can be called to display the loaded file to the screen.

* Parameters:  
  * filename (string) - string containing the exact filename, either as an absolute  or relative, e.g. "~/gratings/grat1.dat" or "home/pi/grating/grat1.dat"

* Returns:
  * Grating object

### load_raw(filename)

Load a raw file into memory. Once loaded in this way, the returned  object can be displayed with display_raw()

* Parameters:  
  * filename: string containint the exact filename, either as an absolute or relative path, e.g. "~/raws/raw1.dat" or "home/pi/raws/raw1.dat"

* Returns:
  * Raw object
  
### display_grating(grating, trigger_pin):

Display the passed grating object (grating objects are loaded with the Screen.load_grating method) either as soon as possible or in response to a 3.3V trigger. Returns a namedtuple (from the collections module) with the fields mean_interframe, stddev_interframe and start_time; these refer  respectively to the average interframe time in microseconds, the standard deviation of the interframe time and grating began to play in Unix Time, respectively.

* Parameters:
  * grating (grating object) - a grating objected loaded with Screen.load_grating()
  * trigger_pin (int) - Deaults to 0. Set to 0 to display gratting as soon as possible or set to the GPIO pin (as defined by wiringPi) to wait for a trigger signal.  Trigger pin cannot be set to 1, as this is reserved for feedback. Note: digital signal is 3.3 volts max, not 5 volt TTL. 5 volt signals risk permanently damaging the raspberry pi.

* Returns:
  * Performance record as a named tuple with the fields fields mean_interframe, stddev_interframe and start_time.

 ### display_raw(raw, trigger_pin):
 
Displays the passed raw object (raw objects are loaded with the Screen.load_raw method) either as soon as possible, or in response to 3.3V trigger. Returns a namedtuple (from the collections module) with the fields mean_interframe, stddev_interframe and start_time; these refer  respectively to the average interframe time in microseconds, the standard deviation of the interframe time and grating began to play in Unix Time, respectively.

* Parameters:
  * raw (raw object) - a raw object loaded with Screen.load_raw()
  * trigger_pin (int) - Deaults to 0. Set to 0 to display raw as soon as possible or set to the GPIO pin (as defined by wiringPi) to wait for a trigger signal. Trigger pin cannot be set to 1, as this is reserved for feedback. Note: digital signal is 3.3 volts max, not 5 volt TTL. 5 volt signals risk permanently damaging the raspberry pi.

* Returns:
  * Performance record as named tuple with the fields fields mean_interframe, stddev_interframe and start_time.
  
### display_greyscale(color):
 
Fill the screen with a solid color until something else is displayed to the screen. 

* Parameters:
  * color (int) - Value between 0 and 255.
        
* Returns:
  * None

### display_gratings_randomly(dir_containing_gratings, intertrial_time, logfile_name):

Attempts to display each file in the directory `dir_containing_gratings` as a grating. The order of display is a fixed but psudorandomised, i.e. the order will be the same every trial. See `_randomize_list` for details.

The location of the log file is `~/rpg/logs/` . Gratings are seperated by intertrial_time seconds of the background color set when Screen object created.

This method is blocking, and will not return until all files in directory displayed.

* Parameters
  * dir_containing_gratings (string) - A relative or absolute directory path to a directory containing gratings. Must not contain any other non grating files, or sub directories.
  * intertrial_time (float) - Time between gratings in seconds. Will have <1 millisecond accuracy.
  * logfile_name (string) - Defaults to `"rpglog.txt"`. Name of log file to write performance record to. Written into directory `~/rpg/logs/`.

* Returns:
  * None
  
  
### display_raw_randomly(dir_containing_raws, intertrial_time, logfile_name):

Attempts to display each file in the directory `dir_containing_raws` as a raw. The order of display is a fixed but psudorandomised, i.e. the order will be the same every trial. See `_randomize_list` for details.

The location of the log file is `~/rpg/logs/` . Raws are seperated by intertrial_time seconds of the background color set when Screen object created.

This method is blocking, and will not return until all files in directory displayed.

* Parameters:
  * dir_containing_rawss (string) - A relative or absolute directory path to a directory containing raws. Must not contain any other non raw files, or sub directories.
  * intertrial_time (float) - Time between raws in seconds. Will have <1 millisecond accuracy.
  * logfile_name (string) - Defaults to `"rpglog.txt"`. Name of log file to write performance record to. Written into directory `~/rpg/logs/`.

* Returns:
  * None
  
 ###  display_rand_grating_on_pulse(dir_containing_gratings, trigger_pin, logfile_name):

Displays a psudorandom grating from the passed directory in response to a 3.3V signal to a GPIO pin. Gauranteed to display each grating in directory before playing gratings again. Will display gratings in a fixed order across sessions. Between gratings, displays Screen.background() shade. Function is blocking, but will return in response to a keystroke.

* Paramaterss:
  * dir_containing_gratings (string) - A relative or absolute directory path to a directory containing gratings. Must not contain any other non grating files, or sub directories.
  * trigger_pin (int) - Which trigger pin the raspberry pi listens on for the 3.3V pulse. Pin number is defined by WiringPi library.
  * logfile_name (string) - Defaults to `"rpglog.txt"`. Name of log file to write performance record to. Written into directory `~/rpg/logs/`.

* Returns:
  * None

### display_rand_raw_on_pulse(dir_containing_raws, trigger_pin, logfile_name):

Displays a psudorandom raw from the passed directory in response to a 3.3V signal to a GPIO pin. Gauranteed to display each raw in directory before playing gratings again. Will display raws in a fixed order across sessions. Between raws, displays Screen.background() shade. Function is blocking, but will return in response to a keystroke.

* Paramaters:
  * dir_containing_raws (string) - A relative or absolute directory path to a directory containing raws. Must not contain any other non grating files, or sub directories.
  * trigger_pin (int) - Which trigger pin the raspberry pi listens on for the 3.3V pulse. Pin number is defined by WiringPi library.
  * logfile_name (string) - Defaults to `"rpglog.txt"`. Name of log file to write performance record to. Written into directory ~/rpg/logs/

* Returns:
  * None
  
### close():

Destroy the screen object, cleaning up its memory and restoring previous screen settings. Only necessary to be called if you are creating a new screen object within the same Python session, for instance if switching between resolutions.

* Parameters:
  * None
  
* Returns:
  * None  
  
### \_print_log(filename, file_type, file_displayed, perf):

Internal function for print log file. Unlikely to be called unless you are using the `display_grating()` method directly and want to record its performance.
 
* Parameters:
  * filename (string) - Filename within `"~/rpg/logs/"` to write data to.
  * file_type (string) - Annotation to the log, typically either `"raw"` or `"grating"`.
  * perf (named tuple) -  The named tuple returned by `display_grating()` or `display_raw()`

* Returns:
  * None
  
### _randomize_list(self, list):
    
Internal function for psudorandomizing gratings paths. Files paths are hashed with MD5 to generate a string, which is then sorted to give order. This achieves a psuedo random order that is fixed across sessions.

If you require this to be truely random on every trial simply replace the contents of this method with:

      import random
      return random.shuffle(list)
      
And then reinstall rpg.

* Parameters:
  * list (list) - A list containing grating/raw path names, or any strings

* Returns:
  * A list of with the same elements as that passed in, shuffled, but in an order that is fixed between sessions
