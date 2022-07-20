#include <SDL2/SDL.h>
#include "cpu.hpp"

class gbDisplay {
    public:
        void renderFullFrame();
        int initSDL2();
        void setWindowTitle(const char* title);
        gbClass* gb;

    private:
        SDL_Window* win;
        SDL_Renderer* render;
        SDL_Texture* tex;
        SDL_Event e;

        uint32_t framebuffer[144][160];
};
