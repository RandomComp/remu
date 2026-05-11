#include "types.h"

#include "std.h"

#include "builtins/string.h"

#include "emush/emush.h"

extern emush_cmd emush_commands[];

typedef enum emush_help_category_e {
	EMUSH_HELP_CATEGORY_ALL,
	EMUSH_HELP_CATEGORY_BUILTIN,
	EMUSH_HELP_CATEGORY_DAILY,
	EMUSH_HELP_CATEGORY_INFO,
	EMUSH_HELP_CATEGORY_POWER,
	EMUSH_HELP_CATEGORY_ADMINISTRATION,
	EMUSH_HELP_CATEGORY_OTHER,

} emush_help_category_e;

typedef struct emush_help_category {
	byte* name;

	emush_help_category_e category;
} emush_help_category;

typedef struct emush_help_cmd {
	byte* name;
	byte* description;
	byte* example;

	emush_help_category_e category;
} emush_help_cmd;

static emush_help_category categories[] = {
	{"All", 			EMUSH_HELP_CATEGORY_ALL},
	{"Builtin", 		EMUSH_HELP_CATEGORY_BUILTIN},
	{"Daily", 			EMUSH_HELP_CATEGORY_DAILY},
	{"Information", 	EMUSH_HELP_CATEGORY_INFO},
	{"Power",			EMUSH_HELP_CATEGORY_POWER},
	{"Administration", 	EMUSH_HELP_CATEGORY_ADMINISTRATION},
	{"Other", 			EMUSH_HELP_CATEGORY_OTHER},
};

static emush_help_cmd emush_help_commands[] = {
	{"help", 		"see help", 							"help touch", 										EMUSH_HELP_CATEGORY_BUILTIN},
	{"set", 		"sets emush variable", 					"set some_var \"Hello, world!\"", 					EMUSH_HELP_CATEGORY_BUILTIN},
	{"del", 		"deletes emush variable", 				"del some_var", 									EMUSH_HELP_CATEGORY_BUILTIN},
	{"clear", 		"clears screen", 						"clear", 											EMUSH_HELP_CATEGORY_BUILTIN},
	{"echo", 		"printing arguments", 					"echo -e \"Welcome to %vfbrE%vfbgM%vfbyU%vd-OS\"", 	EMUSH_HELP_CATEGORY_BUILTIN},
	{"exec", 		"executes scripts", 					"exec autoexec", 									EMUSH_HELP_CATEGORY_BUILTIN},
	
	{"ls", 			"list current directory", 				"ls", 												EMUSH_HELP_CATEGORY_DAILY},
	{"mv", 			"moves file to directory/renames file", "mv old_name.txt new_name.txt", 					EMUSH_HELP_CATEGORY_DAILY},
	{"touch", 		"creates file", 						"touch test.txt \"Welcome to EMU-OS\"", 			EMUSH_HELP_CATEGORY_DAILY},
	{"cat", 		"concatenate files and print", 			"cat test.txt 123.txt", 							EMUSH_HELP_CATEGORY_DAILY},
	{"time", 		"see system time", 						"time", 											EMUSH_HELP_CATEGORY_DAILY},
	{"calc", 		"enter calculator", 					"calc", 											EMUSH_HELP_CATEGORY_DAILY},
	
	{"logo", 		"shows system logo", 					"logo", 											EMUSH_HELP_CATEGORY_INFO},
	{"readme", 		"readme instruction", 					"readme", 											EMUSH_HELP_CATEGORY_INFO},
	{"history", 	"show commands history", 				"history", 											EMUSH_HELP_CATEGORY_INFO},
	{"info", 		"information about system", 			"info", 											EMUSH_HELP_CATEGORY_INFO},
	
	{"shut", 		"shutdown system", 						"shut", 											EMUSH_HELP_CATEGORY_POWER},
	{"reboot", 		"restart system", 						"reboot",											EMUSH_HELP_CATEGORY_POWER},
	
	{"ata", 		"manipulate ata drives", 				"ata dump 0", 										EMUSH_HELP_CATEGORY_ADMINISTRATION},
	{"sfs", 		"manipulate sfs drives", 				"sfs format", 										EMUSH_HELP_CATEGORY_ADMINISTRATION},
	{"pci", 		"manipulate pci devices", 				"pci scan", 										EMUSH_HELP_CATEGORY_ADMINISTRATION},

	{"graphtest", 	"graphical test in text mode", 			"graphtest", 										EMUSH_HELP_CATEGORY_OTHER},
	{"graphtest", 	"graphical test in text mode", 			"graphtest", 										EMUSH_HELP_CATEGORY_OTHER},
};

const static ssize_t help_commands_cnt = sizeof(emush_help_commands) / sizeof(emush_help_commands[0]);

const static ssize_t categories_cnt = sizeof(categories) / sizeof(categories[0]);

void help_all(void) {
	size_t columns = get_columns();

	byte inputed_c = 0;

	size_t max_category_len = 0;

	for (size_t i = 0; i < categories_cnt; i++) {
		size_t category_len = strlen(categories[i].name);

		if (category_len > max_category_len) {
			max_category_len = category_len;
		}
	}

	max_category_len = columns - 5;

	ssize_t rows = (size_t)get_rows() - 4;

	ssize_t selected_category = 0;

	clear_screen(0x0F);

	set_cursor_pos(-1, -1, true);

	bool need_quit = false;
	bool entered = false;

	while (!need_quit) {
		set_cursor_pos(0, 0, false);

		#define SELECT_CATEGORY_STR "Select help category (Press Q to exit)"

		kprintf("┌%0m─*s┐\n\r", max_category_len + 2, "");

		kprintf("│%=*s│\n\r", max_category_len + 2, SELECT_CATEGORY_STR);

		kprintf("├%0m─*s┤\n\r", max_category_len + 2, "");

		for (size_t i = 0; i < rows; i++) {
			if (i >= categories_cnt) {
				kprintf("│ %-*s │\n\r", max_category_len, "");

				continue;
			}

			if (i == selected_category) {
				// ssize_t x = 0, y = 0;

				// get_cursor_pos(&x, &y);

				// set_cursor_pos(x + 2, y, true);

				kprintf("│%vi %-*s %vd│\n\r", max_category_len, categories[i].name);
			}

			else {
				kprintf("│ %-*s │\n\r", max_category_len, categories[i].name);
			}
		}

		kprintf("└%0m─*s┘", max_category_len + 2, "");

		inputed_c = blkgetch();

		if (inputed_c == 'q' ||
			inputed_c == 0x80) {
			need_quit = true;
		}

		else if (inputed_c == '\r') {
			entered = true;
			
			selected_category = categories[selected_category].category;

			break;
		}

		else if (inputed_c == '\x18' || inputed_c == 'w') {
			selected_category = MAX(0, selected_category - 1);
		}

		else if (inputed_c == '\x19' || inputed_c == 's') {
			selected_category = MIN(selected_category + 1, MIN(categories_cnt - 1, rows));
		}
	}

	emush_help_cmd selected_command = { 0 };
		
	if (entered) {
		clear_screen(0x0F);

		set_cursor_pos(-1, -1, true);

		size_t max_name_len = 0;
		size_t max_description_len = 0;

		for (size_t i = 0; i < help_commands_cnt; i++) {
			if (selected_category != EMUSH_HELP_CATEGORY_ALL &&
				emush_help_commands[i].category != selected_category) continue;

			size_t command_name_len = strlen(emush_help_commands[i].name);

			if (command_name_len > max_name_len) {
				max_name_len = command_name_len;
			}
			
			size_t command_description_len = strlen(emush_help_commands[i].description);

			if (command_description_len > max_description_len) {
				max_description_len = command_description_len;
			}
		}
		
		max_description_len = MAX(max_description_len, 	(columns / 2) - 5);

		max_name_len = columns - max_description_len - 7;

		inputed_c = 0;

		size_t start_index = 0;

		ssize_t entries = 0;

		for (ssize_t i = 0; i < help_commands_cnt; i++) {
			if (selected_category != EMUSH_HELP_CATEGORY_ALL &&
				emush_help_commands[i].category != selected_category) continue;

			entries++;
		}

		ssize_t selected_view_command = 0;

		while (!need_quit) {
			set_cursor_pos(0, 0, false);

			byte buf[64] = { 0 };

			snprintf(buf, 64, "Help category: %s", categories[selected_category].name);

			kprintf("┌%0m─*s┐\n\r", max_name_len + max_description_len + 5, "");

			kprintf("│%=*s│\n\r", max_name_len + max_description_len + 5, buf);

			kprintf("├%0m─*s┬%0m─*s┤\n\r", max_name_len + 2, "", max_description_len + 2, "");

			ssize_t entry_index = 0;

			for (ssize_t i = 0; i < rows; i++) {
				ssize_t index = start_index + i;

				if (selected_category != EMUSH_HELP_CATEGORY_ALL &&
					emush_help_commands[index].category != selected_category) continue;

				if (index < 0 || index > help_commands_cnt) {
					kprintf("│%-*s│\n\r", max_name_len + max_description_len + 3, "");

					continue;
				}

				byte right_pad_c = '│';

				ssize_t row_cursor_index = (rows * index) / entries;

				if (row_cursor_index == i) {
					right_pad_c = '≡';
				}

				if (entry_index == selected_view_command) {
					kprintf("│%vi %-*s %vd│%vi %-*s %vd%c\n\r", max_name_len, emush_help_commands[index].name, max_description_len, emush_help_commands[index].description, right_pad_c);
				}

				else {
					kprintf("│ %-*s │ %-*s %c\n\r", max_name_len, emush_help_commands[index].name, max_description_len, emush_help_commands[index].description, right_pad_c);
				}

				entry_index++;
			}

			kprintf("└%0m─*s┴%0m─*s┘", max_name_len + 2, "", max_description_len + 2, "");

			inputed_c = lower(blkgetch());

			if (inputed_c == 'q' || inputed_c == 0x80) {
				need_quit = true;
			}

			else if (inputed_c == '\r') {
				entered = true;

				entry_index = 0;

				for (ssize_t i = 0; i < rows; i++) {
					ssize_t index = start_index + i;

					if (selected_category != EMUSH_HELP_CATEGORY_ALL &&
						emush_help_commands[index].category != selected_category) continue;

					if (index < 0 || index > help_commands_cnt) {
						continue;
					}

					if (entry_index == selected_view_command) {
						selected_command = emush_help_commands[i];

						break;
					}

					entry_index++;
				}

				break;
			}

			else if (inputed_c == '\x18' || inputed_c == 'w') {
				selected_view_command = selected_view_command - 1;

				if (selected_view_command < 0) {
					if (start_index > 0) {
						start_index = start_index + selected_view_command;
					}

					selected_view_command = 0;
				}
			}

			else if (inputed_c == '\x19' || inputed_c == 's') {
				selected_view_command = selected_view_command + 1;

				if (selected_view_command >= MIN(entries, rows)) {
					start_index = MIN(MAX(0, entries - rows), start_index + (selected_view_command - rows + 1));

					selected_view_command = MIN(entries, rows) - 1;
				}
			}
		}
	}

	kprint("\n\r");

	#define note_message "Note: to see more help about those commands:"

	size_t center = (columns / 2) - strlen(note_message) / 2;

	kprintf("%*s" note_message "\n", center, "");

	kprintf("%*s%=*s\n\r", center, "", strlen(note_message) - 1, "use help < command >");

	ssize_t cur_y = 0;

	get_cursor_pos(nullptr, &cur_y);

	set_cursor_pos(0, cur_y, true);

	byte* category_name = "N/A";

	for (size_t i = 0; i < categories_cnt; i++) {
		if (selected_command.category == categories[i].category) {
			category_name = categories[i].name; break;
		}
	}

	kprintf("Selected command: \n");
	kprintf("Command name: %s\n", selected_command.name);
	kprintf("Command category: %s\n", category_name);
	kprintf("Command description: %s\n", selected_command.description);
	kprintf("Command example: %s\n", selected_command.example);
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
