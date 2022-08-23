#include "audio.hpp"
#include <iostream>
#include <fstream>

gbAudio audio;

using namespace std;

uint8_t squareDutyCycle[4][8] =
{
    { 1, 1, 1, 1, 1, 1, 0, 1 },
    { 1, 1, 1, 1, 1, 1, 0, 0 },
    { 1, 1, 1, 1, 0, 0, 0, 0 },
    { 1, 1, 0, 0, 0, 0, 0, 0 }
};

uint32_t pitchSweepSQ1[8] = {
    0xFFFFFFFF,
    70224 >> 1,
    70224,
    (uint32_t)(70224 * 1.5f),
    70224 * 2,
    (uint32_t)(70224 * 2.5f),
    70224 * 3,
    (uint32_t)(70224 * 3.5f),
};

void gbAudio::sendAudio()
{
    uint8_t audioBuffer[735];

    // 70224 total samples divided by samples needed
    float scaleIndex = 836.0f / 735.0f;
    uint8_t sample = 0;

    for(int i = 0; i < 735; i++)
    {
        // Get the state of SQ1 at i * scaleindex
        int8_t sampleValue = SQ1[(int)(i * (scaleIndex))].sample;

        // If it's zero, make it negative 1 so that it can be scaled by the volume
        if(sampleValue == 0)
        {
            sampleValue = -1;
        }

        // Add or Subtract volume from the sample and store it
        sample = 0x80;
        sample += ((sampleValue * 8) * SQ1[(int)(i * (scaleIndex))].volume);

        // Same stuff, but for SQ2
        sampleValue = SQ2[(int)(i * (scaleIndex))].sample;

        // If it's zero, make it negative 1 so that it can be scaled by the volume
        if(sampleValue == 0)
        {
            sampleValue = -1;
        }

        // Add or Subtract volume from the sample and store it
        uint8_t sample2 = 0x80;
        sample2 += ((sampleValue * 8) * SQ2[(int)(i * (scaleIndex))].volume);


        // WAV
        int8_t sample3 = (WAV[(int)(i * (scaleIndex))].sample);


        sampleValue = NOI[(int)(i * (scaleIndex))].sample;

        // If it's zero, make it negative 1 so that it can be scaled by the volume
        if(sampleValue == 0)
        {
            sampleValue = -1;
        }

        uint8_t sample4 = 0x80 + (((sampleValue * 8) * NOI[(int)(i * (scaleIndex))].volume));

        audioBuffer[i] = (sample + sample2 + (uint8_t)sample3 + sample4) / 4;
    }

    // Cue 735 samples for SDL2 to play
    if(SDL_GetQueuedAudioSize(dev) > 22050)
    {
        SDL_ClearQueuedAudio(dev);
    }
    SDL_QueueAudio(dev, audioBuffer, 735);
}

// (NRx2 & 0x7) * 65835
uint32_t envLookup[8] =
{
    0,
    65835,
    131670,
    197505,
    263340,
    329175,
    395010,
    460845
};

void gbAudio::stepSQ1()
{
    // SQ1 Frequency Sweep
    if(freqTimerChangedSQ1)
    {
        freqTimerChangedSQ1 = false;
        sq1FreqTimer = 0;
    }
    else
    {
        while(sq1FreqTimer >= pitchSweepSQ1[(NR10 >> 4) & 0x7])
        {
            sq1FreqTimer -= pitchSweepSQ1[(NR10 >> 4) & 0x7];
            uint16_t freqSweep = (((NR14) & 0x7) << 8) | NR13;
            if(NR10 & 0x8)
            {
                freqSweep = freqSweep - (freqSweep / pow(2, (NR10 & 0x7)));
            }
            else
            {
                freqSweep = freqSweep + (freqSweep / pow(2, (NR10 & 0x7)));
            }
            // Todo: This shouldn't get written back to the I/O Regs
            NR13 = freqSweep;
            NR14 = freqSweep >> 8;
        }
    }



    // SQ1 Envelope
    if((NR12 & 7) != 0)
    {
        while(sq1EnvTimer >= envLookup[(NR12 & 7)])
        {
            sq1EnvTimer -= envLookup[(NR12 & 7)];
            uint8_t envUpdate = (NR12) >> 4;
            if((NR12 & 0x8) && envUpdate < 0xF)
            {
                envUpdate++;
            }
            if(!(NR12 & 0x8) && envUpdate != 0)
            {
                envUpdate--;
            }
            // Todo: This shouldn't get written back to the I/O Regs
            NR12 &= 0xF;
            NR12 |= (envUpdate << 4);
        }
    }
    else
    {
        sq1EnvTimer = 0;
    }


    // SQ1
    uint32_t freq = (((NR14) & 0x7) << 8) | NR13;

    freq = (2048-freq) * 4;
    while(sq1Timer >= freq)
    {
        sq1Timer -= freq;
        sq1DutyPos++;
        if(sq1DutyPos >= 8)
        {
            sq1DutyPos -= 8;
        }
        sq1Value = (squareDutyCycle[NR11 >> 6][sq1DutyPos]);
        sq1Vol = (NR12) >> 4;
    }
}

void gbAudio::stepSQ2()
{
    // SQ1 Envelope
    if((NR22 & 7) != 0)
    {
        while(sq2EnvTimer >= envLookup[(NR22 & 7)])
        {
            sq2EnvTimer -= envLookup[(NR22 & 7)];
            uint8_t envUpdate = (NR22) >> 4;
            if((NR22 & 0x8) && envUpdate < 0xF)
            {
                envUpdate++;
            }
            if(!(NR22 & 0x8) && envUpdate != 0)
            {
                envUpdate--;
            }
            // Todo: This shouldn't get written back to the I/O Regs
            NR22 &= 0xF;
            NR22 |= (envUpdate << 4);
        }
    }
    else
    {
        sq2EnvTimer = 0;
    }

    // SQ2
    uint32_t freq = (((NR24) & 0x7) << 8) | NR23;

    freq = (2048-freq) * 4;
    while(sq2Timer >= freq)
    {
        sq2Timer -= freq;
        sq2DutyPos++;
        if(sq2DutyPos >= 8)
        {
            sq2DutyPos -= 8;
        }
        sq2Value = (squareDutyCycle[NR21 >> 6][sq2DutyPos]);
        sq2Vol = (NR22) >> 4;
    }
}

uint8_t waveVolTable[4] =
{
    4,
    0,
    1,
    2
};

void gbAudio::stepWAV()
{
    if(!(NR30 & 0x80))
    {
        wavTimer = 0;
        return;
    }

    if((NR34 & 0x80))
    {
        //wavDutyPos = 0;
        //NR34 &= 0x7F;
    }

    // SQ2
    uint32_t freq = (((NR34) & 0x7) << 8) | NR33;

    freq = (2048-freq) * 2;
    while(wavTimer >= freq)
    {
        wavTimer -= freq;
        wavDutyPos++;
        if(wavDutyPos >= 32)
        {
            wavDutyPos -= 32;
        }
        wavValue = (WAVERAM[wavDutyPos >> 1]);

        if(!(wavDutyPos & 0x1))
        {
            wavValue = wavValue >> 4;
            wavValue &= 0xF;
        }
        else
        {
            wavValue &= 0xF;
        }

        uint8_t wavVol = (NR32 >> 5) & 0x3;

        wavValue = wavValue >> waveVolTable[wavVol];
        if(wavVol == 0)
        {
            wavValue = 0;
        }


        //wavValue = (squareDutyCycle[NR21 >> 6][wavDutyPos]);
        //sq2Vol = (NR22) >> 4;
    }
}

uint8_t noiClockLookup[8] =
{
    8,
    16,
    32,
    48,
    64,
    80,
    96,
    112
};

void gbAudio::stepNOI()
{
    if((NR44 & 0x80))
    {
        NR44 &= 0x7F;
        LFSR = 0xFFFF;
    }
    if((NR42 & 7) != 0)
    {
        while(noiEnvTimer >= (NR42 & 7)*65835)
        {
            noiEnvTimer -= (NR42 & 7)*65835;
            uint8_t envUpdate = (NR42) >> 4;
            if((NR42 & 0x8) && envUpdate < 0xF)
            {
                envUpdate++;
            }
            if(!(NR42 & 0x8) && envUpdate != 0)
            {
                envUpdate--;
            }
            // Todo: This shouldn't get written back to the I/O Regs
            NR42 &= 0xF;
            NR42 |= (envUpdate << 4);
        }
    }
    else
    {
        noiEnvTimer = 0;
    }

    noiVol = ((NR42 >> 4) & 0xF);


    uint8_t drof = NR43 & 0x7;

    uint8_t scf = (NR43 >> 4) & 0xF;

    uint32_t freq = noiClockLookup[drof] << scf;

    while(noiTimer >= freq)
    {

        noiTimer -= freq;

        uint8_t res;

        res = LFSR & 0x1;
        res ^= (LFSR >> 1) & 0x1;
        LFSR = LFSR >> 1;

        LFSR |= (res & 0x1) << 14;

        if(NR43 & 0x8)
        {
            LFSR &= ~(1 << 6);
            LFSR |= (res & 0x1) << 6;
        }

        noiValue = (~(LFSR)) & 0x1;


    }
}

void gbAudio::handleAudio()
{


    // Add samples to SQ1
    while(mainAudioSampleTimer >= 1.0f)
    {
        stepSQ1();
        stepSQ2();
        stepWAV();
        stepNOI();
        if(sampleTimer >= 836)
        {
            sampleTimer -= 836;
            sendAudio();
        }
        SQ1[sampleTimer].sample = sq1Value;
        SQ1[sampleTimer].volume = sq1Vol;

        SQ2[sampleTimer].sample = sq2Value;
        SQ2[sampleTimer].volume = sq2Vol;

        WAV[sampleTimer].sample = wavValue << 4;

        NOI[sampleTimer].sample = noiValue;
        NOI[sampleTimer].volume = noiVol;


        sampleTimer++;
        mainAudioSampleTimer--;
    }
}

void gbAudio::tickAudioTimers(uint32_t tickAmount)
{
    sq1Timer += tickAmount;
    sq2Timer += tickAmount;
    wavTimer += tickAmount;
    noiTimer += tickAmount;
    mainAudioSampleTimer += (float)tickAmount / 84.0f;
    sq1FreqTimer += tickAmount;
    sq1EnvTimer += tickAmount;
    sq2EnvTimer += tickAmount;
    noiEnvTimer += tickAmount;
}

void gbAudio::sdlAudioInit()
{
    sq1Timer = 0;
    sq1DutyPos = 0;
    sq1FreqTimer = 0;
    want.freq = 44100;
    want.format = AUDIO_U8;
    want.channels = 1;
    want.samples = 735;
    want.callback = NULL;
    dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, SDL_AUDIO_ALLOW_FORMAT_CHANGE);
    SDL_OpenAudio(&want, &have);

    SDL_PauseAudioDevice(dev,0);

    LFSR = 0xFFFF;
}

