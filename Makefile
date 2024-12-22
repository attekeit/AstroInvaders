##TODOs
#
#-make sure that the parity check works
#
#-For operations like JMP, we have
#state->pc = (opcode[2] << 8 | opcode[1]);
#Why do we need to have opcode[1] with bitwise and with 0? That's just opcode[1]
#opcode[2] << 8 = 0 as each entry in opcode is 8 bits

CC = gcc
LIBS = -lSDL2
INCLUDE = -I/usr/include/
CFLAGS = -g -O0 $(INCLUDE)# Flag for implicit rules. Turn on debug info

emulator: emulator.o
	$(CC)  -o emulator emulator.o $(LIBS)


clean:
	rm -f emulator emulator.o