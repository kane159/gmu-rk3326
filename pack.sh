#!/bin/sh
mkdir -p ./dist/emuelec/gmu/
cp readme.install.txt ./dist/readme.txt
cp gamelist.xml ./dist/emuelec/gmu/
cp -r gmu.bin rg351v.keymap gmuinput.rg351v.conf themes frontends decoders ./dist/emuelec/gmu/
cp gmu.rg351v.conf ./dist/emuelec/gmu/gmu.conf
cp gmuinput.rg351v.conf ./dist/emuelec/gmu/gmuinput.rg351v.conf
cp rg351v.keymap ./dist/emuelec/gmu/default.keymap
cd ./dist/
tar -czvf ../gmu-rg341v-bin.tar.gz ./
