#include "cpu/emulator_pic.h"

#include "emulator_io.h"

#include "cpu/emulator_cpu.h"

#include "emulator_logger.h"

#include "main.h"

#include <string.h>

#include <stdlib.h>

static bool idt_initialized = true; // По умолчанию IDT настроен на все исключения перезагружать систему

static bool master_pic_initializing = false,
			slave_pic_initializing = false;

// Комманда для Master PIC и Slave PIC для начала инициализации IDT
// out8(0x20, 0x11);
// out8(0xa0, 0x11);

// Данные для Master PIC и Slave PIC последовательно
// out8(0x21, 0x20);
// out8(0xa1, 0x28);

// out8(0x21, 0x04);
// out8(0xa1, 0x02);

// out8(0x21, 0x01);
// out8(0xa1, 0x01);

// out8(0x21, 0x00);
// out8(0xa1, 0x00);

static byte master_pic_data[3] = { 0 }; static _size_t master_pic_data_index = 0;

static byte slave_pic_data[3] = { 0 }; static _size_t slave_pic_data_index = 0;

static _size_t pic_data_size = 0;

static byte master_pic_int_offset = 0x0;

static byte slave_pic_int_offset = 0x8;

static byte master_int_mask = 0x0;

static byte slave_int_mask = 0x0;

static pic_t* cur = nullptr;

static cpu_t* cur_cpu = nullptr;

static void master_pic_command_handler(_size_t data) {
	data = data & 0xFF;

	if (data & 0x10) {
		idt_initialized = false;

		master_pic_initializing = true;

		pic_data_size = 1;
	}

	if (data & 0x1) {
		pic_data_size = 3;
	}
}

static void slave_pic_command_handler(_size_t data) {
	data = data & 0xFF;

	if (data & 0x10) {
		idt_initialized = false;

		slave_pic_initializing = true;

		pic_data_size = 1;
	}

	if (data & 0x1) {
		pic_data_size = 3;
	}
}

static void master_pic_data_handler(_size_t data) {
	data = data & 0xFF;

	if (master_pic_initializing) {
		if (master_pic_data_index == 0) {
			master_pic_int_offset = data;
		}

		if (master_pic_data_index == pic_data_size) {
			idt_initialized = true;

			master_pic_initializing = false;
		}
	}

	else {
		master_int_mask = data;
	}

	master_pic_data_index = (master_pic_data_index + 1) % pic_data_size;
}

static void slave_pic_data_handler(_size_t data) {
	data = data & 0xFF;

	if (slave_pic_initializing) {
		if (slave_pic_data_index == 0) {
			slave_pic_int_offset = data;
		}

		if (slave_pic_data_index == pic_data_size) {
			idt_initialized = true;

			slave_pic_initializing = false;
		}
	}

	else {
		slave_int_mask = data;
	}

	slave_pic_data_index = (slave_pic_data_index + 1) % pic_data_size;
}

static isr_handler_t idt_table[256] = { 0 };

static void reset_handler() {
	imd_exit_emulator(0);
}

pic_t* init_emulator_pic(cpu_t* cpu) {
	emulator_log(true, LOG_SEVERITY_VERBOSE, "PIC initialization...");

	pic_t* pic = malloc(sizeof(pic_t));

	memset(pic, 0, sizeof(pic_t));

	memset(pic->handler_queue, 0, sizeof(pic->handler_queue));

	pic->lock = false;

	// for (_size_t i = 0; i < 32; i++) {
	// 	idt_table[i] = reset_handler;
	// }

	emulator_setup_port_out(0x20, master_pic_command_handler);
	emulator_setup_port_out(0x21, master_pic_data_handler);

	emulator_setup_port_out(0xa0, slave_pic_command_handler);
	emulator_setup_port_out(0xa1, slave_pic_data_handler);

	cur = pic;

	cur_cpu = cpu;

	emulator_log(true, LOG_SEVERITY_VERBOSE, "PIC initialized!");

	return pic;
}

void exec_all_emulator_ints(pic_t* pic) {
	if (!pic) return;

	size_t queue_size = sizeof(pic->handler_queue) / sizeof(pic->handler_queue[0]);

	for (size_t i = 0; i < queue_size; i++) {
		isr_handler_t handler = pic->handler_queue[i];

		if (!handler) continue;

		handler();

		pic->handler_queue[i] = nullptr;
	}
}

void call_emulator_int(pic_t* _pic, byte _int) {
	pic_t* pic = _pic;
	
	if (!pic) pic = cur;

	if (!pic) {
		emulator_log(false, LOG_SEVERITY_ERROR, "No PIC instance provided to call");

		return;
	}

	_int = _int & 0xFF;

	// emulator_log(false, LOG_SEVERITY_VERBOSE, "Calling interrupt 0x%zx", _int);

	size_t queue_size = sizeof(pic->handler_queue) / sizeof(pic->handler_queue[0]);

	// if (pic->handler_pos >= pic->queue_size) {
	// 	pic->queue_size += HANDLER_QUEUE_SIZE_STEP;

	// 	pic->handler_queue = realloc(pic->handler_queue, pic->queue_size * sizeof(isr_handler_t));
 	// }

	if (idt_table[_int]) {
		pic->handler_queue[pic->handler_pos] = idt_table[_int];

		pic->handler_pos = (pic->handler_pos + 1) % queue_size;

		if (cur_cpu) clear_halt(cur_cpu);
	}
}

void idt_flush_emulator(idt_ptr_t* ptr) {
	if (!ptr || ptr->base <= 0) return;

	size_t entries_cnt = ptr->limit / sizeof(idt_entry_t);

	emulator_log(false, LOG_SEVERITY_INFO, "[KERNEL CALL] IDT Flushing...");

	emulator_log(false, LOG_SEVERITY_VERBOSE, "[KERNEL CALL] Entries cnt: %zu", entries_cnt);

	for (size_t i = 0; i < entries_cnt; i++) {
		idt_entry_t* entry = (idt_entry_t*)(ptr->base) + i;

		if (!entry || entry->always0 != 0) continue;

		uint64 addr = ((((uint64)entry->base_high) << 32) | ((uint64)entry->base_low));

		isr_handler_t handler = (isr_handler_t)(addr);

		if (handler) {
			idt_table[i] = handler;

			emulator_log(false, LOG_SEVERITY_TRACE, "[KERNEL CALL] Saved interrupt 0x%x with handler addr 0x%zx", i, handler);
		}
	}

	emulator_log(false, LOG_SEVERITY_INFO, "[KERNEL CALL] IDT Flushed!", entries_cnt);
}

void free_emulator_pic(pic_t* pic) {
	emulator_log(true, LOG_SEVERITY_VERBOSE, "PIC deinitialization...");

	emulator_release_port_out(0x20);
	emulator_release_port_out(0x21);

	emulator_release_port_out(0xa0);
	emulator_release_port_out(0xa1);

	if (pic) {
		// if (pic->handler_queue) free(pic->handler_queue);

		// pic->handler_queue = nullptr;

		pic->lock = false;

		free(pic);
	}

	emulator_log(true, LOG_SEVERITY_VERBOSE, "PIC deinitialized!");
}
