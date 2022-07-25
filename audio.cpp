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

#define queueSize 10
#define sampleSendAmount 735

uint8_t sampleConvert[2] = {
    0x71, 0x8F
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
        sample += (sampleValue * SQ1[(int)(i * (scaleIndex))].volume);
        audioBuffer[i] = sample;
    }

    // Cue 735 samples for SDL2 to play
    SDL_QueueAudio(dev, audioBuffer, 735);
}

void gbAudio::handleAudio()
{
    if(gb->freqTimerChangedSQ1)
    {
        gb->freqTimerChangedSQ1 = false;
        sq1FreqTimer = 0;
    }

    while(sq1FreqTimer >= pitchSweepSQ1[(gb->NR10 >> 4) & 0x7])
    {
        //cout<<"hthaotihwofaihdwoaidhaw"<<endl;
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
        //cout<<freqSweep<<endl;
        // Todo: This shouldn't get written back to the I/O Regs
        gb->NR13 = freqSweep;
        gb->NR14 = freqSweep >> 8;
    }


    uint32_t freq = (((gb->NR14) & 0x7) << 8) | gb->NR13;

    freq = (2048-freq) * 4;
    while(sq1Timer >= freq)
    {
        //cout<<(int)sq1FreqTimer<<endl;
        //cout<<"Freq: "<<sq1Timer - freq<<endl;
        sq1Timer -= freq;
        sq1DutyPos++;
        if(sq1DutyPos >= 8)
        {
            sq1DutyPos -= 8;
        }
        sq1Value = (squareDutyCycle[gb->NR11 >> 6][sq1DutyPos]);
        sq1Vol = (gb->NR12) >> 4;
    }



    for(int i = 0; i < mainAudioSampleTimer; i++)
    {
        if(sq1curSample >= 70224)
        {
            sq1curSample -= 70224;
            sendAudio();
            memset(SQ1, 0x7, sizeof(SQ1));
        }
        SQ1[sq1curSample].sample = sq1Value;
        SQ1[sq1curSample].volume = sq1Vol;
        sq1curSample++;
    }
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
}

