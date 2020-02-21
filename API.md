**rpg.build_grating(filename, options)**

>Create a raw animation file of a drifting grating. Saves file to hard disc. This file is then loaded with Screen.load_grating, and displayed with one of the Screen methods.
>
>Parameters:
>>* filename (string) - The filename and path to file. ~/dir/filename will generate a file called filename in a directory dir in the users home directory. Can be specified as relative to home with \~ or absolute paths
>>* options (dict) -  A dictionary with the required keys of, duration, angle, spac_freq, and temp_freq, which can be created as follows:
>>>options = {
>>>"duration": 2,     #2 second long grating <br>
>>>"angle": 90,       #90 degree gratings<br>
>>>"spac_freq": 0.1,  #spatial frequency in cycles per degree<br>
>>>"temp_freq": 0.5,  #temporal frequency in cycles per second<br>
>>>}<br>
>>>optional options are:<br>
>>>"contrast": 1      #maximum contrast<br>
>>>"background": 127,   #<br>
>>>"resolution": (1280, 720)   #resolution of gratings. Must match Screen()<br>
>>>"waveform": rpg.SINE #rpg.SQUARE (square wave) or rpg.SINE (sine wave)<br>
>>
>>Returns:
>>>Nothing
