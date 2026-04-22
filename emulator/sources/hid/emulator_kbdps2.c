#include "hid/emulator_kbdps2.h"

#include "main.h"

#include "emulator_io.h"

#include "types.h"

#ifdef IS_UNIX
#include <unistd.h>
#endif

#include <stdlib.h>

#include <string.h>

#include <ctype.h>

static kbdps2_t* cur = nullptr;

static _size_t read_index =  0, writed_cnt = 0;

static bool available = false;

static _size_t kbdps2_read_data() {
	if (!cur) return 0;

	if (!available || read_index >= 4) return 0;
	
	byte result = cur->pressed_key[read_index];

	read_index += 1;

	return result;
}

static _size_t kbdps2_read_command() {
	if (!cur) return 0;

	return available;
}

static void kbdps2_write_command(_size_t data) {
	if (!cur) return;

	if (data == 0xFE)
		reset_emulator(nullptr, 0x20);
}

static bool isshifted(uint8 c) {
	if (isupper(c)) return true;

	const c_str shifted_sym = "!@#$%^&*()_+?<>~";

	for (_size_t i = 0; i < strlen(shifted_sym); i++) {
		if (c == shifted_sym[i]) return true;
	}
	
	return false;
}

static uint8 unshifted(uint8 c) {
	if (isalpha(c)) return tolower(c);

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

	uint8 result = unshift_sym[c];
	
	return (result != 0) ? result : c;
}

static void handle_keys() {
	if (!cur) return;

	uint8 c = '\0';

	_ssize_t read_num = read(STDIN_FILENO, &c, 1);

	if (read_num == 0 || c == 0x3) {
		emulator_log(false, LOG_SEVERITY_INFO, "Exiting emulator because pressed Ctrl+C...\n");
	
		exit_emulator(0);

		return;
	}
	
	if (c != '\0') {
		byte scancode = 0x00;

		const c_str chars = "\x1B0123456789-=\b\x7F\tqwertyuiop[]\rasdfghjkl;'\\zxcvbnm,./* ";

		byte c_to_scancode[] = {
			['\0'] = 	SCANCODE_NULL,
			['\x1B'] = 	SCANCODE_ESCAPE,
			['0'] =		SCANCODE_0,
			['1'] =		SCANCODE_1,
			['2'] =		SCANCODE_2,
			['3'] =		SCANCODE_3,
			['4'] =		SCANCODE_4,
			['5'] =		SCANCODE_5,
			['6'] =		SCANCODE_6,
			['7'] =		SCANCODE_7,
			['8'] =		SCANCODE_8,
			['9'] =		SCANCODE_9,
			['-'] = 	SCANCODE_MINUS_SIGN,
			['='] = 	SCANCODE_EQUAL,
			['\b'] = 	SCANCODE_BACKSPACE,
			['\x7F'] = 	SCANCODE_BACKSPACE,
			['\t'] = 	SCANCODE_TAB,
			['q'] = 	SCANCODE_Q,
			['w'] = 	SCANCODE_W,
			['e'] = 	SCANCODE_E,
			['r'] = 	SCANCODE_R,
			['t'] = 	SCANCODE_T,
			['y'] = 	SCANCODE_Y,
			['u'] = 	SCANCODE_U,
			['i'] = 	SCANCODE_I,
			['o'] = 	SCANCODE_O,
			['p'] = 	SCANCODE_P,
			['['] = 	SCANCODE_LEFT_BRACKET,
			[']'] = 	SCANCODE_RIGHT_BRACKET,
			['\r'] = 	SCANCODE_ENTER,
			['a'] = 	SCANCODE_A,
			['s'] = 	SCANCODE_S,
			['d'] = 	SCANCODE_D,
			['f'] = 	SCANCODE_F,
			['g'] = 	SCANCODE_G,
			['h'] = 	SCANCODE_H,
			['j'] = 	SCANCODE_J,
			['k'] = 	SCANCODE_K,
			['l'] = 	SCANCODE_L,
			[';'] = 	SCANCODE_SEMICOLON,
			['\''] = 	SCANCODE_QUOTE,
			['`'] = 	SCANCODE_BACK_TICK,
			['\\'] = 	SCANCODE_BACKSLASH,
			['z'] = 	SCANCODE_Z,
			['x'] = 	SCANCODE_X,
			['c'] = 	SCANCODE_C,
			['v'] = 	SCANCODE_V,
			['b'] = 	SCANCODE_B,
			['n'] = 	SCANCODE_N,
			['m'] = 	SCANCODE_M,
			[','] = 	SCANCODE_COMMA,
			['.'] = 	SCANCODE_DOT,
			['/'] = 	SCANCODE_SLASH,
			['*'] = 	SCANCODE_NUMBER_PAD_ASTERISK,
			[' '] = 	SCANCODE_SPACE,
			['7'] = 	SCANCODE_NUMBER_PAD_7,
			['8'] = 	SCANCODE_NUMBER_PAD_8,
			['9'] = 	SCANCODE_NUMBER_PAD_9,
			['-'] = 	SCANCODE_NUMBER_PAD_MINUS_SIGN,
			['4'] = 	SCANCODE_NUMBER_PAD_4,
			['5'] = 	SCANCODE_NUMBER_PAD_5,
			['6'] = 	SCANCODE_NUMBER_PAD_6,
			['+'] = 	SCANCODE_NUMBER_PAD_PLUS_SIGN,
			['1'] = 	SCANCODE_NUMBER_PAD_1,
			['2'] = 	SCANCODE_NUMBER_PAD_2,
			['3'] = 	SCANCODE_NUMBER_PAD_3,
			['0'] = 	SCANCODE_NUMBER_PAD_0,
			['.'] = 	SCANCODE_NUMBER_PAD_DOT,
		};

		available = false;

		byte previous_key = cur->pressed_key[read_index];

		cur->pressed_key[0] = 0;

		cur->pressed_key[1] = 0;

		cur->pressed_key[2] = 0;

		writed_cnt = 1;

		read_index = 3;

		if (isshifted(c)) {
			cur->pressed_key[2] = SCANCODE_LEFT_SHIFT;

			read_index = 2;

			writed_cnt += 1;
		}

		// emulator_log(false, LOG_SEVERITY_INFO, "Pressed char \"%x\" \"%c\"\n\r", c, c);
		
		cur->pressed_key[3] = c_to_scancode[unshifted(c)];

		available = true;

		// emulator_log(false, LOG_SEVERITY_INFO, "Pressed bytes: 0x%x 0x%x 0x%x 0x%x\n", cur->pressed_key[0], cur->pressed_key[1], cur->pressed_key[2], cur->pressed_key[3]);
	}
}

kbdps2_t* init_kbdps2() {
	emulator_log(true, LOG_SEVERITY_VERBOSE, "PS/2 Keyboard initialization...\n");

	kbdps2_t* kbdps2 = malloc(sizeof(kbdps2_t));

	memset(kbdps2, 0, sizeof(kbdps2_t));

	memset(kbdps2->pressed_key, 0, 4);
	
	emulator_log(false, LOG_SEVERITY_VERBOSE, "Setting up timer (100 hz) to listen pressed keyboard keys...\n");

	emulator_setup_tick_timer(nullptr, &handle_keys, 10);

	emulator_log(false, LOG_SEVERITY_VERBOSE, "Setting up ports (0x60, 0x64) for PS/2 Keyboard...\n");

	emulator_setup_port_in(0x60, &kbdps2_read_data);

	emulator_setup_port_in(0x64, &kbdps2_read_command);

	emulator_setup_port_out(0x64, &kbdps2_write_command);
	
	cur = kbdps2;

	emulator_log(true, LOG_SEVERITY_VERBOSE, "PS/2 Keyboard initialized\n");

	return kbdps2;
}

void free_kbdps2(kbdps2_t* kbdps2) {
	emulator_log(false, LOG_SEVERITY_VERBOSE, "PS/2 Keyboard deinitialization...\n");

	if (kbdps2) free(kbdps2);

	if (cur == kbdps2) {
		cur = nullptr;
	}

	emulator_log(false, LOG_SEVERITY_VERBOSE, "PS/2 Keyboard deinitialized\n");
}

void reset_kbdps2(kbdps2_t* kbdps2) {
	if (!kbdps2) return;

	emulator_log(false, LOG_SEVERITY_VERBOSE, "PS/2 Keyboard reseting...\n");

	memset(kbdps2->pressed_key, 0, 4);

	emulator_log(false, LOG_SEVERITY_VERBOSE, "PS/2 Keyboard reseting done!\n");
}

void release_all_kbdps2() {
	emulator_release_port_in(0x60);

	emulator_release_port_in(0x64);

	emulator_release_tick_timer(nullptr, &handle_keys);

	if (cur) free(cur);

	cur = nullptr;
}