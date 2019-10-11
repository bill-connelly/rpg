import rpg
from scipy import misc


face = misc.face()
#face is a 3D numpy array of uint8
#face.shape is (768,1024,3)

with open("/home/pi/test/simpleraw.raw", mode="wb") as fh:
    face.tofile(fh)

rpg.convert_raw("~/test/simpleraw.raw", "~/test/convertedsimpleraw.raw", 1, 1024, 768, 100)
myscreen = rpg.Screen(resolution=(1024,768))
raw = myscreen.load_raw("~/test/convertedsimpleraw.raw")
myscreen.display_raw(raw)

