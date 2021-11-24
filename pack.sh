#!/bin/sh
mkdir -p ./dist/gmu/
cp readme.install.txt ./dist/readme.txt
cp gamelist.xml ./dist/gmu/
cp -r gmu.bin rk3326.keymap gmuinput.rk3326.conf themes frontends decoders ./dist/gmu/
cp gmu.rk3326.conf ./dist/gmu/gmu.conf
cp gmuinput.rk3326.conf ./dist/gmu/gmuinput.rk3326.conf
cp rk3326.keymap ./dist/gmu/default.keymap
cd ./dist/
tar -czvf ../gmu-rk3326-bin.tar.gz ./
