VIRTUAL_CFLAGS := 

BAREMETAL_CFLAGS := -m32 -ffreestanding

CFLAGS := 

GASFLAGS := --32

BASE_CSRCFILES := $(shell find sources -name "*.c")

BASE_GASSRCFILES := $(shell find sources -name "*.s")

BASE_OBJFILES := \
	$(BASE_CSRCFILES:.c=.o) \
	$(BASE_GASSRCFILES:.s=.o)

EMULATOR_SRCFILES := $(shell find emulator/sources -name "*.c")

EMULATOR_OBJFILES := $(EMULATOR_SRCFILES:.c=.o)

EMULATOR_OBJFILES := \
	$(addprefix obj/emulator/, $(EMULATOR_OBJFILES)) \
	$(addprefix obj/emulator/, $(BASE_OBJFILES))

BAREMETAL_OBJFILES := \
	$(addprefix obj/baremetal/, $(BASE_OBJFILES))

all: emulator clean baremetal clean vmwareDisk

emulator_all: emulator clean run_emulator

baremetal_all: baremetal vmwareDisk clean run_baremetal

baremetal: kernel.bin image

image:
	@echo "Creating hdd.img..."

	@echo "Copy kernel and grub files on partition..."

	@-rm hdd.img

	@cp kernel.bin img/boot/
	
	@grub-mkrescue img/ -o hdd.img
	
	@echo "Done!"

run_baremetal:
	@qemu-system-i386 -drive file=hdd.img,format=raw

run_emulator:
	@./emulator.out

emulator: $(EMULATOR_OBJFILES)
	@gcc $^ -o $@.out

obj/emulator/%.o: %.c
	@mkdir -p $(dir $@)

	@gcc -Iinclude -Iemulator/include -Wall -Wpedantic -o $@ -c $^

obj/emulator/%.o: %.s
	@mkdir -p $(dir $@)

	@as --defsym LONG_MODE_MACRO=1 $^ -o $@

obj/baremetal/%.o: %.c
	@mkdir -p $(dir $@)

	@gcc -Iinclude $(BAREMETAL_CFLAGS) -o $@ -c $^

obj/baremetal/%.o: %.s
	@mkdir -p $(dir $@)

	@as $(GASFLAGS) -o $@ $^

bare_metal_csources: $(BAREMETAL_OBJFILES)

kernel.bin: CFLAGS := $(BAREMETAL_CFLAGS)
kernel.bin: $(BAREMETAL_OBJFILES)
	@ld -m elf_i386 -T linker.ld -o $@ $^

vmwareDisk:
	@qemu-img convert -f raw hdd.img -O vmdk Emulator_OS.vmdk

clean:
	@rm -f $(BAREMETAL_OBJFILES) $(EMULATOR_OBJFILES) kernel.bin

clean_all: clean
	@rm -f hdd.img emulator.out Emulator_OS.vmdk