#include <iostream>
#include "cpu.hpp"
#include "display.hpp"
#include "time.h"
#include <string>
#include <sstream>

using namespace std;

int main()
{
    int FPS = 0;
    time_t seconds = time(NULL);
    gbClass gb;
    gbDisplay display;

    display.gb = &gb;

    gb.loadROM("rom.gb");

    gb.resetGB();


    if(display.initSDL2())
    {
        return 0;
    }

    char step;

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
        if(gb.PC == 0xC003)
        {
            breakpoint = true;
            //cin>>step;
        }
        if(breakpoint == true)
        {
            //cin>>step;
        }
        //if(gb.E == 0xFF || gb.E == 0)
        //{
            //cin>>step;
        //}
        if(gb.haltMode == false)
        {
            gb.runOpcode();
        }
        else
        {
            // Hack to make things run faster.  V-Blank is the only thing that can get this emu out of halt
            gb.cyclesScanline = 456;
            gb.LY = 143;
            gb.haltMode = false;
        }


        if(gb.cyclesScanline >= 456)
        {
            gb.LY++;
            gb.cyclesScanline -= 456;
            if(gb.LY == 144)
            {
                display.renderFullFrame();
                gb.runInterrupt(0x80);
                FPS++;
                if(seconds != time(NULL))
                {
                    string windowTitle = "nopGB R2 Speedrun: ";
                    std::stringstream ss;
                    ss << FPS;
                    windowTitle += ss.str();
                    display.setWindowTitle(windowTitle.c_str());
                    seconds = time(NULL);
                    FPS = 0;
                }
            }

            if(gb.LY == 153)
            {
                gb.LY = 0;
            }
        }


    }
    return 0;
}
