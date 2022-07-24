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

void audioCallback(void* userdata, Uint8* stream, int len)
{
    for(int i = 0; i < len; i++)
    {
        uint8_t sampleValue = audio.SQ1[i * (70224 / len)];
        uint8_t sample = 0x80 + sampleValue;

        stream[i] = sample;
        //audioBuffer[i] = i;
    }


}

uint8_t sampleConvert[2] = {
    0x70, 0x90
};

void gbAudio::sendAudio()
{
    uint8_t audioBuffer[sampleSendAmount * queueSize];


    for(int i = 0; i < sampleSendAmount; i++)
    {
        uint8_t sampleValue = SQ1[i * (70224 / sampleSendAmount)];
        uint8_t sample = sampleConvert[sampleValue];
        if(sample != sampleConvert[0] && sample != sampleConvert[1])
        {
            cout<<"BAD"<<endl;
        }

        audioBuffer[i + (sampleSendTimer * sampleSendAmount)] = sample;
        //audioBuffer[i] = i;
    }
    //cout<<SDL_GetQueuedAudioSize(dev)<<endl;
    sampleSendTimer++;
    if(sampleSendTimer == queueSize)
    {
        sampleSendTimer = 0;
        SDL_QueueAudio(dev, audioBuffer, sampleSendAmount * queueSize);
        ofstream audioDump("audioDump.bin", std::ios::binary);
        audioDump.write((const char*)audioBuffer, sizeof(audioBuffer));
        audioDump.close();
    }
}

void gbAudio::handleAudio()
{
    uint32_t freq = (((gb->NR14) & 0x7) << 8) | gb->NR13;
    //freq = 2000;

    freq = (2048-freq) * 4;
    while(sq1Timer >= freq)
    {
        //cout<<"Freq: "<<sq1Timer - freq<<endl;
        sq1Timer -= freq;
        sq1DutyPos++;
        if(sq1DutyPos >= 8)
        {
            sq1DutyPos -= 8;
        }
        sq1Value = (squareDutyCycle[2][sq1DutyPos]);
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
        SQ1[sq1curSample] = sq1Value;
        sq1curSample++;
    }
}

void gbAudio::sdlAudioInit()
{
    sq1Timer = 0;
    sq1DutyPos = 0;
    want.freq = 44100;
    want.format = AUDIO_U8;
    want.channels = 1;
    want.samples = 735;
    want.callback = NULL;
    dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, SDL_AUDIO_ALLOW_FORMAT_CHANGE);
    SDL_OpenAudio(&want, &have);

    SDL_PauseAudioDevice(dev,0);
}

