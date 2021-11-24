
CFLAGS=`sdl2-config --cflags` \
LFLAGS=`sdl2-config --libs` \
CC=gcc ./configure \
--sdk-path=/ \
--target-device=rk3326
