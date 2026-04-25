#include "ansi.h"

#include "colors.h"

#include "types.h"

byte color_to_ansi(colors_e color) {
	switch (color) {
		case COLOR_BLUE:
			return BLUE_ANSI_ID;
		case COLOR_GREEN:
			return GREEN_ANSI_ID;
		case COLOR_AQUA:
			return AQUA_ANSI_ID;
		case COLOR_RED:
			return RED_ANSI_ID;
		case COLOR_MAGENTA:
			return MAGENTA_ANSI_ID;
		case COLOR_BROWN:
			return YELLOW_ANSI_ID;
		case COLOR_WHITE:
			return WHITE_ANSI_ID;
		default:
			return 0;
	}

	return 0;
}
