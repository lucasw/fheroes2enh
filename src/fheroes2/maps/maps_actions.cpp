/***************************************************************************
 *   Copyright (C) 2013 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#include "world.h"
#include "text.h"
#include "dialog.h"
#include "settings.h"
#include "game.h"
#include "maps_actions.h"

StreamBase &operator<<(StreamBase &sb, const ActionSimple &st)
{
    return sb << st.type << st.uid;
}

StreamBase &operator>>(StreamBase &sb, ActionSimple &st)
{
    return sb >> st.type >> st.uid;
}

ActionSimple::~ActionSimple() = default;

StreamBase &operator<<(StreamBase &sb, const ActionResources &st)
{
    return sb << static_cast<const ActionSimple &>(st) << st.resources << st.message;
}

StreamBase &operator>>(StreamBase &sb, ActionResources &st)
{
    return sb >> static_cast<ActionSimple &>(st) >> st.resources >> st.message;
}

StreamBase &operator<<(StreamBase &sb, const ActionArtifact &st)
{
    return sb << static_cast<const ActionSimple &>(st) << st.artifact << st.message;
}

StreamBase &operator>>(StreamBase &sb, ActionArtifact &st)
{
    return sb >> static_cast<ActionSimple &>(st) >> st.artifact >> st.message;
}

StreamBase &operator<<(StreamBase &sb, const ActionAccess &st)
{
    return sb << static_cast<const ActionSimple &>(st) << st.allowPlayers << st.allowComputer
              << st.cancelAfterFirstVisit << st.message;
}

StreamBase &operator>>(StreamBase &sb, ActionAccess &st)
{
    return sb >> static_cast<ActionSimple &>(st) >> st.allowPlayers >> st.allowComputer >> st.cancelAfterFirstVisit
              >> st.message;
}

StreamBase &operator<<(StreamBase &sb, const ActionDefault &st)
{
    return sb << static_cast<const ActionSimple &>(st) << st.enabled << st.message;
}

StreamBase &operator>>(StreamBase &sb, ActionDefault &st)
{
    return sb >> static_cast<ActionSimple &>(st) >> st.enabled >> st.message;
}

StreamBase &operator<<(StreamBase &sb, const ActionMessage &st)
{
    return sb << static_cast<const ActionSimple &>(st) << st.message;
}

StreamBase &operator>>(StreamBase &sb, ActionMessage &st)
{
    return sb >> static_cast<ActionSimple &>(st) >> st.message;
}

StreamBase &operator<<(StreamBase &sb, const ListActions &listActions)
{
    sb << static_cast<u32>(listActions.size());
    for (auto listAction : listActions)
    {
        sb << listAction->GetType();

        switch (listAction->GetType())
        {
            case ACTION_DEFAULT:
            {
                const auto *ptr = static_cast<const ActionDefault *>(listAction);
                if (ptr) sb << *ptr;
            }
                break;
            case ACTION_ACCESS:
            {
                const auto *ptr = static_cast<const ActionAccess *>(listAction);
                if (ptr) sb << *ptr;
            }
                break;
            case ACTION_MESSAGE:
            {
                const auto *ptr = static_cast<const ActionMessage *>(listAction);
                if (ptr) sb << *ptr;
            }
                break;
            case ACTION_RESOURCES:
            {
                const auto *ptr = static_cast<const ActionResources *>(listAction);
                if (ptr) sb << *ptr;
            }
                break;
            case ACTION_ARTIFACT:
            {
                const auto *ptr = static_cast<const ActionArtifact *>(listAction);
                if (ptr) sb << *ptr;
            }
                break;
            default:
                sb << *listAction;
                break;
        }
    }

    return sb;
}

StreamBase &operator>>(StreamBase &sb, ListActions &st)
{
    u32 size = 0;
    sb >> size;

    st.clear();

    for (u32 ii = 0; ii < size; ++ii)
    {
        int type;
        sb >> type;

        switch (type)
        {
            case ACTION_DEFAULT:
            {
                auto *ptr = new ActionDefault();
                sb >> *ptr;
                st.push_back(ptr);
            }
                break;
            case ACTION_ACCESS:
            {
                auto *ptr = new ActionAccess();
                sb >> *ptr;
                st.push_back(ptr);
            }
                break;
            case ACTION_MESSAGE:
            {
                auto *ptr = new ActionMessage();
                sb >> *ptr;
                st.push_back(ptr);
            }
                break;
            case ACTION_RESOURCES:
            {
                auto *ptr = new ActionResources();
                sb >> *ptr;
                st.push_back(ptr);
            }
                break;
            case ACTION_ARTIFACT:
            {
                auto *ptr = new ActionArtifact();
                sb >> *ptr;
                st.push_back(ptr);
            }
                break;

            default:
            {
                auto *ptr = new ActionSimple();
                sb >> *ptr;
                st.push_back(ptr);
            }
                break;
        }
    }

    return sb;
}

bool ActionAccess::Action(ActionAccess *act, s32 index, Heroes &hero)
{
    if (act)
    {
        if (act->cancelAfterFirstVisit &&
            hero.isVisited(world.GetTiles(index), Visit::GLOBAL))
            return false;

        if (!act->message.empty())
            Message("", act->message, Font::BIG, Dialog::OK);

        if (hero.isControlAI() && !act->allowComputer)
            return false;

        if (act->cancelAfterFirstVisit)
            hero.SetVisited(index, Visit::GLOBAL);

        if (hero.GetColor() & act->allowPlayers)
            return true;
    }

    return false;
}

bool ActionDefault::Action(ActionDefault *act, s32 index, Heroes &hero)
{
    if (act)
    {
        if (!act->message.empty())
            Message("", act->message, Font::BIG, Dialog::OK);
        return act->enabled;
    }

    return false;
}

bool ActionArtifact::Action(ActionArtifact *act, s32 index, Heroes &hero)
{
    if (act && act->artifact != Artifact::UNKNOWN)
    {
        if (!act->message.empty())
            Dialog::ArtifactInfo("", act->message, act->artifact);
        hero.PickupArtifact(act->artifact);
        act->artifact = Artifact::UNKNOWN;
        return true;
    }

    return false;
}

bool ActionResources::Action(ActionResources *act, s32 index, Heroes &hero)
{
    if (act && 0 < act->resources.GetValidItems())
    {
        Dialog::ResourceInfo("", act->message, act->resources);
        hero.GetKingdom().AddFundsResource(act->resources);
        act->resources.Reset();
        return true;
    }

    return false;
}

bool ActionMessage::Action(ActionMessage *act, s32 index, Heroes &hero)
{
    if (act)
    {
        if (!act->message.empty())
            Message("", act->message, Font::BIG, Dialog::OK);
        return true;
    }

    return false;
}
