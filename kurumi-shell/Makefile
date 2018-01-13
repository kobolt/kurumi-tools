
TOOL_PATH:=/disk/temp/RL78-Toolchain/prefix/bin

CFLAGS = -Wall -Wextra -c -Os -ffunction-sections -fdata-sections -I. -Icommon
LDFLAGS = -Wl,--gc-sections -nostartfiles

kurumi.bin: kurumi.elf
	$(TOOL_PATH)/rl78-elf-objcopy -O binary $^ $@

kurumi.elf: crt0.o main.o led.o uart.o timer.o script.o opcode.o command.o
	$(TOOL_PATH)/rl78-elf-gcc $(LDFLAGS) -T common/rl78_R5F100GJAFB.ld $^ -o $@

crt0.o: common/crt0.S
	$(TOOL_PATH)/rl78-elf-gcc $(CFLAGS) $^ -o $@

led.o: led.c
	$(TOOL_PATH)/rl78-elf-gcc $(CFLAGS) $^ -o $@

timer.o: timer.c
	$(TOOL_PATH)/rl78-elf-gcc $(CFLAGS) $^ -o $@

uart.o: uart.c
	$(TOOL_PATH)/rl78-elf-gcc $(CFLAGS) $^ -o $@

script.o: script.c
	$(TOOL_PATH)/rl78-elf-gcc $(CFLAGS) $^ -o $@

opcode.o: opcode.c
	$(TOOL_PATH)/rl78-elf-gcc $(CFLAGS) $^ -o $@

command.o: command.c
	$(TOOL_PATH)/rl78-elf-gcc $(CFLAGS) $^ -o $@

main.o: main.c
	$(TOOL_PATH)/rl78-elf-gcc $(CFLAGS) $^ -o $@

.PHONY: clean
clean:
	rm -f *.o *.elf *.bin

