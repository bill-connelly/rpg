"""
A module for making and displaying drifting gratings to the 
screen of a raspberry pi (with the default NOOBS installation
of raspbian) while bypassing the windowing system.

It is best to run this module from the fullscreen terminal,
accessed with Ctrl+Alt+F1 (Ctrl+Alt+F7 returns to the desktop)
because otherwise the X windowing system and this module will
both be attempting to configure the framebuffer.

This module is an object-oriented wrapper for the 
_rpigratings module, which is implemented in C for 
performance.

"""
import time as t
import os
import sys
import hashlib
from collections import namedtuple

GratPerfRec = namedtuple("GratingPerformanceRecord",["mean_interframe","stddev_interframe","start_time"])

GRAY = 127
BLACK = 0
WHITE = 255
SINE = 1
SQUARE = 0

import _rpigratings as rpigratings



def build_grating(filename, options):

    """
    Create a raw animation file of a drifting grating. Saves file to hard disc.
    This file is then loaded with Screen.load_grating, and displayed with one
    of the Screen methods.

    Args:
      filename: The filename and path to file. ~/dir/filename will
        generate a file called filename in a directory dir in the users
        home directory.
      options: A dictionary with the required keys of, duration, angle,
        spac_freq, and temp_freq, which can be created as follows:
        options = {
          "duration": 2,     #2 second long grating
          "angle": 90,       #90 degree gratings,
          "spac_freq": 0.1,  #spatial frequency in cycles per degree
          "temp_freq": 0.5,  #temporal frequency in cycles per second
        }
        optional options are:
          "contrast": 1      #maximum contrast

          "background": 127,   #
          "resolution": (1280, 720)   #resolution of gratings. Must match Screen()
          "waveform": rpg.SINE #rpg.SQUARE (square wave) or rpg.SINE (sine wave)

    For smooth propogation of the grating, the pixels-per-frame speed
    is truncated to the nearest interger; low resolutions combined with
    a low temporal frequency:spacial frequency ratio may result in incorrect
    speeds of propogation or even static, unmoving gratings. This also means
    that the temp_freq is approximate only.

    Returns:
      Nothing
    """

    options = _parse_options(options)

    filename = os.path.expanduser(filename)
    rpigratings.build_grating(filename, options["duration"], options["angle"],
                              options["spac_freq"], options["temp_freq"],
                              options["contrast"], options["background"],
                              options["resolution"][0], options["resolution"][1],
                              options["waveform"], 0, 0, 0, 0, 0)

def build_masked_grating(filename, options):
    """
    Create a raw animation file of a drifting grating with a circular mask.
    Saves file to hard disc. This file is then loaded with Screen.load_grating,
    and displayed with one    of the Screen methods.

    Args:
      filename: The filename and path to file. ~/dir/filename will
        generate a file called filename in a directory dir in the users
        home directory.
      options: A dictionary with the required keys of: duration, angle,
        spac_freq and temp_freq. The user will also want to set the keys
        of percent_diameter, percent_center_top, percent_center_left
        and percent_padding which can be created as follows:
        options = {
          "duration": 2,     #2 second long grating
          "angle": 90,       #90 degree gratings,
          "spac_freq": 0.1,  #spatial frequency in cycles per degree
          "temp_freq": 0.5,  #temporal frequency in cycles per second
          "percent_diameter": 50,    #diameter of mask as percentage of screen width
          "percent_center_left": 50, #horizontal center of mask as percentage of screen width
          "percent_ceter_top": 50,   #vertical center of mask as percentage of screen height
        }
        optional options are:
          "percent_padding", 10      #specify whether the edge of mask blurs to background value over what distance
          "contrast": 1,     #maximum contrast
          "background": 127,   #
          "resolution": (1280, 720)   #resolution of gratings. Must match Screen()
          "waveform": rpg.SINE #rpg.SQUARE (square wave) or rpg.SINE (sine wave)

    Returns:
      Nothing.
    """

    options = _parse_options(options)

    filename = os.path.expanduser(filename)
    rpigratings.build_grating(filename, options["duration"], options["angle"],
                              options["spac_freq"], options["temp_freq"],
                              options["contrast"], options["background"],
                              options["resolution"][0], options["resolution"][1],
                              options["waveform"], 0, options["percent_diameter"],
                              options["percent_center_left"], options["percent_center_top"],
                              options["percent_padding"])

def build_gabor(filename, options):
    """
    Create a raw animation file of a drifting gabor patch. Saves file to hard disc.
    This file is then loaded with Screen.load_grating, and displayed with one
    of the Screen methods.

    Args:
      filename: The filename and path to file. ~/dir/filename will
        generate a file called filename in a directory dir in the users
        home directory.
      options: A dictionary with the required keys of: duration, angle,
        spac_freq, and temp_freq. The user will also want to set the keys of
        sigma, percent_center_top, percent_center_left and percent_padding
        which can be created as follows:
        options = {
          "duration": 2,     #2 second long grating
          "angle": 90,       #90 degree gratings,
          "spac_freq": 0.1,  #spatial frequency in cycles per degree
          "temp_freq": 0.5,  #temporal frequency in cycles per second
          "percent_sigma": 10,       #'standard deviation' of the gaussian envelope of the gabor function
                                     #as a percentage of screen width
          "percent_center_left": 50, #horizontal center of mask as percentage of screen width
          "percent_ceter_top": 50,   #vertical center of mask as percentage of screen height
        }
        optional options are:
          "contrast": 1,     #maximum contrast
          "background": 127,   #
          "resolution": (1280, 720)   #resolution of gratings. Must match Screen()
          "waveform": rpg.SINE #rpg.SQUARE (square wave) or rpg.SINE (sine wave)

    Returns:
      Nothing.
    """
    options = _parse_options(options)

    filename = os.path.expanduser(filename)
    rpigratings.build_grating(filename, options["duration"], options["angle"],
                              options["spac_freq"], options["temp_freq"],
                              options["contrast"], options["background"],
                              options["resolution"][0], options["resolution"][1],
                              options["waveform"], options["percent_sigma"], 0,
                              options["percent_center_left"], options["percent_center_top"],
                              0)



def build_list_of_gratings(func_string, directory_path, options):

    """
    Builds a range of gratings varying over one property. One of the options
    supplied can be a list, and the function will iterate over that list building
    gratings matching each element of this list

    Args:
      func_string: String matching either "grating", "mask" or "gabor", to produce
        full screen gratings, gratings with a circular mask, or gabors, respectively
      directory_path: An absolute or relative path to the directory where
        where the above files will be saved. Most likely, each set of gratings
        generated with this function will be saved in their own directory so
        can be displayed with the Screen.display_rand_grating_on_pulse()
      options: A dictionary containing options, see build_grating(),
        build_masked_grating() or build_gabor() for appropriate options, but note
        one of the options must be in the form of a list.

    Files will be saved with names matching the element of the list they are generated
    from. e.g. if generated with options["angle"] = [0 45 90], then there will be three
    files generated with names "0", "45" and "90" in the directory specificied in 
    directory path.

    Returns:
      Nothing
    """

    if func_string == "grating":
        func = build_grating
    elif func_string == "mask":
        func = build_masked_grating
    elif func_string == "gabor":
        func = build_gabor
    else:
        raise ValueError("func_string must be either 'grating', 'mask' or 'gabor', not %s" %func_string)

    path_to_directory = os.path.expanduser(directory_path)
    cwd = os.getcwd()
    os.makedirs(path_to_directory)
    os.chdir(path_to_directory)

    iterable = [];
    for key, value in options.items():
        if isinstance(value, list):
            iterable.append(key)

    if len(iterable) > 1:
        raise ValueError("Supply only a single list as an option, rather than %s" %iterable)

    if len(iterable) == 0:
        raise ValueError("Supply one option as a list")

    for val in options[iterable[0]]:
        options_copy = options.copy()
        options_copy[iterable[0]] = val
        func(str(val), options_copy)

    os.chdir(cwd)

def convert_raw(filename, new_filename, n_frames, width, height, refreshes_per_frame):
    """
    Converts a raw video/image file saves as uint8: RGBRGBRGB... starting
      in the top left pixel and proceeding rowwise, into a form readily 
      displayed by RPG.

    Args:
      filename: the exact path to the raw file, either as relative or absolute
        e.g. "~/videos/raw1.raw".
      new_filename: the exact path of the converted file to be produced e.g.
        "~/raws/raw1.raw".
      n_frames: the number of frames in the raw video/image
      width: the width of the original file in pixels
      height: the height of the original file in pixels
      refreshes_per_frame: the number of monitor refreshes to display each frame for.
        For a movie to display at 30 frames per second, on a 60 Hz monitor, this would
        be 2. On a 75 Hz monitor, 25 frames per second would be acheived by setting this
        to 3. If a still image is displayed, if you require it displayed for X seconds,
        and your monitor refresh rate is R Hz, then this value should be set to X * R.

    Returns:
      None
    """

    filename = os.path.expanduser(filename)
    new_filename = os.path.expanduser(new_filename)
    rpigratings.convertraw(filename, new_filename, n_frames, width, height, refreshes_per_frame)


class Screen:
    def __init__(self, resolution=(1280,720), background = 127):
        """
        A class encapsulating the raspberry pi's framebuffer,
          with methods to display drifting gratings and solid colors to
          the screen.
 
        ONLY ONE INSTANCE OF THIS OBJECT SHOULD EXIST AT ANY ONE TIME.
          Otherwise both objects will be attempting to manipulate the memory
          assosiated with the linux framebuffer. If a resolution change is desired
          first clean up the old instance of this class with the close() method
          and then create the new instance, or del the first instance.

        The resolution of this screen object does NOT need to match the actual
          resolution of the physical display; the linux framebuffer device is
          automatically scaled up to fit the physical display.

        The resolution of this object MUST match the resolution of any
          grating animation files created by draw_grating; if the resolution
          of the animation is smaller pixels will simply be misaligned, but
          if the animation is larger then attempting to play it will cause a
          segmentation fault.

        Args:
          resolution: a tuple of the desired width of the display
            resolution as (width, height). Defaults to (1280,720).
          background: value between0 and 255 for the background 
         """
        if (background < 0 or background > 255):
                raise ValueError("Background must be between 0 and 255")

        self.background = background
        self.capsule = rpigratings.init(resolution[0],resolution[1])


    def load_grating(self,filename):
        """
        Load a grating file called filename into local memory. Once loaded
        in this way, display_grating() can be called to display the loaded file
        to the screen.

        Args:
          filename: string containing the exact filename, either as an absolute
            or relative, e.g. "~/gratings/grat1.dat" or "home/pi/grating/grat1.dat"
        Returns:
          Grating object
        """
        filename = os.path.expanduser(filename)
        return Grating(self,filename)

    def load_raw(self, filename):
        """
        Load a raw file into local memory. Once loaded in this way, the returned
        object can be displayed with display_raw()

        Args:
          filename: string containint the exact filename, either as an absolute
            or relative path.
        Returns:
          Raw object
        """

        filename = os.path.expanduser(filename)
        return Raw(self, filename)

    def display_grating(self, grating, trigger_pin = 0):
        """
        Display the passed grating object (grating files are created with
        the draw_grating function and loaded with the Screen.load_grating
        method). 

        Returns a namedtuple (from the collections module) with the fields
        mean_interframe, stddev_interframe and start_time; these refer
        respectively to the average interframe time in microseconds, the standard
        deviation of the interframe time and grating began to play in Unix Time,
        respectively.

        Args:
          grating: a grating objected loaded with Screen.load_grating()
          trigger_pin: set to 0 to display gratting as soon as possible or set to
            the GPIO pin (as defined by wiringPi) to wait for a trigger signal.
            Note: digital signal is 3.3 volts max, not 5 volt TTL. 5 volt signals
            risk permanently damaging the raspberry pi.

        Returns:
          performance record as a named tuple.
        """
        if trigger_pin == 1:
                raise ValueError("trigger_pin cannot be set to 1. This pin is reserved for feedback")


        rawtuple = rpigratings.display_grating(self.capsule, grating.capsule, trigger_pin)
        if rawtuple is None:
                return None
        else:
                return GratPerfRec(*rawtuple)

    def display_raw(self, raw, trigger_pin = 0):
        """
        Displays the passed raw object (raw objects are loaded with the 
        Screen.load_raw method) either as soon as possible, or in response
        to 3.3V trigger.

        Args:
          raw: a raw object loaded with Screen.load_raw()
          trigger_pin: set to 0 to display raw as soon as possible or set to
            the GPIO pin (as defined by wiringPi) to wait for a trigger signal.
            Note: digital signal is 3.3 volts max, not 5 volt TTL. 5 volt signals
            risk permanently damaging the raspberry pi.

        Returns:
          Performance record as named tuple.
        """

        if trigger_pin == 1:
                raise ValueError("trigger_pin cannot be set to 1. This pin is reserved for feedback")

        rawtuple = rpigratings.display_raw(self.capsule, raw.capsule, trigger_pin)
        if rawtuple is None:
                return None
        else:
                return GratPerfRec(*rawtuple)

    def display_greyscale(self,color):
        """
        Fill the screen with a solid color until something else is
                displayed to the screen. Calling display_color(GRAY) will
                display mid-gray.

        Args:
          color: Value between 0 and 255.

        Returns:
          None
        """
        if (color<0 or color>255):
                self.close()
                raise ValueError("Color must be between each between 0 and 255.")
        rpigratings.display_color(self.capsule,color,color,color)

    def display_gratings_randomly(self, dir_containing_gratings, intertrial_time, logfile_name="rpglog.txt"):
        """
        For each file in directory dir_containing_gratings, attempt to display
        that file as a grating. The order of display is a fixed but psudorandomised.
        The location of the log file is ~/rpg/logs/ . Gratings are seperated
        by intertrial_time seconds of the background color set when Screen
        object created.

        Method is blocking, and will not return until all files in directory
        displayed


        Args:
          dir_containing_gratings: A relative or absolute directory path
            to a directory containing gratings. Must not contain any other 
            non grating files, or sub directories.
          intertrial_time: Time between gratings in seconds. Will have 
            ~1 millisecond accuracy
          logfile_name: Name of log file to write performance record to.
            written into directory ~/rpg/logs/

        Returns:
          None
        """

        dir_containing_gratings = os.path.expanduser(dir_containing_gratings)
        path_to_gratings = [dir_containing_gratings + "/" + file for file in os.listdir(dir_containing_gratings)]
        randomized_path = self._randomize_list(path_to_gratings)

        gratings = []
        print("Loading gratings...")
        for file in randomized_path:
            gratings.append(self.load_grating(file))

        print("Displaying in order of: " + str([grating.filename.split("/")[-1] for grating in gratings ] ))

        for grating in gratings:
            perf = self.display_grating(grating)
            self.display_greyscale(self.background)
            self._print_log(logfile_name, "Grating", grating.filename, perf)
            t.sleep(intertrial_time)


    def display_raw_randomly(self, dir_containing_raws, intertrial_time, logfile_name="rpglog.txt"):
        """
        For each file in directory dir_containing_raws, attempt to display
        that file as a raw. The order of display is a fixed but psudorandomised.
        The location of the log file is ~/rpg/logs/ . Raws are seperated
        by intertrial_time seconds of the background color set when Screen
        object created.

        Method is blocking, and will not return until all files in directory
        displayed


        Args:
          dir_containing_raws: A relative or absolute directory path
            to a directory containing raws. Must not contain any other 
            non raw files, or sub directories.
          intertrial_time: Time between raws in seconds. Will have 
            ~1 millisecond accuracy
          logfile_name: Name of log file to write performance record to.
            written into directory ~/rpg/logs/

        Returns:
          None
        """

        dir_containing_raws = os.path.expanduser(dir_containing_raws)
        path_to_raws = [dir_containing_raws + "/" + file for file in os.listdir(dir_containing_raws)]
        randomized_path = self._randomize_list(path_to_raws)

        raws = []
        print("Loading raws...")
        for file in randomized_path:
            raws.append(self.load_raw(file))

        print("Displaying in order of: " + str([raw.filename.split("/")[-1] for raw in raws ] ))

        for raw in raws:
            perf = self.display_raw(raw)
            self.display_greyscale(self.background)
            self._print_log(logfile_name, "Raw", raw.filename, perf)
            t.sleep(intertrial_time)


    def display_rand_grating_on_pulse(self, dir_containing_gratings, trigger_pin, logfile_name="rpglog.txt"):
        """
        Displays a psudorandom grating from the passed directory in response
        to a 3.3V signal to a GPIO pin. Gauranteed to display each grating
        in directory before playing gratings again. Will display gratings
        in a fixed order across sessions. Between gratings, displays
        Screen.background() shade. Function is blocking, but will return
        in response to a keystroke

        Args:
          dir_containing_gratings: A relative or absolute directory path
            to a directory containing gratings. Must not contain any other 
            non grating files, or sub directories.
          trigger_pin: Which trigger pin the raspberry pi listens on for the
            3.3V pulse.
          logfile_name: Name of log file to write performance record to.
            written into directory ~/rpg/logs/

        Returns:
          None
        """

        self.display_greyscale(self.background)

        dir_containing_gratings = os.path.expanduser(dir_containing_gratings)
        path_to_gratings = [dir_containing_gratings + "/" + file for file in os.listdir(dir_containing_gratings)]
        randomized_path = self._randomize_list(path_to_gratings)

        gratings = []
        print("Loading gratings...")
        for file in randomized_path:
            gratings.append(self.load_grating(file))

        print("Displaying in order of: " + str([grating.filename.split("/")[-1] for grating in gratings ] ))
        print("Waiting for pulse on pin " + str(trigger_pin) + ".")
        print("Press any key to stop waiting...")

        n = 0
        while True:
            perf = self.display_grating(gratings[n], trigger_pin)
            self.display_greyscale(self.background)
            if perf is None:
                break
            self._print_log(logfile_name, "Grating", gratings[n].filename, perf)
            n += 1
            if n >= len(gratings):
                n = 0

        print("Waiting for pulses ended")

    def display_rand_raw_on_pulse(self, dir_containing_raws, trigger_pin, logfile_name="rpglog.txt"):
        """
        Displays a psudorandom raw from the passed directory in response
        to a 3.3V signal to a GPIO pin. Gauranteed to display each raw
        in directory before playing raws again. Will display raw
        in a fixed order across sessions. Between raws, displays
        Screen.background() shade. Function is blocking, but will return
        in response to a keystroke.

        Args:
          dir_containing_raws: A relative or absolute directory path
            to a directory containing raws. Must not contain any other 
            non raw files, or sub directories.
          trigger_pin: Which trigger pin the raspberry pi listens on for the
            3.3V pulse.
          logfile_name: Name of log file to write performance record to.
            written into directory ~/rpg/logs/

        Returns:
          None
        """

        self.display_greyscale(self.background)

        dir_containing_raws = os.path.expanduser(dir_containing_raws)
        path_to_raws = [dir_containing_raws + "/" + file for file in os.listdir(dir_containing_raws)]
        randomized_path = self._randomize_list(path_to_raws)

        raws = []
        print("Loading raws...")
        for file in randomized_path:
            raws.append(self.load_raw(file))

        print("Displaying in order of: " + str([raw.filename.split("/")[-1] for raw in raws ] ))
        print("Waiting for pulse on pin " + str(trigger_pin) + ".")
        print("Press any key to stop waiting...")

        n = 0
        while True:
            perf = self.display_raw(raws[n], trigger_pin)
            self.display_greyscale(self.background)
            if perf is None:
                break
            self._print_log(logfile_name, "Raw", raws[n].filename, perf)
            n += 1
            if n >= len(raws):
                n = 0

        print("Waiting for pulses ended")



    def _print_log(self, filename, file_type, file_displayed, perf):
        """
        Internal function for print log file
 
        Args:
          filename: string of file displayed
          file_type: string of whether the file was a raw or a grating
          perf: Named tuple of performance record

        Returns:
          None
        """

        path_of_logfile = os.path.expanduser("~/rpg/logs/") + filename
        with open(path_of_logfile, "a") as file:
            file.write("%s: \t %s \t Displayed starting at (unix time): %d \t Average frame duration (micros): %.2f \t  Std Dev of frame duration(FPS): %.2f \n" 
                %(file_type, file_displayed, perf.start_time, perf.mean_interframe, perf.stddev_interframe))

    def _randomize_list(self, lst):
        """
        Internal function for psudorandomizing gratings paths. Files paths are hashed
        with MD5 to generate a string, which is then sorted to give order. This
        achieves a psuedo random order that is fixed across sessions.

        Args:
          list: list containing grating path names, or any strings

        Returns:
          list of grating path names psuedorandomzied
        """

        hashed_list = [ hashlib.md5(el.encode()).hexdigest() for el in lst]
        labelled_hashes = enumerate(hashed_list)
        labelled_hashes = [ (x[1], x[0]) for x in labelled_hashes ]
        sorted_hashes = sorted(labelled_hashes)

        randomized_gratings = []
        for el in sorted_hashes:
            randomized_gratings.append( lst[el[1]] )
        return randomized_gratings

    def close(self):
        """
        Destroy this object, cleaning up its memory and restoring previous
        screen settings.

        Args:
          None

        Returns:
          None
        """

        print("Screen object has been closed. You will need to make a new one")
        rpigratings.close_display(self.capsule)
        del self

    def __del__(self):
        self.close()



class Grating:
	def __init__(self, master, filename):
		if type(master).__name__ != "Screen":
			raise ValueError("master must be a Screen instance")
		self.master = master
		self.filename = filename
		self.capsule = rpigratings.load_grating(master.capsule,filename)
	def __del__(self):
		rpigratings.unload_grating(self.capsule)


class Raw:
	def __init__(self, master, filename):
		if type(master).__name__ != "Screen":
			raise ValueError("master must be a Screen instance")
		self.master = master
		self.filename = filename
		self.capsule = rpigratings.load_raw(filename)
	def __del__(self):
		rpigratings.unload_raw(self.capsule)

def _parse_options(options):
    """
    An internal function for testing if options have been
    set correctly, and setting default options if none
    have been provided.

    Args: 
      options: A dictionary containing options.

    Returns:
      Parsed option dictionary suitable for passing to other functions
    """
    op = options.copy()
    if "duration" in op:
        if op["duration"] <= 0:
            raise ValueError("options['duration'] option set to invalid value of %d, must be >0" %op["duration"])
    else:
        raise ValueError("Provide options['duration'] in option dictionary")

    if "angle" not in op:
        raise ValueError("Provide options['angle'] in options dictionary")

    if "spac_freq" in op:
        if op["spac_freq"] <= 0:
            raise ValueError("options['spac_freq'] set to invalid value of %d, must be > 0" %op["spac_freq"])
    else:
        raise ValueError("Provide options['space_freq'] in options dictionary")

    if "temp_freq" in op:
        if op["temp_freq"] < 0:
            raise ValueError("options['temp_freq'] set to invalid value of %d, must be >= 0" %op["temp_freq"])
    else:
        raise ValueError("Provide options['temp_freq'] in options dictionary")

    if "contrast" in op:
        if op["contrast"] < 0 or op["contrast"] > 1:
            raise ValueError("options['contrast'] set to invalid value of %d, must be set between 0 and 1 or not set" %op["contrast"])
    else:
        op["contrast"] = 1

    if "background" in op:
        if op["background"] < 0 or op["background"] > 255:
            raise ValueError("options['background'] set to invalid value of %d, must be set between 0 and 255 or not set" %op["contrast"])
    else:
        op["background"] = 127

    if "resolution" not in op:
        op["resolution"] = (1280, 720)

    if "waveform" not in op:
        op["waveform"] = SINE

    if "percent_sigma" in op:
        if op["percent_sigma"] <= 0:
            raise ValueError("options['percent_sigma'] set to invalid value of %d, must be set > 0 or not set" %op["percent_sigma"])
    else:
        op["percent_sigma"] = 0

    if "percent_diameter" in op:
        if op["percent_diameter"] <= 0:
            raise ValueError("options['percent_diameter'] set to invalid value of %d, must be set > 0 or not set" %op["percent_diameter"])
    else:
        op["percent_diameter"] = 0

    if "percent_center_left" not in op:
        op["percent_center_left"] = 50

    if "percent_center_top" not in op:
        op["percent_center_top"] = 50

    if "percent_padding" in op:
        if op["percent_padding"] <= 0:
            raise ValueError("options['percent_padding'] set to invalid value of %d, must be set > 0 or not set" %op["percent_padding"])
    else:
        op["percent_padding"] = 0

    return op
