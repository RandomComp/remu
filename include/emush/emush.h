#ifndef _EMULATOR_OS_EMUSH_H
#define _EMULATOR_OS_EMUSH_H

#include "types.h"

typedef struct emush_cmd {
	const byte* name;

	int (*handler)(const byte** argv, size_t argc)
} emush_cmd;

typedef enum emush_var_err_e {
	EMUSH_VAR_OK,
	EMUSH_VAR_NOT_AVAILABLE_SPACE_ERR,
	EMUSH_VAR_NOT_FOUND_ERR
} emush_var_err_e;

ssize_t emush_get_command_index(void);
void emush_get_history(byte history[16][64]);

emush_var_err_e set_var(const byte* var_name, const byte* value);
emush_var_err_e get_var(const byte* var_name, byte** value);
emush_var_err_e del_var(const byte* var_name);
byte* emush_get_var_err_description(emush_var_err_e err);

int emush_exec(const byte* command, size_t command_len);

int help_cmd(const byte** argv, size_t argc);
int info_cmd(const byte **argv, size_t argc);
int logo_cmd(const byte **argv, size_t argc);
int readme_cmd(const byte **argv, size_t argc);
int exec_cmd(const byte **argv, size_t argc);

int ata_cmd(const byte **argv, size_t argc);
int sfs_cmd(const byte **argv, size_t argc);

int calc_cmd(const byte** argv, size_t argc);

int time_cmd(const byte** argv, size_t argc);

int set_cmd(const byte** argv, size_t argc);
int del_cmd(const byte** argv, size_t argc);
int clear_cmd(const byte** argv, size_t argc);
int echo_cmd(const byte** argv, size_t argc);
int history_cmd(const byte** argv, size_t argc);
int ls_cmd(const byte** argv, size_t argc);
int mv_cmd(const byte** argv, size_t argc);
int touch_cmd(const byte** argv, size_t argc);
int cat_cmd(const byte** argv, size_t argc);

int graphtest_cmd(const byte** argv, size_t argc);
int shut_cmd(const byte** argv, size_t argc);
int reboot_cmd(const byte** argv, size_t argc);

int pci_cmd(const byte** argv, size_t argc);

#endif
