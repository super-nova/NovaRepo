//MyHUD.h
#pragma once

#ifndef __MYHUD__
#define __MYHUD__

#include "IFlashPlayer.h"
#include "IInput.h"
#include "IHardwareMouse.h"

class MyHUD : 
	public IFSCommandHandler,
	public IInputEventListener,
	public IHardwareMouseEventListener
{
	IFlashPlayer* m_pFlashPlayer;
	/*MyHUD(void);
	~MyHUD(void);*/
public:
	MyHUD(void);
	~MyHUD(void);

	bool Init();
	bool Update(float fDeltaTime);
	bool Shutdown();

	/*static MyHUD& Instance()
	{
		static MyHUD hud;
		return hud;
	}*/

	virtual void HandleFSCommand(const char *szCommand, const char *strArgs, void* pUserData);

	virtual bool OnInputEvent(const SInputEvent &inputEvent); //for keyboard inputs
	virtual void OnHardwareMouseEvent(int x, int y, EHARDWAREMOUSEEVENT mouseEvent, int wheelDelta); //for mouse inputs
};

#endif

