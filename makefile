CC := clang

CFLAGS := -Wall -O2 -Wpadded -Wpacked -Werror=return-type -Wno-unused-parameter -Werror=uninitialized  -Werror=implicit-function-declaration -Werror=address -Werror=type-limits -Werror=shadow -Werror=pointer-arith -Werror=cast-align -Werror=float-conversion -Werror=undef

# -Werror=sign-compare

EMULATOR_CFLAGS := $(CFLAGS)

#  -fsanitize=address -g

EMULATOR_SRCFILES := $(shell find emulator/sources -name "*.c")

EMULATOR_OBJFILES := $(EMULATOR_SRCFILES:.c=.o)

EMULATOR_OBJFILES := \
	$(addprefix obj/emulator/, $(EMULATOR_OBJFILES))

.SUFFIXES:

all: emulator_all clean

emulator_all: emulator clean emulator_to_kernel_path

emulator_to_kernel_path:
	@echo "Copying emulator to kernel path..."

	@rm -f ~/Projects-on-SSD/kernel/emulator.out

	@cp emulator.out ~/Projects-on-SSD/kernel/

	@echo "Copied emulator to kernel path"

emulator: $(EMULATOR_OBJFILES)
	@$(CC) $^ -o $@.out -lSDL2 -lSDL2_image -pthread

#	@$(CC) $^ -o $@.out -pthread

obj/emulator/%.o: %.c
	@mkdir -p $(dir $@)

	@$(CC) -Iemulator/include $(EMULATOR_CFLAGS) -o $@ -c $^

clean:
	@rm -f $(EMULATOR_OBJFILES)

clean_all: clean_emulator clean_os clean
	@rm -f emulator.out

