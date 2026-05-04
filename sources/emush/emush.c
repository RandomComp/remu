#include "emush/emush.h"

#include "types.h"

#include "std.h"

#include "drivers/ata/ata.h"

#include "drivers/sfs/sfs.h"

#include "drivers/time/cmos.h"

#include "drivers/power.h"

#include "drivers/hid/kbdps2.h"

#include "drivers/video/vga.h"

#include "drivers/memory/memory.h"

#include "multiboot.h"

#include "builtins/builtins.h"

#include "builtins/string.h"

#include "math/math.h"

static byte history[16][64] = { 0 };

static ssize_t command_index = 0;

int set_cmd(const byte** argv, size_t argc);
int clear_cmd(const byte** argv, size_t argc);
int echo_cmd(const byte** argv, size_t argc);
int history_cmd(const byte** argv, size_t argc);
int ls_cmd(const byte** argv, size_t argc);
int touch_cmd(const byte** argv, size_t argc);
int cat_cmd(const byte** argv, size_t argc);

int graphtest_cmd(const byte** argv, size_t argc);
int shut_cmd(const byte** argv, size_t argc);
int reboot_cmd(const byte** argv, size_t argc);

emush_cmd emush_commands[] = {
	{"help", 		help_cmd},
	{"set", 		set_cmd},
	{"clear", 		clear_cmd},
	{"echo", 		echo_cmd},
	{"logo", 		logo_cmd},
	{"readme", 		readme_cmd},
	{"history", 	history_cmd},
	{"info", 		info_cmd},
	{"ata", 		ata_cmd},
	{"sfs", 		sfs_cmd},
	{"ls", 			ls_cmd},
	{"touch", 		touch_cmd},
	{"cat", 		cat_cmd},
	{"graphtest", 	graphtest_cmd},
	{"time", 		time_cmd},
	{"calc", 		calc_cmd},
	{"shut", 		shut_cmd},
	{"reboot", 		reboot_cmd}
};

const size_t commands_cnt = sizeof(emush_commands) / sizeof(emush_commands[0]);

static byte vars_name[16][64] = { "?", "PS1" };
static byte vars_value[16][64] = { "0", "emush:$ " };

static const size_t vars_max_cnt = MIN(sizeof(vars_name) / sizeof(vars_name[0]), sizeof(vars_value) / sizeof(vars_value[0]));

int ls_cmd(const byte** argv, size_t argc) {
	size_t files_cnt = 0;

	byte* files = sfs_list_files(ATA_MASTER, &files_cnt);

	if (!files) {
		kprintf("Unavailable.\n");
		
		return -1;
	}

	for (size_t i = 0; i < files_cnt; i++) {
		kprintf("\"%s\"\n", files + (i * 64));
	}

	return 0;
}

int touch_cmd(const byte** argv, size_t argc) {
	if (argc < 2) {
		kprintf("%vfbrToo few arguments for \"touch\", arguments: file name and text\n");
		
		return -1;
	}

	const byte* content = argv[1]; size_t content_size = strlen(content);

	sfs_error_e error = sfs_create_file(ATA_MASTER, argv[0], content, content_size);

	if (error == SFS_ERROR_FILE_EXISTS) {
		kprintf("touch: %vfbr%s: file already exists\n", argv[0]);
	}

	else if (error == SFS_ERROR_NOT_ENGH_MMRY) {
		kprintf("touch: %vfbr%s: is not enough space available on the disk\n", argv[0]);
	}

	return error;
}

int cat_cmd(const byte** argv, size_t argc) {
	sfs_error_e result_error = SFS_OK;

	for (size_t i = 0; i < argc; i++) {
		byte content[5120] = { 0 };

		size_t readed = 0;

		sfs_error_e error = sfs_read_file(ATA_MASTER, argv[i], content, 5120, &readed);

		if (result_error == SFS_OK)
			result_error = error;

		if (error == SFS_ERROR_FILE_NOT_EXISTS) {
			kprintf("cat: %vfbr\"%s\": no such file\n", argv[i]); continue;
		}

		kprintf("%.*s\n", readed, content);
	}

	kprint("\n\r");

	return 0;
}

void graphtest() {
	byte src_style = get_style();

	disable_blink();
	
	for (size_t i = 0; i < ROWS; i++) {
		for (size_t j = 0; j < COLUMNS; j++) {
			set_style(0x00);

			set_cursor_pos(j, i);

			float x = j; float y = i;

			x = (((float)x / (float)COLUMNS) * 2.0f) - 1.0f;
			y = (((float)y / (float)ROWS) * 2.0f) - 1.0f;

			x *= ((float)COLUMNS / (float)ROWS) / ((float)16 / (float)8);
			
			if ((x * x + y * y) < 0.5f) {
			}
			set_style((byte)((x * x + y * y) * 15.0f) << 4);

			putch(' ');
		}
	}

	set_style(src_style);

	const c_str msg = "Press 'q' to quit"; size_t msg_len = strlen(msg);

	size_t spaces = (COLUMNS / 2) - (msg_len / 2);

	for (size_t i = 0; i < spaces; i++) {
		putch(' ');
	}

	kprintf("%s\r", msg);

	byte ch = getch();

	while (ch != 'q') {
		ch = getch(); halt();
	}
}

ssize_t emush_get_command_index(void) {
	return command_index;
}

void emush_get_history(byte _history[16][64]) {
	memcpy(_history, history, sizeof(history));
}

int clear_cmd(const byte **argv, size_t argc) {
	clear_screen(0x00);
	set_style(0x0F);

	return 0;
}

int echo_cmd(const byte **argv, size_t argc) {
	for (size_t i = 0; i < argc; i++) {
		kprintf("%s\n", argv[i]);
	}

	kprint("\n");

	return 0;
}

int set_cmd(const byte** argv, size_t argc) {
	if (!argv || argc < 2) {
		size_t max_var_name_len = 0;

		size_t max_var_value_len = 0;

		for (size_t i = 0; i < vars_max_cnt; i++) {
			byte* name = vars_name[i];
			
			byte* value = vars_value[i];

			if ((!name || name[0] == 0) ||
				(!value || value[0] == 0)) break;

			size_t name_len = strlen(name);

			if (name_len > max_var_name_len) {
				max_var_name_len = name_len;
			}

			size_t value_len = strlen(value);

			if (value_len > max_var_value_len) {
				max_var_value_len = value_len;
			}
		}

		kprintf("Ú%0mÄ*s¿\n", max_var_name_len + 2 + 1 + max_var_value_len + 2, "");

		kprintf("³%vfbq%=*s%vd³\n", max_var_name_len + 2 + 1 + max_var_value_len + 2, "emush variables");

		kprintf("Ã%0mÄ*sÂ%0mÄ*s´\n", max_var_name_len + 2, "", max_var_value_len + 2, "");

		for (size_t i = 0; i < vars_max_cnt; i++) {
			byte* name = vars_name[i];
			byte* value = vars_value[i];

			if ((!name || name[0] == 0) ||
				(!value || value[0] == 0)) break;

			kprintf("³ %vfby%*s%vd ³ %vfbg%*s%vd ³\n", max_var_name_len, name, max_var_value_len, value);
		}

		kprintf("À%0mÄ*sÁ%0mÄ*sÙ\n", max_var_name_len + 2, "", max_var_value_len + 2, "");
		
		return 0;
	}

	set_var(argv[0], argv[1]);

	return 0;
}

int history_cmd(const byte **argv, size_t argc) {
	for (int i = 0; i < 16; i++) {
		c_str msg = history[i];

		if (strlen(msg) > 0) {
			kprintf("\t%i. %s\n", i, msg);
		}
	}

	kprintf("\tCurrent: %i/%i\n", command_index, 16);

	return 0;
}

int graphtest_cmd(const byte **argv, size_t argc) {
	graphtest();

	return 0;
}

int shut_cmd(const byte **argv, size_t argc) {
	kprintf("Shutting down...\n");

	poweroff();

	return 0;
}

int reboot_cmd(const byte **argv, size_t argc) {
	kprintf("Rebooting...\n");

	reboot();

	return 0;
}

int set_var(byte* var_name, byte* value) {
	bool ok = false; size_t index = 0;

	for (size_t i = 0; i < vars_max_cnt; i++) {
		byte* cur_var_name = vars_name[i];
		byte* cur_var_value = vars_value[i];

		if (strcmp(var_name, cur_var_name) == 0) {
			ok = true; index = i; break;
		}
	}

	if (!ok) {
		ok = false;

		for (size_t i = 0; i < vars_max_cnt; i++) {
			byte* cur_var_name = vars_name[i];
			byte* cur_var_value = vars_value[i];

			if ((!cur_var_name ||
				cur_var_name[0] == 0) &&
				(!cur_var_value ||
				cur_var_value[0] == 0)) {
				ok = true; index = i; break;
			}
		}

		if (!ok) return -1;

		memset(vars_name[index], 0, 64);

		memcpy(vars_name[index], var_name, MIN(64, strlen(var_name)));
	} 

	memset(vars_value[index], 0, 64);

	memcpy(vars_value[index], value, MIN(64, strlen(value)));

	return 0;
}

byte* get_var(byte* var_name) {
	for (size_t i = 0; i < vars_max_cnt; i++) {
		byte* cur_var_name = vars_name[i];

		if (!cur_var_name) break;

		if (cur_var_name[0] == 0) break;

		if (strcmp(var_name, cur_var_name) == 0) {
			return vars_value[i];
		}
	}

	return "";
}

static byte last_err = 0;

static byte src_typed[512] = { 0 };

int emush_exec(const byte* _command, size_t command_len) {
	if (!_command) {
		return -1;
	}

	byte buf[4] = { 0 };

	snprint_num(buf, 4, last_err, 10, false, false);

	buf[3] = 0;

	set_var("?", buf);

	memset(src_typed, 0, sizeof(src_typed));

	memcpy(src_typed, _command, MIN(sizeof(src_typed), command_len));

	byte* argv[16] = { "" };

	size_t argc = 0;

	c_str command = parse_cli_args(_command);

	if (command[0] == '$') {
		byte* var_name = command + 1;

		command = get_var(var_name);
	}

	c_str arg = parse_cli_args(nullptr);

	while (arg != nullptr) {
		if (arg[0] == '$') {
			byte* var_name = arg + 1;

			arg = get_var(var_name);
		}

		argv[argc++] = arg;

		arg = parse_cli_args(nullptr);
	}

	bool ok = false;

	for (size_t i = 0; i < commands_cnt; i++) {
		if (strcmp(command, emush_commands[i].name) == 0) {
			if (emush_commands[i].handler) {
				int _last_err = emush_commands[i].handler(argv, argc);

				_last_err = _last_err % 256;

				while (_last_err < 0)
					_last_err += 256;
				
				last_err = _last_err & 0xFF;

				ok = true;
			}
			
			break;
		}
	}

	if (!ok && strlen(command) != 0) {
		kprintf("\"%s\": %vfbrno such command\n", command);

		last_err = 127;
	}

	if (strlen(src_typed) > 0 && 
		strncmp(history[MAX(0, command_index - 1)], src_typed, 64) != 0) {
		memcpy(history[command_index], src_typed, 64);

		command_index = (command_index + 1) % 16;
	}

	return 0;
}
