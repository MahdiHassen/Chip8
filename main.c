#include <SDL2/SDL.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#define SCREEN_WIDTH 64
#define SCREEN_HEIGHT 32
#define SCALE 17  // Scale the window, should be variable


SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;

uint8_t graphics[SCREEN_HEIGHT][SCREEN_WIDTH] = {0}; // 64x32 pixels on the display, initialized with 0's but with 1 for test
uint8_t keyboard[16] = {0}; // 16 keys, initialized with 0's
uint8_t keyPressed = 0; // 0 if no key is pressed, 1 if a key is pressed

/*

Keyboard layout:

Computer         Chip-8
________        ________ 
1 2 3 4         1 2 3 C
q w e r   ==    4 5 6 D
a s d f   ==    7 8 9 E
z x c v         A 0 B F  

*/

uint16_t pc; // Program counter
uint16_t I; // Index register
uint8_t V[16]; // 16 general purpose registers
uint16_t sp=1; // Stack pointer
uint16_t stack[16] = {0}; // Stack
uint8_t memory[0x1000]; // 4KB memory

uint16_t opcode=0; // Current opcode

uint8_t delay_timer;
uint8_t sound_timer;

void loadFont(){ //loads font at 0x000


/*

Font data:                           #            Index in memory:
_____________________________       ___      _____________________________
0xF0, 0x90, 0x90, 0x90, 0xF0,       (0)      0x00, 0x01, 0x02, 0x03, 0x04
0x20, 0x60, 0x20, 0x20, 0x70,       (1)      0x05, 0x06, 0x07, 0x08, 0x09
0xF0, 0x10, 0xF0, 0x80, 0xF0,       (2)      0x0A, 0x0B, 0x0C, 0x0D, 0x0E
0xF0, 0x10, 0xF0, 0x10, 0xF0,      (...)     0x0F, 0x10, 0x11, 0x12, 0x13
0x90, 0x90, 0xF0, 0x10, 0x10,                0x14, 0x15, 0x16, 0x17, 0x18
0xF0, 0x80, 0xF0, 0x10, 0xF0,                0x19, 0x1A, 0x1B, 0x1C, 0x1D
0xF0, 0x80, 0xF0, 0x90, 0xF0,                0x1E, 0x1F, 0x20, 0x21, 0x22
0xF0, 0x10, 0x20, 0x40, 0x40,                0x23, 0x24, 0x25, 0x26, 0x27
0xF0, 0x90, 0xF0, 0x90, 0xF0,                0x28, 0x29, 0x2A, 0x2B, 0x2C
0xF0, 0x90, 0xF0, 0x10, 0xF0,                0x2D, 0x2E, 0x2F, 0x30, 0x31
0xF0, 0x90, 0xF0, 0x90, 0x90,                0x32, 0x33, 0x34, 0x35, 0x36
0xE0, 0x90, 0xE0, 0x90, 0xE0,                0x37, 0x38, 0x39, 0x3A, 0x3B
0xF0, 0x80, 0x80, 0x80, 0xF0,                0x3C, 0x3D, 0x3E, 0x3F, 0x40
0xE0, 0x90, 0x90, 0x90, 0xE0,     (...)      0x41, 0x42, 0x43, 0x44, 0x45
0xF0, 0x80, 0xF0, 0x80, 0xF0,      (E)       0x46, 0x47, 0x48, 0x49, 0x4A
0xF0, 0x80, 0xF0, 0x80, 0x80,      (F)       0x4B, 0x4C, 0x4D, 0x4E, 0x4F


*/



uint8_t fontData[] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, //0
    0x20, 0x60, 0x20, 0x20, 0x70, //1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, //...
    0xF0, 0x10, 0xF0, 0x10, 0xF0,
    0x90, 0x90, 0xF0, 0x10, 0x10,
    0xF0, 0x80, 0xF0, 0x10, 0xF0,
    0xF0, 0x80, 0xF0, 0x90, 0xF0,
    0xF0, 0x10, 0x20, 0x40, 0x40,
    0xF0, 0x90, 0xF0, 0x90, 0xF0,
    0xF0, 0x90, 0xF0, 0x10, 0xF0,
    0xF0, 0x90, 0xF0, 0x90, 0x90,
    0xE0, 0x90, 0xE0, 0x90, 0xE0,
    0xF0, 0x80, 0x80, 0x80, 0xF0,
    0xE0, 0x90, 0x90, 0x90, 0xE0,
    0xF0, 0x80, 0xF0, 0x80, 0xF0,
    0xF0, 0x80, 0xF0, 0x80, 0x80  //F
};

memcpy(&memory[0x000], fontData, sizeof(fontData));

}

void loadROM(char *romName){ //loads rom at 0x200

   FILE *file = fopen(romName, "rb");
   if (file == NULL) {
       printf("Error opening ROM!\n");
       return;
   }

   fseek(file, 0, SEEK_END);
   long fileSize = ftell(file);
   rewind(file);

   if (fileSize > 0x1000 - 0x200) {
       printf("ROM too large\n");
       return;
   }

   fread(&memory[0x200], fileSize, 1, file); 
   fclose(file);
   printf("ROM loaded successfully\n");
}

void fetchOpcode() { //fetches opcode from memory, increments pc by 2
    opcode = memory[pc] << 8 | memory[pc + 1];
    pc += 2;
    printf("Fetched opcode: 0x%X\n", opcode);
}

void executeCycle() {
    fetchOpcode();

    switch (opcode & 0xF000) {
        case 0x0000:
            // Handle opcodes starting with 0x0

        switch (opcode & 0x00FF) {

            case 0x00E0:{ // Clear the display (0x00E0)
                for(int i = 0; i < SCREEN_HEIGHT; i++) {
                    for(int j = 0; j < SCREEN_WIDTH; j++) {
                        graphics[i][j] = 0;
                            }
                        }
                        
                    }break;

            case 0x00EE: { // Return from a subroutine (pop the stack)

                sp--; // Decrement stack pointer
                pc = stack[sp]; // Set pc to the top of the stack
                
            }
                break;
        }
            break;

        case 0x1000:{
            // Handle opcodes starting with 0x1 (Jump to address NNN, 0x1NNN)
            pc = opcode & 0x0FFF;
            }
            break;

        case 0x2000: {
            // Handle opcodes starting with 0x2 (Call subroutine at NNN, 0x2NNN, pushes pc to stack)

            printf("pc: %d\n", pc);
            printf("sp is: %d\n", sp);
            printf("stack[sp], before: %d\n", stack[sp]);
            stack[sp] = pc; // Store current pc on the stack
            printf("stack[sp], set: %d\n", stack[sp]);
            sp++; // Increment stack pointer
            printf("sp is now: %d\n", sp);
            printf("pc going to be set to: %d\n", opcode & 0x0FFF);
            pc = opcode & 0x0FFF; // Set pc to NNN
            }

            break;
        case 0x3000:{
            // Handle opcodes starting with 0x3, (Skip next instruction if Vx == NN, 0x3XNN, increments pc by 2)
            if (V[(opcode & 0x0F00) >> 8] == (opcode & 0x00FF)) {
                pc += 2;
            }
            }        
            break;

        case 0x4000:{
            // Handle opcodes starting with 0x4 (Skip next instruction if Vx != NN, 0x4XNN, increments pc by 2)
            if (V[(opcode & 0x0F00) >> 8] != (opcode & 0x00FF)) {
                pc += 2;
            }
            }

            break;
        case 0x5000:{
            // Handle opcodes starting with 0x5 (Skip next instruction if Vx == Vy, 0x5XY0, increments pc by 2)
            if (V[(opcode & 0x0F00) >> 8] == V[(opcode & 0x00F0) >> 4]) {
                pc += 2;
            }
            }
            break;

        case 0x6000:{
            // Handle opcodes starting with 0x6 (Set Vx to NN 0x6XNN)
            V[(opcode & 0x0F00) >> 8] = opcode & 0x00FF; // Set Vx to NN
            }
            break;

        case 0x7000:{
            // Handle opcodes starting with 0x7 (Add NN to Vx 0x7XNN)
            V[(opcode & 0x0F00) >> 8] += opcode & 0x00FF; // Add NN to Vx
        }
            break;

        case 0x8000:{
            // Handle opcodes starting with 0x8
            switch (opcode & 0x800F) {

            case(0x8000):{
                // Set Vx to Vy 0x8XY0
                V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x00F0) >> 4];
                break;
            }    
        
            case(0x8001):{
                // Set Vx to Vx | Vy 0x8XY1
                V[(opcode & 0x0F00) >> 8] |= V[(opcode & 0x00F0) >> 4];
                break;
            }

            case(0x8002):{
                // Set Vx to Vx & Vy 0x8XY2
                V[(opcode & 0x0F00) >> 8] &= V[(opcode & 0x00F0) >> 4];
                break;
            }

            case(0x8003):{
                // Set Vx to Vx xor Vy 0x8XY3
                V[(opcode & 0x0F00) >> 8] ^= V[(opcode & 0x00F0) >> 4];
                break;
            }

            case(0x8004):{
                // Add Vy to Vx, set VF to 1 if there is a carry 0x8XY4
                if (V[(opcode & 0x00F0) >> 4] > (0xFF - V[(opcode & 0x0F00) >> 8])) {
                    V[0xF] = 1; // Set VF to 1
                } else {
                    V[0xF] = 0; // Set VF to 0
                }
                V[(opcode & 0x0F00) >> 8] += V[(opcode & 0x00F0) >> 4];
                break;
            }

            case(0x8005):{
                // Subtract Vy from Vx, set VF to 0 if there is a borrow 0x8XY5
                if (V[(opcode & 0x00F0) >> 4] > V[(opcode & 0x0F00) >> 8]) {
                    V[0xF] = 0; // Set VF to 0
                } else {
                    V[0xF] = 1; // Set VF to 1
                }
                V[(opcode & 0x0F00) >> 8] -= V[(opcode & 0x00F0) >> 4];
                break;
            }

            case(0x8006):{
                // Store the least significant bit of Vx in VF and then shift Vx to the right by 1 0x8XY6
                V[0xF] = V[(opcode & 0x0F00) >> 8] & 0x1; // Store the least significant bit of Vx in VF
                V[(opcode & 0x0F00) >> 8] >>= 1; // Shift Vx to the right by 1
                break;
            }

            case(0x8007):{
                // Set Vx to Vy - Vx, set VF to 0 if there is a borrow 0x8XY7
                if (V[(opcode & 0x0F00) >> 8] > V[(opcode & 0x00F0) >> 4]) {
                    V[0xF] = 0; // Set VF to 0
                } else {
                    V[0xF] = 1; // Set VF to 1
                }
                V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x00F0) >> 4] - V[(opcode & 0x0F00) >> 8];
                break;
            }

            case(0x800E):{
                // Store the most significant bit of Vx in VF and then shift Vx to the left by 1 0x8XYE
                V[0xF] = V[(opcode & 0x0F00) >> 8] >> 7; // Store the most significant bit of Vx in VF
                V[(opcode & 0x0F00) >> 8] <<= 1; // Shift Vx to the left by 1
                break;
            }

        }
    }
            break;
        case 0x9000:{
            // Handle opcodes starting with 0x9 (Skip next instruction if Vx != Vy 0x9XY0, increments pc by 2)
            if (V[(opcode & 0x0F00) >> 8] != V[(opcode & 0x00F0) >> 4]) {
                pc += 2;
            }
            }
            break;

        case 0xA000:{
            // Handle opcodes starting with 0xA (Set I to NNN 0xANNN)
            I = opcode & 0x0FFF; // Set I to NNN
            }
            break;

        case 0xB000:{
            // Handle opcodes starting with 0xB (Jump to address NNN + V0 0xBNNN)
            pc = (opcode & 0x0FFF) + V[0]; // Set pc to NNN + V0
            }
            break;

        case 0xC000:{
            // Handle opcodes starting with 0xC, (Random number AND NN, 0xCXNN, sets Vx to random number AND NN)
            
            uint8_t random = rand() % 0xFF; // Generate a random number between 0 and 255
            V[(opcode & 0x0F00) >> 8] = random & (opcode & 0x00FF); // Set Vx to random & NN
            }
            break;

        case 0xD000:{
            // Handle opcodes starting with 0xD (Draw sprite at (Vx, Vy) with width 8 and height N 0xDXYN)
                   
            uint8_t x = V[(opcode & 0x0F00) >> 8];
            uint8_t y = V[(opcode & 0x00F0) >> 4];
            uint8_t height = opcode & 0x000F;

            V[0xF] = 0; // Reset VF

            for (int yline = 0; yline < height; yline++) {
                uint8_t pixel = memory[I + yline];
                for (int xline = 0; xline < 8; xline++) {
                    if ((pixel & (0x80 >> xline)) != 0) {
                        if (graphics[y + yline][x + xline] == 1) {
                            V[0xF] = 1; // Set VF to 1
                        }
                        graphics[y + yline][x + xline] ^= 1;
                    }
                }
            }
        }

            break;
        case 0xE000:{
            // Handle opcodes starting with 0xE
            switch (opcode & 0x00FF) {
                case 0x009E:{
                    // Skip next instruction if key with the value of Vx is pressed 0xEX9E
                    if (keyboard[V[(opcode & 0x0F00) >> 8]] != 0) {
                        pc += 2;
                    }
                }
                    break;

                case 0x00A1:{
                    // Skip next instruction if key with the value of Vx is not pressed 0xEXA1
                    if (keyboard[V[(opcode & 0x0F00) >> 8]] == 0) {
                        pc += 2;
                    }
                }
                    break;
            }
        }

            break;
        case 0xF000:{
            // Handle opcodes starting with 0xF
            switch (opcode & 0x00FF) {
                case 0x0007:{
                    // Set Vx to the value of the delay timer 0xFX07
                    V[(opcode & 0x0F00) >> 8] = delay_timer;
                }
                    break;

                case 0x000A:{
                    // Wait for a key press, store the value of the key in Vx 0xFX0A
                    if (keyPressed) {
                        for (int i = 0; i < 16; i++) {
                            if (keyboard[i] != 0) {
                                V[(opcode & 0x0F00) >> 8] = i;
                                keyPressed = 0;
                                break;
                            }
                        }
                    } else {
                        pc -= 2; // Repeat the cycle
                    }
                }
                    break;

                case 0x0015:{
                    // Set the delay timer to Vx 0xFX15
                    delay_timer = V[(opcode & 0x0F00) >> 8];
                }
                    break;

                case 0x0018:{
                    // Set the sound timer to Vx 0xFX18
                    sound_timer = V[(opcode & 0x0F00) >> 8];
                }
                    break;

                case 0x001E:{
                    // Add Vx to I 0xFX1E
                    I += V[(opcode & 0x0F00) >> 8];
                }
                    break;

                case 0x0029:{
                    // Set I to the location of the sprite for the character in Vx 0xFX29
                    uint8_t fontChar = (V[(opcode & 0x0F00) >> 8] & 0x0F); //font memory starts at 0x0000
                    I = fontChar * 5; //each character is 5 bytes long, 0 = 0x0000, 1 = 0x0005, 2 = 0x000A, etc.
                }
                    break;

                case 0x0033:{
                    // Store the binary-coded decimal representation of Vx at the addresses I, I+1, and I+2 0xFX33

                    /*
                    Vx = 123
                    memory[I] = 1
                    memory[I + 1] = 2
                    memory[I + 2] = 3   
                    */

                    memory[I] = V[(opcode & 0x0F00) >> 8] / 100;
                    memory[I + 1] = (V[(opcode & 0x0F00) >> 8] / 10) % 10;
                    memory[I + 2] = V[(opcode & 0x0F00) >> 8] % 10;
                }
                    break;

                case 0x0055:{
                    // Store V0 to Vx in memory starting at address I 0xFX55
                    for (int i = 0; i <= ((opcode & 0x0F00) >> 8); i++) {
                        memory[I + i] = V[i];
                    }
                }
                    break;

                case 0x0065:{
                    // Fill V0 to Vx with values from memory starting at address I 0xFX65
                    for (int i = 0; i <= ((opcode & 0x0F00) >> 8); i++) {
                        V[i] = memory[I + i];
                    }
                }
                    break;
                }
        }

            break;

        default:
            printf("Unknown opcode: 0x%X\n", opcode);
    }
    }

int setupGraphics() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return 0;
    }

    window = SDL_CreateWindow("Chip-8 Emulator", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH * SCALE, SCREEN_HEIGHT * SCALE, SDL_WINDOW_SHOWN);
    if (window == NULL) {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        return 0;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == NULL) {
        printf("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        return 0;
    }

    SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF);
    SDL_RenderClear(renderer);

    return 1;
}

void drawGraphics() {

for (int i = 0; i < SCREEN_HEIGHT; i++) {
        for (int j = 0; j < SCREEN_WIDTH; j++) {
            if (graphics[i][j] == 1) {
                SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
            } else {
                SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF);
            }

            SDL_Rect pixel = {j * SCALE, i * SCALE, SCALE, SCALE}; //pixel scaled to the window
            SDL_RenderFillRect(renderer, &pixel);
        }
    }

    SDL_RenderDrawRect(renderer, NULL);

}

void displayGraphics() {
    SDL_RenderPresent(renderer);
}



int main(int argc, char **argv) {
   
//TODO: Basic function re-org

    loadFont();
    if (argc < 2) {
 
        loadROM("Tested Roms/IBM Logo.ch8");
    }
    else {
        loadROM(argv[1]);
    }
 
    setupGraphics();

    SDL_Event event;
    int quit = 0;

    const int cpuHz = 500; //CHIP-8 clock speed 500Hz
    const uint32_t frameDelay = 1000 / cpuHz; // ms per instruction cycle
    const uint32_t delayRegisterDelay = 1000 / 60; // ms per instruction cycle

    uint32_t lastCycleTime = SDL_GetTicks(); // Get the current time in milliseconds
    uint32_t lastCycleTimeDelayTimer = SDL_GetTicks(); // Get the current time in milliseconds

    while (!quit) {
        while (SDL_PollEvent(&event)) {

            switch(event.type) {

                case SDL_QUIT: {
                    quit = 1;
                }
                    break;

                case SDL_KEYDOWN:
                    switch(event.key.keysym.sym) {
                        case SDLK_1: 
                            keyboard[0x1] = 1;
                            keyPressed = 1; 
                            break;

                        case SDLK_2: 
                            keyboard[0x2] = 1;
                            keyPressed = 1;
                            break;

                        case SDLK_3: 
                            keyboard[0x3] = 1; 
                            keyPressed = 1;
                            break;

                        case SDLK_4: 
                            keyboard[0xC] = 1; 
                            keyPressed = 1;
                            break;

                        case SDLK_q: 
                            keyboard[0x4] = 1; 
                            keyPressed = 1;
                            break;

                        case SDLK_w: 
                            keyboard[0x5] = 1; 
                            keyPressed = 1;
                            break;

                        case SDLK_e: 
                            keyboard[0x6] = 1;
                            keyPressed = 1; 
                            break;

                        case SDLK_r:
                            keyboard[0xD] = 1;
                            keyPressed = 1; 
                            break;

                        case SDLK_a:
                            keyboard[0x7] = 1;
                            keyPressed = 1; 
                            break;

                        case SDLK_s:    
                            keyboard[0x8] = 1;
                            keyPressed = 1; 
                            break;  

                        case SDLK_d:    
                            keyboard[0x9] = 1;
                            keyPressed = 1; 
                            break;

                        case SDLK_f:
                            keyboard[0xE] = 1;
                            keyPressed = 1; 
                            break;

                        case SDLK_z:    
                            keyboard[0xA] = 1;
                            keyPressed = 1; 
                            break; 
                        
                        case SDLK_x:
                            keyboard[0x0] = 1;
                            keyPressed = 1; 
                            break;
                        
                        case SDLK_c:
                            keyboard[0xB] = 1;
                            keyPressed = 1; 
                            break;

                        case SDLK_v:
                            keyboard[0xF] = 1;
                            keyPressed = 1; 
                            break;

                    }

                    break;

                case SDL_KEYUP:
                    switch(event.key.keysym.sym) {
                        case SDLK_1: 
                            keyboard[0x1] = 0;
                            keyPressed = 0; 
                            break;

                        case SDLK_2: 
                            keyboard[0x2] = 0;
                            keyPressed = 0;
                            break;

                        case SDLK_3: 
                            keyboard[0x3] = 0; 
                            keyPressed = 0;
                            break;

                        case SDLK_4: 
                            keyboard[0xC] = 0; 
                            keyPressed = 0;
                            break;

                        case SDLK_q: 
                            keyboard[0x4] = 0; 
                            keyPressed = 0;
                            break;

                        case SDLK_w: 
                            keyboard[0x5] = 0; 
                            keyPressed = 0;
                            break;

                        case SDLK_e: 
                            keyboard[0x6] = 0;
                            keyPressed = 0; 
                            break;

                        case SDLK_r:
                            keyboard[0xD] = 0;
                            keyPressed = 0; 
                            break;

                        case SDLK_a:
                            keyboard[0x7] = 0;
                            keyPressed = 0; 
                            break;

                        case SDLK_s:    
                            keyboard[0x8] = 0;
                            keyPressed = 0; 
                            break;  

                        case SDLK_d:    
                            keyboard[0x9] = 0;
                            keyPressed = 0; 
                            break;

                        case SDLK_f:
                            keyboard[0xE] = 0;
                            keyPressed = 0; 
                            break;

                        case SDLK_z:    
                            keyboard[0xA] = 0;
                            keyPressed = 0; 
                            break; 
                        
                        case SDLK_x:
                            keyboard[0x0] = 0;
                            keyPressed = 0; 
                            break;
                        
                        case SDLK_c:
                            keyboard[0xB] = 0;
                            keyPressed = 0; 
                            break;

                        case SDLK_v:
                            keyboard[0xF] = 0;
                            keyPressed = 0; 
                            break;

                    }

                    break;
            }

           
        }
        
        uint32_t currentTime = SDL_GetTicks();
        uint32_t elapsedTime = currentTime - lastCycleTime;

        if (elapsedTime >= frameDelay) { //if it's time to execute a cycle
            executeCycle(); 
            drawGraphics(); 
            displayGraphics();

            lastCycleTime = currentTime;
        } else {
            SDL_Delay(0); 
        }

        uint32_t currentTimeDelayTimer = SDL_GetTicks();
        uint32_t elapsedTimeDelayTimer = currentTimeDelayTimer - lastCycleTimeDelayTimer;

        if(elapsedTimeDelayTimer >= delayRegisterDelay) {
            if (delay_timer > 0) {
                delay_timer--;
            }

            if (sound_timer > 0) {
                sound_timer--;
                printf("sound active\n"); //fix
            }

            lastCycleTimeDelayTimer = currentTimeDelayTimer;
        }
}
        

    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}