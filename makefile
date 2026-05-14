CC := clang

CFLAGS := -Wall -O2 -Wpadded -Werror=return-type -Wno-unused-parameter -Werror=uninitialized  -Werror=implicit-function-declaration -Werror=address -Werror=type-limits -Werror=shadow -Werror=pointer-arith -Werror=cast-align -Werror=float-conversion -Werror=undef

# -Werror=sign-compare

#  -fsanitize=address -g

EMULATOR_SRCFILES := $(shell find emulator/sources -name "*.c")

EMULATOR_OBJFILES := $(EMULATOR_SRCFILES:.c=.o)

EMULATOR_OBJFILES := \
	$(addprefix obj/, $(EMULATOR_OBJFILES))

.SUFFIXES:

all: emulator_all clean

emulator_all: remu clean emulator_to_kernel_path

emulator_to_kernel_path:
	@echo "Copying remu to kernel path..."

	@rm -f ~/Projects-on-SSD/kernel/remu

	@cp remu ~/Projects-on-SSD/kernel/

	@echo "Copied remu to kernel path"

remu: $(EMULATOR_OBJFILES)
	@$(CC) $^ -o $@ -lSDL2 -lSDL2_image -pthread

#	@$(CC) $^ -o $@.out -pthread

obj/%.o: %.c
	@mkdir -p $(dir $@)

	@$(CC) -Iemulator/include $(CFLAGS) -o $@ -c $^

clean:
	@rm -f $(EMULATOR_OBJFILES)

clean_all: clean_emulator clean_os clean
	@rm -f remu

