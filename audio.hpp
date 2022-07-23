#include <SDL2/SDL.h>
#include "cpu.hpp"

class gbAudio {
    public:
        uint8_t audioBuffer[10][44100];

        void handleAudio();
        void sdlAudioInit();

        void genSQ1(uint32_t length, uint8_t* data);

        uint32_t sq1_lengthForSwap;
        uint32_t sq1_currentPeriod;

        struct gbClass* gb;

        SDL_AudioSpec want, have;
        SDL_AudioDeviceID dev;
    private:


};

extern gbAudio audio;
