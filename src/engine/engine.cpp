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

#include "engine.h"

#include "rand.h"
#include "system.h"
#include "font.h"

#ifdef WITH_NET
#include "sdlnet.h"
#endif

namespace Mixer
{
    void Init();

    void Quit();
}

#ifdef WITH_AUDIOCD
namespace Cdrom
{
    void Open();
    void Close();
}
#endif

bool SDL::Init(u32 system)
{
    if (System::isRunning()) return false;

    if (0 > SDL_Init(system))
    {
        ERROR(SDL_GetError());
        return false;
    }

    if (SDL_INIT_AUDIO & system) Mixer::Init();
#ifdef WITH_AUDIOCD
    if(SDL_INIT_CDROM & system) Cdrom::Open();
#endif
#ifdef WITH_TTF
    FontTTF::Init();
#endif
#ifdef WITH_NET
    Network::Init();
#endif

    SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);


    System::CreateTrayIcon(true);
    System::PowerManagerOff(true);
    Surface::SetDefaultColorKey(0xFF, 0, 0xFF);

    return true;
}

void SDL::Quit()
{
    System::CreateTrayIcon(false);
    System::PowerManagerOff(false);

#ifdef WITH_NET
    Network::Quit();
#endif
#ifdef WITH_TTF
    FontTTF::Quit();
#endif
#ifdef WITH_AUDIOCD
    if(SubSystem(SDL_INIT_CDROM)) Cdrom::Close();
#endif
    if (SubSystem(SDL_INIT_AUDIO)) Mixer::Quit();

    SDL_Quit();
}

bool SDL::SubSystem(u32 system)
{
    return system & SDL_WasInit(system);
}
