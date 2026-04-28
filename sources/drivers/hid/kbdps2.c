#include "drivers/hid/kbdps2.h"

#include "types.h"

#include "idt.h"

#include "std.h"

#include "builtins/builtins.h"

#include "builtins/string.h"

#include "drivers/memory/memory.h"

#include "math/math.h"

static key_state_t key_states[128] = { KEY_STATE_NULL };

static byte updated_keys[128] = { 0 }; static size_t updated_key_index = 0;

static byte updated_key = 0; static key_state_t updated_key_state = KEY_STATE_NULL;

static byte scancode_to_c[] = {
	[SCANCODE_NULL] = 			'\0',
	[SCANCODE_ESCAPE] = 		'\x1B',
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

	const c_str shitftable_syms = "1234567880-=/,.`";

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
		['/'] = '?',
		[','] = '<',
		['.'] = '>',
		['`'] = '~'
	};
	
	return shift_sym[c];
}

static bool isshifted(uint8 c) {
	if (isupper(c)) return true;

	const c_str shifted_sym = "!@#$%^&*()_+?<>~";

	for (size_t i = 0; i < strlen(shifted_sym); i++) {
		if (c == shifted_sym[i]) return true;
	}
	
	return false;
}

static uint8 unshift(uint8 c) {
	if (isalpha(c)) return lower(c);

	const c_str unshitftable_syms = "!@#$%^&*()_+?<>~";

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

static bool shifted = false;

byte getch() {
	if (!updated_key) return '\0';

	byte c = '\0';

	if (updated_key == SCANCODE_CAPSLOCK &&
		(updated_key_state == KEY_STATE_RELEASED)) {
		shifted = !shifted; return '\0';
	}

	else if (updated_key == SCANCODE_LEFT_SHIFT) {
		if (updated_key_state == KEY_STATE_PRESSED) {
			shifted = true; return '\0';
		}

		else if (updated_key_state == KEY_STATE_RELEASED) {
			shifted = false; return '\0';
		}
	}

	else if ((updated_key_state != KEY_STATE_NULL) &&
		(updated_key_state != KEY_STATE_RELEASED)) {
		if (extended) {
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

			extended = false;
		}

		else c = scancode_to_c[updated_key];
	}

	key_states[updated_key] = KEY_STATE_NULL;

	updated_key = 0;

	return shifted ? shift(c) : c;
}

static byte buf[64] = { 0 };

byte* getstr(bool show) {
	byte c = getch();

	ssize_t index = 0;

	while (index < 63) {
		while (!c) {
			c = getch();

			halt();
		}

		if (c == '\r') {
			if (show) kprint("\n\r");
			
			buf[index] = '\0'; break;
		}

		buf[index] = c;

		if (c == '\b') {
			if (index >= 1) {
				if (show) kprint("\b \b");

				index -= 1;
			}
		}

		else {
			if (show) putch(c);
			
			index += 1;
		}
	}
			
	buf[index] = '\0';

	return buf;
}

byte* getstr_hist(bool show, byte history[][64], ssize_t* command_index, size_t history_len) {
	ssize_t index = 0;

	ssize_t cur_x = 0, cur_y = 0;
		
	get_cursor_pos(&cur_x, &cur_y);

	memset(buf, 0, 64);

	while (index < 63) {
		byte c = 0;

		while (!c) {
			c = getch();

			halt();
		}

		if (c == '\x18') {
			*command_index = *command_index - 1;

			while ((*command_index) < 0)
				*command_index += history_len;

			memcpy(buf, history[*command_index], 64);
				
			index = strlen(buf);
				
			set_cursor_pos(cur_x, cur_y);
			clear_line();
			kprintf("%s [%i/%i]", buf, *command_index, history_len);

			continue;
		}

		else if (c == '\x19') {
			*command_index = (*command_index + 1) % history_len;

			memcpy(buf, history[*command_index], 64);
			
			index = strlen(buf);
			
			set_cursor_pos(cur_x, cur_y);
			clear_line();
			kprintf("%s [%i/%i]", buf, *command_index, history_len);
			
			continue;
		}

		else if (c == '\r') {
			buf[index] = '\0'; index = 64;
		}

		else if (c == '\b') {
			if (index >= 1) {
				index -= 1; buf[index] = ' ';
			}
		}

		else {
			buf[index] = c;
			
			index += 1;
		}

		set_cursor_pos(cur_x, cur_y);
		clear_line();
		kprint(buf);
	}

	kprint("\n");

	return buf;
}

void kbdps2_deinit() {
	IDTIRQUninstallHandler(0x21);
}
