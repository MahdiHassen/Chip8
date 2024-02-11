#include <SDL2/SDL.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#define SCREEN_WIDTH 64
#define SCREEN_HEIGHT 32
#define SCALE 17  // Scale the window, should be variable


SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;

uint8_t graphics[SCREEN_HEIGHT][SCREEN_WIDTH] = {1}; // 64x32 pixels on the display, initialized with 0's but with 1 for test

uint16_t pc; // Program counter
uint16_t I; // Index register
uint16_t sp; // Stack pointer
uint8_t V[16]; // 16 general purpose registers
uint16_t stack[16]; // Stack
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
}

void executeCycle() {
    fetchOpcode();

    switch (opcode & 0xF000) {
        case 0x0000:
            // Handle opcodes starting with 0x0
            case 0x00E0:{ // Clear the display
                for(int i = 0; i < SCREEN_HEIGHT; i++) {
                    for(int j = 0; j < SCREEN_WIDTH; j++) {
                        graphics[i][j] = 0;
                    }
                }
            }
                break;
            break;

        case 0x1000:{
            // Handle opcodes starting with 0x1 (Jump to address NNN, 0x1NNN)
            pc = opcode & 0x0FFF;
            }
            break;

        case 0x2000:
            // Handle opcodes starting with 0x2
            break;
        case 0x3000:
            // Handle opcodes starting with 0x3
            break;
        case 0x4000:
            // Handle opcodes starting with 0x4
            break;
        case 0x5000:
            // Handle opcodes starting with 0x5
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

        case 0x8000:
            // Handle opcodes starting with 0x8
            break;
        case 0x9000:
            // Handle opcodes starting with 0x9
            break;

        case 0xA000:{
            // Handle opcodes starting with 0xA (Set I to NNN 0xANNN)
            I = opcode & 0x0FFF; // Set I to NNN
            }
            break;

        case 0xB000:
            // Handle opcodes starting with 0xB
            break;
        case 0xC000:
            // Handle opcodes starting with 0xC
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
            printf("Error, Opcode isnt defined");
            break;
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
   
//TODO: Make this easier to read, split into more functions
//TODO: Add Finish opcode implementation
//TODO: Add input handling
//TODO: Add sound

    loadFont();
    loadROM("IBM Logo.ch8");
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

            lastCycleTime = currentTime;
        } else {
            SDL_Delay(1); 
        } 
}
        
     



    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}