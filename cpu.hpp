// Todo: Split this stuff into more files later
#ifndef CPU_HPP
#define CPU_HPP

struct MBC1Struct {
    uint8_t bankNumberLow;
    bool onSRAM;
    uint8_t bankNumberHigh;
    bool bankHighBehavior;
};

struct MBC3Struct {
    bool onSRAM;
    uint8_t bankNumberROM;
    uint8_t bankNumberRAM;
    uint8_t rtcLatchState;
    bool timerExists;
};

class gbClass {
    public:
        bool runGB;
        uint8_t A, B, C, D, E, F, H, L;
        uint16_t PC, SP;
        uint64_t cyclesScanline;
        uint64_t cyclesTotal;
        uint64_t cyclesTotalPrev;

        uint64_t currentTimerDifference;

        bool haltMode;

        bool loadROM(const char* filename);
        void resetGB();
        void runOpcode();

        uint8_t readRAM(uint16_t location);
        void writeRAM(uint16_t location, uint8_t value);

        uint8_t accessIO(uint8_t port, uint8_t value, bool write);

        void setFlagBit(uint8_t index, bool value);
        uint8_t incDec8(uint8_t value, bool subtract, bool modifyFlags);
        uint16_t incDec16(uint8_t upper, uint8_t lower, bool subtract);

        void setIFBit(uint8_t bit);

        void checkInterrupt();

        void swapBank(uint8_t sectionNum, uint8_t bankNum);
        void swapBankOld(uint8_t sectionNum, uint8_t bankNum);

        void runTimer();

        inline uint8_t readVRAM(uint16_t location)
        {
            return VRAM[0][location & 0x1FFF];
        }

        inline uint8_t readOAM(uint16_t location)
        {
            // Unsafe, but since this is only used in the renderer, it should be fine.
            return OAM[location & 0xFF];
        }


        // I/O Registers

        uint8_t JOYP; // 0xFF00
        uint8_t JOYPR;
        uint8_t JOYP2;
        uint8_t SB; // 0xFF01
        uint8_t SC; // 0xFF02

        uint8_t DIV; // 0xFF04
        uint16_t TIMA; // 0xFF05
        uint8_t TMA; // 0xFF06
        uint8_t TAC; // 0xFF07

        uint8_t IF; // 0xFF0F

        // Audio I/O Goes here, It'll be added later

        uint8_t NR10;
        uint8_t NR11;
        uint8_t NR12;
        uint8_t NR13;
        uint8_t NR14;

        uint8_t LCDC; // 0xFF40
        uint8_t STAT; // 0xFF41
        uint8_t SCY; // 0xFF42
        uint8_t SCX; // 0xFF43
        uint8_t LY; // 0xFF44
        uint8_t LYC; // 0xFF45
        uint8_t DMA; // 0xFF46
        uint8_t BGP; // 0xFF47   (GB Only)
        uint8_t OBP0; // 0xFF48  (GB Only)
        uint8_t OBP1; // 0xFF49  (GB Only)
        uint8_t WY; // 0xFF4A
        uint8_t WX; // 0xFF4B

        uint8_t VBK; // 0xFF4F (CGB Only)

        uint8_t HDMA1; // 0xFF51 (CGB Only)
        uint8_t HDMA2; // 0xFF52 (CGB Only)
        uint8_t HDMA3; // 0xFF53 (CGB Only)
        uint8_t HDMA4; // 0xFF54 (CGB Only)
        uint8_t HDMA5; // 0xFF55 (CGB Only)

        uint8_t BGPI; // 0xFF68 (CGB Only)
        uint8_t BGPD; // 0xFF69 (CGB Only)
        uint8_t OBPI; // 0xFF6A (CGB Only)
        uint8_t OBPD; // 0xFF6B (CGB Only)


        uint8_t IE; // 0xFFFF
        bool IME;

        uint8_t SRAM[16][0x2000];
    private:
        uint8_t* ROMFILE;
        int romLength;
        int MBC;
        void handleMBC(uint16_t location, uint8_t value);
        bool sramExists;
        uint8_t sramState;
        struct MBC1Struct mbc1;
        struct MBC3Struct mbc3;

        uint8_t ROM[2][0x4000]; // This would need to be expanded later
        uint8_t VRAM[8][0x2000];  // This is banked in preperation for GBC support

        uint8_t WRAM[8][0x1000]; // Bankable in preperation for GBC
        uint8_t OAM[0xA0];
        uint8_t HRAM[0x7F];



        void opNOP(uint8_t opcode);
        void opJRCO(uint8_t opcode);

        void opLD16(uint8_t opcode);

        void opLDRR(uint8_t opcode);
        void opLDFF(uint8_t opcode);

        void opCBSHIFT(uint8_t opcode);
        void opCBBIT(uint8_t opcode);
        void opEI(uint8_t opcode);
        void opDAA(uint8_t opcode);
        void opCBRES(uint8_t opcode);
        void opCBSET(uint8_t opcode);
        void opRotateA(uint8_t opcode);

        void opLDRV(uint8_t opcode);
        void opLDHLI(uint8_t opcode);
        void opLDR16(uint8_t opcode);
        void opIDR(uint8_t opcode);
        void opJPCO(uint8_t opcode);
        void opCALLCO(uint8_t opcode);
        void opRETCO(uint8_t opcode);
        void opLD16A(uint8_t opcode);
        void opCPN(uint8_t opcode);

        void opBITWISE(uint8_t opcode);
        void opBITWISEN(uint8_t opcode);
        void opSUB(uint8_t opcode);
        void opADD(uint8_t opcode);
        void opADDN(uint8_t opcode);
        void opADDC(uint8_t opcode);
        void opADDCN(uint8_t opcode);
        void opADD1616(uint8_t opcode);
        void opADDSP8(uint8_t opcode);
        void opADDHLSP8(uint8_t opcode);

        void opLDFFCA(uint8_t opcode);

        void opLD16SP(uint8_t opcode);
        void opLDSPHL(uint8_t opcode);

        void opSUBC(uint8_t opcode);
        void opSUBCN(uint8_t opcode);

        void opCPL(uint8_t opcode);
        void opSCF(uint8_t opcode);
        void opCCF(uint8_t opcode);

        void opJPHL(uint8_t opcode);

        void opCALL(uint8_t opcode);
        void opJR(uint8_t opcode);
        void opRET(uint8_t opcode);
        void opRETI(uint8_t opcode);
        void opPUSH(uint8_t opcode);
        void opRST(uint8_t opcode);

        void opPOP(uint8_t opcode);

        void opIDR16(uint8_t opcode);

        void opDI(uint8_t opcode);

        void opJP(uint8_t opcode);

        void opUNK(uint8_t opcode);
};

#endif // CPU_HPP
