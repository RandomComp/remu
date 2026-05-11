#include "drivers/hid/kbdps2.h"

#include "types.h"

#include "idt.h"

#include "std.h"

#include "drivers/video/vga.h"

#include "builtins/builtins.h"

#include "builtins/string.h"

#include "drivers/io.h"

#include "math/math.h"

#include "terminal.h"

static key_state_t key_states[128] = { KEY_STATE_NULL };

static byte updated_keys[128] = { 0 }; static size_t updated_key_index = 0;

static byte updated_key = 0; static key_state_t updated_key_state = KEY_STATE_NULL;

static byte scancode_to_c[] = {
	[SCANCODE_NULL] = 			'\0',
	[SCANCODE_ESCAPE] = 		0x80,
	[SCANCODE_0] = 				'0',
	[SCANCODE_1] = 				'1',
	[SCANCODE_2] = 				'2',
	[SCANCODE_3] = 				'3',
	[SCANCODE_4] = 				'4',
	[SCANCODE_5] = 				'5',
	[SCANCODE_6] = 				'6',
	[SCANCODE_7] = 				'7',
	[SCANCODE_8] = 				'8',
	[SCANCODE_9] = 				'9',
	[SCANCODE_MINUS_SIGN] = 	'-',
	[SCANCODE_EQUAL] = 			'=',
	[SCANCODE_BACKSPACE] = 		'\b',
	[SCANCODE_TAB] = 			'\t',
	[SCANCODE_Q] = 				'q',
	[SCANCODE_W] = 				'w',
	[SCANCODE_E] = 				'e',
	[SCANCODE_R] = 				'r',
	[SCANCODE_T] = 				't',
	[SCANCODE_Y] = 				'y',
	[SCANCODE_U] = 				'u',
	[SCANCODE_I] = 				'i',
	[SCANCODE_O] = 				'o',
	[SCANCODE_P] = 				'p',
	[SCANCODE_LEFT_BRACKET] = 	'[',
	[SCANCODE_RIGHT_BRACKET] = 	']',
	[SCANCODE_ENTER] = 			'\r',
	[SCANCODE_A] = 				'a',
	[SCANCODE_S] = 				's',
	[SCANCODE_D] = 				'd',
	[SCANCODE_F] = 				'f',
	[SCANCODE_G] = 				'g',
	[SCANCODE_H] = 				'h',
	[SCANCODE_J] = 				'j',
	[SCANCODE_K] = 				'k',
	[SCANCODE_L] = 				'l',
	[SCANCODE_SEMICOLON] = 		';',
	[SCANCODE_QUOTE] = 			'\'',
	[SCANCODE_BACK_TICK] = 		'`',
	[SCANCODE_BACKSLASH] = 		'\\',
	[SCANCODE_Z] = 				'z',
	[SCANCODE_X] = 				'x',
	[SCANCODE_C] = 				'c',
	[SCANCODE_V] = 				'v',
	[SCANCODE_B] = 				'b',
	[SCANCODE_N] = 				'n',
	[SCANCODE_M] = 				'm',
	[SCANCODE_COMMA] = 			',',
	[SCANCODE_DOT] = 			'.',
	[SCANCODE_SLASH] = 			'/',
	[SCANCODE_SPACE] = 			' ',
};

static byte shift(byte c) {
	if (isalpha(c))
		return upper(c);

	const byte* shitftable_syms = "1234567880-=[]\\;'`/,.";

	bool is_c_shiftable = false;

	for (size_t i = 0; i < strlen(shitftable_syms); i++) {
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
		['['] = '{',
		[']'] = '}',
		['\\'] = '|',
		[';'] = ':',
		['\''] = '"',
		['`'] = '~',
		['/'] = '?',
		[','] = '<',
		['.'] = '>'
	};
	
	return shift_sym[c];
}

static bool isshifted(uint8 c) {
	if (isupper(c)) return true;

	const byte* shifted_sym = "!@#$%^&*()_+?<>~";

	for (size_t i = 0; i < strlen(shifted_sym); i++) {
		if (c == shifted_sym[i]) return true;
	}
	
	return false;
}

static uint8 unshift(uint8 c) {
	if (isalpha(c)) return lower(c);

	const byte* unshitftable_syms = "!@#$%^&*()_+?<>~";

	bool is_c_unshiftable = false;

	for (size_t i = 0; i < strlen(unshitftable_syms); i++) {
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

static bool extended = false;

static void kbd_handler(struct registers_t* regs) {
	bool active = in8(0x64);
	
	byte scancode = in8(0x60);

	if (!active || !scancode) return;

	if (scancode == SCANCODE_EXTENDED) {
		extended = true; return;
	}

	bool released = scancode & 0x80;

	scancode &= 0x7f;

	if (!scancode) return;

	if (released) {
		key_states[scancode] = KEY_STATE_RELEASED;

		// kprintf("Key %x released\n", scancode);
	}
	
	else if (key_states[scancode] == KEY_STATE_NULL || 
		key_states[scancode] == KEY_STATE_RELEASED) {
		key_states[scancode] = KEY_STATE_PRESSED;

		// kprintf("Key %x pressed\n", scancode);
	}

	else if (key_states[scancode] != KEY_STATE_RELEASED) {
		key_states[scancode] = KEY_STATE_HOLDED;

		// kprintf("Key %x holded\n", scancode);
	}

	updated_key = scancode; updated_key_state = key_states[scancode];
		
	updated_keys[updated_key_index] = scancode;

	updated_key_index = (updated_key_index + 1) % 128;
}

void kbdps2_init() {
	IDTIRQInstallHandler(1, kbd_handler);
}

terminal_in_t init_kbdps2_stdin() {
	return (terminal_in_t){
		.getch = kbdps2_getch
	};
}

static bool capslocked = false, shifted = false;

byte kbdps2_getch() {
	if (!updated_key) return '\0';

	byte c = '\0';

	if (extended) {
		while (updated_key == c) {
			#ifdef _EMULATOR
			halt();
			#endif
		}

		extended = false;

		if ((updated_key_state != KEY_STATE_NULL) &&
			(updated_key_state != KEY_STATE_RELEASED)) {
			if (updated_key == EXT_SCANCODE_ARROW_UP) {
				c = '\x18';
			}

			else if (updated_key == EXT_SCANCODE_ARROW_DOWN) {
				c = '\x19';
			}

			else if (updated_key == EXT_SCANCODE_ARROW_RIGHT) {
				c = '\x1A';
			}

			else if (updated_key == EXT_SCANCODE_ARROW_LEFT) {
				c = '\x1B';
			}
		}

		extended = false;

		key_states[updated_key] = KEY_STATE_NULL;

		updated_key = 0;

		return c;
	}

	if (updated_key == SCANCODE_CAPSLOCK &&
		(updated_key_state == KEY_STATE_RELEASED)) {
		capslocked = !capslocked; return '\0';
	}

	else if (updated_key == SCANCODE_LEFT_SHIFT) {
		if (updated_key_state == KEY_STATE_PRESSED || 
			updated_key_state == KEY_STATE_HOLDED) {
			shifted = true; return '\0';
		}

		else if (updated_key_state == KEY_STATE_RELEASED) {
			shifted = false; return '\0';
		}
	}

	else if ((updated_key_state != KEY_STATE_NULL) &&
		(updated_key_state != KEY_STATE_RELEASED)) {
		c = scancode_to_c[updated_key];
	}

	key_states[updated_key] = KEY_STATE_NULL;

	updated_key = 0;

	return (shifted || capslocked) ? shift(c) : c;
}

void kbdps2_deinit() {
	IDTIRQUninstallHandler(0x21);
}
