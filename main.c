#include <SDL2/SDL.h>

int main() {
    SDL_Init(SDL_INIT_VIDEO);

    SDL_Window *window = SDL_CreateWindow(
        "SDL2 Window",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        640,
        480,
        SDL_WINDOW_SHOWN
    );

    if (window == NULL) {
        SDL_Log("Could not create window: %s", SDL_GetError());
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);

    if (renderer == NULL) {
        SDL_Log("Could not create renderer: %s", SDL_GetError());
        return 1;
    }

    // Set the draw color to red (RGBA: 255, 0, 0, 255)
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);

    // Clear the window to the draw color
    SDL_RenderClear(renderer);

    // Present the window
    SDL_RenderPresent(renderer);

    // Event loop
    SDL_Event event;
    int running = 1;
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
            }
        }
    }
    

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}