#include "types.h"

#include "std.h"

#include "builtins/string.h"

#include "emush/emush.h"

extern emush_cmd emush_commands[];

extern const size_t commands_cnt;

int help_cmd(const byte** argv, size_t argc) {
	const byte* commands_descriptions[] = {
		"see help",
		"sets emush variable",
		"clears screen",
		"echoes back the arguments",
		"system logo",
		"readme instruction",
		"show commands history",
		"see information about system",
		"manipulate ata drives",
		"manipulate sfs drives",
		"manipulate pci devices",
		"list current directory",
		"creates file ",
		"concatenate files and see",
		"see graphical test in text mode",
		"see system time",
		"enter calculator",
		"shutdown system",
		"restart system"
	};

	size_t max_command_len = 0;

	for (size_t i = 0; i < commands_cnt; i++) {
		size_t command_len = strlen(emush_commands[i].name);

		if (command_len > max_command_len)
			max_command_len = command_len;
	}

	size_t max_description_len = 0;

	for (size_t i = 0; i < commands_cnt; i++) {
		size_t description_len = strlen(commands_descriptions[i]);

		if (description_len > max_description_len)
			max_description_len = description_len;
	}

	size_t center = (COLUMNS / 2) - (max_command_len + max_description_len + 4) / 2;

	kprintf("%*sÚ%0mÄ*sÂ%0mÄ*s¿\n\r", center, "", max_command_len + 2, "", max_description_len + 2, "");

	kprintf("%*s³%=*s³%=*s³\n\r", center, "", max_command_len + 2, "Command", max_description_len + 2, "Description");

	kprintf("%*sÃ%0mÄ*sÅ%0mÄ*s´\n\r", center, "", max_command_len + 2, "", max_description_len + 2, "");

	for (size_t i = 0; i < commands_cnt; i++) {
		kprintf("%*s³ %vfby%*s%vd ³ %vfbg%-*s%vd ³\n", center, "", max_command_len, emush_commands[i].name, max_description_len, commands_descriptions[i]);
	}

	kprintf("%*sÀ%0mÄ*sÁ%0mÄ*sÙ\n\r", center, "", max_command_len + 2, "", max_description_len + 2, "");

	return 0;
}
