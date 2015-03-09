#pragma once

#include "SDK.h"

#include "valve_minmax_off.h"
#include "FunctionRoute.h"
#include "ModuleScanner.h"

#include <bitset>

class MyCGameMovement: public CGameMovement
{
public:
	virtual bool	CheckJumpButton( void );
};

class CBunnyHop: public IGameEventListener2
{
public:
	bool Init(const CModuleScanner& ServerModule);
	void OnJoinClass(edict_t *pEntity);
	void OnPlayerDisconnect(edict_t *pEntity);
	void OnUnload();
	void FireGameEvent(IGameEvent *pEvent);

	void SetSpeedMeter(int iEntIndex, bool bEnabled) { m_bSpeedMeter[iEntIndex] = bEnabled; }

	static void Callback( IConVar *var, const char *pOldValue, float flOldValue );
	static bool __fastcall CheckJumpButton(CGameMovement *pMovement EDX2);

	static void PreventBunnyJumping();
private:
	void *PreventBunnyJumpingAddr = nullptr;

	std::bitset<MAX_PLAYERS+1> m_bSpeedMeter;

	CFunctionRoute PreventBunnyJumpingRoute;
	CFunctionRoute CheckJumpButtonRoute;

	typedef bool (__thiscall *CheckJumpButton_t)( void *thisPtr );
};

extern CBunnyHop g_BunnyHop;

extern ConVar tftrue_bunnyhop;

#ifndef _LINUX
#define CheckJumpButton_FL_DUCKING_CHECK 0x102
#define CheckJumpButton_ALREADY_JUMPING_AND_FL_DUCKING_CHECK 0x113
#else
#define CheckJumpButton_FL_DUCKING_CHECK 0xDB
#define CheckJumpButton_ALREADY_JUMPING_AND_FL_DUCKING_CHECK 0xF8
#endif
