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
import os, sys, random
from collections import namedtuple

GratPerfRec = namedtuple("GratingPerformanceRecord",["fastest_frame","slowest_frame","start_time"])


GRAY = 127
BLACK = 0
WHITE = 255
SINE = 1
SQUARE = 0

import _rpigratings as rpigratings

def build_grating(filename, duration, angle, spac_freq, temp_freq,
                  contrast = 1, background = 127, resolution = (1280,720),
                  waveform = SINE, percent_sigma = 0, percent_diameter = 0,
                  percent_center_left = 50, percent_center_top = 50,
                  percent_padding = 0):
    """
    Create a raw animation file of a drifting grating. Saves file to hard disc.
    This file is then loaded with Screen.load_grating, and displayed with one
    of the Screen methods.

    Args:
      filename: The filename and path to file. ~/dir/filename will
        generate a file called filename in a directory dir in the users
        home directory.
      duration: Gratings duration in seconds.
      spac_freq: Spacial frequency of the grating is in cycles per degree of
        visual angle.
      temp_freq: Temporal frequency in cycles per second.
      angle: The grating's angle of propogation, measured counterclockwise
        from the x-axis, e.g. 0 has vertical gratings moving horiztonally to
        the left.
      resolution: Formatted as (width, height) and must match the resolution
        of any Screen object used to display the grating.
      waveform: can be 0 for a squarewave, or 1 for a sinewave. Constants 
        SQUARE and SINE are available for this purpose.
      background: value between 0 (black) and 255 (white) used for the background
        used when contrast < 1, when percent sigma > 0 or percent_diameter > 0
      contrast: value between 0 and 1, for the contrast 
      percent_sigma: determines the percentage of the screen width that the
        gaussian envelope of gabor waveform uses as sigma. Setting to 0 produces
        either full screen gratings, or gratings with a circular mask
      percent_diameter: How much of the screen should be filled by the grating
        and how much by midgray. Setting to 0 produces either full screen
        gratings or a gabor grating.
      percent_center_left: The center of the grating from the left edge of
        screen. Only has an effect if the grating is not full screen, i.e.
        percent_diameter > 0. 50 produces a horizontally centered grating.
      percent_center_top: The center of the grating from the top edge of the
        screen. Only has an effect if the grating is not full screen, i.e.
        percent_diameter > 0. 50 produces a vertically centered grating.
      percent_padding: the % of the radius used to fade the grating towards
        midgray. Setting to 0 produces a hard edged circular grating. Only has
        an effect if percent_diameter > 0.

    For smooth propogation of the grating, the pixels-per-frame speed
    is truncated to the nearest interger; low resolutions combined with
    a low temporal frequency:spacial frequency ratio may result in incorrect
    speeds of propogation or even static, unmoving gratings. This also means
    that the temp_freq is approximate only.
    """
    if percent_diameter < 0:
        raise ValueError("percent_diameter param set to invalid value of %d, must be in [0,100]"
                         %percent_diameter)
    if percent_sigma < 0:
        raise ValueError("percent_sigma param set to invalid value of %d, must be in [0,100]"
                         %percent_sigma)

    if background < 0 or background > 255:
        raise ValueError("background param set to invalid value of %d, must be in [0,255]"
                         %background)

    if contrast < 0 or contrast > 1:
        raise ValueError("contrast param set to invalid value of %d, must be in [0,1]"
                         %contrast)

    filename = os.path.expanduser(filename)
    rpigratings.build_grating(filename,duration,angle,spac_freq,temp_freq,
                     contrast, background, resolution[0], resolution[1],
                     waveform, percent_sigma, percent_diameter, percent_center_left,
                     percent_center_top, percent_padding)


def build_list_of_gratings(list_of_angles, path_to_directory, duration, spac_freq,
			   temp_freq, contrast = 1, background = 127,
                           resolution = (1280,720), waveform = SINE,
                           percent_sigma = 0, percent_diameter = 0,
                           percent_center_left = 0, percent_center_top = 0,
                           percent_padding = 0):

	"""
	Builds a list of gratings with the same properties but varied angles

        Args:
          list_of_angles: a list containing the angles of the sine waves wanted.
            Also the filename for each grating video. e.g. [0 90] will produce
            two gratings, at 0 and 90 degrees, called saved with file names of 0
            and 90 respectively.
          path_to_directory: An absolute or relative path to the directory where
            where the above files will be saved. Most likely, each set of gratings
            generated with this function will be saved in their own directory so
            can be displayed with the Screen.display_rand_grating_on_pulse()
          See build_grating() for other arguments

	"""

	if len(list_of_angles) != len(set(list_of_angles)):
		raise ValueError("list_of_angles must not contain duplicate elements")


	path_to_directory = os.path.expanduser(path_to_directory)
	cwd = os.getcwd()
	os.makedirs(path_to_directory)
	os.chdir(path_to_directory)
	for angle in list_of_angles:
		build_grating(str(angle), duration, spac_freq, temp_freq, angle,
                              resolution, waveform, background, contrast,
                              percent_sigma, percent_diameter, percent_center_left,
                              percent_center_top, percent_padding)
	os.chdir(cwd)

def convert_raw(filename, new_filename, n_frames, width, height, fps):
	filename = os.path.expanduser(filename)
	new_filename = os.path.expanduser(new_filename)
	rpigratings.convertraw(filename, new_filename, n_frames, width, height, fps)


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

        :param resolution: a tuple of the desired width of the display
          resolution as (width, height). Defaults to (1280,720).
        background, value between 
         """
        if (background < 0 or background > 255):
                raise ValueError("Background must be between 0 and 255")

        self.background = background
        self.capsule = rpigratings.init(resolution[0],resolution[1])


    def load_grating(self,filename):
        """
        Load a raw grating file called filename into local memory. Once loaded
        in this way, display_grating() can be called to display the loaded file
        to the screen.
        """
        filename = os.path.expanduser(filename)
        return Grating(self,filename)

    def load_raw(self, filename):
        filename = os.path.expanduser(filename)
        return Raw(self, filename)

    def display_grating(self, grating, trigger_pin = 0):
        """
        Display the currently loaded grating (grating files are created with
        the draw_grating function and loaded with the Screen.load_grating
        method). Also automatically unloads the currently loaded grating
        after displaying it unless cleanup is set to False.

        Returns a namedtuple (from the collections module) with the fields
        mean_FPS, slowest_frame_FPS,
        and start_time; these refer respectively to the average FPS, the inverse
        of the time of the slowest frame, and the time the
        grating began to play (in Unix Time).
        """
        rawtuple = rpigratings.display_grating(self.capsule, grating.capsule, trigger_pin)
        if rawtuple is None:
                return None
        else:
                return GratPerfRec(*rawtuple)

    def display_raw(self, raw, trigger_pin = 0):
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
        :param color: Value between 0 and 255, 
        :rtype None:
        """
        if (color<0 or color>255):
                self.close()
                raise ValueError("Color must be between each between 0 and 255.")
        rpigratings.display_color(self.capsule,color,color,color)

    def display_gratings_randomly(self, dir_containing_gratings, intertrial_time, logfile_name="rpglog.txt"):
        """
        For each file in directory dir_containing_gratings, attempt to display
        that file as a grating. The order of display is randomised. The location
        of the log file is given by path_of_log_file, and defaults to the
        highest level directory. Gratings are seperated by one second of midgray.
        """

        path_of_log_file = os.path.expanduser(path_of_log_file)

        dir_containg_gratings = os.path.expanduser(dir_containing_gratings)
        gratings = []
        for file in os.listdir(dir_containing_gratings):
            gratings.append((self.load_grating(file),dir_containing_gratings + "/" + file))
        random.shuffle(gratings)
        record = []
        for grating in gratings:
            perf = self.display_grating(grating[0])
            self.display_greyscale(self.background)
            self.print_log(logfile_name, "Grating", grating[1], perf)
            t.sleep(intertrial_time)

    def display_raw_on_pulse(self, filename, trigger_pin, logfile_name="rpglog.txt"):

        self.display_color(self.background)
        raw = self.load_raw(os.path.expanduser(filename))
        print("Waiting for pulse on pin " + str(trigger_pin) + ".")
        print("Press any key to stop waiting...")
        while True:
            perf = self.display_raw(raw, trigger_pin)
            self.display_greyscale(self.background)
            if perf is None:
                break
            self.print_log(logfile_name, "Raw", filename, perf)

        print("Waiting for pulses ended")


    def display_rand_grating_on_pulse(self, dir_containing_gratings, trigger_pin, logfile_name="rpglog.txt"):
        """
        Upon receiving a 3.3V pulse (NOT 5V!!!), to xtrigger_pin,
        choose a grating at random from dir_containing_gratings
        and display it. Each grating in the directory is guaranteed
        to be displayed once before any grating is displayed for the
        second time, and so on. When not displaying gratings the
        screen will be filled by midgray.
        """
 
        self.display_greyscale(self.background)

        dir_containing_gratings = os.path.expanduser(dir_containing_gratings)
        gratings = []
        for file in os.listdir(dir_containing_gratings):
            gratings.append((self.load_grating(dir_containing_gratings + "/" + file),dir_containing_gratings + "/" + file))
        remaining_gratings = gratings.copy()
        print("Waiting for pulse on pin " + str(trigger_pin) + ".")
        print("Press any key to stop waiting...")
        while True:
            index = random.randint(0,len(remaining_gratings)-1)
            perf = self.display_grating(remaining_gratings[index][0], trigger_pin)
            self.display_greyscale(self.background)
            if perf is None:
                break
            self.print_log(logfile_name, "Grating", remaining_gratings[index][1], perf)
            remaining_gratings.pop(index)
            if len(remaining_gratings) == 0:
                remaining_gratings = gratings.copy()

        print("Waiting for pulses ended")

    def print_log(self, filename, file_type, file_displayed, perf):
        path_of_logfile = os.path.expanduser("~/rpg/logs/") + filename
        with open(path_of_logfile, "a") as file:
            file.write("%s: %s \t Displayed starting at (unix time): %d \t Fastest frame (FPS): %.2f \t slowest frame (FPS): %.2f \n" 
                %(file_type, file_displayed, perf.start_time, perf.fastest_frame, perf.slowest_frame))


    def close(self):
        """
        Destroy this object, cleaning up its memory and restoring previous
        screen settings.
        :rtype None:
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
		self.capsule = rpigratings.load_grating(master.capsule,filename)
	def __del__(self):
		rpigratings.unload_grating(self.capsule)


class Raw:
	def __init__(self, master, filename):
		if type(master).__name__ != "Screen":
			raise ValueError("master must be a Screen instance")
		self.master = master
		self.capsule = rpigratings.load_raw(filename)
	def __del__(self):
		rpigratings.unload_raw(self.capsule)
