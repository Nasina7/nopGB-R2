#include "display.hpp"
#include <iostream>
#include <bits/stdc++.h>
#define JOY_A     0
#define JOY_B     1
#define JOY_X     2
#define JOY_Y     3
#define JOY_PLUS  10
#define JOY_MINUS 11
#define JOY_LEFT  12
#define JOY_UP    13
#define JOY_RIGHT 14
#define JOY_DOWN  15

/*
    How to potentially optimize this further:
        * Change VRAM so that it is an array of structs containing *ACTUAL* ARGB8888 pixel data.
            VRAM isn't written to often, so this might work very well (and it would reduce a lot of overhead that comes from
            reading a byte at a time
        * Do the same as above, but with OAM

*/

using namespace std;
int gbDisplay::initSDL2()
{
    if(SDL_Init(SDL_INIT_EVERYTHING) < 0)
    {
        std::cout << "Failed to init sdl: " << SDL_GetError() << std::endl;
        return 1;
    }
    win = SDL_CreateWindow("nopGB R2 Speedrun", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_RESIZABLE);
    render = SDL_CreateRenderer(win, 0, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    tex = SDL_CreateTexture(render, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 160, 144);
    //SDL_RenderSetScale(render, 3, 3);
    SDL_RendererInfo info;
    SDL_GetRendererInfo(render, &info);
    for(int i = 0; i < info.num_texture_formats; i++)
    {
        cout<<"Tex Format: "<<info.texture_formats[i]<<endl;
    }
    return 0;
}

uint32_t gbColorLookup[5] = {
    0xFFFFFFFF,
    0xFF8080C0,
    0xFF404080,
    0xFF000000,
    0xFFFF0000, // Debug Color
};

void gbDisplay::renderSpriteTile(uint8_t xPos, uint8_t yPos, uint8_t num, uint8_t attr, uint8_t line, bool debug)
{
    bool palNum = (attr >> 4) & 0x1;
    bool yFlip = (attr >> 6) & 0x1;
    bool xFlip = (attr >> 5) & 0x1;
    bool prio = (attr >> 7) & 0x1;

    uint16_t locationTile = 0x8000 + num * 16;
    int yT = line;
    for(int xT = 0; xT < 8; xT++)
    {
        uint16_t lineData;
        lineData = (gb->readVRAM(locationTile + (line * 2) + 1) << 8) | gb->readVRAM(locationTile + line * 2);

        uint8_t lineDataHigh = lineData >> 8;
        uint8_t lineDataLow = lineData;

        uint8_t blackWhiteColor;
        if(!xFlip)
        {
            blackWhiteColor = (((lineDataLow >> (7 - xT)) & 0x1)) | ((lineDataHigh >> (7 - xT)) & 0x1) << 1;
        }
        else
        {
            blackWhiteColor = (((lineDataLow >> (xT)) & 0x1)) | ((lineDataHigh >> (xT)) & 0x1) << 1;
        }

        uint8_t ogIndex = blackWhiteColor;

        if(attr & 0x10)
        {
            blackWhiteColor = (gb->OBP1 >> (blackWhiteColor * 2)) & 0x3;
        }
        else
        {
            blackWhiteColor = (gb->OBP0 >> (blackWhiteColor * 2)) & 0x3;
        }
        //blackWhiteColor = (bgpIndex >> (blackWhiteColor * 2)) & 0x3;

        uint8_t finalY, finalX;

        finalX = xPos + xT;
        //if(!yFlip)
        //{
            finalY = yPos + yT;
        //}
        //else
        //{
            //finalY = yPos + (7 - yT);
        //}

        if(debug)
        {
            blackWhiteColor = 4;
        }

        if(!(attr & 0x80))
        {
            if(yPos < 144 && finalX < 160 && yPos >= 0 && finalX >= 0 && ogIndex != 0)
            {
                framebuffer[yPos][finalX] = (gbColorLookup[blackWhiteColor]);
            }
        }
        else
        {
            if(framebufferIndex[yPos][finalX] == 0)
            {
                if(yPos < 144 && finalX < 160 && yPos >= 0 && finalX >= 0 && ogIndex != 0)
                {
                    framebuffer[yPos][finalX] = (gbColorLookup[blackWhiteColor]);
                }
            }
        }



    }

}

struct gbSprite {
    uint8_t x;
    uint8_t y;
    uint8_t oamIndex;
    uint8_t attr;
    uint8_t num;
};

bool sortSprite(struct gbSprite sprite1, struct gbSprite sprite2)
{
    if(sprite1.x < sprite2.x)
    {
        return false;
    }
    else if(sprite1.x == sprite2.x)
    {
        if(sprite1.oamIndex < sprite2.oamIndex)
        {
            return false;
        }
        else
        {
            return true;
        }
    }
    else
    {
        return false;
    }
}

void gbDisplay::renderSprites()
{
    struct gbSprite sprites[0x28];

    for(int i = 0; i < 0xA0; i += 4)
    {
        uint8_t yPos = gb->readOAM(0xFE00 | i);
        yPos -= 16;
        uint8_t xPos = gb->readOAM(0xFE00 | (i + 1));
        xPos -= 8;
        uint8_t num = gb->readOAM(0xFE00 | (i + 2));
        uint8_t attr = gb->readOAM(0xFE00 | (i + 3));

        sprites[i / 4].x = xPos;
        sprites[i / 4].y = yPos;
        sprites[i / 4].attr = attr;
        sprites[i / 4].num = num;
        sprites[i / 4].oamIndex = i / 4;

        if(gb->LCDC & 0x4)
        {
            sprites[i / 4].num &= 0xFE;
        }
    }

    int arrSize = sizeof(sprites) / sizeof(sprites[0]);

    sort(sprites, sprites + arrSize, sortSprite);

    int spriteCount = 0;
    if(!(gb->LCDC & 0x2))
    {
        return;
    }
    for(int i = 0; i < 0x28; i ++)
    {
        uint8_t yPos = sprites[i].y;


        if(yPos < 144)
        {
            if(gb->LY < yPos)
            {
                continue;
            }
            if(gb->LY >= yPos + 8 + (8 * ((gb->LCDC & 0x4) != 0)))
            {
                continue;
            }
        }
        else
        {
            uint8_t fixYPos2 = yPos + 16;
            if(yPos >= 144 && yPos < 240 + (8 * ((gb->LCDC & 0x4) != 0)))
            {
                continue;
            }
            if((gb->LCDC & 0x4) != 0)
            {
                if(gb->LY >= fixYPos2)
                {
                    continue;
                }
            }
            else
            {
                if(gb->LY >= fixYPos2 - 8)
                {
                    continue;
                }
            }
        }









        uint8_t xPos = sprites[i].x;
        uint8_t num = sprites[i].num;
        uint8_t attr = sprites[i].attr;


        //cout<<std::hex<<(int)attr<<endl;

        //yPos = gb->LY - yPos;

        uint8_t yPos2 = yPos + (gb->LY - yPos);
        spriteCount++;
        if(spriteCount > 10)
        {
            return;
        }

        if(attr & 0x40)
        {
            if(gb->LCDC & 0x4)
            {
                if(gb->LY - yPos < 8)
                {
                    renderSpriteTile(xPos, gb->LY, num + 1, attr, 7 - (gb->LY - yPos), false);
                }
                else
                {
                    renderSpriteTile(xPos, gb->LY, num, attr, 7 - ((gb->LY - yPos) - 8), false);
                }
            }
            else
            {
                renderSpriteTile(xPos, gb->LY, num, attr, 7 - (gb->LY - yPos), false);
            }
        }
        else
        {
            if(gb->LCDC & 0x4)
            {
                if(gb->LY - yPos < 8)
                {
                    renderSpriteTile(xPos, gb->LY, num, attr, gb->LY - yPos, false);
                }
                else
                {
                    renderSpriteTile(xPos, gb->LY, num + 1, attr, (gb->LY - yPos) - 8, false);
                }
            }
            else
            {
                // Tetris Arrow
                //cout<<"and"<<endl;
                renderSpriteTile(xPos, gb->LY, num, attr, gb->LY - yPos, false);
            }
        }
    }
}

void gbDisplay::renderWindowScanline()
{

    bool windowTileMap = gb->LCDC & 0x40;
    bool windowTileDataSelect = gb->LCDC & 0x10;

    uint8_t bgpIndex = gb->BGP;
    int xTile = 0;
    for(int x = (gb->WX) / 8; x < (160 / 8); x++)
    {

        uint16_t locationVRAM = 0x9800 + (0x400 * ((gb->LCDC >> 6) & 0x1)) + (xTile) + ((windowScanline >> 3) * 32);
        uint8_t index = gb->readVRAM(locationVRAM);
        uint16_t locationTile = 0x8000 + index * 16;
        if((!(gb->LCDC & 0x10)) && index < 0x80)
        {
            locationTile += 0x1000;
        }

        for(int xT = 0; xT < 8; xT++)
        {
            uint16_t lineData;
            lineData = (gb->readVRAM(locationTile + ((windowScanline & 0x7) * 2) + 1) << 8) | gb->readVRAM(locationTile + (windowScanline & 0x7) * 2);

            uint8_t blackWhiteColor = (((lineData >> (7 - xT)) & 0x1)) | ((lineData >> (7 - xT) + 8) & 0x1) << 1;

            blackWhiteColor = (bgpIndex >> (blackWhiteColor * 2)) & 0x3;

            int finalX = (x * 8) + xT;

            if(finalX >= 160)
            {
                return;
            }


            //if(!(gb->LCDC & 0x1))
            //{
            //    blackWhiteColor = 0;
            //}

            if(finalX < 160 && finalX >= 0)
            {
                //gbColorLookup[blackWhiteColor]
                framebuffer[gb->LY][finalX] = gbColorLookup[blackWhiteColor];
                framebufferIndex[gb->LY][finalX] = blackWhiteColor;
            }


        }
        xTile++;
    }
}

void gbDisplay::renderTilemapScanline()
{
    //cout<<"X: "<<std::hex<<(int)gb->SCX<<endl;
    //cout<<"LCDC: "<<std::hex<<(int)gb->LCDC<<endl;
    uint8_t bgpIndex = gb->BGP;
    for(int x = 0; x < (168 / 8); x++)
    {
        uint8_t xIndex = x + (gb->SCX >> 3);
        if(xIndex >= 32)
        {
            xIndex -= 32;
        }

        uint8_t yTrue = (gb->LY + gb->SCY);

        uint8_t yIndex = yTrue >> 3;
        uint8_t yT = yTrue & 0x7;

        uint16_t locationVRAM = 0x9800 + (0x400 * ((gb->LCDC >> 3) & 0x1)) + (xIndex) + (yIndex * 32);
        uint8_t index = gb->readVRAM(locationVRAM);
        uint16_t locationTile = 0x8000 + index * 16;
        if((!(gb->LCDC & 0x10)) && index < 0x80)
        {
            locationTile += 0x1000;
        }

        for(int xT = 0; xT < 8; xT++)
        {
            uint16_t lineData;
            lineData = (gb->readVRAM(locationTile + (yT * 2) + 1) << 8) | gb->readVRAM(locationTile + yT * 2);

            uint8_t blackWhiteColor = (((lineData >> (7 - xT)) & 0x1)) | ((lineData >> (7 - xT) + 8) & 0x1) << 1;

            blackWhiteColor = (bgpIndex >> (blackWhiteColor * 2)) & 0x3;

            int finalX = (x * 8) + xT;

            finalX -= gb->SCX & 0x7;

            if(!(gb->LCDC & 0x1))
            {
                blackWhiteColor = 0;
            }

            if(finalX < 160 && finalX >= 0)
            {
                framebuffer[gb->LY][finalX] = (gbColorLookup[blackWhiteColor]);
                framebufferIndex[gb->LY][finalX] = blackWhiteColor;
            }


        }
    }
}

void gbDisplay::renderScanline()
{
    if(gb->LY < 144)
    {
        renderTilemapScanline();
        if(gb->LY >= gb->WY)
        {
            if((gb->LCDC & 0x20) && gb->WX < 166 && gb->WY < 143)
            {
                renderWindowScanline();
                windowScanline++;
            }
        }
        renderSprites();
    }

    if(gb->LY == 144)
    {
        SDL_UpdateTexture(tex, NULL, framebuffer, 160 * 4);
        SDL_Rect screen;
        int w, h;
        SDL_GetWindowSize(win, &w, &h);
        screen.w = 160 * (720 / 144);
        screen.h = 720;
        screen.x = (1280 / 2) - (screen.w / 2);
        screen.y = 0;
        SDL_SetRenderDrawColor(render, 0,0,0,0);
        SDL_RenderClear(render);
        SDL_RenderCopy(render, tex, NULL, &screen);
        SDL_RenderPresent(render);
        windowScanline = 0;
    }
}

void gbDisplay::renderTilemapFrame()
{
    cout<<"LCDC: "<<std::hex<<(int)gb->LCDC<<endl;
    uint8_t bgpIndex = gb->BGP;
    SDL_SetRenderDrawColor(render, 0, 0, 255, 0);
    SDL_RenderClear(render);
    for(int y = 0; y < 144 / 8; y++)
    {
        for(int x = 0; x < 160 / 8; x++)
        {

            uint16_t locationVRAM = 0x9800 + (0x400 * ((gb->LCDC >> 3) & 0x1)) + (x) + (y * 32);
            uint8_t index = gb->readRAM(locationVRAM);
            uint16_t locationTile = 0x8000 + index * 16;
            if((!(gb->LCDC & 0x10)) && index < 0x80)
            {
                locationTile += 0x1000;
            }

            for(int yT = 0; yT < 8; yT++)
            {
                for(int xT = 0; xT < 8; xT++)
                {
                    uint16_t lineData;
                    lineData = (gb->readRAM(locationTile + (yT * 2) + 1) << 8) | gb->readRAM(locationTile + yT * 2);

                    uint8_t blackWhiteColor = (((lineData >> (7 - xT)) & 0x1) << 1) | ((lineData >> (7 - xT) + 8) & 0x1);

                    blackWhiteColor = (bgpIndex >> (blackWhiteColor * 2)) & 0x3;

                    framebuffer[(y * 8) + yT][(x * 8) + xT] = (gbColorLookup[blackWhiteColor]);
                }
            }
        }
    }
}

void gbDisplay::renderFullFrame()
{
    renderTilemapFrame();
    renderSprites();

    SDL_UpdateTexture(tex, NULL, framebuffer, 160 * 4);
    SDL_RenderCopy(render, tex, NULL, NULL);
    SDL_RenderPresent(render);
}

void gbDisplay::setWindowTitle(const char* title)
{
    SDL_SetWindowTitle(win, title);
}

void gbDisplay::handleEvents()
{
    gb->JOYP |= 0xCF;
    while(SDL_PollEvent(&e))
    {
        if(e.type == SDL_JOYBUTTONDOWN)
        {
            if(e.jbutton.which == 0)
            {
                switch(e.jbutton.button)
                {
                    case JOY_PLUS:
                        gb->JOYPR &= 0xF7;
                    break;

                    case JOY_A:
                        gb->JOYPR &= 0xFE;
                    break;

                    case JOY_B:
                        gb->JOYPR &= 0xFD;
                    break;

                    case JOY_MINUS:
                        gb->JOYPR &= 0xFB;
                    break;

                    case JOY_UP:
                        gb->JOYPR &= 0xBF;
                    break;

                    case JOY_DOWN:
                        gb->JOYPR &= 0x7F;
                    break;

                    case JOY_LEFT:
                        gb->JOYPR &= 0xDF;
                    break;

                    case JOY_RIGHT:
                        gb->JOYPR &= 0xEF;
                    break;

                    default:

                    break;
                }
            }
        }
        else if(e.type == SDL_JOYBUTTONUP)
        {
            if(e.jbutton.which == 0)
            {
                switch(e.jbutton.button)
                {
                    case JOY_X:
                        gb->resetGB();
                    break;

                    case JOY_PLUS:
                        gb->JOYPR |= 0x8;
                    break;

                    case JOY_B:
                        gb->JOYPR |= 0x2;
                    break;

                    case JOY_MINUS:
                        gb->JOYPR |= 0x4;
                    break;

                    case JOY_A:
                        gb->JOYPR |= 0x1;
                    break;

                    case JOY_UP:
                        gb->JOYPR |= 0x40;
                    break;

                    case JOY_DOWN:
                        gb->JOYPR |= 0x80;
                    break;

                    case JOY_LEFT:
                        gb->JOYPR |= 0x20;
                    break;

                    case JOY_RIGHT:
                        gb->JOYPR |= 0x10;
                    break;

                    case JOY_Y:
                        gb->runGB = false;
                    break;

                    default:

                    break;
                }
            }
        }
    }
}

void gbDisplay::handleModeTimings()
{
    uint8_t prevStat = gb->STAT;
    if(gb->LY >= 144)
    {
        gb->STAT &= 0xFC;
        gb->STAT |= 0x1;
        return;
    }
    // STAT TIMINGS
    // These timings are an approximation, as i can't seem to find documentation for this.
    if((gb->cyclesScanline) < (84) && (gb->LCDC & 0x80) == 0x80) // Mode 2
    {
        gb->STAT &= 0xFC;
        gb->STAT |= 0x2;
        /*
        if((io.LCDCS & 0x20) != 0 && (cpu.prevCycles % cyclesPerScanline) >= 84)
        {
            io.IF = io.IF | 0x2;
            cpu.handleInterrupts();
        }
        */
    }
    else if((gb->cyclesScanline) < (375) && (gb->LCDC & 0x80) == 0x80) // Mode 3
    {
        gb->STAT &= 0xFC;
        gb->STAT |= 0x3;
    }
    else if((gb->cyclesScanline) <= (456) && (gb->LCDC & 0x80) == 0x80) // Mode 0
    {
        gb->STAT &= 0xFC;
        gb->STAT |= 0x0;
        /*
        if((io.LCDCS & 0x8) != 0 && (cpu.prevCycles % cyclesPerScanline) < 375)
        {
            io.IF = io.IF | 0x2;
            cpu.handleInterrupts();
        }
        */
    }

    if(!(gb->LCDC & 0x80))
    {
        gb->STAT &= 0xFC;
        gb->STAT |= 0x0;
    }
    if(prevStat != gb->STAT)
    {
        // Mode Changed
        if((gb->STAT & 0x3) == 0)
        {
            if(gb->STAT & 0x8)
            {
                gb->IF |= 0x2;
            }
        }
        else if((gb->STAT & 0x3) == 1)
        {
            if(gb->STAT & 0x10)
            {
                gb->IF |= 0x2;
            }
        }
        else if((gb->STAT & 0x3) == 2)
        {
            if(gb->STAT & 0x20)
            {
                gb->IF |= 0x2;
            }
        }
    }
}

void gbDisplay::deinitSDL2()
{
    SDL_DestroyRenderer(render);
    SDL_DestroyWindow(win);
    SDL_Quit();
}
