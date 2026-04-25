#ifndef _EMULATOR_CMOS_H
#define _EMULATOR_CMOS_H

#include "types.h"

#include "time/time.h"

#include "drivers/time/cmos.h"

typedef struct cmos_t {
	_time_t unix_time;
	byte reg_a;
	byte reg_b;
	byte reg_c;
	byte reg_d;
} cmos_t;

// typedef enum cmos_regs_e {
// 	CMOS_RTC_SECONDS, 					// Получение значения секунд. 					                                        Чтение и запись разрешены.
// 	CMOS_RTC_SECOND_ALARM, 				// Получение значения секунд будильника. 		                                        Чтение и запись разрешены.
// 	CMOS_RTC_MINUTES,					// Получение значения минут. 					                                        Чтение и запись разрешены.
// 	CMOS_RTC_MINUTE_ALARM,				// Получение значения минут будильника. 		                                        Чтение и запись разрешены.
// 	CMOS_RTC_HOURS,						// Получение значения часов. 					                                        Чтение и запись разрешены.
// 	CMOS_RTC_HOUR_ALARM,				// Получение значения часов будильника. 		                                        Чтение и запись разрешены.
// 	CMOS_RTC_DAY_OF_WEEK,				// Получение значения дня недели. 				                                        Чтение и запись разрешены.
// 	CMOS_RTC_DAY_OF_MONTH,				// Получение значения дня месяца. 				                                        Чтение и запись разрешены.
// 	CMOS_RTC_MONTHS,					// Получение значения месяца. 					                                        Чтение и запись разрешены.
// 	CMOS_RTC_YEARS,						// Получение значения годов. 					                                        Чтение и запись разрешены.
// 	CMOS_REGISTER_A,					// Получение регистра A. см RegisterA_Bits. 	                                        Чтение и запись разрешены.
// 	CMOS_REGISTER_B,					// Получение регистра B. см RegisterB_Bits. 	                                        Чтение и запись разрешены.
// 	CMOS_REGISTER_C,					// Получение регистра C. см RegisterC_Bits. 	                                        Только чтение.
// 	CMOS_REGISTER_D,					// Получение регистра D. см RegisterD_Bits. 	                                        Только чтение.
// 	CMOS_RTC_CENTURY = 0x32            	// Получение значения века.                                                             Чтение и запись разрешены.
// } cmos_regs_e;

// // Чтение и запись разрешена для 0-6 битов.
// typedef enum cmos_reg_a_bits_e {
// 	CMOS_REGISTER_A_RATE_SELECT_0_BIT 		= 0x01, 		// Настраивает частоту периодического прерывания, 				0 бит 4 битного числа - делителя, 0001
// 	CMOS_REGISTER_A_RATE_SELECT_1_BIT 		= 0x02, 		// Настраивает частоту периодического прерывания, 				1 бит 4 битного числа - делителя, 0010
// 	CMOS_REGISTER_A_RATE_SELECT_2_BIT 		= 0x04, 		// Настраивает частоту периодического прерывания, 				2 бит 4 битного числа - делителя, 0100
// 	CMOS_REGISTER_A_RATE_SELECT_3_BIT 		= 0x08, 		// Настраивает частоту периодического прерывания, 				3 бит 4 битного числа - делителя, 1000
// 	CMOS_REGISTER_A_DIVIDER_SELECT_0_BIT 	= 0x10, 		// Настраивает режим работы внутреннего делителя частоты RTC,  	0 бит 3 битного числа - делителя, 0001
// 	CMOS_REGISTER_A_DIVIDER_SELECT_1_BIT 	= 0x20, 		// Настраивает режим работы внутреннего делителя частоты RTC,  	1 бит 3 битного числа - делителя, 0010
// 	CMOS_REGISTER_A_DIVIDER_SELECT_2_BIT 	= 0x40, 		// Настраивает режим работы внутреннего делителя частоты RTC,  	2 бит 3 битного числа - делителя, 0100
// 	CMOS_REGISTER_A_UPDATE_IN_PROGRESS 		= 0x80 			// Флаг обновления RTC, только для чтения.
// } cmos_reg_a_bits_e;

// // Разрешено чтение и запись для всех битов (0-7 бит).
// typedef enum cmos_reg_b_bits_e {
// 	CMOS_REGISTER_B_DAYLIGHT_SAVINGS_ENABLE 	= 0x01,		// Флаг автоматического перехода на летнее время, т.е в последнее воскресенье марта переходят на час вперед, в последнее воскресенье октября начинается обычное время
// 	CMOS_REGISTER_B_IS_24_FORMAT 				= 0x02,		// Флаг 24 часового режима, 1 - 0-24 часа, 0 - 0-12 часа, при 0 - AM/PM режим бит работает.
// 	CMOS_REGISTER_B_IS_BINARY_MODE 				= 0x04,		// Флаг активности binary режима, если 0 то BCD.
// 	CMOS_REGISTER_B_SQUARE_WAVE_ENABLE 			= 0x08,		// Вкл/выкл генерации квадратного сигнала на вывод SQWB.
// 	CMOS_REGISTER_B_UPDATE_INTERRUPT_ENABLE 	= 0x10,		// Вкл/выкл генерации прерывания при каждом обновлении секунды.
// 	CMOS_REGISTER_B_ALARM_INTERRUPT_ENABLE 		= 0x20,		// Вкл/выкл генерации прерывания будильника.
// 	CMOS_REGISTER_B_PERIODIC_INTERRUPT_ENABLE 	= 0x40,		// Вкл/выкл периодическую генерацию прерывания IRQ8.
// 	CMOS_REGISTER_B_TIME_SET 					= 0x80		// Отключить обновление времени RTC. ( для записи )
// } cmos_reg_b_bits_e;

// // Важно: Чтение флагов из регистра C обнулят все флаги в этом регистре. Только чтение. Регистр C является регистром событий.
// typedef enum cmos_reg_c_bits_e {
// 	CMOS_REGISTER_C_UPDATE_FLAG				=	0x10, 		// Если установлено в 1 то произошло обновление секунды по флагу REGISTER_B_UPDATE_INTERRUPT_ENABLE.
// 	CMOS_REGISTER_C_ALARM_FLAG 				=	0x20, 		// Если установлено в 1 то сработал будильник по флагу REGISTER_B_ALARM_INTERRUPT_ENABLE.
// 	CMOS_REGISTER_C_PERIODIC_FLAG 			=	0x40,		// Если установлено в 1 то сработало периодическое прерывание по флагу REGISTER_B_PERIODIC_INTERRUPT_ENABLE.
// 	CMOS_REGISTER_C_INTERRUPT_REQUEST_FLAG 	= 	0x80 		// Если установлено в 1 то есть необработанные события.
// } cmos_reg_c_bits_e;

// // Только для чтения.
// typedef enum cmos_reg_d_bits_e {
// 	CMOS_REGISTER_D_IS_CMOS_BATTERY_CHARGED = 0x80 	// Если установлено в 1 - то заряжена, если в 0 то разряженна ( соответственно некорректная дата и время )
// } cmos_reg_d_bits_e;

cmos_t* init_cmos();

void free_cmos(cmos_t* cmos);

void reset_cmos(cmos_t* cmos);

void release_all_cmos();

#endif
