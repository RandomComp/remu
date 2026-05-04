CC := clang

CFLAGS := -Wall

EMULATOR_CFLAGS := $(CFLAGS)

OS_CFLAGS := -fPIC $(CFLAGS) -ffreestanding -nostdlib -nostdinc -fno-inline -fvisibility=hidden -fno-plt -fno-builtin -fno-builtin-functions -fno-common -fno-asynchronous-unwind-tables -fno-stack-protector -Wno-pointer-sign -D__EMULATOR__

BAREMETAL_CFLAGS := $(CFLAGS) -m32 -ffreestanding

GASFLAGS := --32

OS_CSRCFILES := $(shell find sources -name "*.c")

OS_GASSRCFILES := $(shell find sources -name "*.s")

OS_OBJFILES := \
	$(OS_CSRCFILES:.c=.o) \
	$(OS_GASSRCFILES:.s=.o)

EMULATOR_SRCFILES := $(shell find emulator/sources -name "*.c")

EMULATOR_OBJFILES := $(EMULATOR_SRCFILES:.c=.o)

EMULATOR_OBJFILES := \
	$(addprefix obj/emulator/, $(EMULATOR_OBJFILES))

EMULATOR_OS_OBJFILES := \
	$(addprefix obj/os_objs/, $(OS_OBJFILES))

BAREMETAL_NASMSRCFILES := $(shell find sources -name "*.nasm")

BAREMETAL_OBJFILES := $(BAREMETAL_NASMSRCFILES:.nasm=.o)

BAREMETAL_OBJFILES := \
	$(addprefix obj/baremetal/, $(BAREMETAL_OBJFILES)) \
	$(addprefix obj/baremetal/, $(OS_OBJFILES)) \
	obj/baremetal/loader.o

.SUFFIXES:

all: emulator clean baremetal clean vmwareDisk

emulator_all: emulator clean os clean emulator_run

baremetal_all: baremetal vmwareDisk clean baremetal_run

baremetal: kernel.bin image

image:
	@echo "Creating hdd.img..."

	@rm -f hdd.img

	@cp kernel.bin img/boot/
	
	@grub-mkrescue img/ -o hdd.img
	
	@echo "Done!"

baremetal_run:
	@qemu-system-i386 -drive file=hdd.img,format=raw

emulator_run:
	@./emulator.out ./os.so

emulator: $(EMULATOR_OBJFILES)
	@$(CC) $^ -o $@.out -lSDL2 -lSDL2_image -pthread

os_all: clean_os os clean emulator_run

os: $(EMULATOR_OS_OBJFILES)
	@$(CC) -ffreestanding -nostdlib -shared -Wl,--gc-sections -Wl,-z,norelro $^ -o $@.so

obj/os_objs/%.o: %.c
	@mkdir -p $(dir $@)

	@$(CC) -Iinclude $(OS_CFLAGS) -o $@ -c $^

obj/os_objs/%.o: %.s
	@mkdir -p $(dir $@)

	@as --defsym LONG_MODE_MACRO=1 $^ -o $@

obj/emulator/%.o: %.c
	@mkdir -p $(dir $@)

	@$(CC) -Iemulator/include $(EMULATOR_CFLAGS) -o $@ -c $^

obj/baremetal/%.o: %.c
	@mkdir -p $(dir $@)

	@$(CC) -Iinclude $(BAREMETAL_CFLAGS) -o $@ -c $^

obj/baremetal/%.o: %.s
	@mkdir -p $(dir $@)

	@as $(GASFLAGS) -o $@ $^

obj/baremetal/%.o: %.nasm
	@mkdir -p $(dir $@)

	@nasm -f elf32 $^ -o $@

kernel.bin: $(BAREMETAL_OBJFILES)
	@ld -m elf_i386 -T linker.ld -o $@ $^

vmwareDisk:
	@qemu-img convert -f raw hdd.img -O vmdk Emulator_OS.vmdk

clean_emulator:
	@rm -f $(EMULATOR_OBJFILES) emulator.out

clean_os:
	@rm -f $(EMULATOR_OS_OBJFILES) $(OS_OBJFILES) emulator_os.so

clean:
	@rm -f $(EMULATOR_OS_OBJFILES) $(OS_OBJFILES) $(EMULATOR_OBJFILES) $(BAREMETAL_OBJFILES) kernel.bin

clean_all: clean_emulator clean_os clean
	@rm -f hdd.img Emulator_OS.vmdk

