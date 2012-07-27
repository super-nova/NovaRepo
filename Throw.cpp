/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id$
$DateTime$

-------------------------------------------------------------------------
History:
- 11:9:2005   15:00 : Created by Marcio Martins

*************************************************************************/
#include "StdAfx.h"
#include "Throw.h"
#include "Actor.h"
#include "Player.h"
#include "Game.h"
#include "Projectile.h"
#include "WeaponSystem.h"
#include "OffHand.h"

#include "WeaponSharedParams.h"


//------------------------------------------------------------------------
CThrow::CThrow()
	: m_throwableId(0),
	  m_throwableAction(0),
	  m_usingGrenade(true),
	  m_forceNextThrow(false),
	  m_throw_time(0.0f),
	  m_vFireTarget(ZERO)
{
}

//------------------------------------------------------------------------
CThrow::~CThrow()
{
}


//------------------------------------------------------------------------
void CThrow::Update(float frameTime, uint32 frameId)
{
	CSingle::Update(frameTime, frameId);

	if(m_firing)
	{
		if(!m_pulling && !m_throwing && !m_thrown)
		{

			if(m_hold_timer>0.0f)
			{
				m_hold_timer -= frameTime;

				if(m_hold_timer<0.0f)
					m_hold_timer=0.0f;
			}
		}
		else if(m_throwing && m_throw_time<=0.0f)
		{
			CActor *pOwner = m_pWeapon->GetOwnerActor();

			m_pWeapon->HideItem(true);

			if(m_throwableId)
			{
				IEntity *pEntity = gEnv->pEntitySystem->GetEntity(m_throwableId);

				if(pEntity)
				{
					if(m_throwableAction)
						m_throwableAction->execute(m_pWeapon);

					IPhysicalEntity *pPE=pEntity->GetPhysics();

					if(pPE&&(pPE->GetType()==PE_RIGID||pPE->GetType()==PE_PARTICLE))
						ThrowObject(pEntity,pPE);
					else if(pPE&&(pPE->GetType()==PE_LIVING||pPE->GetType()==PE_ARTICULATED))
						ThrowLivingEntity(pEntity,pPE);
				}


			}
			else if(!m_netfiring)
				ThrowGrenade();

			m_throwing = false;
		}
		else if(m_thrown && m_throw_time<=0.0f)
		{
			m_pWeapon->SetBusy(false);

			m_pWeapon->HideItem(false);

			int ammoCount = m_pWeapon->GetAmmoCount(m_pShared->fireparams.ammo_type_class);

			if(ammoCount > 0)
				m_pWeapon->PlayAction(m_pShared->throwactions.next);
			else if(m_pShared->throwparams.auto_select_last)
				static_cast<CPlayer *>(m_pWeapon->GetOwnerActor())->SelectLastItem(true);

			m_firing = false;
			m_throwing = false;
			m_thrown = false;
		}

		m_throw_time -= frameTime;

		if(m_throw_time<0.0f)
			m_throw_time=0.0f;

		m_pWeapon->RequireUpdate(eIUS_FireMode);
	}
}

//------------------------------------------------------------------------
void CThrow::ResetParams(const struct IItemParamsNode *params)
{
	if(!m_fireParams->Valid())
	{
		CSingle::ResetParams(params);

		const IItemParamsNode *throwp = params?params->GetChild("throw"):0;
		const IItemParamsNode *throwa = params?params->GetChild("actions"):0;

		m_pShared->throwparams.Reset(throwp);
		m_pShared->throwactions.Reset(throwa);
	}
}

//------------------------------------------------------------------------
void CThrow::PatchParams(const struct IItemParamsNode *patch)
{
	if(!m_fireParams->Valid())
	{
		CSingle::PatchParams(patch);

		const IItemParamsNode *throwp = patch->GetChild("throw");
		const IItemParamsNode *throwa = patch->GetChild("actions");

		m_pShared->throwparams.Reset(throwp, false);
		m_pShared->throwactions.Reset(throwa, false);
	}
}

//------------------------------------------------------------------
void CThrow::InitSharedParams()
{
	CWeaponSharedParams *pWSP = m_pWeapon->GetWeaponSharedParams();
	assert(pWSP);

	m_fireParams	= pWSP->GetFireSharedParams("ThrowData", m_fmIdx);
}

//-----------------------------------------------------------------------
void CThrow::CacheSharedParamsPtr()
{
	CSingle::CacheSharedParamsPtr();

	m_pShared			= static_cast<CThrowSharedData *>(m_fireParams.get());
}

//------------------------------------------------------------------------
void CThrow::Activate(bool activate)
{
	CSingle::Activate(activate);

	m_hold_timer=0.0f;

	m_thrown=false;
	m_pulling=false;
	m_throwing=false;
	m_firing=false;
	m_netfiring=false;

	m_throwableId=0;
	m_throwableAction=0;

	CheckAmmo();
}

//------------------------------------------------------------------------
bool CThrow::CanFire(bool considerAmmo) const
{
	return CSingle::CanFire(considerAmmo);// cannot be changed. it's used in CSingle::Shoot()
}

//------------------------------------------------------------------------
bool CThrow::CanReload() const
{
	return CSingle::CanReload() && !m_throwing;
}

//------------------------------------------------------------------------
bool CThrow::IsReadyToFire() const
{
	return CanFire(true) && !m_firing && !m_throwing && !m_pulling && !m_thrown;
}

//------------------------------------------------------------------------
struct CThrow::StartThrowAction
{
	StartThrowAction(CThrow *_throw): pthrow(_throw) {};
	CThrow *pthrow;

	void execute(CItem *_this)
	{
		pthrow->m_pulling = false;
		pthrow->m_pWeapon->PlayAction(pthrow->m_pShared->throwactions.hold, 0, true, CItem::eIPAF_Default|CItem::eIPAF_NoBlend);
	}
};

//------------------------------------------------------------------------
void CThrow::StartFire()
{
	m_netfiring = false;

	if(CanFire(true) && !m_firing && !m_throwing && !m_pulling)
	{
		m_firing = true;
		m_pulling = true;
		m_throwing = false;
		m_thrown = false;

		m_pWeapon->SetBusy(true);
		m_pWeapon->PlayAction(m_pShared->throwactions.pull);

		m_pWeapon->GetScheduler()->TimerAction(m_pWeapon->GetCurrentAnimationTime(eIGS_FirstPerson)+1, CSchedulerAction<StartThrowAction>::Create(this), false);
		m_pWeapon->SetDefaultIdleAnimation(eIGS_FirstPerson, m_pShared->throwactions.hold);

		m_hold_timer=m_pShared->throwparams.hold_duration;

		m_pWeapon->RequestStartFire();

		m_pWeapon->RequireUpdate(eIUS_FireMode);
	}
}

//------------------------------------------------------------------------
void CThrow::StopFire()
{
	if(m_firing && !m_throwing && !m_thrown)
	{
		DoThrow();

		m_pWeapon->RequestStopFire();

		m_pWeapon->RequireUpdate(eIUS_FireMode);
	}
}

//------------------------------------------------------------------------
void CThrow::NetStartFire()
{
	m_firing = true;
	m_throwing = false;
	m_thrown = false;
	m_pulling = false; // false here to not override network orders
	m_netfiring = true;

	m_pWeapon->PlayAction(m_pShared->throwactions.pull);

	m_pWeapon->GetScheduler()->TimerAction(m_pWeapon->GetCurrentAnimationTime(eIGS_FirstPerson)+1, CSchedulerAction<StartThrowAction>::Create(this), false);
	m_pWeapon->SetDefaultIdleAnimation(eIGS_FirstPerson, m_pShared->throwactions.hold);

	m_hold_timer=m_pShared->throwparams.hold_duration;

	m_pWeapon->RequireUpdate(eIUS_FireMode);
}

//------------------------------------------------------------------------
void CThrow::NetStopFire()
{
	if(m_firing && !m_throwing && !m_thrown)
	{
		DoThrow();
		m_pWeapon->RequireUpdate(eIUS_FireMode);
	}
}

//------------------------------------------------------------------------
void CThrow::SetThrowable(EntityId entityId, bool forceThrow, ISchedulerAction *action)
{
	m_throwableId = entityId;
	m_throwableAction = action;
	m_forceNextThrow = forceThrow;
}

//------------------------------------------------------------------------
EntityId CThrow::GetThrowable() const
{
	return m_throwableId;
}

//-----------------------------------------------------
void CThrow::SetProjectileLaunchParams(const SProjectileLaunchParams &launchParams)
{
	CSingle::SetProjectileLaunchParams(launchParams);

	m_vFireTarget = launchParams.vShootTargetPos;
}

//-----------------------------------------------------
Vec3 CThrow::GetFireTarget() const
{
	// When AI throws projectile, a fire target position is specified, so use it if available
	// Note: This is called from CSingle when getting hit position and fire directions for Shoot()
	return (m_vFireTarget.IsZero() ? CSingle::GetFireTarget() : m_vFireTarget);
}

//------------------------------------------------------------------------
void CThrow::CheckAmmo()
{
	m_pWeapon->HideItem(!m_pWeapon->GetAmmoCount(m_pShared->fireparams.ammo_type_class) && m_pShared->throwparams.hide_ammo);
}

//------------------------------------------------------------------------
struct CThrow::ThrowAction
{
	ThrowAction(CThrow *_throw): pthrow(_throw) {};
	CThrow *pthrow;

	void execute(CItem *_this)
	{
		pthrow->m_thrown = true;
	}
};

void CThrow::DoThrow()
{
	m_throw_time = m_pShared->throwparams.delay;
	m_throwing = true;
	m_thrown = false;

	bool drop = false;

	if(!m_pWeapon->IsWeaponLowered() && m_forceNextThrow)
		m_pWeapon->PlayAction(m_pShared->throwactions.throwit);
	else if(m_usingGrenade)
		m_pWeapon->PlayAction(m_pShared->throwactions.throwit);
	else
	{
		m_pWeapon->PlayAction(m_pShared->throwactions.dropit);
		m_throwing = false;
		DoDrop();
		drop = true;
	}

	m_forceNextThrow = false;
	m_pWeapon->GetScheduler()->TimerAction(m_pWeapon->GetCurrentAnimationTime(eIGS_FirstPerson), CSchedulerAction<ThrowAction>::Create(this), false);
	m_pWeapon->SetDefaultIdleAnimation(eIGS_FirstPerson, g_pItemStrings->idle);
}

//--------------------------------------
void CThrow::DoDrop()
{

	m_pWeapon->HideItem(true);

	if(m_throwableId)
	{
		IEntity *pEntity = gEnv->pEntitySystem->GetEntity(m_throwableId);

		if(pEntity)
		{
			IPhysicalEntity *pPE=pEntity->GetPhysics();

			if(pPE&&(pPE->GetType()==PE_RIGID||pPE->GetType()==PE_PARTICLE))
			{
				Vec3 hit = GetProbableHit(WEAPON_HIT_RANGE);
				Vec3 pos = GetFiringPos(hit);

				CActor *pActor = m_pWeapon->GetOwnerActor();
				IMovementController *pMC = pActor ? pActor->GetMovementController() : 0;

				if(pMC)
				{
					SMovementState info;
					pMC->GetMovementState(info);
					float speed=2.5f;

					CPlayer *pPlayer = static_cast<CPlayer *>(m_pWeapon->GetOwnerActor());

					if(info.aimDirection.z<-0.1f)
					{
						if(pPlayer)
						{
							if(SPlayerStats *pStats = static_cast<SPlayerStats *>(pPlayer->GetActorStats()))
							{
								if(pStats->grabbedHeavyEntity)
									speed = 4.0f;
							}
						}
					}

					if(CheckForIntersections(pPE,info.eyeDirection))
					{
						Matrix34 newTM = pEntity->GetWorldTM();
						newTM.SetTranslation(newTM.GetTranslation()-(info.eyeDirection*0.4f));
						pEntity->SetWorldTM(newTM,ENTITY_XFORM_POS);

						pe_action_set_velocity asv;
						asv.v = (-info.eyeDirection*speed);
						pPE->Action(&asv);
					}
					else
					{
						pe_action_set_velocity asv;
						asv.v = (info.eyeDirection*speed);
						pPE->Action(&asv);
					}

					SEntityEvent entityEvent;
					entityEvent.event = ENTITY_EVENT_PICKUP;
					entityEvent.nParam[0] = 0;

					if(pPlayer)
						entityEvent.nParam[1] = pPlayer->GetEntityId();

					entityEvent.fParam[0] = speed;
					pEntity->SendEvent(entityEvent);
				}
			}
		}

		if(m_throwableAction)
			m_throwableAction->execute(m_pWeapon);
	}
}

//-----------------------------------------------------
void CThrow::ThrowGrenade()
{
	//Grenade speed scale is always one (for player)
	CPlayer *pPlayer= static_cast<CPlayer *>(m_pWeapon->GetOwnerActor());

	if(pPlayer)
	{
		if(pPlayer->IsPlayer())
		{
			m_speed_scale = 1.0f;
		}
		else if(pPlayer->GetHealth()<=0 || pPlayer->GetGameObject()->GetAspectProfile(eEA_Physics)==eAP_Sleep)
			return; //Do not throw grenade is player is death (AI "ghost grenades")

		//Hide grenade in hand (FP)
		if(pPlayer->IsClient() && m_pWeapon->GetEntity()->GetClass()==CItem::sOffHandClass)
		{
			if(COffHand *pOffHand= static_cast<COffHand *>(m_pWeapon))
			{
				pOffHand->AttachGrenadeToHand(pOffHand->GetCurrentFireMode());
			}
		}
	}

	m_pWeapon->SetBusy(false);
	Shoot(true);
	m_pWeapon->SetBusy(true);

	// Luciano - send ammo count game event
	if(pPlayer)
	{
		IEntityClass *pAmmo = m_pShared->fireparams.ammo_type_class;

		if(pAmmo)
		{
			int ammoCount = pPlayer->GetInventory()->GetAmmoCount(pAmmo);
			g_pGame->GetIGameFramework()->GetIGameplayRecorder()->Event(pPlayer->GetEntity(),GameplayEvent(eGE_AmmoCount,m_pWeapon->GetEntity()/*->GetClass()*/->GetName(),float(ammoCount),(void *)(pAmmo->GetName())));
		}
	}

}
//-----------------------------------------------------
void CThrow::ThrowObject(IEntity *pEntity, IPhysicalEntity *pPE)
{
	bool strengthMode = false;

	CPlayer *pPlayer = static_cast<CPlayer *>(m_pWeapon->GetOwnerActor());

	if(pPlayer)
	{
		// Report throw to AI system.
		if(pPlayer->GetEntity() && pPlayer->GetEntity()->GetAI())
		{
			SAIEVENT AIevent;
			AIevent.targetId = pEntity->GetId();
			pPlayer->GetEntity()->GetAI()->Event(AIEVENT_PLAYER_THROW, &AIevent);
		}
	}

	Vec3 hit = GetProbableHit(WEAPON_HIT_RANGE);
	Vec3 pos = GetFiringPos(hit);
	Vec3 dir = ApplySpread(GetFiringDir(hit, pos), GetSpread());
	Vec3 vel = GetFiringVelocity(dir);

	float speed = 12.0f;

	if(strengthMode)
		speed *= m_pShared->throwparams.strenght_scale;

	speed = max(2.0f, speed);

	pe_params_pos ppos;
	ppos.pos = pEntity->GetWorldPos();
	pPE->SetParams(&ppos);

	if(CheckForIntersections(pPE,dir))
	{
		Matrix34 newTM = pEntity->GetWorldTM();
		newTM.SetTranslation(newTM.GetTranslation()-(dir*0.4f));
		pEntity->SetWorldTM(newTM,ENTITY_XFORM_POS);
	}
	else
	{
		pe_action_set_velocity asv;
		asv.v = (dir*speed)+vel;
		AABB box;
		pEntity->GetWorldBounds(box);
		Vec3 finalW = -gEnv->pSystem->GetViewCamera().GetMatrix().GetColumn0()*(8.0f/max(0.1f,box.GetRadius()));
		finalW.x *= Random(0.5f,1.3f);
		finalW.y *= Random(0.5f,1.3f);
		finalW.z *= Random(0.5f,1.3f);
		asv.w = finalW;
		//asv.w = Vec3(Random(-4.5f,3.5f),Random(-1.75f,2.5f),Random(-1.5f,2.2f));
		pPE->Action(&asv);
	}

	SEntityEvent entityEvent;
	entityEvent.event = ENTITY_EVENT_PICKUP;
	entityEvent.nParam[0] = 0;

	if(pPlayer)
		entityEvent.nParam[1] = pPlayer->GetEntityId();

	entityEvent.fParam[0] = speed;
	pEntity->SendEvent(entityEvent);
}

//-----------------------------------------------------
void CThrow::ThrowLivingEntity(IEntity *pEntity, IPhysicalEntity *pPE)
{
	Vec3 hit = GetProbableHit(WEAPON_HIT_RANGE);
	Vec3 pos = GetFiringPos(hit);
	Vec3 dir = ApplySpread(GetFiringDir(hit, pos), GetSpread());
	Vec3 vel = GetFiringVelocity(dir);

	CPlayer *pPlayer = static_cast<CPlayer *>(m_pWeapon->GetOwnerActor());

	if(pPlayer)
	{
		float speed = 8.0f;
		dir.Normalize();

		if(CheckForIntersections(pPE,dir))
		{
			Matrix34 newTM = pEntity->GetWorldTM();
			newTM.SetTranslation(newTM.GetTranslation()-(dir*0.6f));
			pEntity->SetWorldTM(newTM,ENTITY_XFORM_POS);
		}

		{
			pe_action_set_velocity asv;
			asv.v = (dir*speed)+vel;
			pPE->Action(&asv);
			// [anton] use thread safe=1 (immediate) if the character is still a living entity at this stage,
			//   but will be ragdollized during the same frame

			pe_params_articulated_body pab;
			pab.bCheckCollisions = 1;	// was set to 0 while carrying
			pPE->SetParams(&pab);
		}

		// Report throw to AI system.
		if(pPlayer->GetEntity() && pPlayer->GetEntity()->GetAI())
		{
			SAIEVENT AIevent;
			AIevent.targetId = pEntity->GetId();
			pPlayer->GetEntity()->GetAI()->Event(AIEVENT_PLAYER_STUNT_THROW_NPC, &AIevent);
		}
	}
}

//----------------------------------------------------
bool CThrow::CheckForIntersections(IPhysicalEntity *heldEntity, Vec3 &dir)
{

	Vec3 pos = m_pWeapon->GetSlotHelperPos(eIGS_FirstPerson, "item_attachment", true);
	ray_hit hit;

	if(gEnv->pPhysicalWorld->RayWorldIntersection(pos-dir*0.4f, dir*0.8f, ent_static|ent_terrain|ent_rigid|ent_sleeping_rigid,
			rwi_stop_at_pierceable|14,&hit, 1, heldEntity))
		return true;

	return false;

}

//-----------------------------------------------------
void CThrow::CheckNearMisses(const Vec3 &probableHit, const Vec3 &pos, const Vec3 &dir, float range, float radius)
{
}

//-----------------------------------------------------
void CThrow::GetMemoryUsage(ICrySizer *s) const
{
	s->Add(*this);
	CSingle::GetMemoryUsage(s);

	if(m_useCustomParams)
	{
		m_pShared->throwactions.GetMemoryUsage(s);
	}
}
