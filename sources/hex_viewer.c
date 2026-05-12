#include "hex_viewer.h"

#include "colors.h"

#include "std/stdio.h"
#include "std/stdlib.h"
#include "std/string.h"
#include "builtins/string.h"

static const byte* num_alphabet = "0123456789ABCDEF";

void hex_viewer(byte* num, size_t offset, size_t size) {
	if (num == nullptr) return;

	size_t i = 0;

	ssize_t pos_x = 0;

	// vga_disable_blink();

	size_t step = 3;

	byte ascii_buf[] = { "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0" };

	size_t digits = MAX(5, get_num_digits(offset + size, 16, false) + 1);

	ssize_t width = get_columns() - (1 + (digits + 3) * 2 + sizeof(ascii_buf));

	size_t ascii_buf_size = MIN(width / 3, sizeof(ascii_buf));

	width = align_down(width, step) - (step * 2);

	kprintf("┌%0m─*s┬%0m─*s┬%0m─*s┬%0m─*s┐\n\r", digits, "", width, "", digits, "", ascii_buf_size, "");

	kprintf("│START│%=*s│%=*s│%=*s│\n\r", width, "HEX VIEW", digits, "END", ascii_buf_size, "ASCII");

	kprintf("├%0m─*s┼%0m─*s┼%0m─*s┼%0m─*s┤\n\r", digits, "", width, "", digits, "", ascii_buf_size, "");

	while (i < size) {
		set_style(COLOR_BRIGHT_WHITE);

		kprintf("│%vfw%.*zx%vd│", digits, MIN(size + offset, i + offset));

		size_t ascii_colon_width = MIN(sizeof(ascii_buf), width / step);

		#define READ_NUM(index) ((index) < (size) ? (num)[(index)] : 0)

		memset(ascii_buf, 0, sizeof(ascii_buf));

		size_t last_i = i;

		pos_x = 0;
		
		while (pos_x < width) {
			byte num_byte = READ_NUM(i);

			byte 	hi = (num_byte >> 4) & 0xF,
					lo = num_byte & 0xF;
			
			size_t color = (size_t)(hi + lo) / 2;
			
			if (hi == 0 && lo == 0) {
				set_style(COLOR_WHITE);
			}

			else {
				set_style(color >= 1 ? color : COLOR_BRIGHT_WHITE);
			}

			if (i >= size) {
				putch(' '); 
				putch(' '); 
				putch(' ');
			}

			else {
				putch(num_alphabet[hi]);
				putch(num_alphabet[lo]);

				putch(' ');
			}

			pos_x += step;

			i += 1;
		}

		set_style(COLOR_BRIGHT_WHITE);

		kprintf("│%vfw%.*zx%vd│", digits, MIN(size + offset, i + offset));

		for (size_t k = 0; k < ascii_buf_size; k++) {
			if (k >= (i - last_i)) {
				putch(' '); continue;
			}

			byte num_byte = READ_NUM(last_i + k);

			byte hi = (num_byte >> 4) & 0xF,
				lo = num_byte & 0xF;
			
			size_t color = (size_t)(hi + lo) / 2;
			
			if (hi == 0 && lo == 0) {
				set_style(COLOR_WHITE);
			}

			else {
				set_style(color >= 1 ? color : COLOR_BRIGHT_WHITE);
			}

			if (isprintable(num_byte))
				putch(num_byte);
			else
				putch('.');
		}

		set_style(COLOR_BRIGHT_WHITE);

		kprint("│\n\r");
	}

	kprintf("└%0m─*s┴%0m─*s┴%0m─*s┴%0m─*s┘", digits, "", width, "", digits, "", ascii_buf_size, "");
}
