#include "audio.hpp"
#include <iostream>

gbAudio audio;
using namespace std;
void gbAudio::handleAudio()
{

}

void gbAudio::genSQ1(uint32_t length, uint8_t* data)
{
    uint32_t freq = (((gb->NR14) & 0x7) << 8) | gb->NR13;

    freq = (2048-freq) * 4;
    freq = (freq * (131072 / 44100)); // Convert from 131072 Hz to 44100 Hz
    freq /= 60; // Convert frequency from per second to per 60th of a second

    float volume = (gb->NR12 >> 4);
    for(int i = 0; i < length; i++)
    {
        uint8_t state = sq1_currentPeriod / freq;

        if((state & 0x1) == 0)
        {
            data[i] = 0x80 + (volume);
        }
        else
        {
            data[i] = 0x80 - (volume);
        }
        sq1_currentPeriod++;
        if(sq1_currentPeriod >= sq1_lengthForSwap * 2)
        {
            sq1_currentPeriod -= sq1_lengthForSwap * 2;
        }
    }
}

void audioCallback(void* userdata, Uint8* stream, int len)
{
    audio.genSQ1(len, stream);

    //memcpy(stream, SQ1, len);

    return;
}

void gbAudio::sdlAudioInit()
{
    sq1_currentPeriod = 0;
    want.freq = 44100;
    want.format = AUDIO_U8;
    want.channels = 1;
    want.samples = 735 * 2;
    want.callback = audioCallback;
    dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, SDL_AUDIO_ALLOW_FORMAT_CHANGE);
    SDL_OpenAudio(&want, &have);
    SDL_PauseAudio(0);
}

