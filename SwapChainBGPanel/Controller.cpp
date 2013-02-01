#include "pch.h"
#include "Controller.h"



Controller::Controller(void)
{ 
	Speed = 1.0f;
}

void Controller::Initialize( _In_ Windows::UI::Core::CoreWindow^ window, _In_ Windows::UI::Xaml::Controls::SwapChainBackgroundPanel^ swapChainPanel )
{
	static bool Initialized = false;
	if (!Initialized)
	{
		m_timer = ref new RunTimer();
		m_timer->Start();
	}

	RenderEngine::Initialize(window, swapChainPanel);

	Initialized = true;
}

void Controller::RunCycle()
{
	m_timer->Update();

	Update(m_timer->TotalTime(), m_timer->DeltaTime());

	Render();
	Present();
}

void Controller::Update( _In_ float timeTotal, _In_ float timeFrame )
{
	XMVECTOR eye = XMVectorSet(0.0f, 0.7f, 1.5f, 0.0f);
	XMVECTOR at = XMVectorSet(0.0f, -0.1f, 0.0f, 0.0f);
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	m_constantBufferData.view = XMMatrixTranspose(XMMatrixLookAtRH(eye, at, up));
	m_constantBufferData.model = XMMatrixTranspose(XMMatrixRotationY(Speed * timeTotal * XM_PIDIV4) * XMMatrixRotationX(Speed * timeTotal * XM_PIDIV4));
}
