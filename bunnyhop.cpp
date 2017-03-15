/*
 * Copyright (C) 2015 AnAkkk <anakin.cs@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "bunnyhop.h"
#include "utils.h"
#include "editablecommands.h"

#include "in_buttons.h"

CBunnyHop g_BunnyHop;

ConVar tftrue_bunnyhop("tftrue_bunnyhop", "0", FCVAR_NOTIFY, "Turn on/off Bunny hopping. It allows you to jump while ducked when enabled.", &CBunnyHop::Callback);

bool CBunnyHop::Init(const CModuleScanner& ServerModule)
{
#ifdef _LINUX
	PreventBunnyJumpingAddr = ServerModule.FindSymbol("_ZN15CTFGameMovement19PreventBunnyJumpingEv");
#else
	PreventBunnyJumpingAddr = ServerModule.FindSignature((unsigned char *)"\x56\x8B\xF1\x6A\x52\x8B\x8E\x00\x00\x00\x00\x81\xC1\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x84\xC0\x75\x7D", "xxxxxxx????xx????x????xxxx");
#endif
	if(!PreventBunnyJumpingAddr)
		Warning("Error Code 18\n");

	CheckJumpButtonRoute.RouteVirtualFunction(g_pGameMovement, &MyCGameMovement::CheckJumpButton, &CBunnyHop::CheckJumpButton);

	gameeventmanager->AddListener(this, "teamplay_round_start", true);

	return true;
}

void CBunnyHop::OnJoinClass(edict_t *pEntity)
{
	if(tftrue_bunnyhop.GetBool())
		Message(IndexOfEdict(pEntity),"\003[TFTrue] Bunny hopping is enabled on this server. Type !speedmeter [on/off] to see your speed while bunny hopping.");
}

void CBunnyHop::OnPlayerDisconnect(edict_t *pEntity)
{
	m_bSpeedMeter[IndexOfEdict(pEntity)-1] = false;
}

void CBunnyHop::OnUnload()
{
	gameeventmanager->RemoveListener(this);
}

void CBunnyHop::FireGameEvent(IGameEvent *pEvent)
{
	if( !strcmp(pEvent->GetName(), "teamplay_round_start"))
	{
		if(tftrue_bunnyhop.GetBool())
		{
			CBaseEntity *pEntity = nullptr;

			while((pEntity = g_pServerTools->FindEntityByClassname(pEntity, "func_door")) != nullptr)
				g_pServerTools->SetKeyValue(pEntity, "speed", 1999);
		}
	}
}

void CBunnyHop::Callback( IConVar *var, const char *pOldValue, float flOldValue )
{
	ConVarRef sv_airaccelerate("sv_airaccelerate");

	ConVar* v = (ConVar*)var;
	if (v->GetBool() && !flOldValue)
	{

		((EditableConVar*)sv_airaccelerate.GetLinkedConVar())->m_nFlags &= ~FCVAR_NOTIFY;
		sv_airaccelerate.SetValue(30);

		CBaseEntity *pEntity = nullptr;

		while((pEntity = g_pServerTools->FindEntityByClassname(pEntity, "func_door")) != nullptr)
			g_pServerTools->SetKeyValue(pEntity, "speed", 1999);

		g_BunnyHop.PreventBunnyJumpingRoute.RouteFunction(g_BunnyHop.PreventBunnyJumpingAddr, (void*)CBunnyHop::PreventBunnyJumping);
	}
	else if(!v->GetBool() && flOldValue)
	{
		((ConVar*)sv_airaccelerate.GetLinkedConVar())->Revert();
		((ConVar*)sv_airaccelerate.GetLinkedConVar())->AddFlags(FCVAR_NOTIFY);

		g_BunnyHop.PreventBunnyJumpingRoute.RestoreFunction();
	}

	if (v->GetBool() && !flOldValue)
	{
		PatchAddress(g_BunnyHop.CheckJumpButtonRoute.GetHookedFunctionAddress(), CheckJumpButton_FL_DUCKING_CHECK, 1, (unsigned char*)"\xEB");
		//PatchAddress(g_BunnyHop.CheckJumpButtonRoute.GetHookedFunctionAddress(), CheckJumpButton_ALREADY_JUMPING_AND_FL_DUCKING_CHECK, 1, (unsigned char*)"\xEB");
	}
	else if(!v->GetBool() && flOldValue)
	{
		PatchAddress(g_BunnyHop.CheckJumpButtonRoute.GetHookedFunctionAddress(), CheckJumpButton_FL_DUCKING_CHECK, 1, (unsigned char*)"\x74");
		//PatchAddress(g_BunnyHop.CheckJumpButtonRoute.GetHookedFunctionAddress(), CheckJumpButton_ALREADY_JUMPING_AND_FL_DUCKING_CHECK, 1, (unsigned char*)"\x74");
	}
}

bool CBunnyHop::CheckJumpButton(CGameMovement *pMovement EDX2)
{
	CBasePlayer *pPlayer = pMovement->player;
	CMoveData *pMoveData = pMovement->GetMoveData();

	if(pPlayer)
	{
		if(tftrue_bunnyhop.GetBool())
		{
			if(*g_EntityProps.GetSendProp<char>(pPlayer, "m_lifeState") == LIFE_ALIVE)
			{
				if((*g_EntityProps.GetDataMapProp<int>(pPlayer, "m_fFlags") & FL_ONGROUND) && (pMoveData->m_nOldButtons & IN_JUMP))
				{
					pMoveData->m_nOldButtons &= ~IN_JUMP;
				}
				if(g_BunnyHop.m_bSpeedMeter[pPlayer->entindex()-1])
				{
					Vector velocity = *g_EntityProps.GetSendProp<Vector>(pPlayer, "m_vecVelocity[0]");
					TextMessage(pPlayer->entindex(), "Speed meter: %.0f", sqrt(velocity.x*velocity.x + velocity.y*velocity.y));
				}
			}
		}
	}
	return g_BunnyHop.CheckJumpButtonRoute.CallOriginalFunction<CheckJumpButton_t>()(pMovement);
}

void CBunnyHop::PreventBunnyJumping()
{

}
