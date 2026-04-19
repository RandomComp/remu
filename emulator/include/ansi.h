#ifndef _EMULATOR_ANSI_H
#define _EMULATOR_ANSI_H

typedef enum ansi_id {
	BLUE_ANSI_ID = 34,
			
	GREEN_ANSI_ID = 32,
				
	AQUA_ANSI_ID = 36,

	RED_ANSI_ID = 31,

	MAGENTA_ANSI_ID = 35,

	YELLOW_ANSI_ID = 33,

	WHITE_ANSI_ID = 37
} ansi_id;

#define red_fg "\x1B[31m"
#define red_bg "\x1B[41m"

#define green_fg "\x1B[32m"
#define green_bg "\x1B[42m"

#define cyan_fg "\x1B[96m"
#define cyan_bg "\x1B[106m"

#define bold "\x1B[1m"
#define darkish "\x1B[2m"
#define italic "\x1B[3m"
#define underline "\x1B[4m"
#define blink "\x1B[6m"
#define inverse "\x1B[7m"
#define invisible_text "\x1B[8m"
#define strikethrough "\x1B[9m"

#define white_bg "\x1B[47m"
#define white_fg "\x1B[37m"

#define gray_bg "\x1B[40m"
#define gray_fg "\x1B[30m"

#define bright_black_bg "\x1B[48;5;16m"
#define bright_black_fg "\x1B[38;5;16m"

#define default_style "\x1B[0m"
#define default_bg "\x1B[49m"
#define default_fg "\x1B[39m"

#define magenta_fg "\x1B[35m"

#define yellow_fg "\x1B[33m"
#define yellow_bg "\x1B[43m"

#define bright_yellow_fg "\x1B[93m"
#define bright_yellow_bg "\x1B[103m"

#define blue_fg "\x1B[34m"
#define blue_bg "\x1B[44m"

#define aqua_fg "\x1B[36m"
#define aqua_bg "\x1B[46m"

#define bright_magenta_fg "\x1B[95m"

#define clear "\x1B[K"
#define ansi_clear_screen "\x1B[2J\x1B[H"

#define visible_cursor "\x1B[?25h"
#define invisible_cursor "\x1B[?25l"

#define save_cursor "\x1B[s"
#define load_cursor "\x1B[u"

#define default_screen_color bright_black_bg white_fg

#include "colors.h"

#include "types.h"

byte color_to_ansi(colors_e color);

#endif