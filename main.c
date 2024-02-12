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
        case 0xE000:
            // Handle opcodes starting with 0xE
            break;
        case 0xF000:
            // Handle opcodes starting with 0xF
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

void getInput() {

    


}


int main(int argc, char **argv) {
   
//TODO: Make this easier to read, split into more functions
//TODO: Add Finish opcode implementation
//TODO: Add input handling
//TODO: Add sound

    loadFont();
    loadROM("3-corax+.ch8");
    setupGraphics();

    SDL_Event event;
    int quit = 0;

    const int cpuHz = 500; //CHIP-8 clock speed 500Hz
    const uint32_t frameDelay = 1000 / cpuHz; // ms per instruction cycle

    uint32_t lastCycleTime = SDL_GetTicks(); // Get the current time in milliseconds

    while (!quit) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                quit = 1;
            }
        }
        
        uint32_t currentTime = SDL_GetTicks();
        uint32_t elapsedTime = currentTime - lastCycleTime;

        if (elapsedTime >= frameDelay) { //if it's time to execute a cycle
            executeCycle(); 
            drawGraphics(); 
            displayGraphics();

            printf("sp (post exec) is: %d\n", sp);

            lastCycleTime = currentTime;
        } else {
            SDL_Delay(1); 
        } 
}
        
     



    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}