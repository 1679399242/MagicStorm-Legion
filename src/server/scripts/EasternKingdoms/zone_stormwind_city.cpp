/*
 * Copyright (C) 2008-2018 TrinityCore <https://www.trinitycore.org/>
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

#include "LFGMgr.h"
#include "LFGQueue.h"
#include "LFGPackets.h"
#include "DynamicObject.h"
#include "ScriptedEscortAI.h"
#include "CreatureTextMgr.h"
#include "MiscPackets.h"
#include "GameObjectAI.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "Player.h"
#include "ObjectMgr.h"
#include "Creature.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "ScriptMgr.h"
#include "ScriptedGossip.h"
#include "Vehicle.h"
#include "MotionMaster.h"
#include "TemporarySummon.h"
#include "GameObject.h"
#include "CombatAI.h"
#include "SpellInfo.h"
#include "GameObjectAI.h"
#include "Map.h"
#include "Transport.h"
#include "InstanceScript.h"
#include "DBCEnums.h"


#define GOSSIP_ACCEPT_DUEL      "Let''s duel"
#define EVENT_SPECIAL 20

//for base sql update
//UPDATE `creature_template` SET gossip_menu_id = 19861, `ScriptName` = 'npc_q42782' WHERE `entry` in(108757, 108743, 108752, 108751, 108723, 108748, 108744, 108746, 108765, 108767, 108755, 108742, 108750, 108754, 108747, 108749, 108753, 108759, 108758, 108745, 108756);
enum eDuelEnums
{
    SAY_DUEL_A = 0,
    SAY_DUEL_B = 1,
    SAY_DUEL_C = 2,
    SAY_DUEL_D = 3,
    SAY_DUEL_E = 4,
    SAY_DUEL_F = 5,
    SAY_DUEL_G = 6,
    SAY_DUEL_H = 7,
    SAY_DUEL_I = 8,

    SPELL_DUEL = 52996,
    //SPELL_DUEL_TRIGGERED        = 52990,
    SPELL_DUEL_VICTORY = 52994,
    SPELL_DUEL_FLAG = 52991,

    QUEST_42782 = 42782,
    QUEST_44281 = 44281,
    FACTION_HOSTILE = 2068,
    SPELL_CAST_GOB = 215387,
	SPELL_CAST_GOB_D = 215598,
	SPELL_CAST_GOB_F = 215607
};

int32 _m_auiRandomSay[] =
{
    SAY_DUEL_A, SAY_DUEL_B, SAY_DUEL_C, SAY_DUEL_D, SAY_DUEL_E, SAY_DUEL_F, SAY_DUEL_G, SAY_DUEL_H, SAY_DUEL_I
};


class npc_q42782 : public CreatureScript
{
public:
    npc_q42782(const std::string str) : CreatureScript(str.c_str()) { }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action) override
    {
        ClearGossipMenuFor(player);
        if (action == GOSSIP_ACTION_INFO_DEF)
        {
            CloseGossipMenuFor(player);
            if (player->IsInCombat() || creature->IsInCombat())
                return true;

            if (npc_q42782AI* pInitiateAI = CAST_AI(npc_q42782::npc_q42782AI, creature->AI()))
            {
                if (pInitiateAI->m_bIsDuelInProgress)
                    return true;
            }
            creature->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC);
            creature->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_UNK_15);

            player->CastSpell(creature, SPELL_DUEL, false);
            player->CastSpell(player, SPELL_DUEL_FLAG, true);
        }
        return true;
    }

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        if ((player->GetQuestStatus(QUEST_42782) == QUEST_STATUS_INCOMPLETE ||
            player->GetQuestStatus(QUEST_44281) == QUEST_STATUS_INCOMPLETE)
            && creature->IsFullHealth())
        {
            if (player->HealthBelowPct(10))
                return true;

            if (player->IsInCombat() || creature->IsInCombat())
                return true;

            AddGossipItemFor(player, GOSSIP_ICON_CHAT, GOSSIP_ACCEPT_DUEL, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF);
            SendGossipMenuFor(player, player->GetGossipTextId(creature), creature->GetGUID());
        }
        return true;
    }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_q42782AI(creature);
    }

    struct npc_q42782AI : public CombatAI
    {
        npc_q42782AI(Creature* creature) : CombatAI(creature)
        {
            m_bIsDuelInProgress = false;
        }

        bool lose;
        ObjectGuid m_uiDuelerGUID;
        uint32 m_uiDuelTimer;
        bool m_bIsDuelInProgress;
        uint32 spelltimer;

        void Reset() override
        {
            lose = false;
            me->RestoreFaction();
            CombatAI::Reset();

            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_UNK_15);

            m_uiDuelerGUID.Clear();
            m_uiDuelTimer = 5000;
            spelltimer = 2000;
            m_bIsDuelInProgress = false;
        }

        void SpellHit(Unit* pCaster, const SpellInfo* spell) override
        {
            if (!m_bIsDuelInProgress && spell->Id == SPELL_DUEL)
            {
                m_uiDuelerGUID = pCaster->GetGUID();
                m_bIsDuelInProgress = true;
            }
        }

        void DamageTaken(Unit* pDoneBy, uint32 &uiDamage/*, DamageEffectType dmgType*/) override
        {
            if (m_bIsDuelInProgress && pDoneBy->IsControlledByPlayer())
            {
                if (pDoneBy->GetGUID() != m_uiDuelerGUID && pDoneBy->GetOwnerGUID() != m_uiDuelerGUID) // other players cannot help
                    uiDamage = 0;
                else if (uiDamage >= me->GetHealth())
                {
                    uiDamage = 0;

                    if (!lose)
                    {
                        if (Player* plr = pDoneBy->ToPlayer())
                            plr->KilledMonsterCredit(108722);

                        pDoneBy->RemoveGameObject(SPELL_DUEL_FLAG, true);
                        pDoneBy->AttackStop();
                        me->CastSpell(pDoneBy, SPELL_DUEL_VICTORY, true);
                        lose = true;
                        me->CastSpell(me, 7267, true);
                        me->RestoreFaction();
                    }
                }
            }
        }

        void UpdateAI(uint32 uiDiff) override
        {
            if (!UpdateVictim())
            {
                if (m_bIsDuelInProgress)
                {
                    if (m_uiDuelTimer <= uiDiff)
                    {
                        me->setFaction(FACTION_HOSTILE);

                        if (Unit* unit = ObjectAccessor::GetUnit(*me, m_uiDuelerGUID))
                            AttackStart(unit);
                    }
                    else
                        m_uiDuelTimer -= uiDiff;
                }
                return;
            }

            if (m_bIsDuelInProgress)
            {
                if (lose)
                {
                    if (!me->HasAura(7267))
                        EnterEvadeMode();
                    return;
                }
                else if (me->GetVictim() && me->GetVictim()->GetTypeId() == TYPEID_PLAYER && me->GetVictim()->HealthBelowPct(10))
                {
                    me->GetVictim()->CastSpell(me->GetVictim(), 7267, true); // beg
                    me->GetVictim()->RemoveGameObject(SPELL_DUEL_FLAG, true);
                    EnterEvadeMode();
                    return;
                }

                if (!UpdateVictim() || me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                if (spelltimer <= uiDiff)
                {
                    uint32 spell = 0;
                    switch (me->GetEntry())
                    {
                    case 108752:
                        spell = 172673;
                        spelltimer = 2000;
                        break;
                    case 108765:
                        spell = 198623;
                        spelltimer = 2000;
                        break;
                    case 108767:
                    case 108750:
                        spell = 172757;
                        spelltimer = 11000;
                        break;
                    case 108723:
                        spell = 171777;
                        spelltimer = 5500;
                        break;
                    default:
                        break;
                    }
                    if (spell > 0)
                        DoCast(spell);
                }
                else
                    spelltimer -= uiDiff;
            }

            // TODO: spells

            CombatAI::UpdateAI(uiDiff);
        }
    };
};

class npc_q44281 : public npc_q42782
{
public:
    npc_q44281() : npc_q42782("npc_q44281") { }
};
 
class gob_q42782 : public GameObjectScript
{
public:
    gob_q42782() : GameObjectScript("gob_q42782") { }
 
    bool OnGossipHello(Player* player, GameObject* go) override
    {
        go->UseDoorOrButton();
        if (player->GetQuestStatus(QUEST_42782) == QUEST_STATUS_INCOMPLETE)
        {
            player->CastSpell(player, SPELL_CAST_GOB, true);
            return true;
        }
        return false;
    }
};

class gob_qq42782 : public GameObjectScript
{
public:
    gob_qq42782() : GameObjectScript("gob_qq42782") { }
 
    bool OnGossipHello(Player* player, GameObject* go) override
    {
        go->UseDoorOrButton();
        if (player->GetQuestStatus(QUEST_42782) == QUEST_STATUS_INCOMPLETE)
        {
            player->CastSpell(player, SPELL_CAST_GOB_D, true);
            return true;
        }
        return false;
    }
};

class gob_qqq42782 : public GameObjectScript
{
public:
    gob_qqq42782() : GameObjectScript("gob_qqq42782") { }
 
    bool OnGossipHello(Player* player, GameObject* go) override
    {
        go->UseDoorOrButton();
        if (player->GetQuestStatus(QUEST_42782) == QUEST_STATUS_INCOMPLETE)
        {
            player->CastSpell(player, SPELL_CAST_GOB_F, true);
            return true;
        }
        return false;
    }
};

class npc_q42740 : public CreatureScript
{
public:
    npc_q42740() : CreatureScript("npc_q42740") { }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action) override
    {
        player->PlayerTalkClass->ClearMenus();

        //214608
        player->KilledMonsterCredit(creature->GetEntry());
        player->CastSpell(player, 216356, false); //scene
        player->TeleportTo(1220, -1879.096436f, 3005.628418f, 0.032851f, player->GetOrientation());
        player->KilledMonsterCredit(90918);
        player->KilledMonsterCredit(108920);

        return true;
    };
};

class npc_q40518 : public CreatureScript
{
public:
    npc_q40518() : CreatureScript("npc_q40518") { }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action) override
    {
        player->PlayerTalkClass->ClearMenus();

        //214608
        player->KilledMonsterCredit(creature->GetEntry());
        player->CastSpell(player, 225147, false); //scene

        return true;
    };
};


//! 227058 WARN! Spell not exits.
class spell_q42740 : public SpellScriptLoader
{
public:
    spell_q42740() : SpellScriptLoader("spell_q42740") { }

    class spell_q42740_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_q42740_SpellScript);

        enum data
        {
            QUEST = 42740,
            NPC_CONV = 581
        };

        void HandleScriptEffect(SpellEffIndex effIndex)
        {
            PreventHitDefaultEffect(EFFECT_0);
            if (Unit* caster = GetCaster())
            {
                Player *player = caster->ToPlayer();
                if (!player)
                    return;

                std::set<uint32> Slot;
                Slot.insert(908);
                sLFGMgr->JoinLfg(player, player->GetPrimarySpecialization(), Slot);
            }
        }

        void Register() override
        {
            OnEffectHitTarget += SpellEffectFn(spell_q42740_SpellScript::HandleScriptEffect, EFFECT_0, SPELL_EFFECT_DUMMY);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_q42740_SpellScript();
    }
};

//! 217781 phase update.
class spell_bi_enter_stage1 : public SpellScriptLoader
{
public:
    spell_bi_enter_stage1() : SpellScriptLoader("spell_bi_enter_stage1") { }

    class spell_bi_enter_stage1_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_bi_enter_stage1_AuraScript);


        void OnApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            if (Unit* caster = GetCaster())
            {
                Player *player = caster->ToPlayer();
                if (!player)
                    return;


                Map* m = player->FindMap();
                if (!m)
                    return;
                InstanceScript *script = player->GetInstanceScript();
                if (!script)
                    return;

                ObjectGuid guid = script->GetGuidData(player->GetTeam() == HORDE ? 255203 : 251604);
                if (!guid)
                    return;

                /*if (GameObject *go = m->GetGameObject(guid))
                    go->SetVisible(true);*/

                /*if (script->GetScenarionStep() != 0)
                    return;*/

                //scenation ID 1189 step 0
                player->UpdateCriteria(CRITERIA_TYPE_SEND_EVENT_SCENARIO, 54140);

                //scenation ID 786 step 0
                player->UpdateCriteria(CRITERIA_TYPE_SEND_EVENT_SCENARIO, 44060);


                Map::PlayerList const &PlList = player->GetMap()->GetPlayers();

                if (PlList.isEmpty())
                    return;

                for (Map::PlayerList::const_iterator i = PlList.begin(); i != PlList.end(); ++i)
                    if (Player* plr = i->GetSource())
                    {
                        if (Transport* transport = plr->GetTransport())
                        {
                            transport->RemovePassenger(plr);
                            plr->SetTransport(NULL);
                            plr->m_movementInfo.transport.Reset();
                        }

                        plr->SendMovieStart(486);
                        plr->CastSpell(plr, plr->GetTeam() == ALLIANCE ? 199358 : 225152, false);
                    }
            }
        }

        void Register() override
        {
            AfterEffectApply += AuraEffectApplyFn(spell_bi_enter_stage1_AuraScript::OnApply, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
        }
    };

    AuraScript* GetAuraScript() const override
    {
        return new spell_bi_enter_stage1_AuraScript();
    }
};

// quest 41106
class npc_tele_q41106 : public CreatureScript
{
public:
    npc_tele_q41106() : CreatureScript("npc_tele_q41106") { }
     struct npc_tele_q41106AI : public ScriptedAI
    {
        npc_tele_q41106AI(Creature* creature) : ScriptedAI(creature) { }
         void MoveInLineOfSight(Unit* who) override
        {
            if (Player* player = who->ToPlayer())
            {
                if (player->GetQuestStatus(41106) == QUEST_STATUS_INCOMPLETE)
                {    
                    if (player->IsInDist(me, 7.5f))
                    {
                       player->KilledMonsterCredit(103136);
                    }
                }
            }
        }
    };
     CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_tele_q41106AI(creature);
    }
};

// quest 41106
class npc_tele_qq41106 : public CreatureScript
{
public:
    npc_tele_qq41106() : CreatureScript("npc_tele_qq41106") { }
     struct npc_tele_qq41106AI : public ScriptedAI
    {
        npc_tele_qq41106AI(Creature* creature) : ScriptedAI(creature) { }
         void MoveInLineOfSight(Unit* who) override
        {
            if (Player* player = who->ToPlayer())
            {
                if (player->GetQuestStatus(41106) == QUEST_STATUS_INCOMPLETE)
                {    
                    if (player->IsInDist(me, 9.5f))
                    {
                       player->KilledMonsterCredit(103184);
                    }
                }
            }
        }
    };
     CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_tele_qq41106AI(creature);
    }
};

// quest 41106
class npc_tele_qqq41106 : public CreatureScript
{
public:
    npc_tele_qqq41106() : CreatureScript("npc_tele_qqq41106") { }
     struct npc_tele_qqq41106AI : public ScriptedAI
    {
        npc_tele_qqq41106AI(Creature* creature) : ScriptedAI(creature) { }
         void MoveInLineOfSight(Unit* who) override
        {
            if (Player* player = who->ToPlayer())
            {
                if (player->GetQuestStatus(41106) == QUEST_STATUS_INCOMPLETE)
                {    
                    if (player->IsInDist(me, 7.5f))
                    {
                       player->KilledMonsterCredit(103133);
                    }
                }
            }
        }
    };
     CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_tele_qqq41106AI(creature);
    }
};

// quest 40644
class npc_tele_q40644 : public CreatureScript
{
public:
    npc_tele_q40644() : CreatureScript("npc_tele_q40644") { }
     struct npc_tele_q40644AI : public ScriptedAI
    {
        npc_tele_q40644AI(Creature* creature) : ScriptedAI(creature) { }
         void MoveInLineOfSight(Unit* who) override
        {
            if (Player* player = who->ToPlayer())
            {
                if (player->GetQuestStatus(40644) == QUEST_STATUS_INCOMPLETE)
                {    
                    if (player->IsInDist(me, 9.5f))
                    {
                       player->KilledMonsterCredit(101064);
                    }
                }
            }
        }
    };
     CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_tele_q40644AI(creature);
    }
};

// quest 40644
class npc_tele_qq40644 : public CreatureScript
{
public:
    npc_tele_qq40644() : CreatureScript("npc_tele_qq40644") { }
     struct npc_tele_qq40644AI : public ScriptedAI
    {
        npc_tele_qq40644AI(Creature* creature) : ScriptedAI(creature) { }
         void MoveInLineOfSight(Unit* who) override
        {
            if (Player* player = who->ToPlayer())
            {
                if (player->GetQuestStatus(40644) == QUEST_STATUS_INCOMPLETE)
                {    
                    if (player->IsInDist(me, 7.5f))
                    {
                       player->KilledMonsterCredit(103343);
                    }
                }
            }
        }
    };
     CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_tele_qq40644AI(creature);
    }
};

// quest 40644
class npc_tele_qqq40644 : public CreatureScript
{
public:
    npc_tele_qqq40644() : CreatureScript("npc_tele_qqq40644") { }
     struct npc_tele_qqq40644AI : public ScriptedAI
    {
        npc_tele_qqq40644AI(Creature* creature) : ScriptedAI(creature) { }
         void MoveInLineOfSight(Unit* who) override
        {
            if (Player* player = who->ToPlayer())
            {
                if (player->GetQuestStatus(40644) == QUEST_STATUS_INCOMPLETE)
                {    
                    if (player->IsInDist(me, 7.5f))
                    {
                       player->KilledMonsterCredit(103346);
                    }
                }
            }
        }
    };
     CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_tele_qqq40644AI(creature);
    }
};

void AddSC_stormwind_city()
{
    new npc_q42782("npc_q42782");
    new gob_q42782();
	new gob_qq42782();
	new gob_qqq42782();
    new npc_q42740();
    new npc_q40518();
    new spell_bi_enter_stage1();
    new spell_q42740();
	new npc_tele_q41106();
	new npc_tele_qq41106();
	new npc_tele_qqq41106();
	new npc_tele_q40644();
	new npc_tele_qq40644();
	new npc_tele_qqq40644();
}

