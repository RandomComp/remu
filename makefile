CFLAGS := -Wall

VIRTUAL_CFLAGS := $(CFLAGS)

OS_CFLAGS := $(CFLAGS) -nostdlib -nostdinc -fno-asynchronous-unwind-tables -fno-stack-protector

BAREMETAL_CFLAGS := $(CFLAGS) -m32 -ffreestanding

GASFLAGS := --32

BASE_CSRCFILES := $(shell find sources -name "*.c")

BASE_GASSRCFILES := $(shell find sources -name "*.s")

BASE_OBJFILES := \
	$(BASE_CSRCFILES:.c=.o) \
	$(BASE_GASSRCFILES:.s=.o)

EMULATOR_SRCFILES := $(shell find emulator/sources -name "*.c")

EMULATOR_OBJFILES := $(EMULATOR_SRCFILES:.c=.o)

EMULATOR_OBJFILES := \
	$(addprefix obj/emulator/, $(EMULATOR_OBJFILES))

OS_OBJFILES := \
	$(addprefix obj/os/, $(BASE_OBJFILES))

BAREMETAL_OBJFILES := \
	$(addprefix obj/baremetal/, $(BASE_OBJFILES)) \
	obj/baremetal/loader.o

all: emulator clean baremetal clean vmwareDisk

emulator_all: emulator clean emulator_os clean run_emulator

baremetal_all: baremetal vmwareDisk clean run_baremetal

baremetal: kernel.bin image

image:
	@echo "Creating hdd.img..."

	@echo "Copy kernel and grub files on partition..."

	@rm -f hdd.img

	@cp kernel.bin img/boot/
	
	@grub-mkrescue img/ -o hdd.img
	
	@echo "Done!"

run_baremetal:
	@qemu-system-i386 -drive file=hdd.img,format=raw

run_emulator:
	@./emulator.out

emulator: $(EMULATOR_OBJFILES)
	@clang -g $^ -o $@.out -lSDL2 -lSDL2_image -pthread -lm

emulator_os: $(OS_OBJFILES)
	@clang -shared -Wl,--gc-sections -Wl,-z,norelro -g $^ -o $@.so

obj/os/%.o: %.c
	@mkdir -p $(dir $@)

	@clang -g -Iinclude -fPIC $(OS_CFLAGS) -o $@ -c $^

obj/os/%.o: %.s
	@mkdir -p $(dir $@)

	@as --defsym LONG_MODE_MACRO=1 $^ -o $@

obj/emulator/%.o: %.c
	@mkdir -p $(dir $@)

	@clang -g -Iemulator/include $(VIRTUAL_CFLAGS) -o $@ -c $^

obj/baremetal/%.o: %.c
	@mkdir -p $(dir $@)

	@clang -Iinclude $(BAREMETAL_CFLAGS) -o $@ -c $^

obj/baremetal/%.o: %.s
	@mkdir -p $(dir $@)

	@as $(GASFLAGS) -o $@ $^

bare_metal_csources: $(BAREMETAL_OBJFILES)

kernel.bin: CFLAGS := $(BAREMETAL_CFLAGS)
kernel.bin: $(BAREMETAL_OBJFILES)
	ld -m elf_i386 -T linker.ld -o $@ $^

vmwareDisk:
	@qemu-img convert -f raw hdd.img -O vmdk Emulator_OS.vmdk

clean:
	@rm -f $(BAREMETAL_OBJFILES) $(EMULATOR_OBJFILES) $(OS_OBJFILES) kernel.bin

clean_all: clean
	@rm -f hdd.img emulator.out emulator_os.so Emulator_OS.vmdk

