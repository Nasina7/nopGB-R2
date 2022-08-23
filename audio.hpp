#include <SDL2/SDL.h>
#include "cpu.hpp"

struct sq1data {
    uint8_t sample;
    uint8_t volume;
};

class gbAudio {
    public:
        uint8_t audioDump[60][735];

        void handleAudio();
        void sendAudio();
        void sdlAudioInit();

        uint32_t mainAudioSampleTimer;
        uint32_t sampleSendTimer;

        struct sq1data SQ1[70224];
        struct sq1data SQ2[70224]; // Todo: These don't need to be this large.
        struct sq1data WAV[70224];
        struct sq1data NOI[70224];

        void stepSQ1();
        void stepSQ2();
        void stepWAV();
        void stepNOI();

        uint32_t sampleTimer;

        uint32_t sq1Timer;
        uint8_t sq1DutyPos;
        uint32_t sq1FreqTimer;
        uint32_t sq1EnvTimer;
        uint16_t lastWrittenFrequencySQ1;

        uint32_t sq2Timer;
        uint32_t sq2EnvTimer;
        uint8_t sq2DutyPos;

        uint32_t wavTimer;
        uint8_t wavDutyPos;

        uint32_t noiTimer;
        uint32_t noiEnvTimer;
        uint16_t LFSR;

        uint8_t sq1Value;
        uint8_t sq1Vol;

        uint8_t sq2Value;
        uint8_t sq2Vol;

        uint8_t wavValue;

        uint8_t noiValue;
        uint8_t noiVol;

        struct gbClass* gb;

        SDL_AudioSpec want, have;
        SDL_AudioDeviceID dev;
    private:


};

extern gbAudio audio;
