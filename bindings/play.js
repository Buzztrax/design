// http://live.gnome.org/GObjectIntrospection
// GI_TYPELIB_PATH=$HOME/buzztrax/lib/girepository-1.0 gjs-console play.js
// GI_TYPELIB_PATH=$HOME/buzztrax/lib/girepository-1.0 LD_LIBRARY_PATH="$LD_LIBRARY_PATH:/usr/lib/xulrunner-1.9.0.9" gjs-console play.js

const Lang = imports.lang;
const BuzztraxCore = imports.gi.BuzztraxCore;
const Mainloop = imports.mainloop;

BuzztraxCore.init(null, 0);

const Player = new Lang.Class({
    Name: 'Player',
    Extends: BuzztraxCore.Application,
});

let app = new Player();
let song = new BuzztraxCore.Song({app:app});
let songio = BuzztraxCore.SongIO.from_file("/home/ensonic/buzztrax/share/buzztrax/songs/melo3.xml");
songio.load(song);
song.play();

song.connect("notify::is-playing", function(song) {
    if (!song.is_playing) {
        log("stop playing");
        Mainloop.quit('playing');
    }
    else {
        log("start playing");
    }
});

Mainloop.run('playing');
