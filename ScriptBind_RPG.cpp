#include "StdAfx.h"
#include "ScriptBind_RPG.h"
#include "Player.h"

CScriptBind_RPG::CScriptBind_RPG(ISystem *pSystem)
{
	Init(pSystem->GetIScriptSystem(), pSystem);

	#undef SCRIPT_REG_CLASSNAME
	#define SCRIPT_REG_CLASSNAME &CScriptBind_RPG::

	SCRIPT_REG_TEMPLFUNC(AddXP, "amount, awardedFrom");
}

int CScriptBind_RPG::AddXP(IFunctionHandler *pHandler, int amount, EntityId awardedFrom)
{
	CPlayer::GetHero()->AddXP(amount, awardedFrom);
	return pHandler->EndFunction();
}