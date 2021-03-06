/* Copyright (C) 2006 - 2011 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* ScriptData
SDName: Stratholme
SD%Complete: 100
SDComment: Misc mobs for instance. GO-script to apply aura and start event for quest 8945
SDCategory: Stratholme
EndScriptData */

/* ContentData
go_gauntlet_gate
mob_freed_soul
mob_restless_soul
mobs_spectral_ghostly_citizen
EndContentData */

#include "precompiled.h"
#include "stratholme.h"

/*######
## go_service_gate
######*/

bool GOUse_go_service_gate(Player* pPlayer, GameObject* pGo)
{
    ScriptedInstance* pInstance = (ScriptedInstance*)pGo->GetInstanceData();

    if (!pInstance)
        return false;

    if (pInstance->GetData(TYPE_BARTHILAS_RUN) != NOT_STARTED)
        return false;

    // if the service gate is used make Barthilas flee
    pInstance->SetData(TYPE_BARTHILAS_RUN, IN_PROGRESS);
    return false;
}

/*######
## go_gauntlet_gate (this is the _first_ of the gauntlet gates, two exist)
######*/

bool GOUse_go_gauntlet_gate(Player* pPlayer, GameObject* pGo)
{
    ScriptedInstance* pInstance = (ScriptedInstance*)pGo->GetInstanceData();

    if (!pInstance)
        return false;

    if (pInstance->GetData(TYPE_BARON_RUN) != NOT_STARTED)
        return false;

    if (Group *pGroup = pPlayer->GetGroup())
    {
        for(GroupReference *itr = pGroup->GetFirstMember(); itr != NULL; itr = itr->next())
        {
            Player* pGroupie = itr->getSource();
            if (!pGroupie)
                continue;

            if (!pGroupie->HasAura(SPELL_BARON_ULTIMATUM))
                pGroupie->CastSpell(pGroupie, SPELL_BARON_ULTIMATUM, true);
        }
    }
    else
    {
        if (!pPlayer->HasAura(SPELL_BARON_ULTIMATUM))
            pPlayer->CastSpell(pPlayer, SPELL_BARON_ULTIMATUM, true);
    }

    pInstance->SetData(TYPE_BARON_RUN, IN_PROGRESS);
    return false;
}

/*######
## mob_freed_soul
######*/

// Possibly more of these quotes around.
enum
{
    SAY_ZAPPED0     = -1329000,
    SAY_ZAPPED1     = -1329001,
    SAY_ZAPPED2     = -1329002,
    SAY_ZAPPED3     = -1329003,
};

struct MANGOS_DLL_DECL mob_freed_soulAI : public ScriptedAI
{
    mob_freed_soulAI(Creature* pCreature) : ScriptedAI(pCreature) { Reset(); }

    void Reset()
    {
        switch(urand(0, 3))
        {
            case 0: DoScriptText(SAY_ZAPPED0, m_creature); break;
            case 1: DoScriptText(SAY_ZAPPED1, m_creature); break;
            case 2: DoScriptText(SAY_ZAPPED2, m_creature); break;
            case 3: DoScriptText(SAY_ZAPPED3, m_creature); break;
        }
    }
};

CreatureAI* GetAI_mob_freed_soul(Creature* pCreature)
{
    return new mob_freed_soulAI(pCreature);
}

/*######
## mob_restless_soul
######*/

enum
{
    QUEST_RESTLESS_SOUL     = 5282,

    SPELL_EGAN_BLASTER      = 17368,
    SPELL_SOUL_FREED        = 17370,

    NPC_RESTLESS_SOUL      = 11122,
    NPC_FREED_SOUL         = 11136,
};

struct MANGOS_DLL_DECL mob_restless_soulAI : public ScriptedAI
{
    mob_restless_soulAI(Creature* pCreature) : ScriptedAI(pCreature) { Reset(); }

    uint64 m_uiTaggerGUID;
    uint32 m_uiDieTimer;
    bool m_bIsTagged;

    void Reset()
    {
        m_uiTaggerGUID = 0;
        m_uiDieTimer = 5000;
        m_bIsTagged = false;
    }

    void SpellHit(Unit* pCaster, const SpellEntry* pSpell)
    {
        if (pCaster->GetTypeId() == TYPEID_PLAYER)
        {
            if (!m_bIsTagged && pSpell->Id == SPELL_EGAN_BLASTER && ((Player*)pCaster)->GetQuestStatus(QUEST_RESTLESS_SOUL) == QUEST_STATUS_INCOMPLETE)
            {
                m_bIsTagged = true;
                m_uiTaggerGUID = pCaster->GetGUID();
            }
        }
    }

    void JustSummoned(Creature* pSummoned)
    {
        pSummoned->CastSpell(pSummoned, SPELL_SOUL_FREED, false);
    }

    void JustDied(Unit* Killer)
    {
        if (m_bIsTagged)
            m_creature->SummonCreature(NPC_FREED_SOUL, 0.0f, 0.0f, 0.0f, 0.0f, TEMPSUMMON_TIMED_DESPAWN, 300000);
    }

    void UpdateAI(const uint32 uiDiff)
    {
        if (m_bIsTagged)
        {
            if (m_uiDieTimer < uiDiff)
            {
                if (Player* pPlayer = m_creature->GetMap()->GetPlayer(m_uiTaggerGUID))
                    pPlayer->DealDamage(m_creature, m_creature->GetHealth(), NULL, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, NULL, false);
            }
            else
                m_uiDieTimer -= uiDiff;
        }
    }
};

CreatureAI* GetAI_mob_restless_soul(Creature* pCreature)
{
    return new mob_restless_soulAI(pCreature);
}

/*######
## mobs_spectral_ghostly_citizen
######*/

enum
{
    SPELL_HAUNTING_PHANTOM  = 16336,
    SPELL_SLAP              = 6754
};

struct MANGOS_DLL_DECL mobs_spectral_ghostly_citizenAI : public ScriptedAI
{
    mobs_spectral_ghostly_citizenAI(Creature* pCreature) : ScriptedAI(pCreature) {Reset();}

    uint32 m_uiDieTimer;
    bool m_bIsTagged;

    void Reset()
    {
        m_uiDieTimer = 5000;
        m_bIsTagged = false;
    }

    void SpellHit(Unit* pCaster, const SpellEntry* pSpell)
    {
        if (!m_bIsTagged && pSpell->Id == SPELL_EGAN_BLASTER)
            m_bIsTagged = true;
    }

    void JustDied(Unit* Killer)
    {
        if (m_bIsTagged)
        {
            for(uint32 i = 0; i < 4; ++i)
            {
                float x, y, z;
                m_creature->GetRandomPoint(m_creature->GetPositionX(), m_creature->GetPositionY(), m_creature->GetPositionZ(), 20.0f, x, y, z);

                // 100%, 50%, 33%, 25% chance to spawn
                uint32 j = urand(0, i);
                if (j == 0)
                    m_creature->SummonCreature(NPC_RESTLESS_SOUL, x, y, z, 0, TEMPSUMMON_DEAD_DESPAWN, 0);
            }
        }
    }

    void UpdateAI(const uint32 uiDiff)
    {
        if (m_bIsTagged)
        {
            if (m_uiDieTimer < uiDiff)
            {
                m_creature->DealDamage(m_creature, m_creature->GetHealth(), NULL, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, NULL, false);
            }
            else
                m_uiDieTimer -= uiDiff;
        }

        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            return;

        DoMeleeAttackIfReady();
    }

    void ReceiveEmote(Player* pPlayer, uint32 uiEmote)
    {
        switch(uiEmote)
        {
            case TEXTEMOTE_DANCE:
                EnterEvadeMode();
                break;
            case TEXTEMOTE_RUDE:
                if (m_creature->IsWithinDistInMap(pPlayer, INTERACTION_DISTANCE))
                    m_creature->CastSpell(pPlayer, SPELL_SLAP, false);
                else
                    m_creature->HandleEmote(EMOTE_ONESHOT_RUDE);
                break;
            case TEXTEMOTE_WAVE:
                m_creature->HandleEmote(EMOTE_ONESHOT_WAVE);
                break;
            case TEXTEMOTE_BOW:
                m_creature->HandleEmote(EMOTE_ONESHOT_BOW);
                break;
            case TEXTEMOTE_KISS:
                m_creature->HandleEmote(EMOTE_ONESHOT_FLEX);
                break;
        }
    }
};

CreatureAI* GetAI_mobs_spectral_ghostly_citizen(Creature* pCreature)
{
    return new mobs_spectral_ghostly_citizenAI(pCreature);
}

void AddSC_stratholme()
{
    Script* pNewScript;

    pNewScript = new Script;
    pNewScript->Name = "go_service_gate";
    pNewScript->pGOUse = &GOUse_go_service_gate;
    pNewScript->RegisterSelf();

    pNewScript = new Script;
    pNewScript->Name = "go_gauntlet_gate";
    pNewScript->pGOUse = &GOUse_go_gauntlet_gate;
    pNewScript->RegisterSelf();

    pNewScript = new Script;
    pNewScript->Name = "mob_freed_soul";
    pNewScript->GetAI = &GetAI_mob_freed_soul;
    pNewScript->RegisterSelf();

    pNewScript = new Script;
    pNewScript->Name = "mob_restless_soul";
    pNewScript->GetAI = &GetAI_mob_restless_soul;
    pNewScript->RegisterSelf();

    pNewScript = new Script;
    pNewScript->Name = "mobs_spectral_ghostly_citizen";
    pNewScript->GetAI = &GetAI_mobs_spectral_ghostly_citizen;
    pNewScript->RegisterSelf();
}
