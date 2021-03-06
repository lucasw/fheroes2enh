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
#ifndef H2MAPS_OBJECTS_H
#define H2MAPS_OBJECTS_H

#include <vector>
#include <string>
#include "resource.h"
#include "monster.h"
#include "artifact.h"
#include "position.h"
#include "pairs.h"

class MapObjectSimple : public MapPosition
{
public:
    explicit MapObjectSimple(int v = 0) : uid(0), type(v)
    {}

    virtual ~MapObjectSimple() = default;

    int GetType() const
    { return type; }

    u32 GetUID() const
    { return uid; }

    void SetUID(u32 v)
    { uid = v; }

protected:
    friend StreamBase &operator<<(StreamBase &, const MapObjectSimple &);

    friend StreamBase &operator>>(StreamBase &, MapObjectSimple &);

    u32 uid;
    int type;
};

StreamBase &operator<<(StreamBase &, const MapObjectSimple &);

StreamBase &operator>>(StreamBase &, MapObjectSimple &);

struct MapEvent : MapObjectSimple
{
    MapEvent();

    void LoadFromMP2(s32 index, ByteVectorReader&);

    bool isAllow(int color) const;

    void SetVisited(int color);

    Funds resources;
    Artifact artifact;
    bool computer;
    bool cancel;
    int colors;
    string message;
};

StreamBase &operator<<(StreamBase &, const MapEvent &);

StreamBase &operator>>(StreamBase &, MapEvent &);

typedef list<string> RiddleAnswers;

struct MapSphinx : MapObjectSimple
{
    MapSphinx();

    void LoadFromMP2(s32 index, ByteVectorReader&);

    bool AnswerCorrect(const string &answer);

    void SetQuiet();

    Funds resources;
    Artifact artifact;
    RiddleAnswers answers;
    string message;
    bool valid;
};

StreamBase &operator<<(StreamBase &, const MapSphinx &);

StreamBase &operator>>(StreamBase &, MapSphinx &);

struct MapSign : MapObjectSimple
{
    MapSign();

    MapSign(s32 index, const string &);

    void LoadFromMP2(s32 index, ByteVectorReader&);

    string message;
};

StreamBase &operator<<(StreamBase &, const MapSign &);

StreamBase &operator>>(StreamBase &, MapSign &);

struct MapResource : MapObjectSimple
{
    MapResource();

    ResourceCount resource;
};

StreamBase &operator<<(StreamBase &, const MapResource &);

StreamBase &operator>>(StreamBase &, MapResource &);

struct MapArtifact : MapObjectSimple
{
    MapArtifact();

    Artifact artifact;
    int condition;
    int extended;

    Funds QuantityFunds() const;

    ResourceCount QuantityResourceCount() const;
};

StreamBase &operator<<(StreamBase &, const MapArtifact &);

StreamBase &operator>>(StreamBase &, MapArtifact &);

struct MapMonster : MapObjectSimple
{
    MapMonster();

    Monster monster;

    int condition;
    int count;

    Troop QuantityTroop() const;

    bool JoinConditionSkip() const;

    bool JoinConditionMoney() const;

    bool JoinConditionFree() const;

    bool JoinConditionForce() const;
};

StreamBase &operator<<(StreamBase &, const MapMonster &);

StreamBase &operator>>(StreamBase &, MapMonster &);

#endif
