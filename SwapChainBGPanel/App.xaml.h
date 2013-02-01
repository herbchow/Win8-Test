//
// App.xaml.h
// Declaration of the App class.
//

#pragma once

#include "App.g.h"
#include "MainPage.xaml.h"
#include "Controller.h"

namespace SwapChainBGPanel
{
	/// <summary>
	/// Provides application-specific behavior to supplement the default Application class.
	/// </summary>
	ref class App sealed
	{
	public:
		App();
		property Controller^ ControlLogic;

		virtual void OnLaunched(Windows::ApplicationModel::Activation::LaunchActivatedEventArgs^ pArgs) override;

	private:
		MainPage^									m_mainPage;
		//Controller^									m_controller;
		Windows::Foundation::EventRegistrationToken	m_eventToken;

		void OnRendering( _In_ Object^ sender, _In_ Object^ args );
		void OnSuspending(Platform::Object^ sender, Windows::ApplicationModel::SuspendingEventArgs^ e);
	};
}
