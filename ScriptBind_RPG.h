#include "IScriptSystem.h"

class CScriptBind_RPG : public CScriptableBase
{
	CScriptBind_RPG(ISystem *pSystem);
	int AddXP(IFunctionHandler *pHandler, int value, EntityId awardedFrom);
};