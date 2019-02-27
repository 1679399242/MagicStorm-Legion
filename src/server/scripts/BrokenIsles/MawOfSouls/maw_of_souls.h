/*
* Copyright (C) 2017-2018 AshamaneProject <https://github.com/AshamaneProject>
* Copyright (C) 2008-2017 TrinityCore <http://www.trinitycore.org/>
*
* This program is free software; you can redistribute it and/or modify it
* under the terms of the GNU General Public License as published by the
* Free Software Foundation; either version 2 of the License, or (at your
* option) any later version.
*
* This program is distributed in the hope that it will be useful, but WITHOUT
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
* FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
* more details.
*
* You should have received a copy of the GNU General Public License along
* with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef MAW_OF_SOULS_H_
#define MAW_OF_SOULS_H_

#define MawOfSoulsScriptName "instance_maw_of_souls"
#define DataHeader "MOS"

uint32 const EncounterCount = 3;

enum MOSDataTypes
{
    // Encounter States/Boss GUIDs
    DATA_YMIRON_MAW     = 0,
    DATA_HARBARON_MAW   = 1,
    DATA_HELYA_MAW      = 2,
};

enum MOSCreatureIds
{
    // NPCs
    NPC_YMIRON_MAW      = 96756,
    NPC_HARBARON_MAW    = 96754,
    NPC_SKJAL_MAW       = 99307,
    NPC_HELYA_MAW       = 96759,

    // Triggers
};
enum GameObjects
{
    GOB_DOOR_HARBARON_DOOR = 246888,
    GOB_DOOR_HARBARON_WALL_SOUL_1 = 246947,//
    GOB_DOOR_HARBARON_WALL_SOUL_2 = 246947,
    GOB_DOOR_HARBARON_COLLISON_1 = 243403,
    GOB_DOOR_HARBARON_COLLISON_2 = 243403,

    GOB_DOOR_SKJAL_WALL_1 = 265476,
    GOB_DOOR_SKJAL_WALL_2 = 265476,//

    GOB_ECHOING_HORN_OF_THE_DAMNED = 247041, //���ͺŽ�
};

template<class AI>
AI* GetMawOfSoulsAI(Creature* creature)
{
    return GetInstanceAI<AI>(creature, MawOfSoulsScriptName);
}

#endif // MAW_OF_SOULS_H_
