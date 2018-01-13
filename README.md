# GR-KURUMI Tools

This is a collection of various support tools for the [Gadget Renesas GR-KURUMI](http://gadget.renesas.com/en/product/kemuri.html) reference board. The goal is to have the board usable under a local Linux development environment. The board uses the RL78/G13 microcontroller, so make sure to get the [RL78 GCC Toolchain](https://gcc-renesas.com/wiki/index.php?title=Building_the_RL78_Toolchain_under_Ubuntu_14.04).

### Kurumi Writer
Kurumi Writer is a program to flash the RL78 microcontroller over the serial protocol. It implements the "RL78 Protocol A" described in Renesas application note R01AN0815EJ0100. I have only tested it with the GR-KURUMI board under Linux. It may work for other RL78-based boards as well.

### Kurumi Shell
The Kurumi Shell is an actual program to run on the GR-KURUMI board itself. Coded in C and to be compiled with the RL78 GCC toolchain. It provides a command shell interface against its LED and timer functions. The shell is spawned on UART #0, the same one used for flashing, since this is most convenient. The DTR signal must be disconnected in order to avoid the chip going into flashing mode though. The shell provides a simple BASIC-style scripting interface. Commands can be put into a script/program buffer, indexed by 0 to 99, which can be run continuously. 

### Kurumi Script
A small Python script to upload and run script files on a GR-KURUMI which has been flashed with the Kurumi Shell code.

