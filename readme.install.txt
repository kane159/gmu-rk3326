复制文件到/emuelec/gmu/
目录结构如此：
.
└── emuelec
    ├── gamelist.xml
    ├── gmu
    │   ├── decoders
    │   │   ├── flac.so
    │   │   ├── mpg123.so
    │   │   ├── speex.so
    │   │   └── vorbis.so
    │   ├── default.keymap
    │   ├── frontends
    │   │   ├── gmuhttp.so
    │   │   ├── log.so
    │   │   └── sdl.so
    │   ├── gmu.bin
    │   ├── gmu.conf
    │   ├── gmuinput.rg351v.conf
    │   ├── rg351v.keymap
    │   └── themes
    │       ├── default-modern
    │       │   ├── arrow-down.png
    │       │   ├── arrow-up.png
    │       │   ├── display-tl.png
    │       │   ├── display-tm.png
    │       │   ├── display-tr.png
    │       │   ├── footer-tl.png
    │       │   ├── footer-tm.png
    │       │   ├── footer-tr.png
    │       │   ├── letters_lcd_1.png
    │       │   ├── letters_lcd_2.png
    │       │   ├── letters_lcd_black.png
    │       │   ├── letters_lcd.png
    │       │   ├── letters_small_1.png
    │       │   ├── letters_small_black.png
    │       │   ├── letters_small_blue.png
    │       │   ├── letters_small_orange.png
    │       │   ├── letters_small_white.png
    │       │   ├── lvmiddle.png
    │       │   ├── symbols.png
    │       │   ├── textarea-m.png
    │       │   └── theme.conf
    │       └── default-modern-large
    │           ├── arrow-down.png
    │           ├── arrow-up.png
    │           ├── display-tl.png
    │           ├── display-tm.png
    │           ├── display-tr.png
    │           ├── footer-tl.png
    │           ├── footer-tm.png
    │           ├── footer-tr.png
    │           ├── letters_large_blue.png
    │           ├── letters_large_white.png
    │           ├── letters_lcd_1.png
    │           ├── letters_lcd_2.png
    │           ├── letters_lcd_black.png
    │           ├── letters_lcd.png
    │           ├── lvmiddle.png
    │           ├── symbols.png
    │           ├── textarea-m.png
    │           └── theme.conf
    └── gmuinput.rg351v.conf
编辑文件/emuelec/scripts/modules/gamelist.xml，增加xml节点:
<game>
		<path>./00 - The Gmu Music Player.sh</path>
		<name>00-音乐播放器</name>
		<desc>The Gmu Music Player</desc>
		<developer>Johannes Heimansberg</developer>
		<publisher>Emuelec</publisher>
		<genre>script</genre>
		<image>./downloaded_images/gmu.png</image>
</game>

编辑文件 "/emuelec/scripts/modules/00 - The Gmu Music Player.sh"，内容如下:
#!/bin/bash
cd /emuelec/gmu/
./gmu.bin 1>/dev/null 2>&1

完成后，重启系统既可以在setup栏目找到gmu运行项
