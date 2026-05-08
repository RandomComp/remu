#include "types.h"

#include "std.h"

#include "builtins/string.h"

#include "emush/emush.h"

extern emush_cmd emush_commands[];

extern const size_t commands_cnt;

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
	"moves file to directory/renames file",
	"creates file ",
	"concatenate files and see",
	"see graphical test in text mode",
	"see system time",
	"enter calculator",
	"shutdown system",
	"restart system"
};

const byte* commands_example[] = {
	"help touch",
	"set some_var \"Hello, world!\"",
	"clear",
	"echo -e \"Welcome to %vfbrE%vfbgM%vfbyU%vd-OS\"",
	"logo",
	"readme",
	"history",
	"info",
	"ata dump 0",
	"sfs format",
	"pci scan",
	"ls",
	"mv old_name.txt new_name.txt",
	"touch test.txt \"Welcome to EMU-OS\"",
	"cat test.txt",
	"graphtest",
	"time",
	"calc",
	"shut",
	"reboot"
};

void help_all(void) {
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

	size_t columns = get_columns();

	size_t center = (columns / 2) - (max_command_len + max_description_len + 2 + 2 + 3) / 2;

	kprintf("%*s┌%0m─*s┬%0m─*s┐\n\r", center, "", max_command_len + 2, "", max_description_len + 2, "");

	kprintf("%*s│%=*s│%=*s│\n\r", center, "", max_command_len + 2, "Command", max_description_len + 2, "Short Description");

	kprintf("%*s├%0m─*s┼%0m─*s┤\n\r", center, "", max_command_len + 2, "", max_description_len + 2, "");

	for (size_t i = 0; i < commands_cnt; i++) {
		kprintf("%*s│ %vfby%-*s%vd │ %vfbg%*s%vd │\n", center, "", max_command_len, emush_commands[i].name, max_description_len, commands_descriptions[i]);
	}

	kprintf("%*s└%0m─*s┴%0m─*s┘\n\r", center, "", max_command_len + 2, "", max_description_len + 2, "");

	// #define note_message "Note: to see more help about those commands:"

	// center = (COLUMNS / 2) - strlen(note_message) / 2;

	// kprintf("%*s" note_message "\n", center, "");

	// kprintf("%*s%=*s\n\r", center, "", strlen(note_message) - 1, "use help < command >");
}

void help(const byte* command) {

}

int help_cmd(const byte** argv, size_t argc) {
	if (argc <= 1 || strcmp(argv[1], "all") == 0) {
		help_all();
		
		return 0;
	}

	for (size_t i = 1; i < argc; i++) {
		help(argv[i]);
	} 

	return 0;
}
