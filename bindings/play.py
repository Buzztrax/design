# http://live.gnome.org/GObjectIntrospection
# GI_TYPELIB_PATH=$HOME/buzztrax/lib/girepository-1.0 python play.py

from gi.repository import GLib
from gi.repository import BuzztraxCore

BuzztraxCore.init(None)
loop = GLib.MainLoop()

# BuzztraxCore.Application is a abstract base class
class Player(BuzztraxCore.Application):
  def __init__(self):
    BuzztraxCore.Application.__init__(self)

def isPlaying(self, x):
  if not self.get_property("is-playing"):
      print("stop playing")
      loop.quit()
  else:
      print("start playing")

app=Player()
song=BuzztraxCore.Song(app=app)
songio=BuzztraxCore.SongIO.from_file("/home/ensonic/buzztrax/share/buzztrax/songs/melo3.xml")
songio.load(song)
song.play()

song.connect("notify::is-playing", isPlaying)

loop.run()
