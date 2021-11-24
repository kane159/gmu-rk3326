
CFLAGS=`/usr/bin/sdl2-config --cflags` \
LFLAGS=`/usr/bin/sdl2-config --libs` \
CC=/bin/gcc ./configure \
--sdk-path=/ \
--target-device=rk3326
