#include <iostream>
#include "cpu.hpp"
#include "display.hpp"
#include "time.h"
#include <string>
#include <sstream>
#include <fstream>
#include "audio.hpp"
#include <switch.h>

using namespace std;

/*
    Todo before 3DS Port:

    Todo Later:
        * Implement MBC 5???
        * Implement Length Counter???
        * Maybe GBC???
*/

int main(int argc, char**argv)
{

    socketInitializeDefault();              // Initialize sockets
    nxlinkStdio();                          // Redirect stdout and stderr over the network to nxlink
    int FPS = 0;
    time_t seconds = time(NULL);
    gbClass gb;
    gbDisplay display;

    display.gb = &gb;
    audio.gb = &gb;

    if(!gb.loadROM("nopGB/roms/pokeTCG.gbc"))
    {
        return 1;
    }

    string saveName;
    saveName += "nopGB/save/";
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
    if(readSav.is_open())
    {
        readSav.read((char*)gb.SRAM, 0x2000 * 16);
    }
    else
    {
        cout<<"Could not find: "<<saveName<<endl;
    }
    readSav.close();

    gb.resetGB();


    if(display.initSDL2())
    {
        return 0;
    }

    SDL_JoystickOpen(0);

    audio.sdlAudioInit();

    bool breakpoint = false;

    gb.haltMode = false;



    while(gb.runGB)
    {
        //cout<<"PC: 0x"<<std::hex<<(int)gb.PC<<endl;
        //cout<<"SP: 0x"<<std::hex<<(int)gb.SP<<endl;
        //cout<<"A: 0x"<<std::hex<<(int)gb.LY<<endl;
        //cout<<"F: 0x"<<std::hex<<(int)gb.F<<endl;
        //cout<<"HL: 0x"<<std::hex<<(int)(gb.H << 8 | gb.L)<<endl;
        //cout<<"OPCODE: 0x"<<std::hex<<(int)gb.readRAM(gb.PC)<<endl;
        if(gb.IE == 1)
        {
            breakpoint = true;
            //cin>>step;
        }
        if(breakpoint == true)
        {
            //cin>>step;
        }
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

        // Todo: Put all of this into it's own function (or make a scheduler for this)
        audio.sq1Timer += (gb.cyclesScanline - prevCycles);
        audio.sq2Timer += (gb.cyclesScanline - prevCycles);
        audio.wavTimer += (gb.cyclesScanline - prevCycles);
        audio.noiTimer += (gb.cyclesScanline - prevCycles);
        audio.mainAudioSampleTimer = (gb.cyclesScanline - prevCycles);
        audio.sq1FreqTimer += (gb.cyclesScanline - prevCycles);
        audio.sq1EnvTimer += (gb.cyclesScanline - prevCycles);
        audio.sq2EnvTimer += (gb.cyclesScanline - prevCycles);
        audio.noiEnvTimer += (gb.cyclesScanline - prevCycles);

        gb.JOYP &= 0xF;
        gb.JOYP |= (gb.JOYP2 & 0x30);


        gb.runTimer();
        display.handleModeTimings();
        gb.checkInterrupt();
        audio.handleAudio();


        if(gb.cyclesScanline >= 456) // New Scanline
        {
            //gb.STAT &= ~0x3;
            display.renderScanline();
            // This could cause jank with timing / ppu (NEED TO FIX LATER)
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
                FPS++;
                if(seconds != time(NULL))
                {
                    string windowTitle = "nopGB R2: ";
                    std::stringstream ss;
                    ss << FPS;
                    windowTitle += ss.str();
                    display.setWindowTitle(windowTitle.c_str());
                    seconds = time(NULL);
                    FPS = 0;
                }
            }


            if(gb.LY == 154)
            {
                gb.LY = 0;
                //gb.IF &= 0xFE;
                //display.renderFullFrame();
                display.handleEvents();
                //audio.sendAudio();
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



    ofstream writeSav(saveName, std::ofstream::binary | std::ios::out);
    if(writeSav.is_open())
    {
        writeSav.write((char*)gb.SRAM, 0x2000 * 16);
    }
    else
    {
        cout<<"Could not output: "<<saveName<<endl;
    }
    writeSav.close();

    socketExit();


    display.deinitSDL2();

    return 0;
}
