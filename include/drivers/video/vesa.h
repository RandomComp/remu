#ifndef _EMULATOR_OS_VESA_H
#define _EMULATOR_OS_VESA_H

#include "types.h"

#include "terminal.h"

terminal_out_t init_vesa_stdout(void* _fb, size_t _fb_w, size_t _fb_h, size_t _fb_bpp, size_t _fb_pitch);

#endif
