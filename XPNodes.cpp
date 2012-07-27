#include "StdAfx.h"
#include "BaseNode.h"
#include "Player.h"

class CAddXPNode : public CBaseNode
{
private:
	enum EInputs
	{
		EIP_Activate,
		EIP_Amount
	};

public:
	CAddXPNode(SActivationInfo *pActInfo) { }

	virtual void GetConfiguration(SFlowNodeConfig &config)
	{
		static const SInputPortConfig inputs[] =
		{
			InputPortConfig_Void("Activate"),
			InputPortConfig<int>("Amount", 0),
			{0}
		};

		config.nFlags |= EFLN_TARGET_ENTITY;
		config.pInputPorts = inputs;
		config.SetCategory(EFLN_APPROVED);
	}

	virtual void OnActivate()
	{
		if(IsActive(EIP_Activate))
			CPlayer::GetHero()->AddXP(GetPortValue<int>(EIP_Amount), GetTargetEntityId());
	}
};

class CXPListenerNode : public CBaseNode, public IPlayerXPEventListener
{
private:
	enum EInputs
	{
		EIP_Enable,
		EIP_Disable
	};

	enum EOutputs
	{
		EOP_OnXPChanged,
		EOP_OnLevelUp,
		EOP_AwardedFrom
	};

public:
	CXPListenerNode(SActivationInfo *pActInfo) { }

	// IPlayerXPEventListener
	virtual void OnXPChange(int xp, EntityId awardedFrom)
	{
		Activate(EOP_AwardedFrom, awardedFrom);
		Activate(EOP_OnXPChanged, xp);
	}

	virtual void OnLevelChange(int level)
	{
		Activate(EOP_OnLevelUp, level);
	}
	// ~IPlayerXPEventListener

	virtual void GetConfiguration(SFlowNodeConfig &config)
	{
		static const SInputPortConfig inputs[] =
		{
			InputPortConfig_Void("Enable"),
			InputPortConfig_Void("Disable"),
			{0}
		};

		static const SOutputPortConfig outputs[] =
		{
			OutputPortConfig<int>("OnXPGained"),
			OutputPortConfig<int>("OnLevelUp"),
			OutputPortConfig<EntityId>("AwardedFrom"),
			{0}
		};

		config.pInputPorts = inputs;
		config.pOutputPorts = outputs;
		config.SetCategory(EFLN_APPROVED);
	}

	virtual void OnActivate()
	{
		if(IsActive(EIP_Enable))
			CPlayer::GetHero()->RegisterXPListener(this);
		else if(IsActive(EIP_Disable))
			CPlayer::GetHero()->UnregisterXPListener(this);
	}
};

REGISTER_FLOW_NODE("RPG:AddXP", CAddXPNode);
REGISTER_FLOW_NODE("RPG:XPListener", CXPListenerNode);