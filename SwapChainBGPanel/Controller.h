#pragma once

#include "pch.h"
#include "renderengine.h"
#include "RunTimer.h"

ref class Controller sealed : public RenderEngine
{
public:
	Controller(void);

	virtual void Initialize( _In_ Windows::UI::Core::CoreWindow^ window, _In_ Windows::UI::Xaml::Controls::SwapChainBackgroundPanel^ swapChainPanel ) override;
	void RunCycle();
	
	property float	Speed;

protected private:
	DirectX::XMFLOAT3	m_minBound;
	DirectX::XMFLOAT3   m_maxBound;

	void Update( _In_ float timeTotal, _In_ float timeFrame );

private:
	RunTimer^			m_timer;

};

