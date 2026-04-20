#include "hid/emulator_kbdps2.h"

#include "main.h"

#include "types.h"

#ifdef IS_UNIX
#include <unistd.h>
#endif

#include <stdlib.h>

#include <string.h>

static kbdps2_t* cur = nullptr;

static _size_t keyboard_data_port_handler() {
	if (!cur) return 0;

	_size_t result = cur->input_buffer[cur->input_buffer_index];

	if (result) {
		cur->input_buffer_index = (cur->input_buffer_index + 1) % INPUT_BUFFER_MAX_SIZE;
	}

	return result;
}

static _size_t keyboard_command_port_handler() {
	if (!cur) return 0;

	return 1;
}

static void handle_keys() {
	if (!cur) return;

	char c = '\0';

	_ssize_t read_num = read(STDIN_FILENO, &c, 1);

	if (read_num == 0 || c == 0x3) {
		emulator_log(true, LOG_SEVERITY_OK, "Exiting emulator because pressed Ctrl+C...\n\r");
	
		exit_emulator(0);

		return;
	}
	
	if (c != '\0') {
		cur->input_buffer[cur->input_buffer_index] = c;

		cur->input_buffer_index = (cur->input_buffer_index + 1) % INPUT_BUFFER_MAX_SIZE;
	}
	
	cur->input_buffer[(cur->input_buffer_index + 1) % INPUT_BUFFER_MAX_SIZE] = 0;
}

kbdps2_t* init_kbdps2() {
	emulator_log(true, LOG_SEVERITY_INFO, "PS/2 Keyboard initialization...\n\r");

	kbdps2_t* kbdps2 = malloc(sizeof(kbdps2_t));

	memset(kbdps2, 0, sizeof(kbdps2_t));

	kbdps2->input_buffer_index = 0;
	
	emulator_log(false, LOG_SEVERITY_INFO, "Setting up timer (100 hz) to listen pressed keyboard keys...\n\r");

	setup_tick_timer(&handle_keys, 10);

	emulator_log(false, LOG_SEVERITY_INFO, "Setting up ports (0x60, 0x64) for PS/2 Keyboard...\n\r");

	setup_port_in(0x60, &keyboard_data_port_handler);

	setup_port_in(0x64, &keyboard_command_port_handler);
	
	cur = kbdps2;

	emulator_log(true, LOG_SEVERITY_OK, "PS/2 Keyboard initialization done!\n\r");

	return kbdps2;
}

void free_kbdps2(kbdps2_t* kbdps2) {
	emulator_log(true, LOG_SEVERITY_INFO, "PS/2 Keyboard deinitialization...\n\r");

	if (kbdps2) free(kbdps2);

	if (cur == kbdps2) {
		cur = nullptr;
	}

	emulator_log(true, LOG_SEVERITY_OK, "PS/2 Keyboard deinitialization done!\n\r");
}

void release_all_kbdps2() {
	release_port_in(0x60);

	release_port_in(0x64);

	release_tick_timer(&handle_keys);

	if (cur) free(cur);

	cur = nullptr;
}