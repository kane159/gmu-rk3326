/* 
 * Gmu GP2X Music Player
 *
 * Copyright (c) 2006-2010 Johannes Heimansberg (wejp.k.vu)
 *
 * File: help.c  Created: 100404
 *
 * Description: Program info
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; version 2 of
 * the License. See the file COPYING in the Gmu's main directory
 * for details.
 */
#include "kam.h"
#include "textbrowser.h"
#include "help.h"
#include "core.h"

static const char *text_help = 
"欢迎使用GMU音乐播放器！\n"
"这是一个简短的介绍，来讲述：\n"
"GMU最重要的功能。\n\n"
"GMU由设备的按钮控制。\n"
"每个按钮可以有两个功能。\n"
"轻轻的按住一个键，主要功能将被激活\n"
"接着，按住第二个按钮\n"
"将激活第二个功能\n"
"主键和修饰键需要同时按住。\n"
"修饰键按钮是**%s**按钮。\n"
"当引用它时，它将被称为\"mod\"。\n"
"在下面的文本中。\n\n"
"要上下滚动，请使用**%s**和\n"
"**%s**按钮。\n\n"
"GMU有几个屏幕显示不同的\n"
"事情。它的主屏幕是文件浏览器，"
"播放列表浏览器和曲目信息屏幕。\n"
"您可以通过按**%s**在它们之间切换。\n\n"
"有一些全局功能可以在所有时刻被激活\n"
"，无论您选择了哪个屏幕\n"
"还有屏幕相关功能。\n\n"
"**重要的全局功能**\n\n"
"播放/跳下一曲…….：**%s**\n"
"播放/后退一曲…………：**%s**\n"
"快进…………：**%s**\n"
"快退…………：**%s**\n"
"暂停…………：**%s**\n"
"停止…………：**%s**\n"
"音量增大………………：**%s**\n"
"音量降低…………：**%s**\n"
"退出GMU……………：**%s**\n"
"程序信息…..：**%s**\n"
"锁定按钮+屏幕关闭。：**%s**\n"
"解锁按钮+屏幕打开：**%s**\n"
"\n"
"**文件浏览器功能**\n\n"
"添加文件/更改目录…：**%s**\n"
"添加目录…………：**%s**\n"
"播放单个文件………：**%s**\n"
"从目录新建播放列表…：**%s**\n"
"\n"
"**播放列表浏览器功能**\n\n"
"播放选定的项目……：**%s**\n"
"将所选项目排队…：**%s**\n"
"删除所选项目…..：**%s**\n"
"清除播放列表………：**%s**\n"
"更改播放模式………：**%s**\n"
"保存/加载播放列表……：**%s**\n"
"\n"
"这些是最重要的功能。\n"
"还有一些不常用的功能\n"
"这些功能会解释在\n"
"GMU附带的readme.txt文件。\n"
"如果您希望稍后打开此帮助屏幕\n"
"按**%s**可以完成此操作。\n\n"
"**入门**\n\n"
"您可能想做的第一件事是\n"
"用一些曲目填充播放列表。\n"
"这很简单。第一次使用**%s**到\n"
"导航到文件浏览器屏幕。到达这里"
"请使用上面描述的按钮，进入某个目录，或者上级目录\n"
"在文件系统树中导航，并添加\n"
"喜欢的歌曲到播放列表，可以添加单个文件或整个目录。\n"
"播放列表中至少有一个文件后\n"
"您可以按**%s**开始播放。\n\n"
"享受GMU音乐播放器的乐趣。\n"
"/作者:wej\n"
"/汉化:netwan\n"
"/打包:kane159\n"
"/QQ:1182389405\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n";

void help_init(TextBrowser *tb_help, Skin *skin, KeyActionMapping *kam)
{
	static char txt[3072];
	//SDL_RWops *pixelsWop = SDL_RWFromConstMem((const unsigned char *)qr_bmp, sizeof(qr_bmp));
	//tb_help->qr = SDL_LoadBMP_RW(pixelsWop, 1);
	snprintf(txt, 3071, text_help,
	                    key_action_mapping_get_button_name(kam, MODIFIER),
	                    key_action_mapping_get_full_button_name(kam, MOVE_CURSOR_UP),
	                    key_action_mapping_get_full_button_name(kam, MOVE_CURSOR_DOWN),
	                    key_action_mapping_get_full_button_name(kam, GLOBAL_TOGGLE_VIEW),
	                    key_action_mapping_get_full_button_name(kam, GLOBAL_NEXT),
	                    key_action_mapping_get_full_button_name(kam, GLOBAL_PREV),
	                    key_action_mapping_get_full_button_name(kam, GLOBAL_SEEK_FWD),
	                    key_action_mapping_get_full_button_name(kam, GLOBAL_SEEK_BWD),
	                    key_action_mapping_get_full_button_name(kam, GLOBAL_PAUSE),
	                    key_action_mapping_get_full_button_name(kam, GLOBAL_STOP),
	                    key_action_mapping_get_full_button_name(kam, GLOBAL_INC_VOLUME),
	                    key_action_mapping_get_full_button_name(kam, GLOBAL_DEC_VOLUME),
	                    key_action_mapping_get_full_button_name(kam, GLOBAL_EXIT),
	                    key_action_mapping_get_full_button_name(kam, GLOBAL_PROGRAM_INFO),
	                    key_action_mapping_get_full_button_name(kam, GLOBAL_LOCK),
	                    key_action_mapping_get_full_button_name(kam, GLOBAL_UNLOCK),
	                    key_action_mapping_get_full_button_name(kam, FB_ADD_FILE_TO_PL_OR_CHDIR),
	                    key_action_mapping_get_full_button_name(kam, FB_ADD_DIR_TO_PL),
	                    key_action_mapping_get_full_button_name(kam, FB_PLAY_FILE),
	                    key_action_mapping_get_full_button_name(kam, FB_NEW_PL_FROM_DIR),
	                    key_action_mapping_get_full_button_name(kam, PL_PLAY_ITEM),
	                    key_action_mapping_get_full_button_name(kam, PL_ENQUEUE),
	                    key_action_mapping_get_full_button_name(kam, PL_REMOVE_ITEM),
	                    key_action_mapping_get_full_button_name(kam, PL_CLEAR_PLAYLIST),
	                    key_action_mapping_get_full_button_name(kam, PL_TOGGLE_RANDOM),
	                    key_action_mapping_get_full_button_name(kam, PL_SAVE_PLAYLIST),
	                    key_action_mapping_get_full_button_name(kam, GLOBAL_HELP),
	                    key_action_mapping_get_full_button_name(kam, GLOBAL_TOGGLE_VIEW),
	                    key_action_mapping_get_full_button_name(kam, GLOBAL_NEXT));

	text_browser_init(tb_help, skin);
	text_browser_set_text(tb_help, txt, "Gmu Help");
}

int help_process_action(TextBrowser *tb_help, View *view, View old_view, int user_key_action)
{
	int update = 0;
	switch (user_key_action) {
		case OKAY:
			*view = old_view;
			update = 1;
			break;
		case MOVE_CURSOR_DOWN:
			text_browser_scroll_down(tb_help);
			update = 1;
			break;
		case MOVE_CURSOR_UP:
			text_browser_scroll_up(tb_help);
			update = 1;
			break;
		case MOVE_CURSOR_LEFT:
			text_browser_scroll_left(tb_help);
			update = 1;
			break;
		case MOVE_CURSOR_RIGHT:
			text_browser_scroll_right(tb_help);
			update = 1;
			break;
	}
	return update;
}
