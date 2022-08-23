#include <iostream>
#include "cpu.hpp"
#include "display.hpp"
#include <string>
#include <sstream>
#include <fstream>
#include "audio.hpp"

using namespace std;

/*
    Todo before 3DS Port:

    Todo (in order of importance):
        * Implement GBC
        * Debug Pokemon Blue Intro Song (I think my MBC 1 might be a bit broken)
        * Implement Length Counter (APU)
        * Implement MBC3 RTC
        * Implement UI (ImGUI?)
        * Implement Link Cable
*/

int main(int argc, char** argv)
{
    gbClass gb;
    gbDisplay display;

    display.gb = &gb;
    audio.gb = &gb;
    gb.audio = &audio;

    if(!gb.loadROM("roms/pokemon.gb"))
    {
        return 1;
    }

    string saveName;
    saveName += "save/";
    for(int i = 0x134; i < 0x143; i++)
    {
        if(gb.readRAM(i) == 0)
        {
            break;
        }
        saveName += gb.readRAM(i);
    }
    saveName += ".sav";

    ifstream readSav(saveName, std::ifstream::binary);
    if(readSav)
    {
        readSav.read((char*)gb.SRAM, 0x2000 * 16);
    }
    else
    {
        cout<<"No save for this game could be found, one will be made on exit."<<endl;
    }
    readSav.close();

    ifstream vramInit("vramInit.bin", std::ifstream::binary);

    if(vramInit.is_open())
    {
        vramInit.read((char*)gb.initVRAM, 0x2000);
    }

    vramInit.close();

    gb.resetGB();

    if(display.initSDL2())
    {
        return 0;
    }

    audio.sdlAudioInit();

    bool breakpoint = false;

    gb.haltMode = false;



    while(gb.runGB)
    {
        uint64_t prevCycles = gb.cyclesScanline;

        if(gb.haltMode == false)
        {
            gb.runOpcode();
        }
        else
        {
            gb.cyclesScanline += 4;
        }

        gb.cyclesTotalPrev = gb.cyclesTotal;
        gb.cyclesTotal += (gb.cyclesScanline - prevCycles);

        audio.tickAudioTimers(gb.cyclesScanline - prevCycles);
        gb.divTimer += (gb.cyclesScanline - prevCycles);
        while(gb.divTimer >= 256)
        {
            gb.divTimer -= 256;
            gb.DIV++;
        }

        gb.JOYP &= 0xF;
        gb.JOYP |= (gb.JOYP2 & 0x30);

        gb.runTimer();
        gb.handleModeTimings();

        // Potential Optimization: Move this function call into only the spots that interact with IF, IE, or IME
        gb.checkInterrupt();

        audio.handleAudio();


        if(gb.cyclesScanline >= 456) // New Scanline
        {

            display.renderScanline();
            // This could cause jank with timing / ppu (NEED TO FIX LATER)
            // Have PPU be tickable by cycles instead of this setup?
            if(gb.LCDC & 0x80)
            {
                gb.LY++;
            }
            gb.cyclesScanline -= 456;
            if(gb.LY == 144)
            {
                if(gb.LCDC & 0x80)
                {
                    gb.setIFBit(0x1);
                }
                display.updateFPS();
            }

            if(gb.LY == 154)
            {
                gb.LY = 0;
                display.handleEvents();
            }

            if(gb.LY == gb.LYC)
            {
                gb.STAT |= 0x4;
                if((gb.STAT & 0x40) && (gb.LCDC & 0x80))
                {
                    gb.setIFBit(0x2);
                }
            }
            else
            {
                gb.STAT &= ~0x4;
            }


        }
    }


    ofstream writeSav(saveName, std::ifstream::binary);
    if(writeSav.is_open())
    {
        writeSav.write((char*)gb.SRAM, 0x2000 * 16);
    }


    writeSav.close();

    return 0;
}
