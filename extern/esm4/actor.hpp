/*
  Copyright (C) 2020 cc9cii

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

  cc9cii cc9c@iinet.net.au

  Much of the information on the data structures are based on the information
  from Tes4Mod:Mod_File_Format and Tes5Mod:File_Formats but also refined by
  trial & error.  See http://en.uesp.net/wiki for details.

*/
#ifndef ESM4_ACTOR_H
#define ESM4_ACTOR_H

#include <cstdint>

#include "formid.hpp"

namespace ESM4
{
#pragma pack(push, 1)
    struct AIData        // NPC_, CREA
    {
        std::uint8_t  aggression;
        std::uint8_t  confidence;
        std::uint8_t  energyLevel;
        std::uint8_t  responsibility;
        std::uint32_t aiFlags;
        std::uint8_t  trainSkill;
        std::uint8_t  trainLevel;
        std::uint16_t unknown;
    };

    struct AttributeValues
    {
        std::uint8_t  strength;
        std::uint8_t  intelligence;
        std::uint8_t  willpower;
        std::uint8_t  agility;
        std::uint8_t  speed;
        std::uint8_t  endurance;
        std::uint8_t  personality;
        std::uint8_t  luck;
    };

    struct ACBS_TES4
    {
        std::uint32_t flags;
        std::uint16_t baseSpell;
        std::uint16_t fatigue;
        std::uint16_t barterGold;
        std::int16_t  levelOrOffset;
        std::uint16_t calcMin;
        std::uint16_t calcMax;
        std::uint32_t padding1;
        std::uint32_t padding2;
    };

    struct ACBS_FO3
    {
        std::uint32_t flags;
        std::uint16_t fatigue;
        std::uint16_t barterGold;
        std::int16_t  levelOrMult;
        std::uint16_t calcMinlevel;
        std::uint16_t calcMaxlevel;
        std::uint16_t speedMultiplier;
        float         karma;
        std::int16_t  dispositionBase;
        std::uint16_t templateFlags;
    };

    struct ACBS_TES5
    {
        std::uint32_t flags;
        std::uint16_t magickaOffset;
        std::uint16_t staminaOffset;
        std::uint16_t levelOrMult;     // TODO: check if int16_t
        std::uint16_t calcMinlevel;
        std::uint16_t calcMaxlevel;
        std::uint16_t speedMultiplier;
        std::uint16_t dispositionBase; // TODO: check if int16_t
        std::uint16_t templateFlags;
        std::uint16_t healthOffset;
        std::uint16_t bleedoutOverride;
    };

    union ActorBaseConfig
    {
        ACBS_TES4 tes4;
        ACBS_FO3  fo3;
        ACBS_TES5 tes5;
    };

    struct ActorFaction
    {
        FormId       faction;
        std::int8_t  rank;
        std::uint8_t unknown1;
        std::uint8_t unknown2;
        std::uint8_t unknown3;
    };
#pragma pack(pop)
}

#endif // ESM4_ACTOR_H
