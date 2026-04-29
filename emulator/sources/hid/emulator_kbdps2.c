#include "hid/emulator_kbdps2.h"

#include "main.h"

#include "emulator.h"

#include "emulator_io.h"

#include "emulator_logger.h"

#include "types.h"

#ifdef IS_UNIX
#include <unistd.h>
#endif

#include <stdlib.h>

#include <string.h>

#include <ctype.h>

#ifdef EMULATOR_SDL_USING
static byte buf_res[2] = { 0 };

static byte* sdl_scancode_to_ps2_set1(byte _scancode) {
	const byte supported_sdl_scan[] = {
		SDL_SCANCODE_A, SDL_SCANCODE_B, 
		SDL_SCANCODE_C, SDL_SCANCODE_D, 
		SDL_SCANCODE_E, SDL_SCANCODE_F, 
		SDL_SCANCODE_G, SDL_SCANCODE_H, 
		SDL_SCANCODE_I, SDL_SCANCODE_J, 
		SDL_SCANCODE_K, SDL_SCANCODE_L, 
		SDL_SCANCODE_M, SDL_SCANCODE_N, 
		SDL_SCANCODE_O, SDL_SCANCODE_P, 
		SDL_SCANCODE_Q, SDL_SCANCODE_R, 
		SDL_SCANCODE_S, SDL_SCANCODE_T, 
		SDL_SCANCODE_U, SDL_SCANCODE_V, 
		SDL_SCANCODE_W, SDL_SCANCODE_X, 
		SDL_SCANCODE_Y, SDL_SCANCODE_Z, 
		SDL_SCANCODE_1, SDL_SCANCODE_2, 
		SDL_SCANCODE_3, SDL_SCANCODE_4, 
		SDL_SCANCODE_5, SDL_SCANCODE_6, 
		SDL_SCANCODE_7, SDL_SCANCODE_8, 
		SDL_SCANCODE_9, SDL_SCANCODE_0, 
		SDL_SCANCODE_RETURN, SDL_SCANCODE_ESCAPE, 
		SDL_SCANCODE_BACKSPACE, SDL_SCANCODE_TAB, 
		SDL_SCANCODE_SPACE, SDL_SCANCODE_MINUS, 
		SDL_SCANCODE_EQUALS, SDL_SCANCODE_LEFTBRACKET, 
		SDL_SCANCODE_RIGHTBRACKET, SDL_SCANCODE_BACKSLASH, 
		SDL_SCANCODE_SEMICOLON, SDL_SCANCODE_APOSTROPHE, 
		SDL_SCANCODE_GRAVE, SDL_SCANCODE_COMMA, 
		SDL_SCANCODE_PERIOD, SDL_SCANCODE_SLASH, 
		SDL_SCANCODE_CAPSLOCK, SDL_SCANCODE_F1, 
		SDL_SCANCODE_F2, SDL_SCANCODE_F3, SDL_SCANCODE_F4, 
		SDL_SCANCODE_F5, SDL_SCANCODE_F6, SDL_SCANCODE_F7, 
		SDL_SCANCODE_F8, SDL_SCANCODE_F9, SDL_SCANCODE_F10, 
		SDL_SCANCODE_F11, SDL_SCANCODE_F12, SDL_SCANCODE_SCROLLLOCK, 
		SDL_SCANCODE_NUMLOCKCLEAR, SDL_SCANCODE_KP_MINUS, 
		SDL_SCANCODE_KP_PLUS, SDL_SCANCODE_KP_1, SDL_SCANCODE_KP_2, 
		SDL_SCANCODE_KP_3, SDL_SCANCODE_KP_4, SDL_SCANCODE_KP_5, 
		SDL_SCANCODE_KP_6, SDL_SCANCODE_KP_7, SDL_SCANCODE_KP_8, 
		SDL_SCANCODE_KP_9, SDL_SCANCODE_KP_0, SDL_SCANCODE_KP_PERIOD, 
		SDL_SCANCODE_LCTRL, SDL_SCANCODE_LSHIFT, SDL_SCANCODE_LALT, 
		SDL_SCANCODE_RCTRL, SDL_SCANCODE_RSHIFT
	};

	const byte sdl_scan_to_ps2_set1[] = {
		[SDL_SCANCODE_A] = SCANCODE_A,
		[SDL_SCANCODE_B] = SCANCODE_B,
		[SDL_SCANCODE_C] = SCANCODE_C,
		[SDL_SCANCODE_D] = SCANCODE_D,
		[SDL_SCANCODE_E] = SCANCODE_E,
		[SDL_SCANCODE_F] = SCANCODE_F,
		[SDL_SCANCODE_G] = SCANCODE_G,
		[SDL_SCANCODE_H] = SCANCODE_H,
		[SDL_SCANCODE_I] = SCANCODE_I,
		[SDL_SCANCODE_J] = SCANCODE_J,
		[SDL_SCANCODE_K] = SCANCODE_K,
		[SDL_SCANCODE_L] = SCANCODE_L,
		[SDL_SCANCODE_M] = SCANCODE_M,
		[SDL_SCANCODE_N] = SCANCODE_N,
		[SDL_SCANCODE_O] = SCANCODE_O,
		[SDL_SCANCODE_P] = SCANCODE_P,
		[SDL_SCANCODE_Q] = SCANCODE_Q,
		[SDL_SCANCODE_R] = SCANCODE_R,
		[SDL_SCANCODE_S] = SCANCODE_S,
		[SDL_SCANCODE_T] = SCANCODE_T,
		[SDL_SCANCODE_U] = SCANCODE_U,
		[SDL_SCANCODE_V] = SCANCODE_V,
		[SDL_SCANCODE_W] = SCANCODE_W,
		[SDL_SCANCODE_X] = SCANCODE_X,
		[SDL_SCANCODE_Y] = SCANCODE_Y,
		[SDL_SCANCODE_Z] = SCANCODE_Z,

		[SDL_SCANCODE_1] = SCANCODE_1,
		[SDL_SCANCODE_2] = SCANCODE_2,
		[SDL_SCANCODE_3] = SCANCODE_3,
		[SDL_SCANCODE_4] = SCANCODE_4,
		[SDL_SCANCODE_5] = SCANCODE_5,
		[SDL_SCANCODE_6] = SCANCODE_6,
		[SDL_SCANCODE_7] = SCANCODE_7,
		[SDL_SCANCODE_8] = SCANCODE_8,
		[SDL_SCANCODE_9] = SCANCODE_9,
		[SDL_SCANCODE_0] = SCANCODE_0,

		[SDL_SCANCODE_RETURN] = SCANCODE_ENTER,
		[SDL_SCANCODE_ESCAPE] = SCANCODE_ESCAPE,
		[SDL_SCANCODE_BACKSPACE] = SCANCODE_BACKSPACE,
		[SDL_SCANCODE_TAB] = SCANCODE_TAB,
		[SDL_SCANCODE_SPACE] = SCANCODE_SPACE,

		[SDL_SCANCODE_MINUS] = SCANCODE_MINUS_SIGN,
		[SDL_SCANCODE_EQUALS] = SCANCODE_EQUAL,
		[SDL_SCANCODE_LEFTBRACKET] = SCANCODE_LEFT_BRACKET,
		[SDL_SCANCODE_RIGHTBRACKET] = SCANCODE_RIGHT_BRACKET,
		[SDL_SCANCODE_BACKSLASH] = SCANCODE_BACKSLASH, 
		[SDL_SCANCODE_SEMICOLON] = SCANCODE_SEMICOLON,
		[SDL_SCANCODE_APOSTROPHE] = SCANCODE_QUOTE,
		[SDL_SCANCODE_GRAVE] = SCANCODE_BACK_TICK, 
		[SDL_SCANCODE_COMMA] = SCANCODE_COMMA,
		[SDL_SCANCODE_PERIOD] = SCANCODE_DOT,
		[SDL_SCANCODE_SLASH] = SCANCODE_SLASH,

		[SDL_SCANCODE_CAPSLOCK] = SCANCODE_CAPSLOCK,

		[SDL_SCANCODE_F1] = SCANCODE_F1,
		[SDL_SCANCODE_F2] = SCANCODE_F2,
		[SDL_SCANCODE_F3] = SCANCODE_F3,
		[SDL_SCANCODE_F4] = SCANCODE_F4,
		[SDL_SCANCODE_F5] = SCANCODE_F5,
		[SDL_SCANCODE_F6] = SCANCODE_F6,
		[SDL_SCANCODE_F7] = SCANCODE_F7,
		[SDL_SCANCODE_F8] = SCANCODE_F8,
		[SDL_SCANCODE_F9] = SCANCODE_F9,
		[SDL_SCANCODE_F10] = SCANCODE_F10,
		[SDL_SCANCODE_F11] = SCANCODE_F11,
		[SDL_SCANCODE_F12] = SCANCODE_F12,

		// [SDL_SCANCODE_PRINTSCREEN] = SCANCODE_PRIN,
		[SDL_SCANCODE_SCROLLLOCK] = SCANCODE_SCROLL_LOCK,
		// [SDL_SCANCODE_PAUSE] = SCANCODE_PAU,
		// [SDL_SCANCODE_INSERT] = SCANCODE_INSER, // < insert on PC
		// [SDL_SCANCODE_HOME] = SCANCODE_HOM,
		// [SDL_SCANCODE_PAGEUP] = SCANCODE_PAGE,
		// [SDL_SCANCODE_DELETE] = SCANCODE_DEL,
		// [SDL_SCANCODE_END] = SCANODE_END,
		// [SDL_SCANCODE_PAGEDOWN] = 78,
		// [SDL_SCANCODE_RIGHT] = SCANCODE_RIGH,
		// [SDL_SCANCODE_LEFT] = 80,
		// [SDL_SCANCODE_DOWN] = 81,
		// [SDL_SCANCODE_UP] = 82,

		[SDL_SCANCODE_NUMLOCKCLEAR] = SCANCODE_NUMBER_PAD_LOCK, // < num lock on PC, clear on Mac keyboards
		// [SDL_SCANCODE_KP_DIVIDE] = SCANCODE_NUMBER,
		// [SDL_SCANCODE_KP_MULTIPLY] = SCANCODE_MULTI,
		[SDL_SCANCODE_KP_MINUS] = SCANCODE_NUMBER_PAD_MINUS_SIGN,
		[SDL_SCANCODE_KP_PLUS] = SCANCODE_NUMBER_PAD_PLUS_SIGN,
		// [SDL_SCANCODE_KP_ENTER] = SCANCODE_NUMBER_PAD_ENT,
		[SDL_SCANCODE_KP_1] = SCANCODE_NUMBER_PAD_1,
		[SDL_SCANCODE_KP_2] = SCANCODE_NUMBER_PAD_2,
		[SDL_SCANCODE_KP_3] = SCANCODE_NUMBER_PAD_3,
		[SDL_SCANCODE_KP_4] = SCANCODE_NUMBER_PAD_4,
		[SDL_SCANCODE_KP_5] = SCANCODE_NUMBER_PAD_5,
		[SDL_SCANCODE_KP_6] = SCANCODE_NUMBER_PAD_6,
		[SDL_SCANCODE_KP_7] = SCANCODE_NUMBER_PAD_7,
		[SDL_SCANCODE_KP_8] = SCANCODE_NUMBER_PAD_8,
		[SDL_SCANCODE_KP_9] = SCANCODE_NUMBER_PAD_9,
		[SDL_SCANCODE_KP_0] = SCANCODE_NUMBER_PAD_0,
		[SDL_SCANCODE_KP_PERIOD] = SCANCODE_NUMBER_PAD_DOT,

		// [SDL_SCANCODE_APPLICATION] = SCANCODE_WIN, /**< windows contextual menu, compose */
		// [SDL_SCANCODE_KP_EQUALS] = SCANCODE_NUMBER_PAD,
		// [SDL_SCANCODE_MENU] = SCANCODE_MENU,    /**< Menu (show menu) */

		// [SDL_SCANCODE_KP_LEFTPAREN] = SCANCODE_NUMBER_PAD_LEF,
		// [SDL_SCANCODE_KP_RIGHTPAREN] = 183,
		// [SDL_SCANCODE_KP_LEFTBRACE] = 184,
		// [SDL_SCANCODE_KP_RIGHTBRACE] = 185,
		// [SDL_SCANCODE_KP_BACKSPACE] = 187,
		// [SDL_SCANCODE_KP_COLON] = 203,

		[SDL_SCANCODE_LCTRL] = SCANCODE_CONTROL,
		[SDL_SCANCODE_LSHIFT] = SCANCODE_LEFT_SHIFT,
		[SDL_SCANCODE_LALT] = SCANCODE_LEFT_ALT, /**< alt, option */
		// [SDL_SCANCODE_LGUI] = SCANCODE_LEFT_SHIFT, /**< windows, command (apple), meta */
		[SDL_SCANCODE_RCTRL] = SCANCODE_CONTROL,
		[SDL_SCANCODE_RSHIFT] = SCANCODE_RIGHT_SHIFT,
		// [SDL_SCANCODE_RALT] = SCANCODE_RIGHT_ALT, /**< alt gr, option */
		// [SDL_SCANCODE_RGUI] = 231,
	};

	const byte supported_sdl_ext_scan[] = {
		SDL_SCANCODE_UP,
		SDL_SCANCODE_LEFT,
		SDL_SCANCODE_RIGHT,
		SDL_SCANCODE_DOWN,
	};

	const byte sdl_ext_scan_to_ps2_set_ext[] = {
		[SDL_SCANCODE_UP] = 	EXT_SCANCODE_ARROW_UP,
		[SDL_SCANCODE_LEFT] = 	EXT_SCANCODE_ARROW_LEFT,
		[SDL_SCANCODE_RIGHT] = 	EXT_SCANCODE_ARROW_RIGHT,
		[SDL_SCANCODE_DOWN] = 	EXT_SCANCODE_ARROW_DOWN,
	};

	const size_t supported_sdl_scan_cnt = sizeof(supported_sdl_scan) / sizeof(supported_sdl_scan[0]);

	const size_t supported_sdl_ext_scan_cnt = sizeof(supported_sdl_ext_scan) / sizeof(supported_sdl_ext_scan[0]);

	memset(buf_res, 0, sizeof(buf_res));

	for (size_t i = 0; i < supported_sdl_scan_cnt; i++) {
		if (supported_sdl_scan[i] == _scancode) {
			buf_res[0] = sdl_scan_to_ps2_set1[_scancode]; break;
		}
	}

	for (size_t i = 0; i < supported_sdl_ext_scan_cnt; i++) {
		if (supported_sdl_ext_scan[i] == _scancode) {
			buf_res[0] = SCANCODE_EXTENDED;

			buf_res[1] = sdl_ext_scan_to_ps2_set_ext[_scancode]; break;
		}
	}

	return buf_res;
}
#endif

static kbdps2_t* cur = nullptr;

static _size_t read_index = 0, writed_cnt = 0;

static bool available = false, previous_shifted = false;

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

static uint8 shift(uint8 c) {
	if (isalpha(c)) return toupper(c);

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

static uint8 unshift(uint8 c) {
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
	
	return unshift_sym[c];
}

static byte c_to_scancode[] = {
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
	[' '] = 	SCANCODE_SPACE,
};

byte previous_pressed_key = '\0';

time_t last_time = 0, cur_time = 0;

static bool pressed = true;

static void send_make(byte scancode) {
	if (!cur) return;

	cur->pressed_key[writed_cnt] = scancode;

	size_t buf_size = sizeof(cur->pressed_key);

	writed_cnt = (writed_cnt + 1) % buf_size;

	call_emulator_int(nullptr, 0x21);
}

static void send_brake(byte scancode) {
	if (!cur) return;

	send_make(scancode | 0x80);
}

#ifdef EMULATOR_SDL_USING
void handle_key_gui(byte sdl_scancode, bool is_key_released) {
	if (sdl_scancode == SDL_SCANCODE_F10) { // OMG six seven
		emulator_log(false, LOG_SEVERITY_INFO, "Exiting emulator because pressed F10...");
	
		exit_emulator(0);

		return;
	}

	if (!cur) return;

	available = false;

	writed_cnt = 0;

	read_index = 0;

	size_t buf_size = sizeof(cur->pressed_key);

	memset(cur->pressed_key, 0, buf_size);

	byte* scancode = sdl_scancode_to_ps2_set1(sdl_scancode);

	for (size_t i = 0; scancode[i] && i < 2; i++) {
		if (is_key_released)
			scancode[i] |= 0x80;

		send_make(scancode[i]);
	}

	available = true;
}
#endif

static byte read_ch() {
	byte c = '\0';

	read(STDIN_FILENO, &c, 1);

	while (!c) {
		read_index = read(STDIN_FILENO, &c, 1);

		if (read_index == 0) {
			emulator_log(false, LOG_SEVERITY_INFO, "Exiting emulator because pressed Ctrl+C...");
	
			exit_emulator(0);

			return '\0';
		}
	}

	return c;
}

void handle_keys_cli() {
	byte c = '\0';

	_ssize_t read_num = read(STDIN_FILENO, &c, 1);

	if (read_num == 0 || c == 0x3) {
		emulator_log(false, LOG_SEVERITY_INFO, "Exiting emulator because pressed Ctrl+C...");
	
		exit_emulator(0);

		return;
	}

	if (!cur) return;

	if (!c) return;

	available = false;

	writed_cnt = 0;

	read_index = 0;

	if (c == '\x1B' && read_ch() == '[') {
		byte letter = read_ch();

		const char arrows_letter[] = "ABCD";

		const char arrows[] = {
			EXT_SCANCODE_ARROW_UP,
			EXT_SCANCODE_ARROW_DOWN,
			EXT_SCANCODE_ARROW_RIGHT,
			EXT_SCANCODE_ARROW_LEFT,
		};

		bool find = false;

		for (size_t i = 0; i < sizeof(arrows); i++) {
			if (arrows_letter[i] == letter) {
				c = arrows[i]; find = true; break;
			}
		}

		if (find) {
			send_make(SCANCODE_EXTENDED);

			send_make(c);

			available = true;

			cur_time += 10;

			return;
		}
	}

	const c_str chars = "\xE0" "\x1B" "0123456789-=\b\x7F\tqwertyuiop[]\rasdfghjkl;'\\zxcvbnm,./* ";

	size_t chars_cnt = strlen(chars);

	bool supported = false;

	for (size_t i = 0; i < chars_cnt; i++) {
		if (chars[i] == c) {
			supported = true; break;
		}
	}

	if (!supported) return;

	size_t buf_size = sizeof(cur->pressed_key);

	memset(cur->pressed_key, 0, buf_size);

	bool is_shifted = isshifted(c);

	if (previous_pressed_key != '\0') {
		if (c != '\0' && previous_pressed_key != c) {
			send_brake(c_to_scancode[unshift(previous_pressed_key)]);
		}

		else if (c == '\0' && (cur_time - last_time) == 100) {
			send_brake(c_to_scancode[unshift(previous_pressed_key)]);

			previous_pressed_key = c;
		}

		if (previous_shifted) {
			send_brake(SCANCODE_LEFT_SHIFT);

			previous_shifted = false;
		}
	}

	if (c != '\0') {
		// byte previous_key = cur->pressed_key[read_index];

		if (is_shifted) {
			send_make(SCANCODE_LEFT_SHIFT);
		}

		previous_shifted = is_shifted;
		
		send_make(c_to_scancode[unshift(c)]);

		last_time = cur_time;

		previous_pressed_key = c;
	}

	cur_time += 10;

	// emulator_log(false, LOG_SEVERITY_WARNING, "Sended bytes: 0x%x 0x%x 0x%x 0x%x", cur->pressed_key[0], cur->pressed_key[1], cur->pressed_key[2], cur->pressed_key[3]);

	available = true;
}

// You need to call function to handle keys it yourself
kbdps2_t* init_kbdps2(bool gui) {
	emulator_log(true, LOG_SEVERITY_VERBOSE, "PS/2 Keyboard initialization...");

	kbdps2_t* kbdps2 = malloc(sizeof(kbdps2_t));

	memset(kbdps2, 0, sizeof(kbdps2_t));

	size_t buf_size = sizeof(kbdps2->pressed_key);

	memset(kbdps2->pressed_key, 0, buf_size);
	
	emulator_log(false, LOG_SEVERITY_VERBOSE, "Setting up ports (0x60, 0x64) for PS/2 Keyboard...");

	emulator_setup_port_in(0x60, kbdps2_read_data);

	emulator_setup_port_in(0x64, kbdps2_read_command);

	emulator_setup_port_out(0x64, kbdps2_write_command);
	
	cur = kbdps2;

	emulator_log(true, LOG_SEVERITY_VERBOSE, "PS/2 Keyboard initialized");

	return kbdps2;
}

void free_kbdps2(kbdps2_t* kbdps2) {
	emulator_log(false, LOG_SEVERITY_VERBOSE, "PS/2 Keyboard deinitialization...");

	if (kbdps2) free(kbdps2);

	if (cur == kbdps2) {
		cur = nullptr;
	}

	emulator_log(false, LOG_SEVERITY_VERBOSE, "PS/2 Keyboard deinitialized");
}

void reset_kbdps2(kbdps2_t* kbdps2) {
	if (!kbdps2) return;

	emulator_log(false, LOG_SEVERITY_VERBOSE, "PS/2 Keyboard reseting...");

	size_t buf_size = sizeof(kbdps2->pressed_key);

	memset(kbdps2->pressed_key, 0, buf_size);

	emulator_log(false, LOG_SEVERITY_VERBOSE, "PS/2 Keyboard reseting done!");
}

void release_all_kbdps2() {
	emulator_release_port_in(0x60);

	emulator_release_port_in(0x64);

	if (cur) free(cur);

	cur = nullptr;
}
