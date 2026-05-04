#include "drivers/time/cmos.h"

#include "types.h"

#include "drivers/io.h"

#include "bcd.h"

#include "std.h"

void cmos_write_reg(byte reg, byte val) {
	out8(0x70, reg);

	out8(0x71, val);
}

byte cmos_read_reg(byte reg) {
	out8(0x70, reg);

	return in8(0x71);
}

bool cmos_update_in_progress() {
	byte reg_a = cmos_read_reg(0xA);

	return (reg_a & CMOS_REGISTER_A_UPDATE_IN_PROGRESS) != 0;
}

void show_rtc_time() {
	byte reg_b = cmos_read_reg(0x0B);

	byte second = cmos_read_reg(0x00);

	byte minute = cmos_read_reg(0x02);

	byte hour = cmos_read_reg(0x04);

	bool pm = false;

	if ((reg_b & CMOS_REGISTER_B_IS_24_FORMAT) == 0) {
		pm = hour & 0x80;

		hour &= 0x7F;
	}

	if ((reg_b & CMOS_REGISTER_B_IS_BINARY_MODE) == 0) {
		second = from_bcd(second);

		minute = from_bcd(minute);

		hour = from_bcd(hour);
	}

	kprintf("%.2i:%.2i:%.2i", hour, minute, second);

	if ((reg_b & CMOS_REGISTER_B_IS_24_FORMAT) == 0) {
		if (pm) kprint(" PM");

		else kprint(" AM");
	}
}

void show_rtc_date() {
	byte reg_b = cmos_read_reg(0x0B);

	byte day = cmos_read_reg(CMOS_RTC_DAY_OF_MONTH);

	byte month = cmos_read_reg(CMOS_RTC_MONTHS);

	byte year = cmos_read_reg(CMOS_RTC_YEARS);

	if ((reg_b & CMOS_REGISTER_B_IS_BINARY_MODE) == 0) {
		day = from_bcd(day);

		month = from_bcd(month);

		year = from_bcd(year);
	}

	kprintf("%.2i.%.2i.%.4i", (int)day, (int)month, (int)(year + 2000));
}

byte read_rtc_seconds() {
	byte reg_b = cmos_read_reg(0x0B);

	byte second = cmos_read_reg(0x00);

	if ((reg_b & CMOS_REGISTER_B_IS_BINARY_MODE) == 0) {
		second = from_bcd(second);
	}

	return second;
}
