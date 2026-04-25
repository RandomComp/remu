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

#include "math/math.h"

#include "drivers/video/vga.h"

static byte* ram = nullptr;

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

static uint8 shift(uint8 c) {
	if (_isalpha(c)) return upper(c);

	const c_str shitftable_syms = "1234567880-=/,.`";

	bool is_c_shiftable = false;

	for (_size_t i = 0; i < strlen(shitftable_syms); i++) {
		if (shitftable_syms[i] == c) {
			is_c_shiftable = true; break;
		}
	}

	if (!is_c_shiftable) return c;

	const uint8 shift_sym[] = {
		['1'] = '!',
		['2'] = '@',
		['3'] = '#',
		['4'] = '$',
		['5'] = '%',
		['6'] = '^',
		['7'] = '&',
		['8'] = '*',
		['9'] = '(',
		['0'] = ')',
		['-'] = '_',
		['='] = '+',
		['/'] = '?',
		[','] = '<',
		['.'] = '>',
		['`'] = '~'
	};
	
	return shift_sym[c];
}

static bool isshifted(uint8 c) {
	if (_isupper(c)) return true;

	const c_str shifted_sym = "!@#$%^&*()_+?<>~";

	for (_size_t i = 0; i < strlen(shifted_sym); i++) {
		if (c == shifted_sym[i]) return true;
	}
	
	return false;
}

static uint8 unshift(uint8 c) {
	if (_isalpha(c)) return lower(c);

	const c_str unshitftable_syms = "!@#$%^&*()_+?<>~";

	bool is_c_unshiftable = false;

	for (_size_t i = 0; i < strlen(unshitftable_syms); i++) {
		if (unshitftable_syms[i] == c) {
			is_c_unshiftable = true; break;
		}
	}

	if (!is_c_unshiftable) return c;

	const uint8 unshift_sym[] = {
		['!'] = '1',
		['@'] = '2',
		['#'] = '3',
		['$'] = '4',
		['%'] = '5',
		['^'] = '6',
		['&'] = '7',
		['*'] = '8',
		['('] = '9',
		[')'] = '0',
		['_'] = '-',
		['+'] = '=',
		['?'] = '/',
		['<'] = ',',
		['>'] = '.',
		['~'] = '`'
	};
	
	return unshift_sym[c];
}

void kmain(uint32 magic, multiboot_info_t* multiboot) {
	ram = (byte*)get_ram();

	init_std(ram + 0xB8000);

	byte style = COLOR_BRIGHT_RED | (COLOR_BLACK << 4);

	clear_screen(style);

	set_style(style);

	for (_size_t i = 0; i < 16; i++) {
		set_style(i | (COLOR_BLACK << 4));

		kprintf("Hello, world!\n");
	}

	for (;;) halt();

	kprintf("Type something: ");

	bool shifted = false, entered = false;

	_ssize_t index = 0;

	byte* buf = ram;

	while (!entered) {
		in8(0x80);

		bool active = in8(0x64) & 1;

		byte scancode = in8(0x60);

		if (active && scancode) {
			kprintf("%x ", scancode);
		}

		// if (active && scancode) {
		// 	bool scancode_released = scancode & 0x80;

		// 	scancode = scancode & 0x7F;

		// 	if (scancode == SCANCODE_CAPSLOCK) {
		// 		shifted = !scancode_released;

		// 		continue;
		// 	}

		// 	if (scancode == SCANCODE_ENTER) {
		// 		entered = true; break;
		// 	}

		// 	byte scancode_to_c[] = {
		// 		[SCANCODE_NULL] = 					'\0',
		// 		[SCANCODE_ESCAPE] = 				'\x1B',
		// 		[SCANCODE_0] = 						'0',
		// 		[SCANCODE_1] = 						'1',
		// 		[SCANCODE_2] = 						'2',
		// 		[SCANCODE_3] = 						'3',
		// 		[SCANCODE_4] = 						'4',
		// 		[SCANCODE_5] = 						'5',
		// 		[SCANCODE_6] = 						'6',
		// 		[SCANCODE_7] = 						'7',
		// 		[SCANCODE_8] = 						'8',
		// 		[SCANCODE_9] = 						'9',
		// 		[SCANCODE_MINUS_SIGN] =				'-',
		// 		[SCANCODE_EQUAL] =					'=',
		// 		[SCANCODE_BACKSPACE] = 				'\b',
		// 		[SCANCODE_TAB] = 					'\t',
		// 		[SCANCODE_Q] = 						'q',
		// 		[SCANCODE_W] = 						'w',
		// 		[SCANCODE_E] = 						'e',
		// 		[SCANCODE_R] = 						'r',
		// 		[SCANCODE_T] = 						't',
		// 		[SCANCODE_Y] = 						'y',
		// 		[SCANCODE_U] = 						'u',
		// 		[SCANCODE_I] = 						'i',
		// 		[SCANCODE_O] = 						'o',
		// 		[SCANCODE_P] = 						'p',
		// 		[SCANCODE_LEFT_BRACKET] = 			'[',
		// 		[SCANCODE_RIGHT_BRACKET] = 			']',
		// 		[SCANCODE_ENTER] = 					'\n',
		// 		[SCANCODE_A] = 						'a',
		// 		[SCANCODE_S] = 						's',
		// 		[SCANCODE_D] = 						'd',
		// 		[SCANCODE_F] = 						'f',
		// 		[SCANCODE_G] = 						'g',
		// 		[SCANCODE_H] = 						'h',
		// 		[SCANCODE_J] = 						'j',
		// 		[SCANCODE_K] = 						'k',
		// 		[SCANCODE_L] = 						'l',
		// 		[SCANCODE_SEMICOLON] = 				':',
		// 		[SCANCODE_QUOTE] = 					'?',
		// 		[SCANCODE_BACK_TICK] = 				'`',
		// 		[SCANCODE_BACKSLASH] = 				'\\',
		// 		[SCANCODE_Z] = 						'z',
		// 		[SCANCODE_X] = 						'x',
		// 		[SCANCODE_C] = 						'c',
		// 		[SCANCODE_V] = 						'v',
		// 		[SCANCODE_B] = 						'b',
		// 		[SCANCODE_N] = 						'n',
		// 		[SCANCODE_M] = 						'm',
		// 		[SCANCODE_COMMA] = 					',',
		// 		[SCANCODE_DOT] = 					'.',
		// 		[SCANCODE_SLASH] = 					'/',
		// 		[SCANCODE_NUMBER_PAD_ASTERISK] = 	'*',
		// 		[SCANCODE_SPACE] = 					' ',
		// 		[SCANCODE_NUMBER_PAD_7] = 			'7',
		// 		[SCANCODE_NUMBER_PAD_8] = 			'8',
		// 		[SCANCODE_NUMBER_PAD_9] = 			'9',
		// 		[SCANCODE_NUMBER_PAD_MINUS_SIGN] = 	'-',
		// 		[SCANCODE_NUMBER_PAD_4] = 			'4',
		// 		[SCANCODE_NUMBER_PAD_5] = 			'5',
		// 		[SCANCODE_NUMBER_PAD_6] = 			'6',
		// 		[SCANCODE_NUMBER_PAD_PLUS_SIGN] = 	'+',
		// 		[SCANCODE_NUMBER_PAD_1] = 			'1',
		// 		[SCANCODE_NUMBER_PAD_2] = 			'2',
		// 		[SCANCODE_NUMBER_PAD_3] = 			'3',
		// 		[SCANCODE_NUMBER_PAD_0] = 			'0',
		// 		[SCANCODE_NUMBER_PAD_DOT] = 		'.',
		// 	};

		// 	byte c = scancode_to_c[scancode];

		// 	c = shifted ? shift(c) : unshift(c);
	
		// 	if ((c >= ' ' && c <= '~')) {
		// 		putch(c);

		// 		buf[index] = c;

		// 		index += 1;
		// 	}

		// 	else if (c == '\b' && index > 0) {
		// 		index--;

		// 		buf[index] = ' ';
				
		// 		kprint("\b \b");
		// 	}

		// 	else if (c == '\n' || c == '\r') {
		// 		entered = true;
		// 	}
		// }
	}

	// buf[index] = '\0';

	// kprintf("\n\rYou writed: %s\n\r", buf);

	for (;;) halt();
}
