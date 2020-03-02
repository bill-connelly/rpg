import rpg

#Make a standard list of 12 gratings, with a circular centered mask
options = {"duration": 2,
           "angle": [0, 30, 60, 90, 120, 150, 180, 210, 240, 270, 330],
           "spac_freq": 0.2,
           "temp_freq": 1,
           "percent_diameter": 40,
           "percent_center_left": 50,
           "percent_center_top": 50
          }

#build those gratings and save them all in one folder for playback.
rpg.build_list_of_gratings("mask", "~/gratings/sf02_tf1/", options)
