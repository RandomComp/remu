#include "blockstd.h"

#include "types.h"

#include "std.h"

#include "drivers/memory/memory.h"

#include "colors.h"

#include "builtins/builtins.h"

#include "builtins/string.h"

#include "drivers/power.h"

#include "drivers/time/cmos.h"

#include "multiboot.h"

void* ram = nullptr;

byte read_kbd_scancode() {
	return in8(0x60);
}

typedef enum scancode_e {
	SCANCODE_NULL,
	SCANCODE_ESCAPE,
	SCANCODE_1,
	SCANCODE_2,
	SCANCODE_3,
	SCANCODE_4,
	SCANCODE_5,
	SCANCODE_6,
	SCANCODE_7,
	SCANCODE_8,
	SCANCODE_9,
	SCANCODE_0,
	SCANCODE_MINUS_SIGN,
	SCANCODE_EQUAL,
	SCANCODE_BACKSPACE,
	SCANCODE_TAB,
	SCANCODE_Q,
	SCANCODE_W,
	SCANCODE_E,
	SCANCODE_R,
	SCANCODE_T,
	SCANCODE_Y,
	SCANCODE_U,
	SCANCODE_I,
	SCANCODE_O,
	SCANCODE_P,
	SCANCODE_LEFT_BRACKET,
	SCANCODE_RIGHT_BRACKET,
	SCANCODE_ENTER,
	SCANCODE_CONTROL,
	SCANCODE_A,
	SCANCODE_S,
	SCANCODE_D,
	SCANCODE_F,
	SCANCODE_G,
	SCANCODE_H,
	SCANCODE_J,
	SCANCODE_K,
	SCANCODE_L,
	SCANCODE_SEMICOLON,
	SCANCODE_QUOTE,
	SCANCODE_BACK_TICK,
	SCANCODE_LEFT_SHIFT,
	SCANCODE_BACKSLASH,
	SCANCODE_Z,
	SCANCODE_X,
	SCANCODE_C,
	SCANCODE_V,
	SCANCODE_B,
	SCANCODE_N,
	SCANCODE_M,
	SCANCODE_COMMA,
	SCANCODE_DOT,
	SCANCODE_SLASH,
	SCANCODE_RIGHT_SHIFT,
	SCANCODE_NUMBER_PAD_ASTERISK,
	SCANCODE_LEFT_ALT,
	SCANCODE_SPACE,
	SCANCODE_CAPSLOCK,
	SCANCODE_F1,
	SCANCODE_F2,
	SCANCODE_F3,
	SCANCODE_F4,
	SCANCODE_F5,
	SCANCODE_F6,
	SCANCODE_F7,
	SCANCODE_F8,
	SCANCODE_F9,
	SCANCODE_F10,
	SCANCODE_NUMBER_PAD_LOCK,
	SCANCODE_SCROLL_LOCK,
	SCANCODE_NUMBER_PAD_7,
	SCANCODE_NUMBER_PAD_8,
	SCANCODE_NUMBER_PAD_9,
	SCANCODE_NUMBER_PAD_MINUS_SIGN,
	SCANCODE_NUMBER_PAD_4,
	SCANCODE_NUMBER_PAD_5,
	SCANCODE_NUMBER_PAD_6,
	SCANCODE_NUMBER_PAD_PLUS_SIGN,
	SCANCODE_NUMBER_PAD_1,
	SCANCODE_NUMBER_PAD_2,
	SCANCODE_NUMBER_PAD_3,
	SCANCODE_NUMBER_PAD_0,
	SCANCODE_NUMBER_PAD_DOT,
	SCANCODE_NONE0,
	SCANCODE_NONE1,
	SCANCODE_NONE2,
	SCANCODE_F11,
	SCANCODE_F12,
	SCANCODE_CONTROL_SEQUENCE = 0xE0
} scancode_e;

void kmain(uint32 magic, multiboot_info_t* multiboot) {
	ram = get_ram();

	init_std(ram + 0xB8000);

	byte style = COLOR_BRIGHT_WHITE | (COLOR_BLACK << 4);

	clear_screen(style);

	set_style(style);

	while (true) {
		in8(0x80);

		bool active = in8(0x64) & 1;

		byte scancode = read_kbd_scancode() & 0x7F;

		if (active && scancode) {
			// bool scancode_released = 

			byte scancode_to_c[] = {
				[SCANCODE_NULL] = 					'\0',
				[SCANCODE_ESCAPE] = 				'\x1B',
				[SCANCODE_0] = 						'0',
				[SCANCODE_1] = 						'1',
				[SCANCODE_2] = 						'2',
				[SCANCODE_3] = 						'3',
				[SCANCODE_4] = 						'4',
				[SCANCODE_5] = 						'5',
				[SCANCODE_6] = 						'6',
				[SCANCODE_7] = 						'7',
				[SCANCODE_8] = 						'8',
				[SCANCODE_9] = 						'9',
				[SCANCODE_MINUS_SIGN] =				'-',
				[SCANCODE_EQUAL] =					'=',
				[SCANCODE_BACKSPACE] = 				'\b',
				[SCANCODE_TAB] = 					'\t',
				[SCANCODE_Q] = 						'Q',
				[SCANCODE_W] = 						'W',
				[SCANCODE_E] = 						'E',
				[SCANCODE_R] = 						'R',
				[SCANCODE_T] = 						'T',
				[SCANCODE_Y] = 						'Y',
				[SCANCODE_U] = 						'U',
				[SCANCODE_I] = 						'I',
				[SCANCODE_O] = 						'O',
				[SCANCODE_P] = 						'P',
				[SCANCODE_LEFT_BRACKET] = 			'[',
				[SCANCODE_RIGHT_BRACKET] = 			']',
				[SCANCODE_ENTER] = 					'\n',
				[SCANCODE_A] = 						'A',
				[SCANCODE_S] = 						'S',
				[SCANCODE_D] = 						'D',
				[SCANCODE_F] = 						'F',
				[SCANCODE_G] = 						'G',
				[SCANCODE_H] = 						'H',
				[SCANCODE_J] = 						'J',
				[SCANCODE_K] = 						'K',
				[SCANCODE_L] = 						'L',
				[SCANCODE_SEMICOLON] = 				':',
				[SCANCODE_QUOTE] = 					'?',
				[SCANCODE_BACK_TICK] = 				'`',
				[SCANCODE_BACKSLASH] = 				'\\',
				[SCANCODE_Z] = 						'Z',
				[SCANCODE_X] = 						'X',
				[SCANCODE_C] = 						'C',
				[SCANCODE_V] = 						'V',
				[SCANCODE_B] = 						'B',
				[SCANCODE_N] = 						'N',
				[SCANCODE_M] = 						'M',
				[SCANCODE_COMMA] = 					',',
				[SCANCODE_DOT] = 					'.',
				[SCANCODE_SLASH] = 					'/',
				[SCANCODE_NUMBER_PAD_ASTERISK] = 	'*',
				[SCANCODE_SPACE] = 					' ',
				[SCANCODE_NUMBER_PAD_7] = 			'7',
				[SCANCODE_NUMBER_PAD_8] = 			'8',
				[SCANCODE_NUMBER_PAD_9] = 			'9',
				[SCANCODE_NUMBER_PAD_MINUS_SIGN] = 	'-',
				[SCANCODE_NUMBER_PAD_4] = 			'4',
				[SCANCODE_NUMBER_PAD_5] = 			'5',
				[SCANCODE_NUMBER_PAD_6] = 			'6',
				[SCANCODE_NUMBER_PAD_PLUS_SIGN] = 	'+',
				[SCANCODE_NUMBER_PAD_1] = 			'1',
				[SCANCODE_NUMBER_PAD_2] = 			'2',
				[SCANCODE_NUMBER_PAD_3] = 			'3',
				[SCANCODE_NUMBER_PAD_0] = 			'0',
				[SCANCODE_NUMBER_PAD_DOT] = 		'.',
			};

			byte c = scancode_to_c[scancode];
	
			if ((c >= ' ' && c <= '~')) {
				putch(c);
			}

			else if (c == '\n' || c == '\r') {
				kprint("\n\r");
			}

			else if (c == '\b') {
				kprint("\b \b");
			}
		}
	}

	for (;;) halt();
}