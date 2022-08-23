#include <SDL2/SDL.h>
#include "cpu.hpp"

#ifndef AUDIO_HPP
#define AUDIO_HPP

struct sq1data {
    uint8_t sample;
    uint8_t volume;
};

class gbAudio {
    public:

        void handleAudio();
        void sendAudio();
        void sdlAudioInit();
        void tickAudioTimers(uint32_t tickAmount);

        float mainAudioSampleTimer;
        uint32_t sampleSendTimer;

        // 70224
        struct sq1data SQ1[836];
        struct sq1data SQ2[836]; // Todo: These don't need to be this large.
        struct sq1data WAV[836];
        struct sq1data NOI[836];

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
        bool freqTimerChangedSQ1;

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

        // Audio I/O Goes here, It'll be added later

        uint8_t NR10;
        uint8_t NR11;
        uint8_t NR12;
        uint8_t NR13;
        uint8_t NR14;

        uint8_t NR21;
        uint8_t NR22;
        uint8_t NR23;
        uint8_t NR24;

        uint8_t NR30;
        uint8_t NR31;
        uint8_t NR32;
        uint8_t NR33;
        uint8_t NR34;
        uint8_t WAVERAM[0x10];

        uint8_t NR41;
        uint8_t NR42;
        uint8_t NR43;
        uint8_t NR44;

        uint8_t NR50;
        uint8_t NR51;
        uint8_t NR52;
    private:


};

extern gbAudio audio;

#endif
