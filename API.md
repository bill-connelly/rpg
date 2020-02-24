# Table of Contents



## rpg.build_grating(filename, options)

    Create a raw animation file of a drifting grating. Saves file to hard disc. This file is then loaded with Screen.load_grating, and displayed with one of the Screen methods.

    Parameters:

        * filename (string) - The filename and path to file. ~/dir/filename will generate a file called filename in a directory dir in the users home directory. Can be specified as relative to home with \~ or absolute paths
        * options (dict) -  A dictionary with the required keys of, duration, angle, spac_freq, and temp_freq, which can be created as follows:
            options = {  
            "duration": 2,     #2 second long grating  
            "angle": 90,       #90 degree gratings  
            "spac_freq": 0.1,  #spatial frequency in cycles per degree  
            "temp_freq": 0.5,  #temporal frequency in cycles per second  
            }  
    
        optional options are:  
            "contrast": 1      #maximum contrast  
            "background": 127,   #  
            "resolution": (1280, 720)   #resolution of gratings. Must match Screen()  
            "waveform": rpg.SINE #rpg.SQUARE (square wave) or rpg.SINE (sine wave)  
    Returns:
        None

## rpg.build_masked_grating(filename, options)

>Create a raw animation file of a drifting grating with a circular mask. Saves file to hard disc. This file is then loaded with Screen.load_grating, and displayed with one of the Screen methods.
>
>Parameters:
>>* filename (string) - The filename and path to file. ~/dir/filename will generate a file called filename in a directory dir in the users home directory.
>>* options (dict) - A dictionary with the required keys of: duration, angle, spac_freq and temp_freq. The user will also want to set the keys of percent_diameter, percent_center_top, percent_center_left and percent_padding which can be created as follows:
>>>options = {  
>>>"duration": 2,     #2 second long grating  
>>>"angle": 90,       #90 degree gratings,  
>>>"spac_freq": 0.1,  #spatial frequency in cycles per degree  
>>>"temp_freq": 0.5,  #temporal frequency in cycles per second  
>>>"percent_diameter": 50,    #diameter of mask as percentage of screen width  
>>>"percent_center_left": 50, #horizontal center of mask as percentage of screen width  
>>>"percent_ceter_top": 50,   #vertical center of mask as percentage of screen height  
>>>}  
>>>optional options are:  
>>>"percent_padding", 10      #specify whether the edge of mask blurs to background value over what distance  
>>>"contrast": 1,     #maximum contrast  
>>>"background": 127,   #  
>>>"resolution": (1280, 720)   #resolution of gratings. Must match Screen()  
>>>"waveform": rpg.SINE #rpg.SQUARE (square wave) or rpg.SINE (sine wave)
>>
>Returns:  
>>None


## rpg.build_gabor(filename, options):
>Create a raw animation file of a drifting gabor patch. Saves file to hard disc. This file is then loaded with Screen.load_grating, and displayed with one of the Screen methods.
>
>Parameters:
>>* filename (string) - The filename and path to file. ~/dir/filename will generate a file called filename in a directory dir in the users home directory.
>>* options (dict) - A dictionary with the required keys of: duration, angle, spac_freq, and temp_freq. The user will also want to set the keys of sigma, percent_center_top, percent_center_left and percent_padding which can be created as follows:
>>>options = {  
>>>"duration": 2,     #2 second long grating  
>>>"angle": 90,       #90 degree gratings  
>>>"spac_freq": 0.1,  #spatial frequency in cycles per degree  
>>>"temp_freq": 0.5,  #temporal frequency in cycles per second  
>>>"percent_sigma": 10,       #'standard deviation' of the gaussian envelope of the gabor function as a percentage of screen width  
>>>"percent_center_left": 50, #horizontal center of mask as percentage of screen width  
>>>"percent_ceter_top": 50,   #vertical center of mask as percentage of screen height  
>>>}    
>>>optional options are:  
>>>"contrast": 1,     #maximum contrast  
>>>"background": 127,   #  
>>>"resolution": (1280, 720)   #resolution of gratings. Must match Screen()  
>>>"waveform": rpg.SINE #rpg.SQUARE is not allowed for gabor
>>
>>Returns:  
>>>None

## rpg.build_list_of_gratings(func_string, directory_path, options):
>Builds a range of gratings varying over one property. One of the options supplied can be a list, and the function will iterate over that list building gratings matching each element of this list
>
>Parameters:
>>* func_string (string) - String matching either "grating", "mask" or "gabor", to produce full screen gratings, gratings with a circular mask, or gabors, respectively  
>>* directory_path (string) - An absolute or relative path to the directory where where the above files will be saved. Most likely, each set of gratings generated with this function will be saved in their own directory so can be displayed with the Screen.display_rand_grating_on_pulse()  
>>* options: A dictionary containing options, see build_grating(), build_masked_grating() or build_gabor() for appropriate options, but note one of the options must be in the form of a list.  
>>
>Returns:
>>Nothing
>
>Files will be saved with names matching the element of the list they are generated from. e.g. if generated with options["angle"] = [0 45 90], then there will be three files generated with names "0", "45" and "90" in the directory specificied in  directory path.  

## rpg.convert_raw(filename, new_filename, n_frames, width, height, refreshes_per_frame)
>Converts a raw video/image file saves as uint8: RGBRGBRGB... starting in the top left pixel and proceeding rowwise, into a form readily displayed by RPG.
>
>Parameters
>>* filename (string) - The exact path to the raw file, either as relative or absolute e.g. "~/videos/raw1.raw".
>>* new_filename (string) - The exact path of the converted file to be produced e.g. "~/raws/raw_converted.raw".
>>* n_frames (int) - The number of frames in the raw video/image. Do not use to shorten movies. Only limits how long the movie is played. Not how many frames are converted.
>>* width (int) - The width of the original file in pixels. Cannot be used to resize images/movie
>>* height (int) - The height of the original file in pixels. Cannot be used to resize image/movie
>>* refreshes_per_frame (int) - The number of monitor refreshes to display each frame for. For a movie to display at 30 frames per second, on a 60 Hz monitor, this would be 2. On a 75 Hz monitor, 25 frames per second would be acheived by setting this to 3. If a still image is displayed, if you require it displayed for X seconds, and your monitor refresh rate is R Hz, then this value should be set to X * R.
>>
>Returns:
>>None
