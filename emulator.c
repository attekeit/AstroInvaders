#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <SDL2/SDL.h>
#include <sys/time.h>

/*

All work here is done following the emulator101.com guide.

This is a disassembler for the 8080 processor.
The goal is to have each hex code to "readable" format.
In this case (I think) it means we change it to assembly language

*/

//This includes all the conditions 8080 uses (not sure where emulator101 found these)
typedef struct Conditions
{
    uint8_t z:1;
    uint8_t s:1;
    uint8_t p:1;
    uint8_t cy:1;
    uint8_t ac:1;
    uint8_t pad:3;
} 
Conditions;


//This includes all the registers, memory, progress counter, stack pointer
//and conditions. Also int_enable, whatever that means
typedef struct State_8080
{
    uint8_t A;
    uint8_t B;
    uint8_t C;
    uint8_t D;
    uint8_t E;
    uint8_t H;
    uint8_t L;
    uint8_t *mem;
    uint16_t sp;
    uint16_t pc;
    struct Conditions conds;
    uint8_t interrupt_enable;
    uint32_t ticks;
    uint8_t shift1; //TODO should init these to 0
    uint8_t shift2;
    uint8_t shift_offset;
    uint8_t first_shift_write;

} State_8080;

const int opcode_cycles[256] = {
    4, 10, 7, 5, 5, 5, 7, 4, 4, 10, 7, 5, 5, 5, 7, 4,
    4, 10, 7, 5, 5, 5, 7, 4, 4, 10, 7, 5, 5, 5, 7, 4,
    4, 10, 16, 5, 5, 5, 7, 4, 4, 10, 16, 5, 5, 5, 7, 4,
    4, 10, 13, 5, 10, 10, 10, 4, 4, 10, 13, 5, 5, 5, 7, 4,
    5, 5, 5, 5, 5, 5, 7, 5, 5, 5, 5, 5, 5, 5, 7, 5,
    5, 5, 5, 5, 5, 5, 7, 5, 5, 5, 5, 5, 5, 5, 7, 5,
    5, 5, 5, 5, 5, 5, 7, 5, 5, 5, 5, 5, 5, 5, 7, 5,
    7, 7, 7, 7, 7, 7, 7, 7, 5, 5, 5, 5, 5, 5, 7, 5,
    4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4,
    4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4,
    4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4,
    4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4,
    5, 10, 10, 10, 11, 11, 7, 11, 5, 10, 10, 10, 11, 17, 7, 11,
    5, 10, 10, 10, 11, 11, 7, 11, 5, 10, 10, 10, 11, 17, 7, 11,
    5, 10, 10, 18, 11, 11, 7, 11, 5, 5, 10, 4, 11, 17, 7, 11,
    5, 10, 10, 4, 11, 11, 7, 11, 5, 5, 10, 4, 11, 17, 7, 11
};


//This is actually a RST, equivalent to call to a low memory location
//Push pc to current stack
void handle_interrupt(State_8080 *state, int interrupt_num) {
    state->mem[state->sp-1] = (state->pc & 0xff00) >> 8; //TODO why like this?
    state->mem[state->sp-2] = state->pc & 0xff;
    state->sp -= 2;
    state->pc = 8*interrupt_num;
    return;
}

/*
The function disassemble8080 takes as input a pointer  *buffer,
in emulating101: "*codebuffer is a valid pointer to 8080 assembly code"
Also takes in pc (program counter), which is the current offset of the code

*/

//All I/O insstructions are not implemented, if any of those are
//called, exit the program
void UnimplementedIO(State_8080 *state){
    printf("ERROR: Unimplemented IO operation, stopping. \n");
    exit(2);
}

//TODO these will be implemented later
uint8_t machineIN(State_8080 *state, uint8_t port){
    /*Port 0, not sure if this is needed?
    bit 0 DIP4 (Seems to be self-test-request read at power up)
    bit 1 Always 1
    bit 2 Always 1
    bit 3 Always 1
    bit 4 Fire
    bit 5 Left
    bit 6 Right
    bit 7 ? tied to demux port 7 ?

    */

    SDL_Event event;
    uint8_t result = 0;

    switch (port) {
        case 0:{
            printf("IN port 0 needs to be implemented! \n");
            UnimplementedIO(state);
            break;
        }

        /*Port 1
        bit 0 = CREDIT (1 if deposit)       1
        bit 1 = 2P start (1 if pressed)     2
        bit 2 = 1P start (1 if pressed)     4
        bit 3 = Always 1                    8
        bit 4 = 1P shot (1 if pressed)      16
        bit 5 = 1P left (1 if pressed)      32
        bit 6 = 1P right (1 if pressed)     64
        bit 7 = Not connected               128*/             
        case 1:{
            result |= 0x08;
            while (SDL_PollEvent(&event)) {
                switch (event.type){
                    case SDL_QUIT:
                        exit(42);
                    //We care only when something has been pressed down, right?
                    case SDL_KEYDOWN:
                        switch (event.key.keysym.sym) {
                            case SDLK_c: {  
                                result |= 0x01; 
                                printf("You have pressed 'c', inserting coin! \n");
                                exit(6969);
                                break;
                            }
                            case SDLK_2: {
                                result |= 0x02;
                                printf("Player 2 start \n");
                                break;
                            }
                            case SDLK_1: {
                                result |= 0x04;
                                printf("Player 1 start! \n");
                                break;
                            }
                            case SDLK_SPACE: {
                                result |= 0x10;
                                printf("Player 1 shoots! \n");
                                break;
                            }
                            case SDLK_LEFT: {
                                result |= 0x20;
                                printf("PLayer 1 goes left! \n");
                                break;
                            }
                            case SDLK_RIGHT: {
                                result |= 0x40;
                                printf("Player 1 goes right! \n");
                                break;
                            }
                        }
                    case SDL_KEYUP:
                        switch (event.key.keysym.sym) 
                        {    case SDLK_c: {  
                                result &= (0xff-0x01); 
                                printf("You have lifted 'c'! \n");
                                break;
                            }
                            case SDLK_2: {
                                result &= (0xff-0x02);
                                printf("Lifting player 2 start \n");
                                break;
                            }
                            case SDLK_1: {
                                result &= (0xff-0x04);
                                printf("Lifting player 1 start! \n");
                                break;
                            }
                            case SDLK_SPACE: {
                                result &= (0xff-0x10);
                                printf("Lifting player 1 shoots! \n");
                                break;
                            }
                            case SDLK_LEFT: {
                                result &= (0xff-0x20);
                                printf("Lifting player 1 goes left! \n");
                                break;
                            }
                            case SDLK_RIGHT: {
                                result &= (0xff-0x40);
                                printf("Lifting player 1 goes right! \n");
                                break;
                            }
                        }
                        

                    default:
                        break;
                }
            }
            break;
        }

        /*Port 2
        bit 0 = DIP3 00 = 3 ships  10 = 5 ships
        bit 1 = DIP5 01 = 4 ships  11 = 6 ships (number of lives)
        bit 2 = Tilt
        bit 3 = DIP6 0 = extra ship at 1500, 1 = extra ship at 1000
        bit 4 = P2 shot (1 if pressed)
        bit 5 = P2 left (1 if pressed)
        bit 6 = P2 right (1 if pressed)
        bit 7 = DIP7 Coin info displayed in demo screen 0=ON*/
        case 2:{
            
            //printf("IN port 2 needs to be implemented! Missing bits 0, 1, 3 and 7!\n");
            //UnimplementedIO(state);
            result |= 0x08;
            while (SDL_PollEvent(&event)) {
                switch (event.type){
                    case SDL_QUIT:
                        exit(42);
                    //We care only when something has been pressed down, right?
                    case SDL_KEYDOWN:
                        switch (event.key.keysym.sym) { 
                            //First 2 bits tell the number of lives, no idea how to implement. They'll just be zero.
                            /*case SDLK_c: {  
                                result |= 0x01; 
                                printf("You have pressed 'c', inserting coin! \n");
                                exit(6969);
                                break;
                            }
                            case SDLK_2: {
                                result |= 0x02;
                                printf("Player 2 start \n");
                                break;
                            }*/
                            case SDLK_t: {
                                result |= 0x04;
                                printf("Someone tilted the machine! \n");
                                break;
                            }
                            //Bit 3 tells how many points needed for extra life (1000 1500)
                            //same as number of lives, no idea how to implement currently
                            /*case SDLK_w: {
                                result |= 0x10;
                                printf("Player 2 shoots! \n");
                                break;
                            }*/
                            case SDLK_a: {   //bit 5
                                result |= 0x20;
                                printf("PLayer 2 goes left! \n");
                                break;
                            }
                            case SDLK_d: { //bit 6
                                result |= 0x40;
                                printf("Player 2 goes right! \n");
                                break;
                            }
                            //Nothing for bit seven, let's just keep it at zero for now
                        }
                    case SDL_KEYUP:
                        switch (event.key.keysym.sym) 
                        {    case SDLK_c: {  
                                result &= (0xff-0x01); 
                                printf("You have lifted 'c'! \n");
                                break;
                            }
                            case SDLK_2: {
                                result &= (0xff-0x02);
                                printf("Lifting player 2 start \n");
                                break;
                            }
                            case SDLK_1: {
                                result &= (0xff-0x04);
                                printf("Lifting player 1 start! \n");
                                break;
                            }
                            case SDLK_SPACE: {
                                result &= (0xff-0x10);
                                printf("Lifting player 1 shoots! \n");
                                break;
                            }
                            case SDLK_LEFT: {
                                result &= (0xff-0x20);
                                printf("Lifting player 1 goes left! \n");
                                break;
                            }
                            case SDLK_RIGHT: {
                                result &= (0xff-0x40);
                                printf("Lifting player 1 goes right! \n");
                                break;
                            }
                        }
                        

                    default:
                        break;
                }
            }
            break;
        }

        //Port 3
        //bit 0-7 Shift register data
        case 3:{
            //printf("IN port 3 needs to be implemented! \n");
            //UnimplementedIO(state);

            uint16_t shift;
            uint8_t shift1, shift2, shift_offset;
            shift1 = state->shift1;
            shift2 = state->shift2;
            shift_offset = state->shift_offset;

            shift = (shift2 <<8) | shift1; //is this correct?
            result = (shift >> (8-shift_offset)) & 0xff;

            break;
        }  

    default:
        break;
    }


    //TODO Maybe works without the while, we just check for one input per loop.
    /*while (SDL_PollEvent(&event)) {
        switch (event.type){
            case SDL_QUIT:
                exit(42);
            //We care only when something has been pressed down, right?
            case SDL_KEYDOWN:
                case SDLK_c: {
                    printf("You have pressed 'c', inserting coin! \n");
                    exit(6969);
                    break;
                }
                

            default:
                break;
        }
    }*/
    


    return result;
}

void machineOUT(State_8080 *state, uint8_t port){
    /*

    Port 3: (discrete sounds)
    bit 0=UFO (repeats)        SX0 0.raw
    bit 1=Shot                 SX1 1.raw
    bit 2=Flash (player die)   SX2 2.raw
    bit 3=Invader die          SX3 3.raw
    bit 4=Extended play        SX4
    bit 5= AMP enable          SX5
    bit 6= NC (not wired)
    bit 7= NC (not wired)
    

    Port 5:
    bit 0=Fleet movement 1     SX6 4.raw
    bit 1=Fleet movement 2     SX7 5.raw
    bit 2=Fleet movement 3     SX8 6.raw
    bit 3=Fleet movement 4     SX9 7.raw
    bit 4=UFO Hit              SX10 8.raw
    bit 5= NC (Cocktail mode control ... to flip screen)
    bit 6= NC (not wired)
    bit 7= NC (not wired)

    Port 6:
    Watchdog ... read or write to reset*/

    uint8_t val = state->A;
    switch (port) {
    
        //Port 2: bit 0,1,2 Shift amount
        case 2:{
            state->shift_offset = val & 0x7; 
            break;
        }
        
        //Port 4: (discrete sounds)
        //bit 0-7 shift     data (LSB on 1st write, MSB on 2nd)
        case 4:{
            state->shift2 = state->shift1;
            state->shift1 = val;
            break;
        }

        default:  {
            printf("Some OUT ports still need to be implemented! \n");
            //UnimplementedIO(state);
            break;
        }
        
    }


    return;
}

int disassemble8080(unsigned char *buffer, int offset){
    
    
    //Get the code we want to disassemble (now code[0] will be the command in hex)
    //Next command (or byte) would be code[1] etc.
    unsigned char *code = &buffer[offset];
    //bytes of the operation (most of these wll be one, so set to one)
    int opbytes = 1;
    printf("%04x \n", offset);


    /*
    summary of instructions, from the 8080 data book:

    -Move, load and store:
    MOV(a,b): move register to register, or register to memory, or memory to register (b to a)
    MVI: move imediate register
    LXI C: Load immediate register pair (C and D, or i and next letter on alhpabet)
    STAX: Store A indirect
    LDAX: Load A indirect 
    STA: Store A direct
    LDA: Load A direct
    SHLD: Store H & L direct
    LHLD: Load H & L direct
    XCHG: Excange H & L, D & E Registers

    -Stack ops:
    PUSH B: Push register pair B & C on stack 
    PUSH D or H: similart as B & C with D & E or H & L
    PUSH PSW: Push A and flags(?) on stack
    POP B: Pop register pair B & C off stack
    POP D or H: similar as B & C with D & E or H & L
    POP PSW: Pop A and flags(?) from stack
    XTHL: Exhange top stack, H & L
    SPHL: H & L to stack pointer
    LXI SP: Load immediate stack pointer
    INX SP: Increment stack pointer
    DCX SP: Decrement stack pointer

    -Jump:
    JMP: Jump unconditional
    JC: Jump on carry
    JNC: Jump on no carry
    JZ: Jump on zero
    JNZ: Jump on no zero
    JP: Jump on positive
    JM: Jump on minus
    JPE: Jump on parity even
    JPO: Jump on odd parity
    PCHL: H & L to program counter

    -Call:
    CALL: Call unconditional
    CC: Call on carry
    CNC: Call on no carry
    CZ: Call on zero
    CNZ: Call on nonzero
    CP: Call on positive
    CM: Call on negative
    CPE: Call on even parity
    CPO: Call on odd parity

    -Return:
    RET: Return
    RC: Return on carry
    RNC: Return on no carry
    RZ: Return on zero
    RNZ: Return on nonzero
    RP: Return on positive
    RM: Return on negative
    RPE: Return on even parity
    RPO: Return on odd parity

    -Restart:
    RST: Restart

    -Increment and decrement:
    INR r: Increment register
    DCR r: Decrement register
    INR m and DCR m: same but for memory
    INX B: Increment B & C registers (INX D for D & E and INX H for H & L)
    DCX B: Decrement B & C registers (DCX D for D & E and DCX H for H & L)

    -Add:
    ADD r (or M): Add register (or memory) to A
    ADC (or M): Add register (or memory) to A with carry 
    ADI: Add immediate to A
    ACI: Add immediate to A with carry
    DAD B: Add B & C to H & L (DAD D for D & E and DAD H for H & L, both add to H & L)
    DAD SP: Add stack pointer to H & L
    
    -Subtract:
    SUB r (or M): Subtract register (or memory) from A
    SBB r (or M): Subtract register (or memory) from A with borrow(?)
    SUI: Subtract immediate from A
    SBI: Subtract immediate from A with borrow

    -Logical:
    ANA r (or M): And register (or memory) with A
    XRA r (or M): Exclusive or register (or memory) with A
    ORA r (or M): Or register (or memory) with A
    CMP r (or M): Compare register (or memory) with A
    ANI: And immediate with A
    XRI: Exclusive or immediate with A
    ORI: Or immediate with A
    CPI: Compare immediate with A

    -Rotate:
    RLC: Rotate A left
    RRC: Rotate A right
    RAL: Rotate A left through carrry
    RAR: Rotate A right through carry

    Specials:
    CMA: Complement A
    STC: Set carry
    CMC: Complement carry
    DAA: Decimal adjust A

    -Input/output:
    IN: Input
    OUT: Output

    -Control:
    EI: Enable interrupts
    DI: Disable interrupts
    NOP: No-operation
    HLT: Halt

    */

    switch(*code)
    {
        case 0x00:  printf("NOP"); break;
        case 0x01:  opbytes = 3; printf("LXI B,#$%02x%02x", code[2], code[1]);  break;
        case 0x02:  printf("STAX   B"); break;    
        case 0x03:  printf("INX    B"); break;    
        case 0x04:  printf("INR    B"); break;    
        case 0x05:  printf("DCR    B"); break;
        case 0x06:  opbytes = 2; printf("MVI B,#$%02x", code[1]);  break;
        case 0x07:  printf("RLC     ");  break;
        case 0x08:  printf("-       ");  break;
        case 0x09:  printf("DAD    B");  break;
        case 0x0a:  printf("LDAX   B");  break;
        case 0x0b:  printf("DCX    B");  break;
        case 0x0c:  printf("INR    C");  break;
        case 0x0d:  printf("DCR    C");  break;
        case 0x0e:  opbytes = 2; printf("MVI C,#$%02x", code[1]);  break;
        case 0x0f:  printf("RRC     ");  break;
        case 0x10:  printf("-       ");  break;
        case 0x11:  opbytes = 3; printf("LXI D,#$%02x%02x", code[2], code[1]);  break;
        case 0x12:  printf("STAX    D");  break;
        case 0x13:  printf("INX     D");  break;
        case 0x14:  printf("INR     D");  break;
        case 0x15:  printf("DCR     D");  break;
        case 0x16:  opbytes = 2; printf("MVI D,#$%02x", code[1]);  break;
        case 0x17:  printf("RAL      ");  break;
        case 0x18:  printf("-        ");  break;
        case 0x19:  printf("DAD      D");  break;
        case 0x1a:  printf("LDAX     D");  break;
        case 0x1b:  printf("DCX      D");  break;
        case 0x1c:  printf("INR      E");  break;
        case 0x1d:  printf("DCR      E");  break;
        case 0x1e:  opbytes = 2; printf("MVI E,#$%02x", code[1]);  break;
        case 0x1f:  printf("RAR ");  break;   
        case 0x20:  printf("-");  break;
        case 0x21:  opbytes = 3; printf("LXI H,#$%02x%02x", code[2], code[1]);
        case 0x22:  opbytes = 3; printf("SHLD adr,#$%02x%02x", code[2], code[1]);
        case 0x23:  printf("INX      H");  break;
        case 0x24:  printf("INR      H");  break;
        case 0x25:  printf("DCR      H");  break;
        case 0x26:  opbytes = 2; printf("MVI H,#$%02x", code[1]);  break;
        case 0x27:  printf("DAA      ");  break;
        case 0x28:  printf("-        ");  break;
        case 0x29:  printf("DAD      H");  break;
        case 0x2a:  opbytes = 3; printf("LHLD adr,#$%02x%02x", code[2], code[1]);
        case 0x2b:  printf("DCX      H");  break;
        case 0x2c:  printf("INR      L");  break;
        case 0x2d:  printf("DCR      L");  break;
        case 0x2e:  opbytes = 2; printf("MVI L,#$%02x", code[1]);  break;
        case 0x2f:  printf("CMA      ");  break;  
        case 0x30:  printf("-      ");  break;
        case 0x31:  opbytes = 3; printf("LXI SP,#$%02x%02x", code[2], code[1]);
        case 0x32:  opbytes = 3; printf("STA adr,#$%02x%02x", code[2], code[1]);
        case 0x33:  printf("INX SP   ");  break;
        case 0x34:  printf("INR M    ");  break;
        case 0x35:  printf("DCR M    ");  break;
        case 0x36:  opbytes = 2; printf("MVI M #$%02x", code[1]);   break;
        case 0x37:  printf("STC      ");  break;
        case 0x38:  printf("-        ");  break;
        case 0x39:  printf("DAD SP   ");  break;
        case 0x3a:  opbytes = 3; printf("LDA adr,#$%02x%02x", code[2], code[1]);
        case 0x3b:  printf("DCX SP");  break;
        case 0x3c:  printf("INR      A");  break;
        case 0x3d:  printf("DCR      A");  break;
        case 0x3e: opbytes = 2; printf("MVI A #$%02x", code[1]);   break;
        case 0x3f:  printf("CMC       ");  break;
        case 0x40:  printf("MOV    B,B");  break;
        case 0x41:  printf("MOV    B,C");  break;
        case 0x42:  printf("MOV    B,D");  break;
        case 0x43:  printf("MOV    B,E");  break;
        case 0x44:  printf("MOV    B,H");  break;
        case 0x45:  printf("MOV    B,L");  break;
        case 0x46:  printf("MOV    B,M");  break;
        case 0x47:  printf("MOV    B,A");  break;
        case 0x48:  printf("MOV    C,B");  break;
        case 0x49:  printf("MOV    C,C");  break;
        case 0x4a:  printf("MOV    C,D");  break;
        case 0x4b:  printf("MOV    C,E");  break;
        case 0x4c:  printf("MOV    C,H");  break;
        case 0x4d:  printf("MOV    C,L");  break;
        case 0x4e:  printf("MOV    C,M");  break;
        case 0x4f:  printf("MOV    C,A");  break;
        case 0x50:  printf("MOV    D,B");  break;
        case 0x51:  printf("MOV    D,C");  break;
        case 0x52:  printf("MOV    D,D");  break;
        case 0x53:  printf("MOV    D,E");  break;
        case 0x54:  printf("MOV    D,H");  break;
        case 0x55:  printf("MOV    D,L");  break;
        case 0x56:  printf("MOV    D,M");  break;
        case 0x57:  printf("MOV    D,A");  break;
        case 0x58:  printf("MOV    E,B");  break;
        case 0x59:  printf("MOV    E,C");  break;
        case 0x5a:  printf("MOV    E,D");  break;
        case 0x5b:  printf("MOV    E,E");  break;
        case 0x5c:  printf("MOV    E,H");  break;
        case 0x5d:  printf("MOV    E,L");  break;
        case 0x5e:  printf("MOV    E,M");  break;
        case 0x5f:  printf("MOV    E,A");  break;
        case 0x60:  printf("MOV    H,B");  break;
        case 0x61:  printf("MOV    H,C");  break;
        case 0x62:  printf("MOV    H,D");  break;
        case 0x63:  printf("MOV    H,E");  break;
        case 0x64:  printf("MOV    H,H");  break;
        case 0x65:  printf("MOV    H,L");  break;
        case 0x66:  printf("MOV    H,M");  break;
        case 0x67:  printf("MOV    H,A");  break;
        case 0x68:  printf("MOV    L,B");  break;
        case 0x69:  printf("MOV    L,C");  break;
        case 0x6a:  printf("MOV    L,D");  break;
        case 0x6b:  printf("MOV    L,E");  break;
        case 0x6c:  printf("MOV    L,H");  break;
        case 0x6d:  printf("MOV    L,L");  break;
        case 0x6e:  printf("MOV    L,M");  break;
        case 0x6f:  printf("MOV    L,A");  break;
        case 0x70:  printf("MOV    M,B");  break;
        case 0x71:  printf("MOV    M,C");  break;
        case 0x72:  printf("MOV    M,D");  break;
        case 0x73:  printf("MOV    M,E");  break;
        case 0x74:  printf("MOV    M,H");  break;
        case 0x75:  printf("MOV    M,L");  break;
        case 0x76:  printf("HLT       ");  break;
        case 0x77:  printf("MOV    M,A");  break;
        case 0x78:  printf("MOV    A,B");  break;
        case 0x79:  printf("MOV    A,C");  break;
        case 0x7a:  printf("MOV    A,D");  break;
        case 0x7b:  printf("MOV    A,E");  break;
        case 0x7c:  printf("MOV    A,H");  break;
        case 0x7d:  printf("MOV    A,L");  break;
        case 0x7e:  printf("MOV    A,M");  break;
        case 0x7f:  printf("MOV    A,A");  break;
        case 0x80:  printf("ADD      B");  break;
        case 0x81:  printf("ADD      C");  break;
        case 0x82:  printf("ADD      D");  break;
        case 0x83:  printf("ADD      E");  break;
        case 0x84:  printf("ADD      H");  break;
        case 0x85:  printf("ADD      L");  break;
        case 0x86:  printf("ADD      M");  break;
        case 0x87:  printf("ADD      A");  break;
        case 0x88:  printf("ADC      B");  break;
        case 0x89:  printf("ADC      C");  break;
        case 0x8a:  printf("ADC      D");  break;
        case 0x8b:  printf("ADC      E");  break;
        case 0x8c:  printf("ADC      H");  break;
        case 0x8d:  printf("ADC      L");  break;
        case 0x8e:  printf("ADC      M");  break;
        case 0x8f:  printf("ADC      A");  break;
        case 0x90:  printf("SUB      B");  break;
        case 0x91:  printf("SUB      C");  break;
        case 0x92:  printf("SUB      D");  break;
        case 0x93:  printf("SUB      E");  break;
        case 0x94:  printf("SUB      H");  break;
        case 0x95:  printf("SUB      L");  break;
        case 0x96:  printf("SUB      M");  break;
        case 0x97:  printf("SUB      A");  break;
        case 0x98:  printf("SBB      B");  break;
        case 0x99:  printf("SBB      C");  break;
        case 0x9a:  printf("SBB      D");  break;
        case 0x9b:  printf("SBB      E");  break;
        case 0x9c:  printf("SBB      H");  break;
        case 0x9d:  printf("SBB      L");  break;
        case 0x9e:  printf("SBB      M");  break;
        case 0x9f:  printf("SBB      A");  break;
        case 0xa0:  printf("ANA      B");  break;
        case 0xa1:  printf("ANA      C");  break;
        case 0xa2:  printf("ANA      D");  break;
        case 0xa3:  printf("ANA      E");  break;
        case 0xa4:  printf("ANA      H");  break;
        case 0xa5:  printf("ANA      L");  break;
        case 0xa6:  printf("ANA      M");  break;
        case 0xa7:  printf("ANA      A");  break;
        case 0xa8:  printf("XRA      B");  break;
        case 0xa9:  printf("XRA      C");  break;
        case 0xaa:  printf("XRA      D");  break;
        case 0xab:  printf("XRA      E");  break;
        case 0xac:  printf("XRA      H");  break;
        case 0xad:  printf("XRA      L");  break;
        case 0xae:  printf("XRA      M");  break;
        case 0xaf:  printf("XRA      A");  break;
        case 0xb0:  printf("ORA      B");  break;
        case 0xb1:  printf("ORA      C");  break;
        case 0xb2:  printf("ORA      D");  break;
        case 0xb3:  printf("ORA      E");  break;
        case 0xb4:  printf("ORA      H");  break;
        case 0xb5:  printf("ORA      L");  break;
        case 0xb6:  printf("ORA      M");  break;
        case 0xb7:  printf("ORA      A");  break;
        case 0xb8:  printf("CMP      B");  break;
        case 0xb9:  printf("CMP      C");  break;
        case 0xba:  printf("CMP      D");  break;
        case 0xbb:  printf("CMP      E");  break;
        case 0xbc:  printf("CMP      H");  break;
        case 0xbd:  printf("CMP      L");  break;
        case 0xbe:  printf("CMP      M");  break;
        case 0xbf:  printf("CMP      A");  break;
        case 0xc0:  printf("RNZ       ");  break;
        case 0xc1:  printf("POP      B");  break;
        case 0xc2:  opbytes = 3; printf("JNZ #$%02x%02x", code[2], code[1]);  break;
        case 0xc3:  opbytes = 3; printf("JMP #$%02x%02x", code[2], code[1]);  break;
        case 0xc4:  opbytes = 3; printf("CNZ #$%02x%02x", code[2], code[1]);  break;
        case 0xc5:  printf("PUSH     B");  break;
        case 0xc6:  opbytes = 2; printf("ADI ,#$02x", code[1]);  break;
        case 0xc7:  printf("RST      0");  break;
        case 0xc8:  printf("RZ        ");  break;
        case 0xc9:  printf("RET       ");  break;
        case 0xca:  opbytes = 3; printf("JZ #$%02x%02x", code[2], code[1]);  break;
        case 0xcb:  printf("-         ");  break;
        case 0xcc:  opbytes = 3; printf("CZ #$%02x%02x", code[2], code[1]);  break;
        case 0xcd:  opbytes = 3; printf("CALL #$%02x%02x", code[2], code[1]);  break;
        case 0xce:  opbytes = 2; printf("ACI #$%02x", code[1]);  break;
        case 0xcf:  printf("RST 1     ");  break;
        case 0xd0:  printf("RNC       ");  break;
        case 0xd1:  printf("POP      D");  break;
        case 0xd2:  opbytes = 3; printf("JNC #$%02x%02x", code[2], code[1]);  break;
        case 0xd3:  opbytes = 2; printf("OUT #$%02x", code[1]);  break;
        case 0xd4:  opbytes = 3; printf("CNC #$%02x%02x", code[2], code[1]);  break;
        case 0xd5:  printf("PUSH     D");  break;
        case 0xd6:  opbytes = 2; printf("SUI #$%02x", code[1]);  break;
        case 0xd7:  printf("RST 2     ");  break;
        case 0xd8:  printf("RC        ");  break;
        case 0xd9:  printf("-         ");  break;
        case 0xda:  opbytes = 3; printf("JC #$%02x%02x", code[2], code[1]);  break;
        case 0xdb:  opbytes = 2; printf("IN #$%02x", code[1]);  break;
        case 0xdc:  opbytes = 3; printf("CC #$%02x%02x", code[2], code[1]);  break;
        case 0xdd:  printf("-         ");  break;
        case 0xde:  opbytes = 2; printf("SBI #$%02x", code[1]);  break;
        case 0xdf:  printf("RST 3     ");  break;
        case 0xe0:  printf("RPO       ");  break;
        case 0xe1:  printf("POP      H");  break;
        case 0xe2:  opbytes = 3; printf("JPO #$%02x%02x", code[2], code[1]);  break;
        case 0xe3:  printf("XTHL      ");  break;
        case 0xe4:  opbytes = 3; printf("CPO #$%02x%02x", code[2], code[1]);  break;
        case 0xe5:  printf("PUSH     H");  break;
        case 0xe6:  opbytes = 2; printf("ANI #$%02x", code[1]);  break;
        case 0xe7:  printf("RST 4     ");  break;
        case 0xe8:  printf("RPE       ");  break;
        case 0xe9:  printf("PCHL      ");  break;
        case 0xea:  opbytes = 3; printf("JPE #$%02x%02x", code[2], code[1]);  break;
        case 0xeb:  printf("XCHG      ");  break;
        case 0xec:  opbytes = 3; printf("CPE #$%02x%02x", code[2], code[1]);  break;
        case 0xed:  printf("-         ");  break;
        case 0xee:  opbytes = 2; printf("XRI #$%02x", code[1]);  break;
        case 0xef:  printf("RST 5     ");  break;
        case 0xf0:  printf("RP        ");  break;
        case 0xf1:  printf("POP PSW   ");  break;
        case 0xf2:  opbytes = 3; printf("JP #$%02x%02x", code[2], code[1]);  break;
        case 0xf3:  printf("DI        ");  break;
        case 0xf4:  opbytes = 3; printf("CP #$%02x%02x", code[2], code[1]);  break;
        case 0xf5:  printf("PUSH PSW  ");  break;
        case 0xf6:  opbytes = 2; printf("ORI #$%02x", code[1]);  break;
        case 0xf7:  printf("RST 6     ");  break;
        case 0xf8:  printf("RM        ");  break;
        case 0xf9:  printf("SPHL      ");  break;
        case 0xfa:  opbytes = 3; printf("JM #$%02x%02x", code[2], code[1]);  break;
        case 0xfb:  printf("EI");  break;
        case 0xfc:  opbytes = 3; printf("CM #$%02x%02x", code[2], code[1]);  break;
        case 0xfd:  printf("-         ");  break;
        case 0xfe:  opbytes = 2; printf("CPI #$%02x", code[1]);  break;
        case 0xff:  printf("RST 7     ");  break;
        default:
            break;
    }

    printf("\n");
    //Return the number of bytes the operation needed/read
    return opbytes;
}

//All opcodes are not implemented, if any of those are
//called, exit the program
void UnimplementedOp(State_8080 *state){
    printf("ERROR: Unimplemented opcode, stopping. \n");
    exit(1);
}

// Here we actually perform the operations
// Auxilliary carrys won't be implemented, as Space Invaders doesn't use them
// Part of math done with higher precision, as carry out needs to be captured
int Emulate8080op(State_8080 *state){
    unsigned char *opcode = &state->mem[state->pc];
    uint16_t offset;
    state->ticks+= opcode_cycles[*opcode];

    state->pc+=1;
    switch (*opcode)
    {
    case 0x00: break;  //NOP

    //little endian (store from smallest to biggest)
    case 0x01: { //LXI B (load immediate pair B and C)
        state->C = opcode[1];
        state->B = opcode[2];
        state->pc += 2;
        break;
    }

    case 0x02: { //STAX B
        offset = (state->B << 8) | state->C;
        state->mem[offset] = state->A;
        break;
    }

    case 0x03: { //INX B
        //add one to pair B & C (i.e. to C)
        state->C++;
        //Did we go back to zero?
        if (state->C==0) {
            state->B++;
        }
        break;
    }

    case 0x04: { //INR B (increment)
        uint16_t answer = (uint16_t) state->B + (uint16_t) 1;
        state->B = answer & 0xff;
        state->conds.z = ((answer & 0xff) == 0);
        state->conds.s = ((answer & 0x80) != 0);
        state->conds.cy = (answer > 0xff);
        state->conds.p = (((answer & 0xff) & 1) == 0);
        break;
    } 

    case 0x05: { //DCR B (decrement)
        uint16_t answer = (uint16_t) state->B - (uint16_t) 1;
        state->B = answer & 0xff;
        state->conds.z = ((answer & 0xff) == 0);
        state->conds.s = ((answer & 0x80) != 0);
        state->conds.cy = (answer > 0xff);
        state->conds.p = (((answer & 0xff) & 1) == 0);
        break;
    } 

    case 0x06: { //MVI B
        state->B = opcode[1];
        state->pc++;
        break;
    } 

    case 0x07: { //RLC (rotate A left)
        uint8_t acc = state->A;
        state->A = acc << 1; //I think this is okay
        state->conds.cy = (0x80 == (acc&0x80));
        break;
    }

    case 0x08: { //-
        break;
    }

    case 0x09: { //DAD B
        uint16_t BC = (state->B<<8) | (state->C);
        uint32_t answer = ((state->H<<8) | (state->L)) + BC;
        state->conds.cy = (answer > 0xffff0000);
        state->L = answer & 0xff;
        state->H = answer & 0xff00 >> 8;
        break;
    }

    case 0x0a: { //LDAX B
        offset = state->B<<8 | state->C;
        state->A = state->mem[offset];
        break;
    }

    case 0x0b: { //DCX B 
        state->C--;
        if (state->C == 0xff) {
            state->B--;
        }
        break;
    }

    case 0x0c: { //INR C
        uint16_t answer = (uint16_t) state->C + (uint16_t) 1;
        state->C = answer & 0xff;
        state->conds.z = ((answer & 0xff) == 0);
        state->conds.s = ((answer & 0x80) != 0);
        state->conds.cy = (answer > 0xff);
        state->conds.p = (((answer & 0xff) & 1) == 0);
        break;
    }

    case 0x0d: { //DCR C
        uint16_t answer = (uint16_t) state->C - (uint16_t) 1;
        state->C = answer & 0xff;
        state->conds.z = ((answer & 0xff) == 0);
        state->conds.s = ((answer & 0x80) != 0);
        state->conds.cy = (answer > 0xff);
        state->conds.p = (((answer & 0xff) & 1) == 0);
        break;
    }

    case 0x0e:  { //MVI C
        state->C = opcode[1];
        state->pc++;
        break;
    } 

    case 0x0f: { //RRC (rotate A right)
        uint8_t acc = state->A;
        state->A = acc >> 1; //I think this is okay
        state->conds.cy = (1 == (acc&1));
        break;
    }

    case 0x10: { //-
        break;
    }

    case 0x11: { //LXI D 
        state->E = opcode[1];
        state->D = opcode[2];
        state->pc += 2;
        break;
    }

    case 0x12: { //STAX D
        offset = (state->D << 8) | state->E;
        state->mem[offset] = state->A;
        break;
    }

    case 0x13: { //INX D
        state->E++;
        if (state->E==0) {
            state->D++;
        }
        break;
    }

    case 0x14: {  //INR D
        uint16_t answer = (uint16_t) state->D + (uint16_t) 1;
        state->D = answer & 0xff;
        state->conds.z = ((answer & 0xff) == 0);
        state->conds.s = ((answer & 0x80) != 0);
        state->conds.cy = (answer > 0xff);
        state->conds.p = (((answer & 0xff) & 1) == 0);
        break;
    }

    case 0x15: {  //DCR D
        uint16_t answer = (uint16_t) state->D - (uint16_t) 1;
        state->D = answer & 0xff;
        state->conds.z = ((answer & 0xff) == 0);
        state->conds.s = ((answer & 0x80) != 0);
        state->conds.cy = (answer > 0xff);
        state->conds.p = (((answer & 0xff) & 1) == 0);
        break;
    }

    case 0x16: { //MVI D
        state->D = opcode[1];
        state->pc++;
        break;
    } 

    case 0x18: { //-
        break;
    }

    case 0x19: { //DAD D
        uint16_t DE = (state->D<<8) | (state->E);
        uint32_t answer = ((state->H<<8) | (state->L)) + DE;
        state->conds.cy = (answer > 0xffff0000);
        state->L = answer & 0xff;
        state->H = answer & 0xff00 >> 8;
        break;
    }


    case 0x1a: { // LDAX D
        offset = state->D<<8 | state->E;
        state->A = state->mem[offset];
        break;
    }

    case 0x1b: { //DCX D
        state->E--;
        if (state->E == 0xff) {
            state->D--;
        }
        break;
    }

    case 0x1c: {  //INR E
        uint16_t answer = (uint16_t) state->E + (uint16_t) 1;
        state->E = answer & 0xff;
        state->conds.z = ((answer & 0xff) == 0);
        state->conds.s = ((answer & 0x80) != 0);
        state->conds.cy = (answer > 0xff);
        state->conds.p = (((answer & 0xff) & 1) == 0);
        break;
    }

    case 0x1d: {  //DCR E
        uint16_t answer = (uint16_t) state->E - (uint16_t) 1;
        state->E = answer & 0xff;
        state->conds.z = ((answer & 0xff) == 0);
        state->conds.s = ((answer & 0x80) != 0);
        state->conds.cy = (answer > 0xff);
        state->conds.p = (((answer & 0xff) & 1) == 0);
        break;
    }

    case 0x1e: { //MVI E
        state->E = opcode[1];
        state->pc++;
        break;
    } 

    case 0x1f: { //RAR
        uint8_t acc = state->A;
        state->conds.cy = (1 == (acc&1));
        state->A = (acc >> 1) + state->conds.cy << 7;
        break;
    }

    case 0x20: { //-
        break;
    }

    case 0x21: { //LXI H
        state->L = opcode[1];
        state->H = opcode[2];
        state->pc += 2;
        break;
    }

    case 0x22: { //SHLD
        offset = (opcode[2] << 8) | opcode[1];
        state->mem[offset] = state->L;
        state->mem[offset+1] = state->H;
        state->pc+=2;
        break;
    }

    case 0x23: { //INX H
        state->L++;
        if (state->L==0) {
            state->H++;
        }
        break;
    }

    case 0x24:{  //INR H
        uint16_t answer = (uint16_t) state->H + (uint16_t) 1;
        state->H = answer & 0xff;
        state->conds.z = ((answer & 0xff) == 0);
        state->conds.s = ((answer & 0x80) != 0);
        state->conds.cy = (answer > 0xff);
        state->conds.p = (((answer & 0xff) & 1) == 0);
        break;
    }

    case 0x25:{  //DCR H
        uint16_t answer = (uint16_t) state->H - (uint16_t) 1;
        state->H = answer & 0xff;
        state->conds.z = ((answer & 0xff) == 0);
        state->conds.s = ((answer & 0x80) != 0);
        state->conds.cy = (answer > 0xff);
        state->conds.p = (((answer & 0xff) & 1) == 0);
        break;
    }

    case 0x26: { //MVI H
        state->H = opcode[1];
        state->pc++;
        break;
    }

    case 0x28: { //-
        break;
    }

    case 0x29: { //DAD H
        uint16_t HL = (state->H<<8) | (state->L);
        uint32_t answer = HL + HL;
        state->conds.cy = (answer > 0xffff0000);
        state->L = answer & 0xff;
        state->H = answer & 0xff00 >> 8;
        break;
    }

    case 0x2a: { //LHLD
        offset = (opcode[2] << 8) | opcode[1];
        state->L = state->mem[offset];
        state->H = state->mem[offset+1];
        state->pc+=2;
        break;
    }

    case 0x2b: { //DCX H
        state->L--;
        if (state->L == 0xff) {
            state->H--;
        }
        break;
    }

    case 0x2c:{  //INR L
        uint16_t answer = (uint16_t) state->L + (uint16_t) 1;
        state->L = answer & 0xff;
        state->conds.z = ((answer & 0xff) == 0);
        state->conds.s = ((answer & 0x80) != 0);
        state->conds.cy = (answer > 0xff);
        state->conds.p = (((answer & 0xff) & 1) == 0);
        break;
    }

    case 0x2d:{  //DCR L
        uint16_t answer = (uint16_t) state->L - (uint16_t) 1;
        state->L = answer & 0xff;
        state->conds.z = ((answer & 0xff) == 0);
        state->conds.s = ((answer & 0x80) != 0);
        state->conds.cy = (answer > 0xff);
        state->conds.p = (((answer & 0xff) & 1) == 0);
        break;
    }

    case 0x2e: { //MVI L
        state->L = opcode[1];
        state->pc++;
        break;
    } 

    case 0x2f: { // CMA
        state->A = 0xff -state->A;
        break;
    }

    case 0x30: { //-
        break;
    }

    case 0x31: { //LXI SP
        state->sp = opcode[2] <<8 | opcode[1];
        state->pc += 2;
        break;
    }

    case 0x32: { //STA (store A direct)
        offset = (opcode[2] << 8) | opcode[1];
        state->mem[offset] = state->A;
        state->pc+=2;
        break;
    } 

    case 0x33: { //INX SP
        state->sp++;
        break;
    }

    case 0x34:{ //INR M
        offset = (state->H << 8) | state-> L;
        uint16_t answer = (uint16_t) state->mem[offset] + (uint16_t) 1;
        state->mem[offset] = answer & 0xff;
        state->conds.z = ((answer & 0xff) == 0);
        state->conds.s = ((answer & 0x80) != 0);
        state->conds.cy = (answer > 0xff);
        state->conds.p = (((answer & 0xff) & 1) == 0);
        break;
    }

    case 0x35:{ //DCR M
        offset = (state->H << 8) | state-> L;
        uint16_t answer = (uint16_t) state->mem[offset] - (uint16_t) 1;
        state->mem[offset] = answer & 0xff;
        state->conds.z = ((answer & 0xff) == 0);
        state->conds.s = ((answer & 0x80) != 0);
        state->conds.cy = (answer > 0xff);
        state->conds.p = (((answer & 0xff) & 1) == 0);
        break;
    }

    case 0x36: { //MVI M
        offset = (state->H << 8) | state-> L;
        state->mem[offset] = opcode[1];
        state->pc++;
        break;
    } 

    case 0x37: { //STC
        state->conds.cy = 1;
        break;
    }

    case 0x38: { //-
        break;
    }

    case 0x39: { //DAD SP
        //Not sure if this is correct
        //also not sure if sp+=2 needs to be added
        uint16_t sp = (state->mem[state->sp+1]<<8) | (state->mem[state->sp]);
        uint32_t answer = ((state->H<<8) | (state->L)) + sp;
        state->conds.cy = (answer > 0xffff0000);
        state->L = answer & 0xff;
        state->H = answer & 0xff00 >> 8;
        break;
    }    

    case 0x3a: { //LDA
        offset = (opcode[2] << 8) | opcode[1];
        state->A = state->mem[offset];
        state->pc+=2;
        break;
    }

    case 0x3b: { //DCX SP
        state->sp--;
        break;
    }

    case 0x3c:{  //INR A
        uint16_t answer = (uint16_t) state->A + (uint16_t) 1;
        state->A = answer & 0xff;
        state->conds.z = ((answer & 0xff) == 0);
        state->conds.s = ((answer & 0x80) != 0);
        state->conds.cy = (answer > 0xff);
        state->conds.p = (((answer & 0xff) & 1) == 0);
        break;
    }

    case 0x3d:{  //DCR A
        uint16_t answer = (uint16_t) state->A - (uint16_t) 1;
        state->A = answer & 0xff;
        state->conds.z = ((answer & 0xff) == 0);
        state->conds.s = ((answer & 0x80) != 0);
        state->conds.cy = (answer > 0xff);
        state->conds.p = (((answer & 0xff) & 1) == 0);
        break;
    }

    case 0x3e: { //MVI A
        state->A = opcode[1];
        state->pc++;
        break;
    } 

    case 0x3f: { //CMC
        state->conds.cy = state->conds.cy ^ 1;
        break;
    }

    case 0x40: state->B = state->B; break;
    case 0x41: state->B = state->C; break; // MOV B,C
    case 0x42: state->B = state->D; break; // MOV B,D
    case 0x43: state->B = state->E; break; // MOV B,E 
    case 0x44: state->B = state->H; break; // MOV B,H
    case 0x45: state->B = state->L; break;//MOV    B,L");  break;
    case 0x46: offset = (state->H<<8) | (state->L); state->B = state->mem[offset]; printf("MOV    B,M");  break;
    case 0x47: state->B = state->A; break;//MOV    B,A");  break;
    case 0x48: state->C = state->B; break;//MOV    C,B");  break;
    case 0x49: state->C = state->C; break;//"MOV    C,C");  break;
    case 0x4a: state->C = state->D; break;//MOV    C,D");  break;
    case 0x4b: state->C = state->E; break;//MOV    C,E");  break;
    case 0x4c: state->C = state->H; break;//MOV    C,H");  break;
    case 0x4d: state->C = state->L; break;//MOV    C,L");  break;
    case 0x4e: offset = (state->H<<8) | (state->L); state->C = state->mem[offset]; printf("MOV    C,M");  break;
    case 0x4f: state->C = state->A; break;//MOV    C,A");  break;
    case 0x50: state->D = state->B; break;// MOV    D,B");  break;
    case 0x51: state->D = state->C; break;//MOV    D,C");  break;
    case 0x52: state->D = state->D; break;//MOV    D,D");  break;
    case 0x53: state->D = state->E; break;//MOV    D,E");  break;
    case 0x54: state->D = state->H; break;//MOV    D,H");  break;
    case 0x55: state->D = state->L; break;//MOV    D,L");  break;
    case 0x56: offset = (state->H<<8) | (state->L); state->D = state->mem[offset]; printf("MOV    D,M");  break;
    case 0x57: state->D = state->A; break;//MOV    D,A"
    case 0x58: state->E = state->B; break;//MOV    E,B"  
    case 0x59: state->E = state->C; break;//MOV    E,C"  
    case 0x5a: state->E = state->D; break;//MOV    E,D"  
    case 0x5b: state->E = state->E; break;//MOV    E,E
    case 0x5c: state->E = state->H; break;//"MOV    E,H  
    case 0x5d: state->E = state->L; break;//MOV    E,L
    case 0x5e: offset = (state->H<<8) | (state->L); state->E = state->mem[offset]; printf("MOV    E,M");  break;
    case 0x5f: state->E = state->A; break;//"MOV    E,A
    case 0x60: state->H = state->B; break;
    case 0x61: state->H = state->C; break;
    case 0x62: state->H = state->D; break;
    case 0x63: state->H = state->E; break;
    case 0x64: state->H = state->H; break;
    case 0x65: state->H = state->L; break;
    case 0x66: offset = (state->H<<8) | (state->L); state->H = state->mem[offset]; printf("MOV    H,M");  break;
    case 0x67: state->H = state->A; break;
    case 0x68: state->L = state->B; break;
    case 0x69: state->L = state->C;   break;
    case 0x6a: state->L = state->D;   break;
    case 0x6b: state->L = state->E; break;
    case 0x6c: state->L = state->H;  break;
    case 0x6d: state->L = state->L; break;
    case 0x6e: offset = (state->H<<8) | (state->L); state->L = state->mem[offset]; printf("MOV    L,M");  break;
    case 0x6f: state->L = state->A;  break;
    case 0x70: { //MOV M,B
        offset = (state->H<<8) | (state->L);
        state->mem[offset] = state->B;
        break;
    }
    case 0x71: offset = (state->H<<8) | (state->L); state->mem[offset] = state->C; break;
    case 0x72: offset = (state->H<<8) | (state->L); state->mem[offset] = state->D; break;
    case 0x73: offset = (state->H<<8) | (state->L); state->mem[offset] = state->E; break;
    case 0x74: offset = (state->H<<8) | (state->L); state->mem[offset] = state->H; break;
    case 0x75: offset = (state->H<<8) | (state->L); state->mem[offset] = state->L; break;

    case 0x77: offset = (state->H<<8) | (state->L); state->mem[offset] = state->A; break;

    case 0x78: state->A= state->B;  break;
    case 0x79: state->A= state->C;  break;
    case 0x7a: state->A= state->D;  break;
    case 0x7b: state->A= state->E;  break;
    case 0x7c: state->A= state->H;  break;
    case 0x7d: state->A= state->L;  break;
    case 0x7e: offset = (state->H<<8) | (state->L); state->A = state->mem[offset]; break;
    case 0x7f: state->A= state->A;  break;

    case 0x80:  //ADD B (to A), flags Z, S, P, CY, AC
        {//carry out
        uint16_t answer = (uint16_t) state->A + (uint16_t) state->B;
        state->A = answer & 0xff;
        //Z flag: set flag to 0 if result is 0
        //0xff has all bits in binary form as 1
        //if answer is 0 it's each binary bit is 0.
        //Therefore & (bitwise and) operator gives 0 (10-base form)
        // if answer is 0.
        if ((answer & 0xff) == 0)
            state->conds.z = 1;
        else
            state->conds.z = 0;  

        //Sign flag: check bit 7
        //works with similar logic as the z flag check
        if (answer & 0x80)
            state->conds.s = 1;
        else
            state->conds.s = 0;        

        //Carry flag: check if larger than what 8bit value can hold
        if (answer > 0xff)
            state->conds.cy = 1;
        else
            state->conds.cy = 0;

        //TODO CHECK THAT THIS ACTUALLY WORKS
        state->conds.p = (((answer & 0xff) & 1) == 0); 
        break; 
        }

    case 0x81:  {//ADD C (to A), condensed version of 0x80
        uint16_t answer = (uint16_t) state->A + (uint16_t) state->C;
        state->A = answer & 0xff;
        state->conds.z = ((answer & 0xff) == 0);
        state->conds.s = ((answer & 0x80) != 0);
        state->conds.cy = (answer > 0xff);
        state->conds.p = (((answer & 0xff) & 1) == 0);
        break;
        }
    case 0x82:  {//ADD D (to A), condensed version of 0x80
        uint16_t answer = (uint16_t) state->A + (uint16_t) state->D;
        state->A = answer & 0xff;
        state->conds.z = ((answer & 0xff) == 0);
        state->conds.s = ((answer & 0x80) != 0);
        state->conds.cy = (answer > 0xff);
        state->conds.p = (((answer & 0xff) & 1) == 0);
        break;
        }
    case 0x83:  {//ADD E (to A), condensed version of 0x80
        uint16_t answer = (uint16_t) state->A + (uint16_t) state->E;
        state->A = answer & 0xff;
        state->conds.z = ((answer & 0xff) == 0);
        state->conds.s = ((answer & 0x80) != 0);
        state->conds.cy = (answer > 0xff);
        state->conds.p = (((answer & 0xff) & 1) == 0);
        break;
        }
    case 0x84:  {//ADD H (to A), condensed version of 0x80
        uint16_t answer = (uint16_t) state->A + (uint16_t) state->H;
        state->A = answer & 0xff;
        state->conds.z = ((answer & 0xff) == 0);
        state->conds.s = ((answer & 0x80) != 0);
        state->conds.cy = (answer > 0xff);
        state->conds.p = (((answer & 0xff) & 1) == 0);
        break;
        }
    case 0x85:  {//ADD L (to A), condensed version of 0x80
        uint16_t answer = (uint16_t) state->A + (uint16_t) state->L;
        state->A = answer & 0xff;
        state->conds.z = ((answer & 0xff) == 0);
        state->conds.s = ((answer & 0x80) != 0);
        state->conds.cy = (answer > 0xff);
        state->conds.p = (((answer & 0xff) & 1) == 0);
        break;
        
        }


    case 0x86:  //ADD M (memory/byte stored in HL register pair)
        {
        //find the offset (location pointed by HL)
        offset = (state->H<<8) | (state->L);
        uint16_t answer = (uint16_t) state->A + state->mem[offset];
        state->A = answer & 0xff;
        state->conds.z = ((answer & 0xff) == 0);
        state->conds.s = ((answer & 0x80) != 0);
        state->conds.cy = (answer > 0xff);
        state->conds.p = (((answer & 0xff) & 1) == 0);
        break;
        }

    case 0x87:  {//ADD A
        uint16_t answer = (uint16_t) state->A + (uint16_t) state->A;
        state->A = answer & 0xff;
        state->conds.z = ((answer & 0xff) == 0);
        state->conds.s = ((answer & 0x80) != 0);
        state->conds.cy = (answer > 0xff);
        state->conds.p = (((answer & 0xff) & 1) == 0);
        break;
        }    

    case 0x88: {//ADC B
        uint16_t answer = (uint16_t) state->B + (uint16_t) state->A + state->conds.cy;
        state->A = answer & 0xff;
        state->conds.z = ((answer & 0xff) == 0);
        state->conds.s = ((answer & 0x80) != 0);
        state->conds.cy = (answer > 0xff);
        state->conds.p = (((answer & 0xff) & 1) == 0);
        break;
    }  
    case 0x89: {//ADC C
        uint16_t answer = (uint16_t) state->C + (uint16_t) state->A + state->conds.cy;
        state->A = answer & 0xff;
        state->conds.z = ((answer & 0xff) == 0);
        state->conds.s = ((answer & 0x80) != 0);
        state->conds.cy = (answer > 0xff);
        state->conds.p = (((answer & 0xff) & 1) == 0);
        break;
    }  
    case 0x8a: {//ADC D
        uint16_t answer = (uint16_t) state->D + (uint16_t) state->A + state->conds.cy;
        state->A = answer & 0xff;
        state->conds.z = ((answer & 0xff) == 0);
        state->conds.s = ((answer & 0x80) != 0);
        state->conds.cy = (answer > 0xff);
        state->conds.p = (((answer & 0xff) & 1) == 0);
        break;
    }  
    case 0x8b: {//ADC E
        uint16_t answer = (uint16_t) state->E + (uint16_t) state->A + state->conds.cy;
        state->A = answer & 0xff;
        state->conds.z = ((answer & 0xff) == 0);
        state->conds.s = ((answer & 0x80) != 0);
        state->conds.cy = (answer > 0xff);
        state->conds.p = (((answer & 0xff) & 1) == 0);
        break;
    }  
    case 0x8c: {//ADC H
        uint16_t answer = (uint16_t) state->H + (uint16_t) state->A + state->conds.cy;
        state->A = answer & 0xff;
        state->conds.z = ((answer & 0xff) == 0);
        state->conds.s = ((answer & 0x80) != 0);
        state->conds.cy = (answer > 0xff);
        state->conds.p = (((answer & 0xff) & 1) == 0);
        break;
    }  
    case 0x8d: {//ADC L
        uint16_t answer = (uint16_t) state->L + (uint16_t) state->A + state->conds.cy;
        state->A = answer & 0xff;
        state->conds.z = ((answer & 0xff) == 0);
        state->conds.s = ((answer & 0x80) != 0);
        state->conds.cy = (answer > 0xff);
        state->conds.p = (((answer & 0xff) & 1) == 0);
        break;
    }  
    case 0x8e: {//ADC A
        offset = state->H << 8 | state->L;
        uint16_t answer = (uint16_t) state->mem[offset] + (uint16_t) state->A + state->conds.cy;
        state->A = answer & 0xff;
        state->conds.z = ((answer & 0xff) == 0);
        state->conds.s = ((answer & 0x80) != 0);
        state->conds.cy = (answer > 0xff);
        state->conds.p = (((answer & 0xff) & 1) == 0);
        break;
    }  
    case 0x8f: {//ADC A
        uint16_t answer = (uint16_t) state->A + (uint16_t) state->A + state->conds.cy;
        state->A = answer & 0xff;
        state->conds.z = ((answer & 0xff) == 0);
        state->conds.s = ((answer & 0x80) != 0);
        state->conds.cy = (answer > 0xff);
        state->conds.p = (((answer & 0xff) & 1) == 0);
        break;
    }    
    
    case 0x90: { //SUB B
        uint16_t answer = state->A - state->B;
        state->conds.z = ((answer & 0xff) == 0);
        state->conds.s = ((answer & 0x80) != 0);
        state->conds.cy = (answer > 0xff);
        state->conds.p = (((answer & 0xff) & 1) == 0);
        state->A = answer & 0xff;
        break;
    }
    case 0x91: { //SUB C
        uint16_t answer = state->A - state->C;
        state->conds.z = ((answer & 0xff) == 0);
        state->conds.s = ((answer & 0x80) != 0);
        state->conds.cy = (answer > 0xff);
        state->conds.p = (((answer & 0xff) & 1) == 0);
        state->A = answer & 0xff;
        break;
    }
    case 0x92: { //SUB D
        uint16_t answer = state->A - state->D;
        state->conds.z = ((answer & 0xff) == 0);
        state->conds.s = ((answer & 0x80) != 0);
        state->conds.cy = (answer > 0xff);
        state->conds.p = (((answer & 0xff) & 1) == 0);
        state->A = answer & 0xff;
        break;
    }
    case 0x93: { //SUB E
        uint16_t answer = state->A - state->E;
        state->conds.z = ((answer & 0xff) == 0);
        state->conds.s = ((answer & 0x80) != 0);
        state->conds.cy = (answer > 0xff);
        state->conds.p = (((answer & 0xff) & 1) == 0);
        state->A = answer & 0xff;
        break;
    }
    case 0x94: { //SUB H
        uint16_t answer = state->A - state->H;
        state->conds.z = ((answer & 0xff) == 0);
        state->conds.s = ((answer & 0x80) != 0);
        state->conds.cy = (answer > 0xff);
        state->conds.p = (((answer & 0xff) & 1) == 0);
        state->A = answer & 0xff;
        break;
    }
    case 0x95: { //SUB L
        uint16_t answer = state->A - state->L;
        state->conds.z = ((answer & 0xff) == 0);
        state->conds.s = ((answer & 0x80) != 0);
        state->conds.cy = (answer > 0xff);
        state->conds.p = (((answer & 0xff) & 1) == 0);
        state->A = answer & 0xff;
        break;
    }
    case 0x96: { //SUB M
        offset = (state->H << 8 | state->L);
        uint16_t answer = state->A - state->mem[offset];
        state->conds.z = ((answer & 0xff) == 0);
        state->conds.s = ((answer & 0x80) != 0);
        state->conds.cy = (answer > 0xff);
        state->conds.p = (((answer & 0xff) & 1) == 0);
        state->A = answer & 0xff;
        break;
    }
    case 0x97: { //SUB A
        uint16_t answer = state->A - state->A;
        state->conds.z = ((answer & 0xff) == 0);
        state->conds.s = ((answer & 0x80) != 0);
        state->conds.cy = (answer > 0xff);
        state->conds.p = (((answer & 0xff) & 1) == 0);
        state->A = answer & 0xff;
        break;
    }

    case 0x98: { //SBB B
        uint16_t answer = state->A - state->B - state->conds.cy;
        state->conds.z = ((answer & 0xff) == 0);
        state->conds.s = ((answer & 0x80) != 0);
        state->conds.cy = (answer > 0xff);
        state->conds.p = (((answer & 0xff) & 1) == 0);
        state->A = answer & 0xff;
        break;
    }
    case 0x99: { //SBB C
        uint16_t answer = state->A - state->C - state->conds.cy;
        state->conds.z = ((answer & 0xff) == 0);
        state->conds.s = ((answer & 0x80) != 0);
        state->conds.cy = (answer > 0xff);
        state->conds.p = (((answer & 0xff) & 1) == 0);
        state->A = answer & 0xff;
        break;
    }
    case 0x9a: { //SBB D
        uint16_t answer = state->A - state->D - state->conds.cy;
        state->conds.z = ((answer & 0xff) == 0);
        state->conds.s = ((answer & 0x80) != 0);
        state->conds.cy = (answer > 0xff);
        state->conds.p = (((answer & 0xff) & 1) == 0);
        state->A = answer & 0xff;
        break;
    }
    case 0x9b: { //SBB E
        uint16_t answer = state->A - state->E - state->conds.cy;
        state->conds.z = ((answer & 0xff) == 0);
        state->conds.s = ((answer & 0x80) != 0);
        state->conds.cy = (answer > 0xff);
        state->conds.p = (((answer & 0xff) & 1) == 0);
        state->A = answer & 0xff;
        break;
    }
    case 0x9c:  { //SBB H
        uint16_t answer = state->A - state->H - state->conds.cy;
        state->conds.z = ((answer & 0xff) == 0);
        state->conds.s = ((answer & 0x80) != 0);
        state->conds.cy = (answer > 0xff);
        state->conds.p = (((answer & 0xff) & 1) == 0);
        state->A = answer & 0xff;
        break;
    }
    case 0x9d: { //SBB L
        uint16_t answer = state->A - state->L - state->conds.cy;
        state->conds.z = ((answer & 0xff) == 0);
        state->conds.s = ((answer & 0x80) != 0);
        state->conds.cy = (answer > 0xff);
        state->conds.p = (((answer & 0xff) & 1) == 0);
        state->A = answer & 0xff;
        break;
    }
    case 0x9e: { //SBB M
        offset = state->H << 8 | state->L;
        uint16_t answer = state->A - state->mem[offset] - state->conds.cy;
        state->conds.z = ((answer & 0xff) == 0);
        state->conds.s = ((answer & 0x80) != 0);
        state->conds.cy = (answer > 0xff);
        state->conds.p = (((answer & 0xff) & 1) == 0);
        state->A = answer & 0xff;
        break;
    }
    case 0x9f: { //SBB A
        uint16_t answer = state->A - state->A - state->conds.cy;
        state->conds.z = ((answer & 0xff) == 0);
        state->conds.s = ((answer & 0x80) != 0);
        state->conds.cy = (answer > 0xff);
        state->conds.p = (((answer & 0xff) & 1) == 0);
        state->A = answer & 0xff;
        break;
    }

    case 0xa0:{ //ANA B
        state->A = state->B & state->A;
        state->conds.z = ((state->A & 0xff) == 0);
        state->conds.s = ((state->A & 0x80) != 0);
        state->conds.p = (((state->A & 0xff) & 1) == 0);
        state->conds.cy = 0;
        break;
    }  
    case 0xa1: { //ANA C
        state->A = state->C & state->A;
        state->conds.z = ((state->A & 0xff) == 0);
        state->conds.s = ((state->A & 0x80) != 0);
        state->conds.p = (((state->A & 0xff) & 1) == 0);
        state->conds.cy = 0;
        break;
    }  
    case 0xa2:{ //ANA D
        state->A = state->D & state->A;
        state->conds.z = ((state->A & 0xff) == 0);
        state->conds.s = ((state->A & 0x80) != 0);
        state->conds.p = (((state->A & 0xff) & 1) == 0);
        state->conds.cy = 0;
        break;
    }  
    case 0xa3:{ //ANA E
        state->A = state->E & state->A;
        state->conds.z = ((state->A & 0xff) == 0);
        state->conds.s = ((state->A & 0x80) != 0);
        state->conds.p = (((state->A & 0xff) & 1) == 0);
        state->conds.cy = 0;
        break;
    }  
    case 0xa4: { //ANA H
        state->A = state->H & state->A;
        state->conds.z = ((state->A & 0xff) == 0);
        state->conds.s = ((state->A & 0x80) != 0);
        state->conds.p = (((state->A & 0xff) & 1) == 0);
        state->conds.cy = 0;
        break;
    }  
    case 0xa5: { //ANA L
        state->A = state->L & state->A;
        state->conds.z = ((state->A & 0xff) == 0);
        state->conds.s = ((state->A & 0x80) != 0);
        state->conds.p = (((state->A & 0xff) & 1) == 0);
        state->conds.cy = 0;
        break;
    }  
    case 0xa6:{ //ANA M
        offset = (state->H<<8) | (state->L);
        state->A = state->mem[offset];
        state->conds.z = ((state->A & 0xff) == 0);
        state->conds.s = ((state->A & 0x80) != 0);
        state->conds.p = (((state->A & 0xff) & 1) == 0);
        state->conds.cy = 0;
        break;
    }  
    case 0xa7: { //ANA A
        state->A = state->A & state->A;
        state->conds.z = ((state->A & 0xff) == 0);
        state->conds.s = ((state->A & 0x80) != 0);
        state->conds.p = (((state->A & 0xff) & 1) == 0);
        state->conds.cy = 0;
        break;
    }  
    
    case 0xa8: {//XRA B (exclusive or)
        state->A = state->A ^ state->B;
        state->conds.z = ((state->A & 0xff) == 0);
        state->conds.s = ((state->A & 0x80) != 0);
        state->conds.p = (((state->A & 0xff) & 1) == 0);
        state->conds.cy = 0;
        break;
    }
     
    case 0xa9: {//XRA C 
        state->A = state->A ^ state->C;
        state->conds.z = ((state->A & 0xff) == 0);
        state->conds.s = ((state->A & 0x80) != 0);
        state->conds.p = (((state->A & 0xff) & 1) == 0);
        state->conds.cy = 0;
        break;
    }
    case 0xaa: {//XRA D
        state->A = state->A ^ state->D;
        state->conds.z = ((state->A & 0xff) == 0);
        state->conds.s = ((state->A & 0x80) != 0);
        state->conds.p = (((state->A & 0xff) & 1) == 0);
        state->conds.cy = 0;
        break;
    }
    case 0xab: {//XRA E
        state->A = state->A ^ state->E;
        state->conds.z = ((state->A & 0xff) == 0);
        state->conds.s = ((state->A & 0x80) != 0);
        state->conds.p = (((state->A & 0xff) & 1) == 0);
        state->conds.cy = 0;
        break;
    }
    case 0xac: {//XRA H
        state->A = state->A ^ state->H;
        state->conds.z = ((state->A & 0xff) == 0);
        state->conds.s = ((state->A & 0x80) != 0);
        state->conds.p = (((state->A & 0xff) & 1) == 0);
        state->conds.cy = 0;
        break;
    }
    case 0xad: {//XRA L
        state->A = state->A ^ state->L;
        state->conds.z = ((state->A & 0xff) == 0);
        state->conds.s = ((state->A & 0x80) != 0);
        state->conds.p = (((state->A & 0xff) & 1) == 0);
        state->conds.cy = 0;
        break;
    }
    case 0xae: {//XRA M 
        offset = (state->H<<8) | (state->L);
        state->A = state->A ^ state->mem[offset];
        state->conds.z = ((state->A & 0xff) == 0);
        state->conds.s = ((state->A & 0x80) != 0);
        state->conds.p = (((state->A & 0xff) & 1) == 0);
        state->conds.cy = 0;
        break;
    }
    case 0xaf: {//XRA A 
        state->A = state->A ^ state->A;
        state->conds.z = ((state->A & 0xff) == 0);
        state->conds.s = ((state->A & 0x80) != 0);
        state->conds.p = (((state->A & 0xff) & 1) == 0);
        state->conds.cy = 0;
        break;
    }
    
    case 0xb0: { //ORA B (or with A)
        state->A = state->A | state->B;
        state->conds.cy = 0;
        state->conds.z = ((state->A & 0xff) == 0);
        state->conds.s = ((state->A & 0x80) != 0);
        state->conds.p = (((state->A & 0xff) & 1) == 0);
        break;
    }
    case 0xb1:  { //ORA C 
        state->A = state->A | state->C;
        state->conds.cy = 0;
        state->conds.z = ((state->A & 0xff) == 0);
        state->conds.s = ((state->A & 0x80) != 0);
        state->conds.p = (((state->A & 0xff) & 1) == 0);
        break;
    }
    case 0xb2: { //ORA D
        state->A = state->A | state->D;
        state->conds.cy = 0;
        state->conds.z = ((state->A & 0xff) == 0);
        state->conds.s = ((state->A & 0x80) != 0);
        state->conds.p = (((state->A & 0xff) & 1) == 0);
        break;
    }
    case 0xb3:  { //ORA E
        state->A = state->A | state->E;
        state->conds.cy = 0;
        state->conds.z = ((state->A & 0xff) == 0);
        state->conds.s = ((state->A & 0x80) != 0);
        state->conds.p = (((state->A & 0xff) & 1) == 0);
        break;
    }
    case 0xb4:   { //ORA H
        state->A = state->A | state->H;
        state->conds.cy = 0;
        state->conds.z = ((state->A & 0xff) == 0);
        state->conds.s = ((state->A & 0x80) != 0);
        state->conds.p = (((state->A & 0xff) & 1) == 0);
        break;
    }
    case 0xb5:  { //ORA L
        state->A = state->A | state->L;
        state->conds.cy = 0;
        state->conds.z = ((state->A & 0xff) == 0);
        state->conds.s = ((state->A & 0x80) != 0);
        state->conds.p = (((state->A & 0xff) & 1) == 0);
        break;
    }
    case 0xb6:  { //ORA M
        offset = (state->H<<8) | (state->L);
        state->A = state->A | state->mem[offset];
        state->conds.cy = 0;
        state->conds.z = ((state->A & 0xff) == 0);
        state->conds.s = ((state->A & 0x80) != 0);
        state->conds.p = (((state->A & 0xff) & 1) == 0);
        break;
    }
    case 0xb7:  { //ORA A
        state->A = state->A | state->A;
        state->conds.cy = 0;
        state->conds.z = ((state->A & 0xff) == 0);
        state->conds.s = ((state->A & 0x80) != 0);
        state->conds.p = (((state->A & 0xff) & 1) == 0);
        break;
    }
    case 0xb8: { //CMP B
        uint16_t answer = state->A - state->B;
        state->conds.z = ((answer & 0xff) == 0);
        state->conds.s = ((answer & 0x80) != 0);
        state->conds.cy = (answer > 0xff);
        state->conds.p = (((answer & 0xff) & 1) == 0);
        break;
    }
    case 0xb9: { //CMP C
        uint16_t answer = state->A - state->C;
        state->conds.z = ((answer & 0xff) == 0);
        state->conds.s = ((answer & 0x80) != 0);
        state->conds.cy = (answer > 0xff);
        state->conds.p = (((answer & 0xff) & 1) == 0);
        break;
    }
    case 0xba: { //CMP D
        uint16_t answer = state->A - state->D;
        state->conds.z = ((answer & 0xff) == 0);
        state->conds.s = ((answer & 0x80) != 0);
        state->conds.cy = (answer > 0xff);
        state->conds.p = (((answer & 0xff) & 1) == 0);
        break;
    }
    case 0xbb: { //CMP E
        uint16_t answer = state->A - state->E;
        state->conds.z = ((answer & 0xff) == 0);
        state->conds.s = ((answer & 0x80) != 0);
        state->conds.cy = (answer > 0xff);
        state->conds.p = (((answer & 0xff) & 1) == 0);
        break;
    }
    case 0xbc: { //CMP H
        uint16_t answer = state->A - state->H;
        state->conds.z = ((answer & 0xff) == 0);
        state->conds.s = ((answer & 0x80) != 0);
        state->conds.cy = (answer > 0xff);
        state->conds.p = (((answer & 0xff) & 1) == 0);
        break;
    }
    case 0xbd: { //CMP L
        uint16_t answer = state->A - state->L;
        state->conds.z = ((answer & 0xff) == 0);
        state->conds.s = ((answer & 0x80) != 0);
        state->conds.cy = (answer > 0xff);
        state->conds.p = (((answer & 0xff) & 1) == 0);
        break;
    }
    case 0xbe: { //CMP M
        offset = (state->H<<8) | (state->L);
        uint16_t answer = state->A - state->mem[offset];
        state->conds.z = ((answer & 0xff) == 0);
        state->conds.s = ((answer & 0x80) != 0);
        state->conds.cy = (answer > 0xff);
        state->conds.p = (((answer & 0xff) & 1) == 0);
        break;
    }
    case 0xbf: { //CMP A
        uint16_t answer = state->A - state->A;
        state->conds.z = ((answer & 0xff) == 0);
        state->conds.s = ((answer & 0x80) != 0);
        state->conds.cy = (answer > 0xff);
        state->conds.p = (((answer & 0xff) & 1) == 0);
        break;
    }
    case 0xc0: { //RNZ
        if (!state->conds.z){
            state->pc = state->mem[state->sp] | (state->mem[state->sp+1] >> 8);
            state->sp+=2;;
        } 
        break;
    }

    case 0xc1: { //POP B
        state->C = state->mem[state->sp];
        state->B = state->mem[state->sp+1];
        state->sp+=2;
        break;
    }

    case 0xc2: { //JNZ
        if (!state->conds.z){
            state->pc = (opcode[2] << 8 | opcode[1]);
        } else{
            state->pc +=2;
        }
        break;
    }

    case 0xc3: { //JMP
        state->pc = (opcode[2] << 8 | opcode[1]);
        break;
    }

    case 0xc4: { //CNZ
        if (!state->conds.z){
            //Like jump, but also push result onto stack
            //as in RET
            uint16_t return_address = state->pc+2;
            state->mem[state->sp-1] = (return_address >> 8) & 0xff;
            state->mem[state->sp-2] = return_address & 0xff;
            state->sp-=2;
            state->pc = (opcode[2] << 8 | opcode[1]);
        } else {
            state->pc +=2;
        }
        break;
    }

    case 0xc5: { //PUSH B
        state->mem[state->sp-1] = state->B;
        state->mem[state->sp-2] = state->C;
        state->sp = state->sp-2;
        break;
    } 
    
    case 0xc6:  //ADI (add immediate, i.e. next byte after instruction)
        { 
        uint16_t answer = (uint16_t) state->A + (uint16_t) opcode[1];
        state->A = answer & 0xff;
        state->conds.z = ((answer & 0xff) == 0);
        state->conds.s = ((answer & 0x80) != 0);
        state->conds.cy = (answer > 0xff);
        state->conds.p = (((answer & 0xff) & 1) == 0);
        break;
        }

    case 0xc7: { //RST 0
        handle_interrupt(state, 0);
        break;
    }    

    case 0xc8: { //RZ
        if (state->conds.z){
            state->pc = state->mem[state->sp] | (state->mem[state->sp+1] >> 8);
            state->sp+=2;;
        } 
        break;
    }

    case 0xc9: { //RET
        state->pc = state->mem[state->sp] | (state->mem[state->sp+1] << 8);
        state->sp+=2;
        break;
    }
    

    case 0xca: { //JZ
        if (state->conds.z){
            state->pc = (opcode[2] << 8 | opcode[1]);
        } else{
            state->pc +=2;
        }
        break;
    }

    case 0xcb: { //-
        break;
    }

    case 0xcc: { // CZ
        if (state->conds.z){
            uint16_t return_address = state->pc+2;
            state->mem[state->sp-1] = (return_address >> 8) & 0xff;
            state->mem[state->sp-2] = return_address & 0xff;
            state->sp-=2;
            state->pc = (opcode[2] << 8 | opcode[1]);
        } else{
            state->pc +=2;
        }
        break;
    }

    case 0xcd: {  //CALL
        uint16_t return_address = state->pc+2;
        state->mem[state->sp-1] = (return_address >> 8) & 0xff;
        state->mem[state->sp-2] = return_address & 0xff;
        state->sp-=2;
        state->pc = (opcode[2] << 8 | opcode[1]);
        break;
    } 

    case 0xcf: { //RST 1
        handle_interrupt(state, 1);
        break;
    }    

    case 0xd0: { //RNC
        if (!state->conds.cy){
            state->pc = state->mem[state->sp] | (state->mem[state->sp+1] << 8);
            state->sp+=2;
        }  
        break;
    }

    case 0xd1: { //POP D
        state->E = state->mem[state->sp];
        state->D = state->mem[state->sp+1];
        state->sp+=2;
        break;
    }

    case 0xd2: { //JNC
        if (!state->conds.cy){
            state->pc = (opcode[2] << 8 | opcode[1]);
        } else{
            state->pc +=2;
        }
        break;
    }

    case 0xd3: { //OUT
        //should send contents of A to device number opcode[1]
        uint8_t port = opcode[1];
        machineOUT(state, port);
        state->pc+=1;
        break;
    }

    case 0xd4: { //CNC
        if (!state->conds.cy){
            uint16_t return_address = state->pc+2;
            state->mem[state->sp-1] = (return_address >> 8) & 0xff;
            state->mem[state->sp-2] = return_address & 0xff;
            state->sp-=2;
            state->pc = (opcode[2] << 8 | opcode[1]);
        } else{
            state->pc +=2;
        }
        break;
    }

    case 0xd5:  { //PUSH D
        state->mem[state->sp-1] = state->D;
        state->mem[state->sp-2] = state->E;
        state->sp = state->sp-2;
        break;
    } 

    case 0xd6: { //SUI
        uint16_t answer = state->A - (opcode[1]);
        state->conds.z = ((answer & 0xff) == 0);
        state->conds.s = ((answer & 0x80) != 0);
        state->conds.cy = (answer > 0xff);
        state->conds.p = (((answer & 0xff) & 1) == 0);
        state->pc+=1;
        break;
    }

    case 0xd7: { //RST 2
        handle_interrupt(state, 2);
        break;
    }    

    case 0xd8: { //RC
        if (state->conds.cy){
            state->pc = state->mem[state->sp] | (state->mem[state->sp+1] << 8);
            state->sp+=2;
        } 
        break;
    }

    case 0xd9: { //-
        break;
    }

    case 0xda: { //JC (jump on carry)
        if (state->conds.cy){
            state->pc = (opcode[2] << 8 | opcode[1]);
        } else{
            state->pc +=2;
        }
        break;
    }

    case 0xdb: { //IN
        uint8_t port = opcode[1];
        state->A = machineIN(state, port);
        state->pc += 1;
        break;
    }

    case 0xdd: { //-
        break;
    }

    case 0xde: { //SBI 
        uint16_t answer = state->A - (opcode[1] + state->conds.cy);
        state->conds.z = ((answer & 0xff) == 0);
        state->conds.s = ((answer & 0x80) != 0);
        state->conds.cy = (answer > 0xff);
        state->conds.p = (((answer & 0xff) & 1) == 0);
        state->pc+=1;
        break;
    }

    case 0xdf: { //RST 3
        handle_interrupt(state, 3);
        break;
    }    

    case 0xe0: { //RPO
        if (!state->conds.p) {
            state->pc = state->mem[state->sp] | (state->mem[state->sp+1] >> 8);
            state->sp+=2;;
        } 
        break;
    }

    case 0xe1: { //POP H
        state->L = state->mem[state->sp];
        state->H = state->mem[state->sp+1];
        state->sp+=2;
        break;
    }

    case 0xe2: { //JPO
        if (!state->conds.p) {
            state->pc = (opcode[2] << 8 | opcode[1]);
        } else {
            state->pc +=2;
        }
        break;
    }

    case 0xe3: { //XTHL (exchange top stack with HL)
        uint8_t L_prev = state->L;
        uint8_t H_prev = state->H;
        state->L = state->mem[state->sp];
        state->H = state->mem[state->sp+1];
        state->mem[state->sp] = L_prev;
        state->mem[state->sp+1] = H_prev;
        break;
    }

    case 0xe5:  {//PUSH H
        state->mem[state->sp-1] = state->H;
        state->mem[state->sp-2] = state->L;
        state->sp = state->sp-2; 
        break;
    }

    case 0xe6: { //ANI  (and immediate with A)
        state->A = state->A & opcode[1];
        state->conds.z = ((state->A & 0xff) == 0);
        state->conds.s = ((state->A & 0x80) != 0);
        state->conds.p = (((state->A & 0xff) & 1) == 0);
        state->conds.cy = 0;
        state->pc++;
        break;
    }

    case 0xe7: { //RST 4
        handle_interrupt(state, 4);
        break;
    }    

    case 0xe9: { //PCHL
        state->pc = (state->H << 8 | state->L);
        break;
    }

    case 0xeb: { //XCHG
        uint8_t L_prev = state->L;
        uint8_t H_prev = state->H;
        state->H = state->D;
        state->L = state->E;
        state->D = H_prev;
        state->E = L_prev;
        break;
    }

    case 0xec: { //CPE
         if (state->conds.p){
            uint16_t return_address = state->pc+2;
            state->mem[state->sp-1] = (return_address >> 8) & 0xff;
            state->mem[state->sp-2] = return_address & 0xff;
            state->sp-=2;
            state->pc = (opcode[2] << 8 | opcode[1]);
        } else{
            state->pc +=2;
        }
        break;
    }

    case 0xed: { //-
        break;
    }

    case 0xee: { //XRI
        state->A = state->A ^ opcode[1];
        state->conds.z = ((state->A & 0xff) == 0);
        state->conds.s = ((state->A & 0x80) != 0);
        state->conds.p = (((state->A & 0xff) & 1) == 0);
        state->conds.cy = 0;
        state->pc +=1;
        break;
    }

    case 0xef: { //RST 5
        handle_interrupt(state, 5);
        break;
    }    

    case 0xf0: { //RP
        if (!state->conds.s){
            state->pc = state->mem[state->sp] | (state->mem[state->sp+1] >> 8);
            state->sp+=2;;
        } 
        break;
    }

    case 0xf1: { //POP PSW
        state->A = state->mem[state->sp+1];
        uint8_t psw = state->mem[state->sp];
        state->conds.cy = 0x01 == (psw & 0x01);
        state->conds.p = 0x04 == (psw & 0x04);
        state->conds.ac = 0x10 == (psw & 0x10);
        state->conds.z = 0x40 == (psw & 0x40);
        state->conds.s = 0x80 == (psw & 0x80);
        state->sp+=2;
        break;
    }

    case 0xf3: { //DI
        state->interrupt_enable = 0;
    }

    case 0xf5: { //PUSH PSW
        state->mem[state->sp-1] = state->A;
        uint8_t psw = (state->conds.cy |    
                            state->conds.p << 2 |    
                            state->conds.ac << 4 |    
                            state->conds.z << 6 |    
                            state->conds.s << 7 );                          
        state->mem[state->sp-2] = psw;  
        state->sp-=2;
    }

    case 0xf6: { //ORI
        state->A = state->A | opcode[1];
        state->conds.cy = 0;
        state->conds.z = ((state->A & 0xff) == 0);
        state->conds.s = ((state->A & 0x80) != 0);
        state->conds.p = (((state->A & 0xff) & 1) == 0);
        state->pc+=1;
        break;
    }

    case 0xf7: { //RST 6
        handle_interrupt(state, 6);
        break;
    }    

    case 0xf8: { //RM
        if (state->conds.s){
            state->pc = state->mem[state->sp] | (state->mem[state->sp+1] >> 8);
            state->sp+=2;;
        } 
        break;
    }

    case 0xfa: {// JM
        if (state->conds.s) {
            state->pc = (opcode[2] << 8 | opcode[1]);
        } else {
            state->pc +=2;
        }
        break;
    }

    case 0xfb: {//EI 
        state->interrupt_enable = 1;
        break;
    }
 
    case 0xfc: { //CM
        if (state->conds.s){
            uint16_t return_address = state->pc+2;
            state->mem[state->sp-1] = (return_address >> 8) & 0xff;
            state->mem[state->sp-2] = return_address & 0xff;
            state->sp-=2;
            state->pc = (opcode[2] << 8 | opcode[1]);
        } else{
            state->pc +=2;
        }
        break;
    }

    case 0xfd: { //-
        break;
    }

    case 0xfe: { //CPI Compare immediate
        uint16_t answer = state->A - opcode[1];
        state->conds.z = ((answer & 0xff) == 0);
        state->conds.s = ((answer & 0x80) != 0);
        state->conds.cy = (answer > 0xff);
        state->conds.p = (((answer & 0xff) & 1) == 0);
        break;
    }

    case 0xff: { //RST 7
        handle_interrupt(state, 7);
        break;
    }    

    default:
        UnimplementedOp(state);
        break;
    }

    //printf("z %d s %d cy %d p %d \n", state->conds.z, state->conds.s, state->conds.cy, state->conds.p);
    //printf("A $%02x B $%02x C $%02x D $%02x E $%02x H $%02x L $%02x SP %04x\n", state->A, state->B, state->C,
	//			state->D, state->E, state->H, state->L, state->sp);
    printf("pc = %d \n", state->pc);         
}


//TODO should be done in two halfs
void draw_screen(SDL_Window *window, SDL_Surface *screen, uint8_t *mem) {

    int x;
    int y;
    int npix_x = 224;
    int npix_y = 256;
    int pixel;
    
    int VRAM_OFFSET = 0x2400;
    uint32_t *pixels = (uint32_t *)screen->pixels;

    for (x = 0; x<npix_x;x++) {
        for (y=0; y<npix_y;y++){
            pixel = mem[VRAM_OFFSET+y + npix_x*x]; //this is either 0 (black) or 1 (white)
            if (pixel == 0) {
                pixels[y+npix_x*x] = 0x000000;
            } else {
                pixels[y+npix_x*x] = 0xFFFFFF;
            } 
            
        }
    }


    SDL_UpdateWindowSurface(window);
    return;
}



//The main routine which takes the data file and changes the 
//hex codes to assembly language commands (currently just prints)

int main (int argc, char**argv){

    //TODO there are things to move avaiy from main
    //Allocate memory
    State_8080 *state = calloc(1, sizeof(State_8080)); //allocates and sets everything to 0
    state->mem = malloc(0x10000); //16K

    //open file, exit if file doesn't exist
    FILE *f = fopen(argv[1], "rb");
    if (f==NULL){
        printf("ERROR: Couldn't open %s, stopping.\n", argv[1]);
        exit(2);
    }

    
    //Get and read file size, read it into a memory buffer
    //takes file, offset end place as input, "goes" to the end
    fseek(f, 0L, SEEK_END);
    //We are at the end, save location(=size)
    int fsize = ftell(f);
    //Go back to start
    fseek(f, 0L, SEEK_SET);

    uint8_t *buf = &state->mem[0];
    //where we write, size, count, stream(thing to be read)
    fread(buf, fsize, 1, f);
    fclose(f);

    int pc = 0;

    //Read the whole thing, keeping track where we are in the file

    int i = 0;

    //All the init stuff could be done better
    state->first_shift_write = 1;
    
    //Open a game window
    //TODO edit heigth and width?, Should the last number be something else?
    SDL_Window *window = SDL_CreateWindow("Space Invaders", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                            224,256, 0);
    SDL_Surface *screen = SDL_GetWindowSurface(window);
    SDL_Surface *pixels = SDL_CreateRGBSurfaceWithFormat(0, 224, 256, 32, SDL_PIXELFORMAT_RGBX8888);


    //Some debug stuff needed if using cpudiag
    //Fix the first instruction to be JMP 0x100    
    /*state->mem[0]=0xc3;    
    state->mem[1]=0;    
    state->mem[2]=0x01;    

    //Fix the stack pointer from 0x6ad to 0x7ad    
    // this 0x06 byte 112 in the code, which is    
    // byte 112 + 0x100 = 368 in memory    
    state->mem[368] = 0x7;    

    //Skip DAA test    
    state->mem[0x59c] = 0xc3; //JMP    
    state->mem[0x59d] = 0xc2;    
    state->mem[0x59e] = 0x05;  */

    int draw_frame = 1;
    time_t current_time = SDL_GetTicks64();
    time_t time_before = SDL_GetTicks64();
    int allow_interrupt = 0;
    int CPU_clock = 2000000; //2 MHz
    int rate = CPU_clock/60;
    time_t ticks = 0;
    state->ticks=0;

    while (1){ //(state->pc < fsize)

        
        draw_screen(window, screen, state->mem);
        
        time_before = SDL_GetTicks64();

        while (state->ticks < rate){
            i+=1;

            //not exactly 60 fps but it's a start.
            
            //printf("Instruction %d \n", i);
            pc += disassemble8080(buf, state->pc);

            Emulate8080op(state);
            /*if (SDL_GetTicks64()-current_time > 16) {
                draw_frame = 1;
                current_time = SDL_GetTicks64();
            }*/
            printf("\n");           

        }

        printf("ticks %d \n", state->ticks);

        if (state->interrupt_enable) {
            printf("interruptions are enabled! Need to interrupt! \n");
            handle_interrupt(state, 2);
            
            state->interrupt_enable = 0;
        }

        state->ticks = 0;

        //SDL_Delay(1000/60-(SDL_GetTicks64()-time_before));
        //SDL_Delay(1000/60);


        printf("\n");
        
        SDL_Event event;
    
    
        //This does work, quits the program
        //Let's keep this here for now
        while (SDL_PollEvent(&event)) {
            switch (event.type){
                case SDL_QUIT:
                    exit(42);
                //We care only when something has been pressed down, right?
                case SDL_KEYDOWN:
                    switch (event.key.keysym.sym) {
                        case SDLK_BACKSPACE: {
                            printf("You have pressed 'backspace', quitting!\n");
                            exit(6969);
                            break;
                        }
                    }
                default:
                    break;
            }
        }
    }
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
