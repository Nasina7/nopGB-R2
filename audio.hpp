#include <SDL2/SDL.h>
#include "cpu.hpp"

struct sq1data {
    uint8_t sample;
    uint8_t volume;
};

class gbAudio {
    public:
        uint8_t audioBuffer[10][44100];

        void handleAudio();
        void sendAudio();
        void sdlAudioInit();

        uint32_t mainAudioSampleTimer;
        uint32_t sampleSendTimer;

        struct sq1data SQ1[70224];

        void genSQ1(uint32_t length, uint8_t* data);

        uint32_t sq1curSample;
        uint32_t sq1Timer;
        uint8_t sq1DutyPos;
        uint32_t sq1FreqTimer;
        uint16_t lastWrittenFrequencySQ1;

        uint8_t sq1Value;
        uint8_t sq1Vol;

        struct gbClass* gb;

        SDL_AudioSpec want, have;
        SDL_AudioDeviceID dev;
    private:


};

extern gbAudio audio;
