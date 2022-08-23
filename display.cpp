#include "display.hpp"
#include <iostream>
#include <bits/stdc++.h>

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
    win = SDL_CreateWindow("nopGB R2", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 160 * 3, 144 * 3, SDL_WINDOW_RESIZABLE);
    render = SDL_CreateRenderer(win, 1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    tex = SDL_CreateTexture(render, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 160, 144);
    SDL_RenderSetScale(render, 3, 3);
    SDL_RendererInfo info;
    SDL_GetRendererInfo(render, &info);
    vsync = true;
    return 0;
}

void gbDisplay::toggleVSYNC()
{
    SDL_DestroyTexture(tex);
    SDL_DestroyRenderer(render);


    if(vsync)
    {
        render = SDL_CreateRenderer(win, 1, SDL_RENDERER_ACCELERATED);
    }
    else
    {
        render = SDL_CreateRenderer(win, 1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    }
    tex = SDL_CreateTexture(render, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 160, 144);

    vsync = !vsync;
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
        SDL_RenderCopy(render, tex, NULL, NULL);
        SDL_RenderPresent(render);
        windowScanline = 0;
    }
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
        if(e.type == SDL_KEYDOWN)
        {
            switch(e.key.keysym.sym)
            {
                case SDLK_RETURN:
                    gb->JOYPR &= 0xF7;
                break;

                case SDLK_x:
                    gb->JOYPR &= 0xFE;
                break;

                case SDLK_z:
                    gb->JOYPR &= 0xFD;
                break;

                case SDLK_LSHIFT:
                    gb->JOYPR &= 0xFB;
                break;

                case SDLK_UP:
                    gb->JOYPR &= 0xBF;
                break;

                case SDLK_DOWN:
                    gb->JOYPR &= 0x7F;
                break;

                case SDLK_LEFT:
                    gb->JOYPR &= 0xDF;
                break;

                case SDLK_RIGHT:
                    gb->JOYPR &= 0xEF;
                break;

                default:

                break;
            }
        }
        else if(e.type == SDL_KEYUP)
        {
            switch(e.key.keysym.sym)
            {
                case SDLK_v:
                    toggleVSYNC();
                break;

                case SDLK_p:
                    cout<<"PC: "<<std::hex<<gb->PC<<endl;
                    cout<<"IF: "<<std::hex<<(int)gb->IF<<endl;
                    cout<<"IE: "<<std::hex<<(int)gb->IE<<endl;
                    cout<<"IME: "<<std::hex<<gb->IME<<endl;
                    gb->gbDebug = true;
                break;

                case SDLK_m:
                    gb->audioDumpEnable = 1;
                break;

                case SDLK_TAB:
                    gb->resetGB();
                break;

                case SDLK_RETURN:
                    gb->JOYPR |= 0x8;
                break;

                case SDLK_z:
                    gb->JOYPR |= 0x2;
                break;

                case SDLK_LSHIFT:
                    gb->JOYPR |= 0x4;
                break;

                case SDLK_x:
                    gb->JOYPR |= 0x1;
                break;

                case SDLK_UP:
                    gb->JOYPR |= 0x40;
                break;

                case SDLK_DOWN:
                    gb->JOYPR |= 0x80;
                break;

                case SDLK_LEFT:
                    gb->JOYPR |= 0x20;
                break;

                case SDLK_RIGHT:
                    gb->JOYPR |= 0x10;
                break;

                default:

                break;
            }
        }
        else if(e.type == SDL_QUIT)
        {
            gb->runGB = false;
        }
    }
}

void gbDisplay::updateFPS()
{
    FPS++;
    if(seconds != time(NULL))
    {
        string windowTitle = "nopGB R2: ";
        std::stringstream ss;
        ss << FPS;
        windowTitle += ss.str();
        setWindowTitle(windowTitle.c_str());
        seconds = time(NULL);
        FPS = 0;
    }
}
