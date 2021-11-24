
CFLAGS=`/usr/bin/sdl2-config --cflags` \
LFLAGS=`/usr/bin/sdl2-config --libs` \
CC=/gcc ./configure \
--sdk-path=/ \
--target-device=rk3326
