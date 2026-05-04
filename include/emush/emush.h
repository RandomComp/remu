#ifndef _EMULATOR_OS_EMUSH_H
#define _EMULATOR_OS_EMUSH_H

#include "types.h"

typedef struct emush_cmd {
	const byte* name;

	int (*handler)(const byte** argv, size_t argc)
} emush_cmd;

ssize_t emush_get_command_index(void);
void emush_get_history(byte history[16][64]);

int set_var(byte* var_name, byte* value);
byte* get_var(byte* var_name);

int emush_exec(const byte* command, size_t command_len);

int help_cmd(const byte** argv, size_t argc);
int info_cmd(const byte **argv, size_t argc);
int logo_cmd(const byte **argv, size_t argc);
int readme_cmd(const byte **argv, size_t argc);

int ata_cmd(const byte **argv, size_t argc);
int sfs_cmd(const byte **argv, size_t argc);

int calc_cmd(const byte** argv, size_t argc);

int time_cmd(const byte** argv, size_t argc);

#endif
