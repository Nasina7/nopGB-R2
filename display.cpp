#include "display.hpp"
#include <iostream>
int gbDisplay::initSDL2()
{
    if(SDL_Init(SDL_INIT_EVERYTHING) < 0)
    {
        std::cout << "Failed to init sdl: " << SDL_GetError() << std::endl;
        return 1;
    }
    win = SDL_CreateWindow("nopGB R2 Speedrun", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 160 * 3, 144 * 3, SDL_WINDOW_RESIZABLE);
    render = SDL_CreateRenderer(win, 1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    tex = SDL_CreateTexture(render, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 160, 144);
    SDL_RenderSetScale(render, 3, 3);
    return 0;
}

uint32_t gbColorLookup[4] = {
    0xFFFFFFFF,
    0xFF808080,
    0xFF404040,
    0xFF000000
};

void gbDisplay::renderFullFrame()
{
    SDL_SetRenderDrawColor(render, 0, 0, 255, 0);
    SDL_RenderClear(render);
    for(int y = 0; y < 144 / 8; y++)
    {
        for(int x = 0; x < 160 / 8; x++)
        {
            uint16_t locationVRAM = 0x9800 + (x) + (y * 32);
            uint8_t index = gb->readRAM(locationVRAM);
            uint16_t locationTile = 0x8000 + index * 16;

            for(int yT = 0; yT < 8; yT++)
            {
                for(int xT = 0; xT < 8; xT++)
                {
                    uint16_t lineData;
                    lineData = (gb->readRAM(locationTile + (yT * 2) + 1) << 8) | gb->readRAM(locationTile + yT * 2);

                    uint8_t blackWhiteColor = (((lineData >> (7 - xT)) & 0x1) << 1) | ((lineData >> (7 - xT) + 8) & 0x1);

                    framebuffer[(y * 8) + yT][(x * 8) + xT] = (gbColorLookup[blackWhiteColor]);
                }
            }
        }
    }

    SDL_UpdateTexture(tex, NULL, framebuffer, 160 * 4);
    SDL_RenderCopy(render, tex, NULL, NULL);
    SDL_RenderPresent(render);
}

void gbDisplay::setWindowTitle(const char* title)
{
    SDL_SetWindowTitle(win, title);
}
