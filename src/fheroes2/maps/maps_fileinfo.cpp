/***************************************************************************
 *   Copyright (C) 2009 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#if defined(ANDROID)
#include <locale>
#endif

#include "gamedefs.h"

#include <string>
#include <iosfwd>
#include <cstring>
#include <algorithm>
#include "difficulty.h"
#include "color.h"
#include "race.h"
#include "world.h"
#include "settings.h"
#include "game.h"
#include "game_io.h"
#include "game_over.h"

#define LENGTHNAME        16
#define LENGTHDESCRIPTION    143

using namespace std;

bool AlphabeticalCompare(const std::string &lhs, const std::string &rhs)
{
    return lhs < rhs;
}

namespace Editor
{
    enum
    {
        Wins = 0x1000,
        CaptureTown = 0x1001,
        DefeatHero = 0x1002,
        FindArtifact = 0x1003,
        SideWins = 0x1004,
        AccumulateGold = 0x1005,
        CompAlsoWins = 0x0100,
        AllowNormalVictory = 0x0200,
        Loss = 0x2000,
        LoseTown = 0x2001,
        LoseHero = 0x2002,
        OutTime = 0x2003
    };
}

int ByteToColor(int byte)
{
    switch (byte)
    {
        case 0:
            return Color::BLUE;
        case 1:
            return Color::GREEN;
        case 2:
            return Color::RED;
        case 3:
            return Color::YELLOW;
        case 4:
            return Color::ORANGE;
        case 5:
            return Color::PURPLE;

        default:
            break;
    }

    return Color::NONE;
}

int ByteToRace(int byte)
{
    switch (byte)
    {
        case 0x00:
            return Race::KNGT;
        case 0x01:
            return Race::BARB;
        case 0x02:
            return Race::SORC;
        case 0x03:
            return Race::WRLK;
        case 0x04:
            return Race::WZRD;
        case 0x05:
            return Race::NECR;
        case 0x06:
            return Race::MULT;
        case 0x07:
            return Race::RAND;

        default:
            break;
    }

    return Race::NONE;
}

Maps::FileInfo::FileInfo()
{
    Reset();
}

Maps::FileInfo::FileInfo(const FileInfo &f)
{
    *this = f;
}

Maps::FileInfo &Maps::FileInfo::operator=(const FileInfo &f)
{
    file = f.file;
    name = f.name;
    description = f.description;
    size_w = f.size_w;
    size_h = f.size_h;
    difficulty = f.difficulty;

    for (u32 ii = 0; ii < KINGDOMMAX; ++ii)
    {
        races[ii] = f.races[ii];
        unions[ii] = f.unions[ii];
    }

    kingdom_colors = f.kingdom_colors;
    allow_human_colors = f.allow_human_colors;
    allow_comp_colors = f.allow_comp_colors;
    rnd_races = f.rnd_races;
    conditions_wins = f.conditions_wins;
    comp_also_wins = f.comp_also_wins;
    allow_normal_victory = f.allow_normal_victory;
    wins1 = f.wins1;
    wins2 = f.wins2;
    conditions_loss = f.conditions_loss;
    loss1 = f.loss1;
    loss2 = f.loss2;
    localtime = f.localtime;
    with_heroes = f.with_heroes;

    return *this;
}

void Maps::FileInfo::Reset()
{
    file.clear();
    name.clear();
    description.clear();
    size_w = 0;
    size_h = 0;
    difficulty = 0;
    kingdom_colors = 0;
    allow_human_colors = 0;
    allow_comp_colors = 0;
    rnd_races = 0;
    conditions_wins = 0;
    comp_also_wins = false;
    allow_normal_victory = false;
    wins1 = 0;
    wins2 = 0;
    conditions_loss = 0;
    loss1 = 0;
    loss2 = 0;
    localtime = 0;
    with_heroes = false;

    for (u32 ii = 0; ii < KINGDOMMAX; ++ii)
    {
        races[ii] = Race::NONE;
        unions[ii] = ByteToColor(ii);
    }
}

bool Maps::FileInfo::ReadSAV(const string &filename)
{
    Reset();
    return Game::LoadSAV2FileInfo(filename, *this);
}

bool Maps::FileInfo::ReadMAP(const string &filename)
{
    return false;
}

bool Maps::FileInfo::ReadMP2(const string &filename)
{
    Reset();
    StreamFile fs;

    if (!fs.open(filename, "rb"))
    {
        return false;
    }

    file = filename;
    kingdom_colors = 0;
    allow_human_colors = 0;
    allow_comp_colors = 0;
    rnd_races = 0;
    localtime = 0;

    // magic byte
    if (fs.getBE32() != 0x5C000000)
    {
        return false;
    }

    // level
    switch (fs.getLE16())
    {
        case 0x00:
            difficulty = Difficulty::EASY;
            break;
        case 0x01:
            difficulty = Difficulty::NORMAL;
            break;
        case 0x02:
            difficulty = Difficulty::HARD;
            break;
        case 0x03:
            difficulty = Difficulty::EXPERT;
            break;
        default:
            difficulty = Difficulty::NORMAL;
            break;
    }

    // width
    size_w = fs.get();

    // height
    size_h = fs.get();

    Colors colors(Color::ALL);

    // kingdom color - blue, green, red, yellow, orange, purple
    for (Colors::const_iterator
                 it = colors.begin(); it != colors.end(); ++it)
        if (fs.get()) kingdom_colors |= *it;

    // allow human color - blue, green, red, yellow, orange, purple
    for (Colors::const_iterator
                 it = colors.begin(); it != colors.end(); ++it)
        if (fs.get()) allow_human_colors |= *it;

    // allow comp color - blue, green, red, yellow, orange, purple
    for (Colors::const_iterator
                 it = colors.begin(); it != colors.end(); ++it)
        if (fs.get()) allow_comp_colors |= *it;

    // kingdom count
    // fs.seekg(0x1A, std::ios_base::beg);
    // fs.get();

    // wins
    fs.seek(0x1D);
    conditions_wins = fs.get();
    // data wins
    comp_also_wins = fs.get();
    // data wins
    allow_normal_victory = fs.get();
    // data wins
    wins1 = fs.getLE16();
    // data wins
    fs.seek(0x2c);
    wins2 = fs.getLE16();

    // loss
    fs.seek(0x22);
    conditions_loss = fs.get();
    // data loss
    loss1 = fs.getLE16();
    // data loss
    fs.seek(0x2e);
    loss2 = fs.getLE16();

    // start with hero
    fs.seek(0x25);
    with_heroes = 0 == fs.get();

    // race color
    for (Colors::const_iterator
                 it = colors.begin(); it != colors.end(); ++it)
    {
        int race = ByteToRace(fs.get());
        races[Color::GetIndex(*it)] = race;
        if (Race::RAND == race) rnd_races |= *it;
    }

    // name
    fs.seek(0x3A);
    name = Game::GetEncodeString(fs.toString(LENGTHNAME));

    // description
    fs.seek(0x76);
    description = Game::GetEncodeString(fs.toString(LENGTHDESCRIPTION));

    //fill unions
    if (4 == conditions_wins)
        FillUnions();

    return true;
}

void Maps::FileInfo::FillUnions()
{
    int side1 = 0;
    int side2 = 0;

    const Colors colors(kingdom_colors);

    for (int color : colors)
    {
        if (Color::GetIndex(color) < wins1)
            side1 |= color;
        else
            side2 |= color;
    }

    for (u32 ii = 0; ii < KINGDOMMAX; ++ii)
    {
        int cl = ByteToColor(ii);

        if (side1 & cl)
            unions[ii] = side1;
        else if (side2 & cl)
            unions[ii] = side2;
        else
            unions[ii] = cl;
    }
}

bool Maps::FileInfo::FileSorting(const FileInfo &fi1, const FileInfo &fi2)
{
    return AlphabeticalCompare(fi1.file, fi2.file);
}

bool Maps::FileInfo::NameSorting(const FileInfo &fi1, const FileInfo &fi2)
{
    return AlphabeticalCompare(fi1.name, fi2.name);
}

bool Maps::FileInfo::NameCompare(const FileInfo &fi1, const FileInfo &fi2)
{
    return fi1.name == fi2.name;
}

int Maps::FileInfo::KingdomRace(int color) const
{
    switch (color)
    {
        case Color::BLUE:
            return races[0];
        case Color::GREEN:
            return races[1];
        case Color::RED:
            return races[2];
        case Color::YELLOW:
            return races[3];
        case Color::ORANGE:
            return races[4];
        case Color::PURPLE:
            return races[5];
        default:
            break;
    }
    return 0;
}

int Maps::FileInfo::ConditionWins() const
{
    switch (conditions_wins)
    {
        case 0:
            return GameOver::WINS_ALL;
        case 1:
            return allow_normal_victory ? GameOver::WINS_TOWN | GameOver::WINS_ALL : GameOver::WINS_TOWN;
        case 2:
            return allow_normal_victory ? GameOver::WINS_HERO | GameOver::WINS_ALL : GameOver::WINS_HERO;
        case 3:
            return allow_normal_victory ? GameOver::WINS_ARTIFACT | GameOver::WINS_ALL : GameOver::WINS_ARTIFACT;
        case 4:
            return GameOver::WINS_SIDE;
        case 5:
            return allow_normal_victory ? GameOver::WINS_GOLD | GameOver::WINS_ALL : GameOver::WINS_GOLD;
        default:
            break;
    }

    return GameOver::COND_NONE;
}

int Maps::FileInfo::ConditionLoss() const
{
    switch (conditions_loss)
    {
        case 0:
            return GameOver::LOSS_ALL;
        case 1:
            return GameOver::LOSS_TOWN;
        case 2:
            return GameOver::LOSS_HERO;
        case 3:
            return GameOver::LOSS_TIME;
        default:
            break;
    }

    return GameOver::COND_NONE;
}

bool Maps::FileInfo::WinsCompAlsoWins() const
{
    return comp_also_wins && ((GameOver::WINS_TOWN | GameOver::WINS_GOLD) & ConditionWins());
}

bool Maps::FileInfo::WinsAllowNormalVictory() const
{
    return allow_normal_victory &&
           ((GameOver::WINS_TOWN | GameOver::WINS_ARTIFACT | GameOver::WINS_GOLD) & ConditionWins());
}

int Maps::FileInfo::WinsFindArtifactID() const
{
    return wins1 ? wins1 - 1 : Artifact::UNKNOWN;
}

bool Maps::FileInfo::WinsFindUltimateArtifact() const
{
    return 0 == wins1;
}

u32 Maps::FileInfo::WinsAccumulateGold() const
{
    return wins1 * 1000;
}

Point Maps::FileInfo::WinsMapsPositionObject() const
{
    return Point(wins1, wins2);
}

Point Maps::FileInfo::LossMapsPositionObject() const
{
    return Point(loss1, loss2);
}

u32 Maps::FileInfo::LossCountDays() const
{
    return loss1;
}

int Maps::FileInfo::AllowCompHumanColors() const
{
    return allow_human_colors & allow_comp_colors;
}

int Maps::FileInfo::AllowHumanColors() const
{
    return allow_human_colors;
}

int Maps::FileInfo::AllowComputerColors() const
{
    return allow_comp_colors;
}

int Maps::FileInfo::HumanOnlyColors() const
{
    return allow_human_colors & ~(allow_comp_colors);
}

int Maps::FileInfo::ComputerOnlyColors() const
{
    return allow_comp_colors & ~(allow_human_colors);
}

bool Maps::FileInfo::isAllowCountPlayers(u32 colors) const
{
    u32 human_only = Color::Count(HumanOnlyColors());
    u32 comp_human = Color::Count(AllowCompHumanColors());

    return human_only <= colors && colors <= human_only + comp_human;
}

bool Maps::FileInfo::isMultiPlayerMap() const
{
    return 1 < Color::Count(HumanOnlyColors());
}

string Maps::FileInfo::String() const
{
    ostringstream os;

    os << "file: " << file << ", " << "name: " << name << ", " << "kingdom colors: " << static_cast<int>(kingdom_colors)
       << \
    ", " << "allow human colors: " << static_cast<int>(allow_human_colors) << ", " << "allow comp colors: "
       << static_cast<int>(allow_comp_colors) << ", " << "rnd races: " << \
    static_cast<int>(rnd_races) << ", " << "conditions wins: " << static_cast<int>(conditions_wins) << ", "
       << "comp also wins: " << (comp_also_wins ? "true" : "false") << \
    ", " << "allow normal victory: " << (allow_normal_victory ? "true" : "false") << ", " << "wins1: " << wins1 << ", "
       << "wins2: " << wins2 << ", " << "conditions loss: " << static_cast<int>(conditions_loss) << \
        ", " << "loss1: " << loss1 << ", " << "loss2: " << loss2;

    return os.str();
}

ListFiles GetMapsFiles(const char *suffix)
{
    const Settings &conf = Settings::Get();
    ListFiles maps = conf.GetListFiles("maps", suffix);
    const ListDirs &list = conf.GetMapsParams();

    if (list.empty())
        return maps;
    for (const auto &it : list)
        if (it != "maps")
            maps.Append(conf.GetListFiles(it, suffix));

    return maps;
}

bool PrepareMapsFileInfoList(MapsFileInfoList &lists, bool multi)
{
    const Settings &conf = Settings::Get();

    ListFiles maps_old = GetMapsFiles(".mp2");
    if (conf.PriceLoyaltyVersion())
        maps_old.Append(GetMapsFiles(".mx2"));

    for (ListFiles::const_iterator
                 it = maps_old.begin(); it != maps_old.end(); ++it)
    {
        Maps::FileInfo fi;
        if (fi.ReadMP2(*it)) lists.push_back(fi);
    }

    if (lists.empty()) return false;

    sort(lists.begin(), lists.end(), Maps::FileInfo::NameSorting);
    lists.resize(unique(lists.begin(), lists.end(), Maps::FileInfo::NameCompare) - lists.begin());

    if (!multi)
    {
        auto it = remove_if(lists.begin(), lists.end(),
                                                       mem_fun_ref(&Maps::FileInfo::isMultiPlayerMap));
        if (it != lists.begin()) lists.resize(distance(lists.begin(), it));
    }

    // set preferably count filter
    if (conf.PreferablyCountPlayers())
    {

        auto it = remove_if(lists.begin(), lists.end(),
                                                       not1(bind2nd(
                                                               mem_fun_ref(&Maps::FileInfo::isAllowCountPlayers),
                                                               conf.PreferablyCountPlayers())));
        if (it != lists.begin()) lists.resize(distance(lists.begin(), it));
    }

    return !lists.empty();
}

StreamBase &Maps::operator<<(StreamBase &msg, const FileInfo &fi)
{
    msg << System::GetBasename(fi.file) << fi.name << fi.description <<
        fi.size_w << fi.size_h << fi.difficulty << static_cast<u8>(KINGDOMMAX);

    for (u32 ii = 0; ii < KINGDOMMAX; ++ii)
        msg << fi.races[ii] << fi.unions[ii];

    msg << fi.kingdom_colors << fi.allow_human_colors << fi.allow_comp_colors <<
        fi.rnd_races << fi.conditions_wins << fi.comp_also_wins << fi.allow_normal_victory << fi.wins1 << fi.wins2 <<
        fi.conditions_loss << fi.loss1 << fi.loss2 << fi.localtime << fi.with_heroes;

    return msg;
}

StreamBase &Maps::operator>>(StreamBase &msg, FileInfo &fi)
{
    u8 kingdommax;

    msg >> fi.file >> fi.name >> fi.description >>
        fi.size_w >> fi.size_h >> fi.difficulty >> kingdommax;

    for (u32 ii = 0; ii < kingdommax; ++ii)
        msg >> fi.races[ii] >> fi.unions[ii];

    msg >> fi.kingdom_colors >> fi.allow_human_colors >> fi.allow_comp_colors >>
        fi.rnd_races >> fi.conditions_wins >> fi.comp_also_wins >> fi.allow_normal_victory >> fi.wins1 >> fi.wins2 >>
        fi.conditions_loss >> fi.loss1 >> fi.loss2 >> fi.localtime >> fi.with_heroes;

    return msg;
}
