/***************************************************************************
 *   Copyright (C) 2008 by Andrey Afletdinov <fheroes2@gmail.com>          *
 *                                                                         *
 *   Part of the Free Heroes2 Engine:                                      *
 *   http://sourceforge.net/projects/fheroes2                              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <algorithm>

#include "engine.h"

#include "system.h"
#include "tools.h"

#include "audio.h"
#include "audio_mixer.h"
#include "audio_music.h"

namespace Mixer
{
    void Init();

    void Quit();

    bool valid = false;
}

bool Mixer::isValid()
{
    return valid;
}

#ifdef WITH_MIXER
#include "SDL_mixer.h"

void FreeChannel(int channel)
{
    Mixer::chunk_t* sample = Mix_GetChunk(channel);
    if(sample) Mix_FreeChunk(sample);
}

void Mixer::Init()
{
    if(SDL::SubSystem(SDL_INIT_AUDIO))
    {
        Audio::Spec & hardware = Audio::GetHardwareSpec();
        hardware.freq = 22050;
        hardware.format = AUDIO_S16;
        hardware.channels = 2;
        hardware.samples = 2048;

        if(0 != Mix_OpenAudio(hardware.freq, hardware.format, hardware.channels, hardware.samples))
        {
            ERROR(SDL_GetError());
            valid = false;
        }
        else
        {
            int channels = 0;
            Mix_QuerySpec(&hardware.freq, &hardware.format, &channels);
            hardware.channels = channels;

            valid = true;
        }
    }
    else
    {
        ERROR("audio subsystem not initialize");
        valid = false;
    }
}

void Mixer::Quit()
{
    if(!SDL::SubSystem(SDL_INIT_AUDIO) || !valid)
        return;
    Music::Reset();
    Mixer::Reset();
    valid = false;
    Mix_CloseAudio();
}

void Mixer::SetChannels(u8 num)
{
    Mix_AllocateChannels(num);
    Mix_ReserveChannels(1);
}

void Mixer::FreeChunk(chunk_t* sample)
{
    if(sample) Mix_FreeChunk(sample);
}


Mixer::chunk_t* Mixer::LoadWAV(const char* file)
{
    Mix_Chunk *sample = Mix_LoadWAV(file);
    if(!sample) ERROR(SDL_GetError());
    return sample;
}

Mixer::chunk_t* Mixer::LoadWAV(const u8* ptr, u32 size)
{
    Mix_Chunk *sample = Mix_LoadWAV_RW(SDL_RWFromConstMem(ptr, size), 1);
    if(!sample) ERROR(SDL_GetError());
    return sample;
}

int Mixer::Play(chunk_t* sample, int channel, bool loop)
{
    int res = Mix_PlayChannel(channel, sample, loop ? -1 : 0);
    if(res == -1) ERROR(SDL_GetError());
    return res;
}

int Mixer::Play(const char* file, int channel, bool loop)
{
    if(!valid)
    {
        return -1;
    }
    chunk_t* sample = LoadWAV(file);
    if(!sample)
        return -1;
    Mix_ChannelFinished(FreeChannel);
    return Play(sample, channel, loop);
}

int Mixer::Play(const u8* ptr, u32 size, int channel, bool loop)
{
    if(!valid || !ptr)
    {
        return -1;
    }
    chunk_t* sample = LoadWAV(ptr, size);
    if(!sample)
        return -1;
    Mix_ChannelFinished(FreeChannel);
    return Play(sample, channel, loop);
}

u16 Mixer::MaxVolume()
{
    return MIX_MAX_VOLUME;
}

u16 Mixer::Volume(int channel, s16 vol)
{
    if(!valid) return 0;
    return Mix_Volume(channel, vol > MIX_MAX_VOLUME ? MIX_MAX_VOLUME : vol);
}

void Mixer::Pause(int channel)
{
    Mix_Pause(channel);
}

void Mixer::Resume(int channel)
{
    Mix_Resume(channel);
}

void Mixer::Stop(int channel)
{
    Mix_HaltChannel(channel);
}

void Mixer::Reset()
{
    Music::Reset();
#ifdef WITH_AUDIOCD
    if(Cdrom::isValid()) Cdrom::Pause();
#endif
    Mix_HaltChannel(-1);
}

u8 Mixer::isPlaying(int channel)
{
    return Mix_Playing(channel);
}

u8 Mixer::isPaused(int channel)
{
    return Mix_Paused(channel);
}

void Mixer::Reduce()
{
}

void Mixer::Enhance()
{
}

#else

enum
{
    MIX_PLAY = 0x01, MIX_LOOP = 0x02, MIX_REDUCE = 0x04, MIX_ENHANCE = 0x08
};

struct chunk_t
{
    chunk_t() : data(nullptr), length(0), position(0), volume1(0), volume2(0), state(0)
    {};

    bool this_ptr(const chunk_t *ch) const
    { return ch == this; };

    const u8 *data;
    u32 length;
    u32 position;
    s16 volume1;
    s16 volume2;
    u8 state;
};


namespace Mixer
{
    bool PredicateIsFreeSound(const chunk_t &);

    void AudioCallBack(void *, u8 *, int);

    vector<chunk_t> chunks;
    u8 reserved_channels;
    vector<const u8 *> paused;
}

bool Mixer::PredicateIsFreeSound(const chunk_t &ch)
{
    return !(ch.state & MIX_PLAY);
}

void Mixer::AudioCallBack(void *unused, u8 *stream, int length)
{
    for (auto &ch : chunks)
    {
        if ((ch.state & MIX_PLAY) && ch.volume1)
        {
            if (ch.state & MIX_REDUCE)
            {
                ch.volume1 -= 10;
                if (ch.volume1 <= 0)
                {
                    ch.volume1 = 0;
                    ch.state &= ~MIX_REDUCE;
                }
            } else if (ch.state & MIX_ENHANCE)
            {
                ch.volume1 += 10;
                if (ch.volume1 >= ch.volume2)
                {
                    ch.volume1 = ch.volume2;
                    ch.state &= ~MIX_ENHANCE;
                }
            }

            SDL_MixAudio(stream, &ch.data[ch.position],
                         (ch.position + length > ch.length ? ch.length - ch.position : length), ch.volume1);
            ch.position += length;
            if (ch.position >= ch.length)
            {
                ch.position = 0;
                if (!(ch.state & MIX_LOOP)) ch.state &= ~MIX_PLAY;
            }
        }
    }
}

void Mixer::Init()
{
    if (SDL::SubSystem(SDL_INIT_AUDIO))
    {
        Audio::Spec spec;
        spec.freq = 22050;
        spec.format = AUDIO_S16;
        spec.channels = 2;
        spec.samples = 2048;
        spec.callback = AudioCallBack;

        if (0 > SDL_OpenAudio(&spec, &Audio::GetHardwareSpec()))
        {
            ERROR(SDL_GetError());
            valid = false;
        } else
        {
            SDL_PauseAudio(0);
            valid = true;
            reserved_channels = 0;
        }
    } else
    {
        ERROR("audio subsystem not initialize");
        valid = false;
    }
}

void Mixer::Quit()
{
    if (SDL::SubSystem(SDL_INIT_AUDIO) && valid)
    {
        Music::Reset();
        Reset();
        SDL_CloseAudio();
        chunks.clear();
        paused.clear();
        valid = false;
    }
}

void Mixer::SetChannels(u8 num)
{
    chunks.resize(num);
    reserved_channels = 1;
}

u16 Mixer::MaxVolume()
{
    return SDL_MIX_MAXVOLUME;
}

u16 Mixer::Volume(int ch, s16 vol)
{
    if (!valid) return 0;

    if (vol > SDL_MIX_MAXVOLUME) vol = SDL_MIX_MAXVOLUME;

    if (ch < 0)
    {
        for (auto &chunk : chunks)
        {
            SDL_LockAudio();
            chunk.volume1 = vol;
            chunk.volume2 = vol;
            SDL_UnlockAudio();
        }
    } else if (ch < static_cast<int>(chunks.size()))
    {
        if (0 > vol)
        {
            vol = chunks[ch].volume1;
        } else
        {
            SDL_LockAudio();
            chunks[ch].volume1 = vol;
            chunks[ch].volume2 = vol;
            SDL_UnlockAudio();
        }
    }
    return vol;
}

int Mixer::Play(const u8 *ptr, u32 size, int channel, bool loop)
{
    if (valid && ptr)
    {
        chunk_t *ch = nullptr;

        if (0 > channel)
        {
            //TODO: Fix to compile with C++ 11
            auto it = chunks.end();
                    //std::find_if(chunks.begin(), chunks.end(),
                      //                                       std::bind2nd(std::mem_fun_ref(&chunk_t::this_ptr), ptr));
            if (it == chunks.end())
            {
                it = find_if(chunks.begin() + reserved_channels, chunks.end(), PredicateIsFreeSound);
                if (it == chunks.end())
                {
                    ERROR("mixer is full");
                    return -1;
                }
            }
            ch = &(*it);
            channel = it - chunks.begin();
        } else if (channel < static_cast<int>(chunks.size()))
            ch = &chunks[channel];

        if (ch)
        {
            SDL_LockAudio();
            ch->state = (loop ? MIX_LOOP | MIX_PLAY : MIX_PLAY);
            ch->data = ptr;
            ch->length = size;
            ch->position = 0;
            SDL_UnlockAudio();
        }
        return channel;
    }
    return -1;
}

void Mixer::Pause(int ch)
{
    if (valid)
    {
        SDL_LockAudio();

        if (0 > ch)
        {
            for (auto &chunk : chunks)
                if (chunk.state & MIX_PLAY)
                {
                    paused.push_back(chunk.data);
                    chunk.state &= ~MIX_PLAY;
                }
        } else if (ch < static_cast<int>(chunks.size()))
            chunks[ch].state &= ~MIX_PLAY;

        SDL_UnlockAudio();
    }
}

void Mixer::Resume(int ch)
{
    if (valid)
    {
        SDL_LockAudio();

        if (0 > ch)
        {
            if (!paused.empty())
                for (auto &chunk : chunks)
                    if (paused.end() != find(paused.begin(), paused.end(), chunk.data))
                        chunk.state |= MIX_PLAY;

            paused.clear();
        } else if (ch < static_cast<int>(chunks.size()))
            chunks[ch].state |= MIX_PLAY;

        SDL_UnlockAudio();
    }
}

u8 Mixer::isPlaying(int ch)
{
    return 0 <= ch && ch < static_cast<int>(chunks.size()) && (chunks[ch].state & MIX_PLAY);
}

u8 Mixer::isPaused(int ch)
{
    return 0 <= ch && ch < static_cast<int>(chunks.size()) && !(chunks[ch].state & MIX_PLAY);
}

void Mixer::Stop(int ch)
{
    Pause(ch);
}

void Mixer::Reset()
{
    Music::Reset();
#ifdef WITH_AUDIOCD
    if(Cdrom::isValid()) Cdrom::Pause();
#endif
    Pause(-1);
}

void Mixer::Reduce()
{
    if (valid)
    {
        for (auto &chunk : chunks)
        {
            if (chunk.state & MIX_PLAY)
            {
                SDL_LockAudio();
                chunk.state |= MIX_REDUCE;
                SDL_UnlockAudio();
            }
        }
    }
}

void Mixer::Enhance()
{
    if (valid)
    {
        for (auto &chunk : chunks)
        {
            if (chunk.state & MIX_PLAY)
            {
                SDL_LockAudio();
                chunk.state |= MIX_ENHANCE;
                SDL_UnlockAudio();
            }
        }
    }
}

#endif
