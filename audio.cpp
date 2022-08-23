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
    float scaleIndex = 70224.0f / 735.0f;
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
    if(gb->audioDumpEnable > 0)
    {
        memcpy(audioDump[gb->audioDumpEnable - 1], audioBuffer, 735);
        gb->audioDumpEnable++;
        if(gb->audioDumpEnable == 61)
        {
            gb->audioDumpEnable = 0;
            ofstream dump("audioDump.raw", std::ifstream::binary);

            dump.write((char*)audioDump, 735 * 60);
            dump.close();
            cout<<"Audio Dumped!"<<endl;
        }
    }
    SDL_QueueAudio(dev, audioBuffer, 735);
}

void gbAudio::stepSQ1()
{
    // SQ1 Frequency Sweep
    if(gb->freqTimerChangedSQ1)
    {
        gb->freqTimerChangedSQ1 = false;
        sq1FreqTimer = 0;
    }

    while(sq1FreqTimer >= pitchSweepSQ1[(gb->NR10 >> 4) & 0x7])
    {
        sq1FreqTimer -= pitchSweepSQ1[(gb->NR10 >> 4) & 0x7];
        uint16_t freqSweep = (((gb->NR14) & 0x7) << 8) | gb->NR13;
        if(gb->NR10 & 0x8)
        {
            freqSweep = freqSweep - (freqSweep / pow(2, (gb->NR10 & 0x7)));
        }
        else
        {
            freqSweep = freqSweep + (freqSweep / pow(2, (gb->NR10 & 0x7)));
        }
        // Todo: This shouldn't get written back to the I/O Regs
        gb->NR13 = freqSweep;
        gb->NR14 = freqSweep >> 8;
    }

    // SQ1 Envelope
    if((gb->NR12 & 7) != 0)
    {
        while(sq1EnvTimer >= (gb->NR12 & 7)*65835)
        {
            sq1EnvTimer -= (gb->NR12 & 7)*65835;
            uint8_t envUpdate = (gb->NR12) >> 4;
            if((gb->NR12 & 0x8) && envUpdate < 0xF)
            {
                envUpdate++;
            }
            if(!(gb->NR12 & 0x8) && envUpdate != 0)
            {
                envUpdate--;
            }
            // Todo: This shouldn't get written back to the I/O Regs
            gb->NR12 &= 0xF;
            gb->NR12 |= (envUpdate << 4);
        }
    }
    else
    {
        sq1EnvTimer = 0;
    }


    // SQ1
    uint32_t freq = (((gb->NR14) & 0x7) << 8) | gb->NR13;

    freq = (2048-freq) * 4;
    while(sq1Timer >= freq)
    {
        sq1Timer -= freq;
        sq1DutyPos++;
        if(sq1DutyPos >= 8)
        {
            sq1DutyPos -= 8;
        }
        sq1Value = (squareDutyCycle[gb->NR11 >> 6][sq1DutyPos]);
        sq1Vol = (gb->NR12) >> 4;
    }
}

void gbAudio::stepSQ2()
{
    // SQ1 Envelope
    if((gb->NR22 & 7) != 0)
    {
        while(sq2EnvTimer >= (gb->NR22 & 7)*65835)
        {
            sq2EnvTimer -= (gb->NR22 & 7)*65835;
            uint8_t envUpdate = (gb->NR22) >> 4;
            if((gb->NR22 & 0x8) && envUpdate < 0xF)
            {
                envUpdate++;
            }
            if(!(gb->NR22 & 0x8) && envUpdate != 0)
            {
                envUpdate--;
            }
            // Todo: This shouldn't get written back to the I/O Regs
            gb->NR22 &= 0xF;
            gb->NR22 |= (envUpdate << 4);
        }
    }
    else
    {
        sq2EnvTimer = 0;
    }

    // SQ2
    uint32_t freq = (((gb->NR24) & 0x7) << 8) | gb->NR23;

    freq = (2048-freq) * 4;
    while(sq2Timer >= freq)
    {
        sq2Timer -= freq;
        sq2DutyPos++;
        if(sq2DutyPos >= 8)
        {
            sq2DutyPos -= 8;
        }
        sq2Value = (squareDutyCycle[gb->NR21 >> 6][sq2DutyPos]);
        sq2Vol = (gb->NR22) >> 4;
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
    if(!(gb->NR30 & 0x80))
    {
        wavTimer = 0;
        return;
    }

    if((gb->NR34 & 0x80))
    {
        //wavDutyPos = 0;
        //gb->NR34 &= 0x7F;
    }

    // SQ2
    uint32_t freq = (((gb->NR34) & 0x7) << 8) | gb->NR33;

    freq = (2048-freq) * 2;
    while(wavTimer >= freq)
    {
        wavTimer -= freq;
        wavDutyPos++;
        if(wavDutyPos >= 32)
        {
            wavDutyPos -= 32;
        }
        wavValue = (gb->WAVERAM[wavDutyPos >> 1]);

        if(!(wavDutyPos & 0x1))
        {
            wavValue = wavValue >> 4;
            wavValue &= 0xF;
        }
        else
        {
            wavValue &= 0xF;
        }

        uint8_t wavVol = (gb->NR32 >> 5) & 0x3;

        wavValue = wavValue >> waveVolTable[wavVol];
        if(wavVol == 0)
        {
            wavValue = 0;
        }


        //wavValue = (squareDutyCycle[gb->NR21 >> 6][wavDutyPos]);
        //sq2Vol = (gb->NR22) >> 4;
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
    if((gb->NR44 & 0x80))
    {
        gb->NR44 &= 0x7F;
        LFSR = 0xFFFF;
    }
    if((gb->NR42 & 7) != 0)
    {
        while(noiEnvTimer >= (gb->NR42 & 7)*65835)
        {
            noiEnvTimer -= (gb->NR42 & 7)*65835;
            uint8_t envUpdate = (gb->NR42) >> 4;
            if((gb->NR42 & 0x8) && envUpdate < 0xF)
            {
                envUpdate++;
            }
            if(!(gb->NR42 & 0x8) && envUpdate != 0)
            {
                envUpdate--;
            }
            // Todo: This shouldn't get written back to the I/O Regs
            gb->NR42 &= 0xF;
            gb->NR42 |= (envUpdate << 4);
        }
    }
    else
    {
        noiEnvTimer = 0;
    }

    noiVol = ((gb->NR42 >> 4) & 0xF);


    uint8_t drof = gb->NR43 & 0x7;

    uint8_t scf = (gb->NR43 >> 4) & 0xF;

    uint32_t freq = noiClockLookup[drof] << scf;

    while(noiTimer >= freq)
    {

        noiTimer -= freq;

        uint8_t res;

        res = LFSR & 0x1;
        res ^= (LFSR >> 1) & 0x1;
        LFSR = LFSR >> 1;

        LFSR |= (res & 0x1) << 14;

        if(gb->NR43 & 0x8)
        {
            LFSR &= ~(1 << 6);
            LFSR |= (res & 0x1) << 6;
        }

        noiValue = (~(LFSR)) & 0x1;


    }
}

void gbAudio::handleAudio()
{
    stepSQ1();
    stepSQ2();
    stepWAV();
    stepNOI();

    // Add samples to SQ1
    for(int i = 0; i < mainAudioSampleTimer; i++)
    {
        if(sampleTimer >= 70224)
        {
            sampleTimer -= 70224;
            sendAudio();
            //memset(SQ1, 0x7, sizeof(SQ1)); // Init the buffer to a default value for testing
            //memset(SQ2, 0x7, sizeof(SQ2));
            //memset(WAV, 0x7, sizeof(SQ2));
        }
        SQ1[sampleTimer].sample = sq1Value;
        SQ1[sampleTimer].volume = sq1Vol;

        SQ2[sampleTimer].sample = sq2Value;
        SQ2[sampleTimer].volume = sq2Vol;

        WAV[sampleTimer].sample = wavValue << 4;

        NOI[sampleTimer].sample = noiValue;
        NOI[sampleTimer].volume = noiVol;


        sampleTimer++;
    }
}

void gbAudio::sdlAudioInit()
{
    sq1Timer = 0;
    sq1DutyPos = 0;
    sq1FreqTimer = 0;
    want.freq = 44100;
    want.format = AUDIO_S8;
    want.channels = 1;
    want.samples = 735;
    want.callback = NULL;
    dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, SDL_AUDIO_ALLOW_FORMAT_CHANGE);
    SDL_OpenAudio(&want, &have);

    SDL_PauseAudioDevice(dev,0);

    LFSR = 0xFFFF;
}

