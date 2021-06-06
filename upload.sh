#!/bin/sh
scp -r gmu.bin rg351v.keymap gmuinput.rg351v.conf themes frontends decoders root@192.168.2.108:/emuelec/gmu/
scp gmu.rg351v.conf root@192.168.2.108:/emuelec/gmu/gmu.conf
scp gmuinput.rg351v.conf root@192.168.2.108:/emuelec/gmuinput.rg351v.conf
scp rg351v.keymap root@192.168.2.108:/emuelec/gmu/default.keymap
