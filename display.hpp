#include <SDL2/SDL.h>
#include "cpu.hpp"
#include <time.h>

class gbDisplay {
    public:
        gbDisplay()
        {
           seconds = time(NULL);
        }
        void renderFullFrame();
        int initSDL2();
        void setWindowTitle(const char* title);
        void handleEvents();
        void toggleVSYNC();
        void renderTilemapFrame();
        void renderSprites();
        void renderSpriteTile(uint8_t xPos, uint8_t yPos, uint8_t num, uint8_t attr, uint8_t line, bool debug);
        void renderScanline();
        void renderTilemapScanline();
        void renderWindowScanline();
        void updateFPS();
        gbClass* gb;

    private:
        SDL_Window* win;
        SDL_Renderer* render;
        SDL_Texture* tex;
        SDL_Event e;

        time_t seconds;
        int FPS;
        bool vsync;

        uint8_t windowScanline;

        uint32_t framebuffer[144][160];
        uint8_t framebufferIndex[144][160];
};
