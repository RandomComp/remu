#include "types.h"

#include "std.h"

#include "builtins/string.h"

#include "drivers/hid/kbdps2.h"

#include "emush/emush.h"

int calc_cmd(const byte **argv, size_t argc) {
	byte buf[64] = { 0 };

	kprintf("Enter radix (10 default): ");

	getstr(true, buf, 64);

	uintmax_t radix = 0;
	
	parse_num(buf, 10, &radix, nullptr);

	radix = radix <= 1 ? 10 : radix;

	memset(buf, 0, 64);

	kprintf("Enter first number: ");

	getstr(true, buf, 64);

	intmax_t first_num = 0;
	
	parse_num(buf, radix, &first_num, nullptr);

	memset(buf, 0, 64);

	kprintf("Enter second number: ");

	getstr(true, buf, 64);

	intmax_t second_num = 0;
	
	parse_num(buf, radix, &second_num, nullptr);

	memset(buf, 0, 64);

	kprintf("Enter operator: ");

	getstr(true, buf, 64);

	char op_c = buf[0];

	const char supported_ops[] = "+-*/%~&|^";

	bool supported = false;

	for (size_t i = 0; i < sizeof(supported_ops) - 1; i++) {
		if (supported_ops[i] == op_c) {
			supported = true; break;
		}
	}

	if (!supported) {
		kprintf("Operation '%c' not supported.\n", op_c);
		
		return -1;
	}

	memset(buf, 0, 64);

	kprintf("Enter result radix (10 default): ");

	getstr(true, buf, 64);

	uintmax_t result_radix = 0;
	
	parse_num(buf, 10, &result_radix, nullptr);

	result_radix = result_radix <= 1 ? 10 : result_radix;

	intmax_t result = 0;

	switch (op_c) {
		case '+':
			result = first_num + second_num;
			break;

		case '-':
			result = first_num - second_num;
			break;

		case '*':
			result = first_num * second_num;
			break;

		case '/':
			if (second_num == 0) {
				kprintf("%vfbrError: Division by zero is an illegal operation.\n"); return -1;
			}
		
			result = first_num / second_num;
			break;

		case '%':
			if (second_num == 0) {
				kprintf("%vfbrError: Division by zero is an illegal operation.\n"); return -1;
			}
		
			result = first_num % second_num;
			break;
		case '~':
			result = (~first_num) + second_num;
			break;
		case '&':
			result = first_num & second_num;
			break;
		case '|':
			result = first_num | second_num;
			break;
		case '^':
			result = first_num ^ second_num;
			break;
		
		default:
			break;
	}

	kprintf("%in %c %in = %in\n", first_num, radix, op_c, second_num, radix, result, result_radix);

	return 0;
}
