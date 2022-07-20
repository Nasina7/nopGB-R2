#include <iostream>
#include <fstream>
#include "cpu.hpp"
#include <string.h>

using namespace std; // Todo: Remove This

#define get16Val(x) ((readRAM(x + 2) << 8) | readRAM(x + 1))

bool gbClass::loadROM(const char* filename)
{
    ifstream openRom(filename, std::ifstream::binary);

    openRom.seekg(0, openRom.end);
    romLength = openRom.tellg();
    openRom.seekg(0, openRom.beg);
    ROMFILE = new uint8_t [romLength];
    openRom.read((char*)ROMFILE, romLength);

    if(romLength < 0)
    {
        return false; // Just in case
    }

    if(!openRom)
    {
        return false;
    }
    openRom.close();

    int romLoadAmount;
    if(romLength < 0x8000)
    {
        romLoadAmount = romLength;
    }
    else
    {
        romLoadAmount = 0x8000;
    }

    for(int i = 0; i < 0x4000; i++)
    {
        ROM[0][i & 0x3FFF] = ROMFILE[i];
    }
    for(int i = 0; i < 0x4000; i++)
    {
        ROM[1][i & 0x3FFF] = ROMFILE[i + 0x4000];
    }

    return true;
}

void gbClass::resetGB()
{
    PC = 0x100;
    SP = 0xFFFE;
    A = 0x11;
    B = 0x00;
    C = 0x00;
    D = 0xFF;
    E = 0x56;
    F = 0x80;
    H = 0x00;
    L = 0x0D;
    IME = false;
    LCDC = 0x91;
    STAT = 0x81;
    LY = 0x90;
    cyclesScanline = 0;
    IE = 0;
    IF = 0xE1;
    runGB = true;
}

void gbClass::runOpcode()
{
    uint8_t opcode = readRAM(PC);
    switch(opcode)
    {
        case 0x00:
            opNOP(opcode);
        break;

        case 0x01:
        case 0x11:
        case 0x21:
        case 0x31:
            opLD16(opcode);
        break;

        case 0x02:
        case 0x0A:
        case 0x12:
        case 0x1A:
            opLDR16(opcode);
        break;

        case 0x3:
        case 0xB:
        case 0x13:
        case 0x1B:
        case 0x23:
        case 0x2B:
        case 0x33:
        case 0x3B:
            opIDR16(opcode);
        break;

        case 0x4:
        case 0x5:
        case 0xC:
        case 0xD:
        case 0x14:
        case 0x15:
        case 0x1C:
        case 0x1D:
        case 0x24:
        case 0x25:
        case 0x2C:
        case 0x2D:
        case 0x34:
        case 0x35:
        case 0x3C:
        case 0x3D:
            opIDR(opcode);
        break;

        case 0x06:
        case 0x0E:
        case 0x16:
        case 0x1E:
        case 0x26:
        case 0x2E:
        case 0x36:
        case 0x3E:
            opLDRV(opcode);
        break;

        case 0x07:
        case 0x0F:
        case 0x17:
        case 0x1F:
            opRotateA(opcode);
        break;

        case 0x08:
            opLD16SP(opcode);
        break;

        case 0x09:
        case 0x19:
        case 0x29:
        case 0x39:
            opADD1616(opcode);
        break;

        case 0x18:
            opJR(opcode);
        break;

        case 0x20:
        case 0x28:
        case 0x30:
        case 0x38:
            opJRCO(opcode);
        break;

        case 0x22:
        case 0x2A:
        case 0x32:
        case 0x3A:
            opLDHLI(opcode);
        break;

        case 0x27:
            opDAA(opcode);
        break;

        case 0x2F:
            opCPL(opcode);
        break;

        case 0x37:
            opSCF(opcode);
        break;

        case 0x3F:
            opCCF(opcode);
        break;

        case 0x40 ... 0x7F:
            if(opcode == 0x76)
            {
                // HALT
                PC++;
                haltMode = true;
            }
            else
            {
                opLDRR(opcode);
            }
        break;

        case 0x80 ... 0x87:
            opADD(opcode);
        break;

        case 0x88 ... 0x8F:
            opADDC(opcode);
        break;

        case 0x90 ... 0x97:
        case 0xB8 ... 0xBF:
            opSUB(opcode);
        break;

        case 0x98 ... 0x9F:
            opSUBC(opcode);
        break;

        case 0xA0 ... 0xB7:
            opBITWISE(opcode);
        break;

        case 0xC0:
        case 0xC8:
        case 0xD0:
        case 0xD8:
            opRETCO(opcode);
        break;

        case 0xC1:
        case 0xD1:
        case 0xE1:
        case 0xF1:
            opPOP(opcode);
        break;

        case 0xC2:
        case 0xCA:
        case 0xD2:
        case 0xDA:
            opJPCO(opcode);
        break;

        case 0xC3:
            opJP(opcode);
        break;

        case 0xC4:
        case 0xCC:
        case 0xD4:
        case 0xDC:
            opCALLCO(opcode);
        break;

        case 0xC5:
        case 0xD5:
        case 0xE5:
        case 0xF5:
            opPUSH(opcode);
        break;

        case 0xC6:
            opADDN(opcode);
        break;

        case 0xC7:
        case 0xCF:
        case 0xD7:
        case 0xDF:
        case 0xE7:
        case 0xEF:
        case 0xF7:
        case 0xFF:
            opRST(opcode);
        break;

        case 0xC9:
            opRET(opcode);
        break;

        case 0xCB:
            switch(readRAM(PC + 1))
            {
                case 0x00 ... 0x3F:
                    opCBSHIFT(readRAM(PC + 1));
                break;

                case 0x40 ... 0x7F:
                    opCBBIT(readRAM(PC + 1));
                break;

                case 0x80 ... 0xBF:
                    opCBRES(readRAM(PC + 1));
                break;

                case 0xC0 ... 0xFF:
                    opCBSET(readRAM(PC + 1));
                break;

                default:
                    opUNK(opcode);
                break;
            }
        break;

        case 0xCD:
            opCALL(opcode);
        break;

        case 0xCE:
            opADDCN(opcode);
        break;

        case 0xD6:
        case 0xFE:
            opCPN(opcode);
        break;

        case 0xD9:
            opRETI(opcode);
        break;

        case 0xDE:
            opSUBCN(opcode);
        break;

        case 0xE0:
        case 0xF0:
            opLDFF(opcode);
        break;

        case 0xE2:
        case 0xF2:
            opLDFFCA(opcode);
        break;

        case 0xE6:
        case 0xEE:
        case 0xF6:
            opBITWISEN(opcode);
        break;

        case 0xE8:
            opADDSP8(opcode);
        break;

        case 0xE9:
            opJPHL(opcode);
        break;

        case 0xEA:
        case 0xFA:
            opLD16A(opcode);
        break;

        case 0xF3:
            opDI(opcode);
        break;

        case 0xF8:
            opADDHLSP8(opcode);
        break;

        case 0xF9:
            opLDSPHL(opcode);
        break;

        case 0xFB:
            opEI(opcode);
        break;

        default:
            opUNK(opcode);
        break;
    }
}

uint16_t gbClass::incDec16(uint8_t upper, uint8_t lower, bool subtract)
{
    uint8_t lowerBack = lower;
    if(!subtract)
    {
        lower++;
        if(lower < lowerBack)
        {
            upper++;
        }
    }
    else
    {
        lower--;
        if(lower > lowerBack)
        {
            upper--;
        }
    }
    return (upper << 8) | lower;
}

void gbClass::setFlagBit(uint8_t index, bool value)
{
    uint8_t valueSet = 1 << index;

    if(value == true)
    {
        F |= valueSet;
    }
    else
    {
        F &= ~valueSet;
    }
}

uint8_t gbClass::incDec8(uint8_t value, bool subtract, bool modifyFlags)
{
    if(!subtract)
    {
        value++;
        if(modifyFlags)
        {
            bool zeroFlag = value == 0;
            setFlagBit(7, zeroFlag);
            setFlagBit(6, 0);
            bool halfCarry = (((((value - 1) & 0xF) + (1 & 0xF)) & 0x10) == 0x10);
            setFlagBit(5, halfCarry);
        }
    }
    if(subtract)
    {
        value--;
        if(modifyFlags)
        {
            bool zeroFlag = value == 0;
            setFlagBit(7, zeroFlag);
            setFlagBit(6, 1);
            bool halfCarry = (((((value + 1) & 0xF) - (1 & 0xF)) & 0x10) == 0x10);
            setFlagBit(5, halfCarry);
        }
    }
    return value;
}

void gbClass::writeRAM(uint16_t location, uint8_t value)
{
    if(location < 0x4000)
    {
        if(location >= 0x2000 && location <= 0x3FFF)
        {
            swapBank(1, value);
        }
        //cout<<"Tried to write to rom in bank 0"<<endl;
    }
    else if(location < 0x8000)
    {
        //cout<<"Tried to write to rom in bank 1"<<endl;
    }
    else if(location < 0xA000)
    {
        VRAM[0][location & 0x1FFF] = value; // Banking goes here later
    }
    else if(location < 0xC000)
    {
        SRAM[0][location & 0x1FFF] = value;
    }
    else if(location < 0xD000)
    {
        WRAM[0][location & 0xFFF] = value;
    }
    else if(location < 0xE000)
    {
        WRAM[1][location & 0xFFF] = value; // Banking goes here later
    }
    else if(location < 0xF000)
    {
        WRAM[0][location & 0xFFF] = value;
    }
    else if(location < 0xFE00)
    {
        WRAM[1][location & 0xFFF] = value; // Banking goes here later
    }
    else if(location < 0xFEA0)
    {
        OAM[location % 0xA0] = value; // TODO: Remove Modulo
    }
    else if(location < 0xFF00)
    {
        //return 0; // BAD RAM
    }
    else if(location < 0xFF80)
    {
        //return 0; // IO REGS
        if(location == 0xFF01)
        {
            cout<<value;
        }
    }
    else if(location < 0xFFFF)
    {
        HRAM[location & 0x7F] = value;
    }
    else
    {
        IE = value;
    }
}

uint8_t gbClass::readRAM(uint16_t location)
{
    if(location < 0x4000)
    {
        return ROM[0][location & 0x3FFF];
    }
    else if(location < 0x8000)
    {
        return ROM[1][location & 0x3FFF]; // Banking goes here later
    }
    else if(location < 0xA000)
    {
        return VRAM[0][location & 0x1FFF]; // Banking goes here later
    }
    else if(location < 0xC000)
    {
        return SRAM[0][location & 0x1FFF];
    }
    else if(location < 0xD000)
    {
        return WRAM[0][location & 0xFFF];
    }
    else if(location < 0xE000)
    {
        return WRAM[1][location & 0xFFF]; // Banking goes here later
    }
    else if(location < 0xF000)
    {
        return WRAM[0][location & 0xFFF];
    }
    else if(location < 0xFE00)
    {
        return WRAM[1][location & 0xFFF]; // Banking goes here later
    }
    else if(location < 0xFEA0)
    {
        return OAM[location % 0xA0]; // TODO: Remove Modulo
    }
    else if(location < 0xFF00)
    {
        return 0; // BAD RAM
    }
    else if(location < 0xFF80)
    {
        if(location == 0xFF44)
        {
            return LY;
        }
        else
        {
            return 0xFF; // IO REGS
        }

    }
    else if(location < 0xFFFF)
    {
        return HRAM[location & 0x7F];
    }
    else
    {
        return IE;
    }
}

void gbClass::opLD16A(uint8_t opcode)
{
    uint16_t location = (readRAM(PC + 2) << 8) | readRAM(PC + 1);
    bool load = (opcode >> 4) & 0x1;

    if(load)
    {
        A = readRAM(location);
    }
    else
    {
        writeRAM(location, A);
    }

    PC += 3;
    cyclesScanline += 12;
}

void gbClass::opNOP(uint8_t opcode)
{
    PC++;
    cyclesScanline += 4;
}

void gbClass::opJR(uint8_t opcode)
{
    int8_t value = readRAM(PC + 1);
    PC += value;

    PC += 2;
    cyclesScanline += 12;
}

void gbClass::opRET(uint8_t opcode)
{
    PC = readRAM(SP + 1) << 8 | readRAM(SP);
    SP += 2;
    cyclesScanline += 16;
}

void gbClass::opRST(uint8_t opcode)
{
    uint8_t jumpVec = (opcode >> 3) & 0x7;

    PC += 1;
    SP--;
    writeRAM(SP, PC >> 8);
    SP--;
    writeRAM(SP, PC);

    PC = jumpVec * 8;

    cyclesScanline += 16;
}

void gbClass::opRETI(uint8_t opcode)
{
    PC = readRAM(SP + 1) << 8 | readRAM(SP);
    SP += 2;
    IME = true;
    cyclesScanline += 16;
}

void gbClass::opBITWISEN(uint8_t opcode)
{
    uint8_t op, value;
    op = (opcode >> 3) & 0x3;

    value = readRAM(PC + 1);

    switch(op)
    {
        case 0:
            A &= value;
            setFlagBit(5, 1);
        break;

        case 1:
            A ^= value;
            setFlagBit(5, 0);
        break;

        case 2:
            A |= value;
            setFlagBit(5, 0);
        break;
    }

    setFlagBit(7, A == 0);
    setFlagBit(6, 0);
    setFlagBit(4, 0);

    PC += 2;
    cyclesScanline += 8;
}

void gbClass::opBITWISE(uint8_t opcode)
{
    uint8_t reg, op, value;
    reg = opcode & 0x7;
    op = (opcode >> 3) & 0x3;

    switch(reg)
    {
        case 0:
            value = B;
        break;
        case 1:
            value = C;
        break;
        case 2:
            value = D;
        break;
        case 3:
            value = E;
        break;
        case 4:
            value = H;
        break;
        case 5:
            value = L;
        break;
        case 6:
            value = readRAM((H << 8) | L);
        break;
        case 7:
            value = A;
        break;
    }

    switch(op)
    {
        case 0:
            A &= value;
            setFlagBit(5, 1);
        break;

        case 1:
            A ^= value;
            setFlagBit(5, 0);
        break;

        case 2:
            A |= value;
            setFlagBit(5, 0);
        break;
    }

    setFlagBit(7, A == 0);
    setFlagBit(6, 0);
    setFlagBit(4, 0);

    PC++;
    cyclesScanline += 4;
}

void gbClass::opCPN(uint8_t opcode)
{
    uint8_t reg, value;
    bool writeback;
    reg = opcode & 0x7;
    writeback = !((opcode >> 3) & 0x1);


    value = readRAM(PC + 1);

    uint8_t value2 = A - value;


    setFlagBit(7, value2 == 0);
    setFlagBit(6, 1);

    bool halfCarry = (((((A) & 0xF) - (value2 & 0xF)) & 0x10) == 0x10);
    setFlagBit(5, halfCarry);

    setFlagBit(4, value2 > A);

    if(writeback)
    {
        A = value2;
    }

    PC += 2;
    cyclesScanline += 8;
}

void gbClass::opADDN(uint8_t opcode)
{
    uint8_t reg, value;
    reg = opcode & 0x7;

    value = readRAM(PC + 1);

    uint8_t value2 = A + value;


    setFlagBit(7, value2 == 0);
    setFlagBit(6, 0);

    bool halfCarry = (((((A) & 0xF) + (value & 0xF)) & 0x10) == 0x10);
    setFlagBit(5, halfCarry);

    setFlagBit(4, value2 < A);

    A = value2;

    PC += 2;
    cyclesScanline += 8;
}

void gbClass::opADDCN(uint8_t opcode)
{
    uint8_t reg, value;
    reg = opcode & 0x7;

    value = readRAM(PC + 1);

    bool carry = (F & 0x10) != 0;

    uint8_t value2 = A + value + carry;


    setFlagBit(7, value2 == 0);
    setFlagBit(6, 0);

    setFlagBit(5, ((((A & 0xF) + (value & 0xF) + (carry & 0xF)) & 0x10) == 0x10));

    setFlagBit(4, (((uint16_t)A + (uint16_t)value + (uint16_t)carry) >= 0x100));

    A = value2;

    PC += 2;
    cyclesScanline += 8;
}

void gbClass::opADD(uint8_t opcode)
{
    uint8_t reg, value;
    reg = opcode & 0x7;

    switch(reg)
    {
        case 0:
            value = B;
        break;
        case 1:
            value = C;
        break;
        case 2:
            value = D;
        break;
        case 3:
            value = E;
        break;
        case 4:
            value = H;
        break;
        case 5:
            value = L;
        break;
        case 6:
            value = readRAM((H << 8) | L);
        break;
        case 7:
            value = A;
        break;
    }

    uint8_t value2 = A + value;


    setFlagBit(7, value2 == 0);
    setFlagBit(6, 0);

    bool halfCarry = (((((A) & 0xF) + (value & 0xF)) & 0x10) == 0x10);
    setFlagBit(5, halfCarry);

    setFlagBit(4, value2 < A);

    A = value2;

    PC++;
    cyclesScanline += 4;
}

void gbClass::opADDC(uint8_t opcode)
{
    uint8_t reg, value;
    reg = opcode & 0x7;

    switch(reg)
    {
        case 0:
            value = B;
        break;
        case 1:
            value = C;
        break;
        case 2:
            value = D;
        break;
        case 3:
            value = E;
        break;
        case 4:
            value = H;
        break;
        case 5:
            value = L;
        break;
        case 6:
            value = readRAM((H << 8) | L);
        break;
        case 7:
            value = A;
        break;
    }

    bool carry = (F & 0x10) != 0;

    uint8_t value2 = A + value + carry;


    setFlagBit(7, value2 == 0);
    setFlagBit(6, 0);

    setFlagBit(5, ((((A & 0xF) + (value & 0xF) + (carry & 0xF)) & 0x10) == 0x10));

    setFlagBit(4, (((uint16_t)A + (uint16_t)value + (uint16_t)carry) >= 0x100));

    A = value2;

    PC++;
    cyclesScanline += 4;
}

void gbClass::opSUBCN(uint8_t opcode)
{
    uint8_t reg, value;
    bool writeback;
    reg = opcode & 0x7;

    value = readRAM(PC + 1);

    uint8_t value2 = A - (value + ((F & 0x10) != 0));


    setFlagBit(7, value2 == 0);
    setFlagBit(6, 1);
    // Todo: Optimize this
    setFlagBit(5, (((((uint16_t)A & 0xF) - (uint16_t)(((uint16_t)value & 0xF) + ((uint16_t)((F & 0x10) != 0) & 0xF))) & 0x10) == 0x10));

    setFlagBit(4, (((((uint16_t)A & 0xFF) - (uint16_t)(((uint16_t)value & 0xFF) + ((uint16_t)((F & 0x10) != 0) & 0xFF))) & 0x100) == 0x100));
    A = value2;

    PC += 2;
    cyclesScanline += 8;
}

void gbClass::opSUBC(uint8_t opcode)
{
    uint8_t reg, value;
    bool writeback;
    reg = opcode & 0x7;

    switch(reg)
    {
        case 0:
            value = B;
        break;
        case 1:
            value = C;
        break;
        case 2:
            value = D;
        break;
        case 3:
            value = E;
        break;
        case 4:
            value = H;
        break;
        case 5:
            value = L;
        break;
        case 6:
            value = readRAM((H << 8) | L);
        break;
        case 7:
            value = A;
        break;
    }

    uint8_t value2 = A - (value + ((F & 0x10) != 0));


    setFlagBit(7, value2 == 0);
    setFlagBit(6, 1);
    // Todo: Optimize this
    setFlagBit(5, (((((uint16_t)A & 0xF) - (uint16_t)(((uint16_t)value & 0xF) + ((uint16_t)((F & 0x10) != 0) & 0xF))) & 0x10) == 0x10));

    setFlagBit(4, (((((uint16_t)A & 0xFF) - (uint16_t)(((uint16_t)value & 0xFF) + ((uint16_t)((F & 0x10) != 0) & 0xFF))) & 0x100) == 0x100));
    A = value2;

    PC++;
    cyclesScanline += 4;
}

void gbClass::opSUB(uint8_t opcode)
{
    uint8_t reg, value;
    bool writeback;
    reg = opcode & 0x7;
    writeback =  !((opcode >> 3) & 0x1);

    switch(reg)
    {
        case 0:
            value = B;
        break;
        case 1:
            value = C;
        break;
        case 2:
            value = D;
        break;
        case 3:
            value = E;
        break;
        case 4:
            value = H;
        break;
        case 5:
            value = L;
        break;
        case 6:
            value = readRAM((H << 8) | L);
        break;
        case 7:
            value = A;
        break;
    }

    uint8_t value2 = A - value;


    setFlagBit(7, value2 == 0);
    setFlagBit(6, 1);

    bool halfCarry = (((((A) & 0xF) - (value2 & 0xF)) & 0x10) == 0x10);
    setFlagBit(5, halfCarry);

    setFlagBit(4, value2 > A);

    if(writeback)
    {
        A = value2;
    }

    PC++;
    cyclesScanline += 4;
}

void gbClass::opLD16(uint8_t opcode)
{
    uint8_t valueHigh = readRAM(PC + 2);
    uint8_t valueLow = readRAM(PC + 1);

    switch(opcode >> 4)
    {
        case 0:
            B = valueHigh;
            C = valueLow;
        break;

        case 1:
            D = valueHigh;
            E = valueLow;
        break;

        case 2:
            H = valueHigh;
            L = valueLow;
        break;

        case 3:
            SP = (valueHigh << 8) | valueLow;
        break;
    }

    PC += 3;
    cyclesScanline += 12;
}

void gbClass::opLDRV(uint8_t opcode)
{
    uint8_t value = readRAM(PC + 1);
    uint8_t op2;
    op2 = (opcode >> 3) & 0x7;

    switch(op2)
    {
        case 0:
            B = value;
        break;
        case 1:
            C = value;
        break;
        case 2:
            D = value;
        break;
        case 3:
            E = value;
        break;
        case 4:
            H = value;
        break;
        case 5:
            L = value;
        break;
        case 6:
            writeRAM((H << 8) | L, value);
        break;
        case 7:
            A = value;
        break;
    }

    PC += 2;
    cyclesScanline += 8;
}

void gbClass::opIDR(uint8_t opcode)
{
    bool dec = opcode & 0x1;
    uint8_t op2;
    op2 = (opcode >> 3) & 0x7;

    switch(op2)
    {
        case 0:
            B = incDec8(B, dec, true);
        break;
        case 1:
            C = incDec8(C, dec, true);
        break;
        case 2:
            D = incDec8(D, dec, true);
        break;
        case 3:
            E = incDec8(E, dec, true);
        break;
        case 4:
            H = incDec8(H, dec, true);
        break;
        case 5:
            L = incDec8(L, dec, true);
        break;
        case 6:
            writeRAM((H << 8) | L, incDec8(readRAM((H << 8) | L), dec, true));
        break;
        case 7:
            A = incDec8(A, dec, true);
        break;
    }
    PC++;
    cyclesScanline += 4;
}

void gbClass::opLDRR(uint8_t opcode)
{
    uint8_t value;
    uint8_t op1, op2;
    op1 = opcode & 0x7;
    op2 = (opcode >> 3) & 0x7;

    switch(op1)
    {
        case 0:
            value = B;
        break;
        case 1:
            value = C;
        break;
        case 2:
            value = D;
        break;
        case 3:
            value = E;
        break;
        case 4:
            value = H;
        break;
        case 5:
            value = L;
        break;
        case 6:
            value = readRAM((H << 8) | L);
        break;
        case 7:
            value = A;
        break;
    }

    switch(op2)
    {
        case 0:
            B = value;
        break;
        case 1:
            C = value;
        break;
        case 2:
            D = value;
        break;
        case 3:
            E = value;
        break;
        case 4:
            H = value;
        break;
        case 5:
            L = value;
        break;
        case 6:
            writeRAM((H << 8) | L, value);
        break;
        case 7:
            A = value;
        break;
    }

    PC++;
    cyclesScanline += 4;
}

void gbClass::opLDR16(uint8_t opcode)
{
    uint16_t temp16;
    bool read = (opcode >> 3) & 0x1;
    bool useDE = (opcode >> 4) & 0x1;

    if(!useDE)
    {
        temp16 = (B << 8) | C;
    }
    else
    {
        temp16 = (D << 8) | E;
    }

    if(read)
    {
        A = readRAM(temp16);
    }
    else
    {
        writeRAM(temp16, A);
    }

    PC++;
    cyclesScanline += 8;
}

void gbClass::opLDHLI(uint8_t opcode)
{
    uint16_t temp16 = (H << 8) | L;
    bool read = (opcode >> 3) & 0x1;
    bool dec = (opcode >> 4) & 0x1;

    if(read)
    {
        A = readRAM(temp16);
    }
    else
    {
        writeRAM(temp16, A);
    }

    temp16 = incDec16(H, L, dec);
    H = temp16 >> 8;
    L = temp16;

    PC++;
    cyclesScanline += 8;
}

void gbClass::opLDFFCA(uint8_t opcode)
{
    uint16_t location = 0xFF00 + C;
    bool read = (opcode >> 4) & 0x1;
    if(read)
    {
        A = readRAM(location);
    }
    else
    {
        writeRAM(location, A);
    }

    PC += 1;
    cyclesScanline += 8;
}

void gbClass::opLDFF(uint8_t opcode)
{
    uint16_t location = 0xFF00 + readRAM(PC + 1);
    bool read = (opcode >> 4) & 0x1;
    if(read)
    {
        A = readRAM(location);
    }
    else
    {
        writeRAM(location, A);
    }

    PC += 2;
    cyclesScanline += 12;
}

void gbClass::opJPCO(uint8_t opcode)
{
    uint8_t cond = (opcode >> 3) & 0x3;
    bool condOutcome;
    switch(cond)
    {
        case 0:
            condOutcome = (F & 0x80) == 0;
        break;

        case 1:
            condOutcome = (F & 0x80) != 0;
        break;

        case 2:
            condOutcome = (F & 0x10) == 0;
        break;

        case 3:
            condOutcome = (F & 0x10) != 0;
        break;
    }

    if(condOutcome)
    {
        PC = (readRAM(PC + 2) << 8) | readRAM(PC + 1);
        cyclesScanline += 16;
    }
    else
    {
        PC += 3;
        cyclesScanline += 12;
    }
}

void gbClass::opCALLCO(uint8_t opcode)
{
    uint8_t cond = (opcode >> 3) & 0x3;
    bool condOutcome;
    switch(cond)
    {
        case 0:
            condOutcome = (F & 0x80) == 0;
        break;

        case 1:
            condOutcome = (F & 0x80) != 0;
        break;

        case 2:
            condOutcome = (F & 0x10) == 0;
        break;

        case 3:
            condOutcome = (F & 0x10) != 0;
        break;
    }

    if(condOutcome)
    {
        PC += 3;
        SP--;
        writeRAM(SP, PC >> 8);
        SP--;
        writeRAM(SP, PC);
        PC -= 3;


        PC = (readRAM(PC + 2) << 8) | readRAM(PC + 1);
        cyclesScanline += 24;
    }
    else
    {
        PC += 3;
        cyclesScanline += 12;
    }
}

void gbClass::opRETCO(uint8_t opcode)
{
    uint8_t cond = (opcode >> 3) & 0x3;
    bool condOutcome;
    switch(cond)
    {
        case 0:
            condOutcome = (F & 0x80) == 0;
        break;

        case 1:
            condOutcome = (F & 0x80) != 0;
        break;

        case 2:
            condOutcome = (F & 0x10) == 0;
        break;

        case 3:
            condOutcome = (F & 0x10) != 0;
        break;
    }

    if(condOutcome)
    {
        PC = readRAM(SP + 1) << 8 | readRAM(SP);
        SP += 2;
        cyclesScanline += 20;
    }
    else
    {
        PC += 1;
        cyclesScanline += 8;
    }
}

void gbClass::opJRCO(uint8_t opcode)
{
    uint8_t cond = (opcode >> 3) & 0x3;
    bool condOutcome;
    int8_t value = readRAM(PC + 1);
    switch(cond)
    {
        case 0:
            condOutcome = (F & 0x80) == 0;
        break;

        case 1:
            condOutcome = (F & 0x80) != 0;
        break;

        case 2:
            condOutcome = (F & 0x10) == 0;
        break;

        case 3:
            condOutcome = (F & 0x10) != 0;
        break;
    }

    if(condOutcome)
    {
        PC += value;
        PC += 2;
        cyclesScanline += 12;
    }
    else
    {
        PC += 2;
        cyclesScanline += 8;
    }
}

void gbClass::opDI(uint8_t opcode)
{
    IME = false;
    PC++;
    cyclesScanline += 4;
}

void gbClass::opEI(uint8_t opcode)
{
    IME = true;
    PC++;
    cyclesScanline += 4;
}


void gbClass::opJP(uint8_t opcode)
{
    PC = (readRAM(PC + 2) << 8) | readRAM(PC + 1);
    cyclesScanline += 16;
}

void gbClass::opCALL(uint8_t opcode)
{
    PC += 3;
    SP--;
    writeRAM(SP, PC >> 8);
    SP--;
    writeRAM(SP, PC);
    PC -= 3;


    PC = (readRAM(PC + 2) << 8) | readRAM(PC + 1);
    cyclesScanline += 24;
}

void gbClass::opPUSH(uint8_t opcode)
{
    switch((opcode >> 4) & 0x3)
    {
        case 0:
            SP--;
            writeRAM(SP, B);
            SP--;
            writeRAM(SP, C);
        break;

        case 1:
            SP--;
            writeRAM(SP, D);
            SP--;
            writeRAM(SP, E);
        break;

        case 2:
            SP--;
            writeRAM(SP, H);
            SP--;
            writeRAM(SP, L);
        break;

        case 3:
            SP--;
            writeRAM(SP, A);
            SP--;
            writeRAM(SP, F);
        break;
    }

    PC += 1;
    cyclesScanline += 16;
}

void gbClass::opPOP(uint8_t opcode)
{
    switch((opcode >> 4) & 0x3)
    {
        case 0:
            C = readRAM(SP);
            SP++;
            B = readRAM(SP);
            SP++;
        break;

        case 1:
            E = readRAM(SP);
            SP++;
            D = readRAM(SP);
            SP++;
        break;

        case 2:
            L = readRAM(SP);
            SP++;
            H = readRAM(SP);
            SP++;
        break;

        case 3:
            F = readRAM(SP) & 0xF0;
            SP++;
            A = readRAM(SP);
            SP++;
        break;
    }

    PC += 1;
    cyclesScanline += 12;
}

void gbClass::opIDR16(uint8_t opcode)
{
    uint8_t var = (opcode >> 4) & 0x3;
    bool sub = (opcode >> 3) & 0x1;
    uint16_t endValue;
    switch(var)
    {
        case 0:
            endValue = incDec16(B, C, sub);
            B = (endValue >> 8);
            C = endValue;
        break;

        case 1:
            endValue = incDec16(D, E, sub);
            D = (endValue >> 8);
            E = endValue;
        break;

        case 2:
            endValue = incDec16(H, L, sub);
            H = (endValue >> 8);
            L = endValue;
        break;

        case 3:
            if(!sub)
            {
                SP++;
            }
            else
            {
                SP--;
            }
        break;
    }
    PC++;
    cyclesScanline += 8;
}

void gbClass::opRotateA(uint8_t opcode)
{
    uint8_t op = (opcode >> 3) & 0x3;
    uint8_t oldA = A;

    switch(op)
    {
        case 0:
            // When you get back to this
            A = (A << 1) | (A >> 7);
            setFlagBit(4, (oldA & 0x80) != 0);
        break;

        case 1:
            A = (A << 7) | (A >> 1);
            setFlagBit(4, oldA & 0x1);
        break;

        case 2:
            A = (A << 1) | ((F & 0x10) != 0);
            setFlagBit(4, (oldA & 0x80) != 0);
        break;

        case 3:
            A = (((F & 0x10) != 0) << 7) | (A >> 1);
            setFlagBit(4, oldA & 0x1);
        break;
    }
    setFlagBit(7, 0);
    setFlagBit(6, 0);
    setFlagBit(5, 0);
    PC++;
    cyclesScanline += 4;

}

void gbClass::opUNK(uint8_t opcode)
{
    // Uhoh
    cout<<"Unknown Opcode: 0x" << std::hex << (int)opcode << endl;
    runGB = false;
}

void gbClass::opCBSHIFT(uint8_t opcode)
{
    uint8_t reg = opcode & 0x7;
    uint8_t op = (opcode >> 3) & 0x7;
    uint8_t value;

    switch(reg)
    {
        case 0:
            value = B;
        break;
        case 1:
            value = C;
        break;
        case 2:
            value = D;
        break;
        case 3:
            value = E;
        break;
        case 4:
            value = H;
        break;
        case 5:
            value = L;
        break;
        case 6:
            value = readRAM((H << 8) | L);
        break;
        case 7:
            value = A;
        break;
    }

    switch(op)
    {
        case 0: // Good
            setFlagBit(4, (value & 0x80) != 0);
            value = (value >> 7) | (value << 1);
            setFlagBit(7, value == 0);
            setFlagBit(6, 0);
            setFlagBit(5, 0);
        break;

        case 1: // Good
            setFlagBit(4, value & 0x1);
            value = (value << 7) | (value >> 1);
            setFlagBit(7, value == 0);
            setFlagBit(6, 0);
            setFlagBit(5, 0);
        break;

        case 2: // Good
        {
            uint8_t value2 = value;

            value = (value << 1) | ((F & 0x10) != 0);
            setFlagBit(4, (value2 & 0x80) != 0);
            setFlagBit(7, value == 0);
            setFlagBit(6, 0);
            setFlagBit(5, 0);
        }
        break;

        case 3: // Good
        {
            uint8_t value2 = value;

            value = (((F & 0x10) != 0) << 7) | (value >> 1);
            setFlagBit(4, value2 & 0x1);
            setFlagBit(7, value == 0);
            setFlagBit(6, 0);
            setFlagBit(5, 0);
        }
        break;

        case 4: // Good
            setFlagBit(4, (value & 0x80) != 0);
            value = (value << 1);
            setFlagBit(7, value == 0);
            setFlagBit(6, 0);
            setFlagBit(5, 0);
        break;

        case 5: // Good
            setFlagBit(4, value & 0x1);
            value = (value & 0x80) | (value >> 1);
            setFlagBit(7, value == 0);
            setFlagBit(6, 0);
            setFlagBit(5, 0);
        break;

        case 6:
            value = (value << 4) | (value >> 4);
            setFlagBit(7, value == 0);
            setFlagBit(6, 0);
            setFlagBit(5, 0);
            setFlagBit(4, 0);
        break;

        case 7:
            setFlagBit(4, value & 0x1);
            value = value >> 1;
            setFlagBit(7, value == 0);
            setFlagBit(6, 0);
            setFlagBit(5, 0);
        break;
    }

    switch(reg)
    {
        case 0:
            B = value;
        break;
        case 1:
            C = value;
        break;
        case 2:
            D = value;
        break;
        case 3:
            E = value;
        break;
        case 4:
            H = value;
        break;
        case 5:
            L = value;
        break;
        case 6:
            writeRAM((H << 8) | L, value);
            cyclesScanline += 8;
        break;
        case 7:
            A = value;
        break;
    }

    PC += 2;
    cyclesScanline += 8;
}

void gbClass::opADDHLSP8(uint8_t opcode)
{
    uint8_t reg = (opcode >> 4) & 0x3;
    uint16_t value2;
    int8_t value1;

    value2 = SP;
    value1 = readRAM(PC + 1);

    uint16_t value3 = value2 + value1;

    setFlagBit(7, 0);
    setFlagBit(6, 0);


    bool halfCarry = ((((((uint8_t)value2) & 0xF) + ((uint8_t)value1 & 0xF)) & 0x10) == 0x10);
    setFlagBit(5, halfCarry);

    setFlagBit(4, (uint8_t)value3 < (uint8_t)value2);

    H = value3 >> 8;
    L = value3;

    PC += 2;
    cyclesScanline += 12;
}

void gbClass::opADDSP8(uint8_t opcode)
{
    uint8_t reg = (opcode >> 4) & 0x3;
    uint16_t value2;
    int8_t value1;

    value2 = SP;
    value1 = readRAM(PC + 1);

    uint16_t value3 = value2 + value1;

    setFlagBit(7, 0);
    setFlagBit(6, 0);


    bool halfCarry = ((((((uint8_t)value2) & 0xF) + ((uint8_t)value1 & 0xF)) & 0x10) == 0x10);
    setFlagBit(5, halfCarry);

    setFlagBit(4, (uint8_t)value3 < (uint8_t)value2);

    SP = value3;

    PC += 2;
    cyclesScanline += 16;
}

void gbClass::opADD1616(uint8_t opcode)
{
    uint8_t reg = (opcode >> 4) & 0x3;
    uint16_t value1, value2;

    value2 = (H << 8) | L;

    switch(reg)
    {
        case 0:
            value1 = (B << 8) | C;
        break;

        case 1:
            value1 = (D << 8) | E;
        break;

        case 2:
            value1 = (H << 8) | L;
        break;

        case 3:
            value1 = SP;
        break;
    }

    uint16_t value3 = value2 + value1;

    setFlagBit(6, 0);


    bool halfCarry = (((((value2) & 0xFFF) + (value1 & 0xFFF)) & 0x1000) == 0x1000);
    setFlagBit(5, halfCarry);

    setFlagBit(4, value3 < value2);

    H = value3 >> 8;
    L = value3;

    PC++;
    cyclesScanline += 8;
}

void gbClass::opLD16SP(uint8_t opcode)
{
    uint16_t location = (readRAM(PC + 2) << 8) | readRAM(PC + 1);

    writeRAM(location + 1, SP >> 8);
    writeRAM(location, SP);

    PC += 3;
    cyclesScanline += 20;
}

void gbClass::opJPHL(uint8_t opcode)
{
    PC = (H << 8) | L;
    cyclesScanline += 8;
}

void gbClass::opCPL(uint8_t opcode)
{
    A = ~A;

    setFlagBit(6, 1);
    setFlagBit(5, 1);

    PC++;
    cyclesScanline += 4;
}

void gbClass::opSCF(uint8_t opcode)
{

    setFlagBit(6, 0);
    setFlagBit(5, 0);
    setFlagBit(4, 1);

    PC++;
    cyclesScanline += 4;
}

void gbClass::opCCF(uint8_t opcode)
{

    setFlagBit(6, 0);
    setFlagBit(5, 0);
    setFlagBit(4, !((F & 0x10) != 0));

    PC++;
    cyclesScanline += 4;
}

void gbClass::opCBRES(uint8_t opcode)
{
    uint8_t bit = (opcode >> 3) & 0x7;
    uint8_t reg = (opcode) & 0x7;

    uint8_t value = 1 << bit;
    uint8_t value2;

    switch(reg)
    {
        case 0:
            B &= ~value;
        break;
        case 1:
            C &= ~value;
        break;
        case 2:
            D &= ~value;
        break;
        case 3:
            E &= ~value;
        break;
        case 4:
            H &= ~value;
        break;
        case 5:
            L &= ~value;
        break;
        case 6:
            writeRAM((H << 8) | L, readRAM((H << 8) | L) & ~value);
            cyclesScanline += 8;
        break;
        case 7:
            A &= ~value;
        break;
    }

    PC += 2;
}

void gbClass::opCBBIT(uint8_t opcode)
{
    uint8_t bit = (opcode >> 3) & 0x7;
    uint8_t reg = (opcode) & 0x7;

    uint8_t value2 = 1 << bit;
    uint8_t value;

    switch(reg)
    {
        case 0:
            value = B;
        break;
        case 1:
            value = C;
        break;
        case 2:
            value = D;
        break;
        case 3:
            value = E;
        break;
        case 4:
            value = H;
        break;
        case 5:
            value = L;
        break;
        case 6:
            value = readRAM((H << 8) | L);
            cyclesScanline += 4;
        break;
        case 7:
            value = A;
        break;
    }

    setFlagBit(7, (value & value2) == 0);
    setFlagBit(6, 0);
    setFlagBit(5, 1);

    PC += 2;
    cyclesScanline += 8;
}

void gbClass::opCBSET(uint8_t opcode)
{
    uint8_t bit = (opcode >> 3) & 0x7;
    uint8_t reg = (opcode) & 0x7;

    uint8_t value = 1 << bit;
    uint8_t value2;

    switch(reg)
    {
        case 0:
            B |= value;
        break;
        case 1:
            C |= value;
        break;
        case 2:
            D |= value;
        break;
        case 3:
            E |= value;
        break;
        case 4:
            H |= value;
        break;
        case 5:
            L |= value;
        break;
        case 6:
            writeRAM((H << 8) | L, readRAM((H << 8) | L) | value);
            cyclesScanline += 8;
        break;
        case 7:
            A |= value;
        break;
    }

    PC += 2;
}

void gbClass::opLDSPHL(uint8_t opcode)
{
    SP = (H << 8) | L;

    PC++;
    cyclesScanline += 8;
}

void gbClass::opDAA(uint8_t opcode)
{
    PC++;
    //uint8_t beforeHcheck = cpu.a;
    //uint8_t Fbitbuffer = cpu.f;
    bool Z,N,H,C;
    Z = ((F & 0x80) != 0);
    N = ((F & 0x40) != 0);
    H = ((F & 0x20) != 0);
    C = ((F & 0x10) != 0);

    if (N == 0) {
        if (C || A > 0x99) {
            A = (A + 0x60) & 0xFF;
            setFlagBit(4, 1);
        }
        if (H || (A & 0xF) > 0x9) {
            A = (A + 0x06) & 0xFF;
            setFlagBit(5, 0);
        }
    }
    else if (C == 1 && H == 1) {
        A = (A + 0x9A) & 0xFF;
        setFlagBit(5, 0);
    }
    else if (C == 1) {
        A = (A + 0xA0) & 0xFF;
    }
    else if (H == 1) {
        A = (A + 0xFA) & 0xFF;
        setFlagBit(5, 1);
    }
    if (A == 0x00)
    {
        setFlagBit(7, 1);
    }
    if (A != 0x00)
    {
        setFlagBit(7, 0);
    }
    setFlagBit(5, 0);
    cyclesScanline += 4;
}

void gbClass::runInterrupt(uint8_t intNumber)
{
    IF |= intNumber;

    if(IME && (IE & IF) != 0x00)
    {
        if(IF & 0x80)
        {
            IF &= 0x7F;
            SP--;
            writeRAM(SP, PC >> 8);
            SP--;
            writeRAM(SP, PC);
            PC = 0x40;
        }
    }
}

void gbClass::swapBank(uint8_t sectionNum, uint8_t bankNum)
{
    memcpy(ROM[sectionNum], &ROMFILE[0x4000 * bankNum], 0x4000);
}