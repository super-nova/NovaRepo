//MyHUD.cpp
#include "StdAfx.h"
#include "MyHUD.h"

#include "IActionMapManager.h"
#include "Game.h"
#include "Player.h"


MyHUD::MyHUD(void): m_pFlashPlayer(NULL)
{
	this->Init();
}


MyHUD::~MyHUD(void)
{

}

bool MyHUD::Init()
{
	if(!m_pFlashPlayer)
	{
		m_pFlashPlayer = gEnv->pSystem->CreateFlashPlayerInstance();
		bool bResult = m_pFlashPlayer->Load("Libs\\UI\\test_button.gfx");
		CryLogAlways("Loading Libs\\UI\\nova_test.gfx : %s", bResult ? "Done" : "Error");

		if( !bResult )
			return false; //Debug Point

		m_pFlashPlayer->SetViewport(0, 0, gEnv->pRenderer->GetWidth(), gEnv->pRenderer->GetHeight(), (float)gEnv->pRenderer->GetWidth()/(float)gEnv->pRenderer->GetHeight());
	}

	m_pFlashPlayer->SetFSCommandHandler(this);
	gEnv->pHardwareMouse->AddListener(this);
	gEnv->pInput->AddEventListener(this);

	
	return true;
}


bool MyHUD::Update(float fDeltaTime)
{
	static bool once = true;
	if(once)
	{
		
		IItemSystem* pItemSystem = gEnv->pGame->GetIGameFramework()->GetIItemSystem();

		IActor* pActor = gEnv->pGame->GetIGameFramework()->GetClientActor();

		int itemId = pItemSystem->GiveItem(pActor, "RocketLauncher", false, false, true); // I'm not sure if "Binoculars" or "Binocular"

		pItemSystem->SetActorItem(pActor,itemId, true);

		IInventory *inv = pActor->GetInventory();

		IEntityClass *entCls = gEnv->pEntitySystem->GetClassRegistry()->FindClass("rocket");

		inv->SetAmmoCapacity(entCls,100);

		inv->SetAmmoCount(entCls,50);

		/*IActor* pActor = gEnv->pGame->GetIGameFramework()->GetClientActor();

		CPlayer* pPlayer = (CPlayer*) pActor;
		if(!pPlayer)
			return false;

		IInventory* pInventory = pPlayer->GetInventory();
		//asd
		if(pInventory)
		{
			pInventory->Clear();   
			SEntitySpawnParams spawnParams;
			spawnParams.sName = "RocketLauncher";
			spawnParams.pClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass("RocketLauncher");

			IEntity* tEntity = gEnv->pEntitySystem->SpawnEntity(spawnParams, true);

			pPlayer->GetInventory()->AddItem(tEntity->GetId());

			//pPlayer->PickUpItem(tEntity->GetId(),false);   
		}*/

		once = false;
	}

	if (m_pFlashPlayer)
	{
		m_pFlashPlayer->Advance(fDeltaTime);
		m_pFlashPlayer->Render();
	}

	return true;
}

void MyHUD::HandleFSCommand(const char *szCommand,const char *strArgs, void* pUserData)
{
	if(!strcmp(szCommand, "AddImpulse"))
	{
		CryLogAlways("Flash command received...");
	}
}

bool MyHUD::OnInputEvent(const SInputEvent &inputEvent)
{
	static bool ok = false;
	if( inputEvent.keyId == eKI_F && inputEvent.state & eIS_Pressed )
	{
		m_pFlashPlayer->Invoke0("test1");
		IActionMapManager* pActionMapMan = g_pGame->GetIGameFramework()->GetIActionMapManager();
		IActionFilter* pFilter = pActionMapMan->GetActionFilter("no_mouse");
		pFilter->Enable(ok);
		ok = !ok;
	}
	return true;
}

void MyHUD::OnHardwareMouseEvent(int x, int y, EHARDWAREMOUSEEVENT mouseEvent, int wheelDelta = 0)
{
	SFlashCursorEvent::ECursorState eCursorState = SFlashCursorEvent::eCursorMoved;
	if(HARDWAREMOUSEEVENT_LBUTTONDOWN == mouseEvent)
	{
		eCursorState = SFlashCursorEvent::eCursorPressed;
	}
	else if(HARDWAREMOUSEEVENT_LBUTTONUP == mouseEvent)
	{
		eCursorState = SFlashCursorEvent::eCursorReleased;
	}

	if(m_pFlashPlayer)
	{
		int x1(x), y1(y);
		m_pFlashPlayer->ScreenToClient(x1,y1);
		m_pFlashPlayer->SendCursorEvent(SFlashCursorEvent(eCursorState,x1,y1));
	}
}
