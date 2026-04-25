#include "power/emulator_power_control.h"

#include "types.h"

#include "emulator_io.h"

#include "main.h"

#include "emulator_logger.h"

static void old_qemu_bochs_shutdown(_size_t value) {
	if (value != 0x2000) return;

	emulator_log(false, LOG_SEVERITY_INFO, "Old QEMU or Bochs power command, power off...");

	exit_emulator(0);
}

static void modern_qemu_power_command(_size_t value) {
	// if (value != 0x2000) return;

	emulator_log(false, LOG_SEVERITY_INFO, "Modern QEMU power command, power off...");

	exit_emulator(0);
}

static void virtual_box_acpi_pm1a_cnt_command(_size_t value) {
	if (value != 0x3400) return;

	emulator_log(false, LOG_SEVERITY_INFO, "Virtual box acpi power command, power off...");

	exit_emulator(0);
}

static void cloud_hypervisor_power_command(_size_t value) {
	if (value != 0x34) return;

	emulator_log(false, LOG_SEVERITY_INFO, "Cloud hypervisor power command, power off...");
	
	exit_emulator(0);
}

void init_power_control() {
	emulator_log(true, LOG_SEVERITY_VERBOSE, "Power control initialization...");
	
	emulator_log(false, LOG_SEVERITY_VERBOSE, "Setting up ports (0xB004, 0x604, 0x4004, 0x600) for power off on emulators...");
	
	emulator_setup_port_out(0xB004, old_qemu_bochs_shutdown);

	emulator_setup_port_out(0x604, modern_qemu_power_command);

	emulator_setup_port_out(0x4004, virtual_box_acpi_pm1a_cnt_command);

	emulator_setup_port_out(0x600, cloud_hypervisor_power_command);
	
	emulator_log(true, LOG_SEVERITY_VERBOSE, "Power control initialization done!");
}

void release_power_control() {
	emulator_log(false, LOG_SEVERITY_VERBOSE, "Power control deinitialization...");
	
	emulator_log(false, LOG_SEVERITY_VERBOSE, "Releasing ports (0xB004, 0x604, 0x4004, 0x600) for power off on emulators...");
	
	emulator_release_port_out(0xB004);

	emulator_release_port_out(0x604);

	emulator_release_port_out(0x4004);

	emulator_release_port_out(0x600);
	
	emulator_log(false, LOG_SEVERITY_VERBOSE, "Power control deinitialization done!");
}
