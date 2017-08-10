# soundd
Simple sound daemon. Plays wav files.

soundd is meant to be really light and fast, usable also in
constrained environments like embedded.

It has a daemon component soundd, client side library libsoundd.so,
and a test command line utility soundc.

Idea is to register "sounds" with the daemon once, and then 
play then multiple times. Care has taken that delay is very
low when staring new sounds.

soundc usage :
  soundc register <alias> <filename>
  soundc play <alias>


libsoundd interface :

  int soundc_register(const char *alias, const char *filename);
  int soundc_play(const char *alias);



--Mika Penttilä
mika.penttila@gmail.com

