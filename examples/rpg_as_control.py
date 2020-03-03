import rpg
import gpiozero #new default GPIO control library
import os
import time

# This code is an example of the Pi acting as the controller
# for your experiemtn, where some other device (for instance a
# 2-photon microscope) needs to be triggered to turn on
#
# The code is written under the assumption that you need a pulse
# to be HIGH to keep the device operating so that the experiment
# looks like this
#
#             _________
# RPG   _____|         |______
#          ________________
# 3.3V  __|                |__
#
#
#
#
# If changes are required to perform an experiment like this
#             _________
# RPG   _____|         |______
#
# 3.3V  __|___________________
#
# Move the pin_out.off() on line 108 to line 103 (before 
# time.sleep(delay) ). You may need to add time.sleep(0.001) 
# between the on() and off() calls, if your pulse is too fast.



## Experiment parameters

delay = 1         #1 second from when the trial starts, and the pi
                  #puts out a 3.3V signal, to when the grating starts
post_delay = 1    #1 second after grating finishes before 3.3V goes low
intertrial = 1    #1 second before trial starts again
repeats = 10      #how many repeats through each file we will get
                  #So in the example below, with 3 gratings, we will
                  #get a total of 30 trials, which each grating show 10
                  #times

#Each trial will take a total of delay + grating duration + post_delay + intertrial seconds



#Build some gratings
# This part is only necessary once, just so this example
# script works out of the box. Comment out if running
# a second  time

options = {"duration": 1, "angle": [0, 45, 90], "spac_freq": 0.2, "temp_freq": 1}
rpg.build_list_of_gratings("grating", "~/gratings/variable_ori/", options)


## GPIO Stuff.
# Initilize a GPIO pin as a digital out.
# If argument is provided as an integer
# then using BCM numbering scheme, where
# 22 is GPIO3, or physical pin 15. Run
# gpio readall from the command line for
# a guide
pin_out = gpiozero.DigitalOutputDevice(22)

myscreen = rpg.Screen()

# As we have a directory of gratings, typically
# one would use the display_rand_grating_on_pulse
# method or similar, but if you are using the pi
# as the controler, then we should populate a
# list manually


path_to_gratings_dir = "~/gratings/variable_ori"
full_path_to_gratings_dir = os.path.expanduser(path_to_gratings_dir)
path_to_gratings = [full_path_to_gratings_dir + "/" +  file for file in os.listdir(full_path_to_gratings_dir)]

## Randomizing grating order
# This function uses an approach that produces a
# psuedorandom order the is fixed across trials
# and sessions. If you prefer a version that is
# random on every sessions, using random.shuffle()
# instead of _randomize_list()

randomized_path_to_gratings = myscreen._randomize_list(path_to_gratings)


print("Loading gratings...")
gratings = []
for file in randomized_path_to_gratings:
  gratings.append(myscreen.load_grating(file))

print("Displaying in order of: " + str([x.split("/")[-1] for x in randomized_path_to_gratings ] ))
myscreen.display_greyscale(myscreen.background)
# You might want to add another time.delay()
# So that the subject aclimatizes to the background

for _ in range(repeats):
  for grating in gratings:
    pin_out.on()
    time.sleep(delay)
    performance_record = myscreen.display_grating(grating)
    myscreen.display_greyscale(myscreen.background)
    myscreen._print_log("log.txt", "grating", grating.filename, performance_record)
    time.sleep(post_delay)
    pin_out.off()
    time.sleep(intertrial)
