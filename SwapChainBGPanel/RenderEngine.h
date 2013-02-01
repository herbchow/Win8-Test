#pragma once

#include "DirectXBase.h"
#include "windows.ui.xaml.media.dxinterop.h"

using namespace Windows::UI::Core;
using namespace Windows::UI::Xaml::Controls;

struct ModelViewProjectionConstantBuffer
{
	DirectX::XMMATRIX model;
	DirectX::XMMATRIX view;
	DirectX::XMMATRIX projection;
};

struct VertexPositionColor
{
	DirectX::XMFLOAT3 pos;
	DirectX::XMFLOAT3 color;
};


ref class RenderEngine : public DirectXBase
{
internal:
	RenderEngine(void);

public:
	virtual void Initialize(_In_ CoreWindow^ window, _In_ SwapChainBackgroundPanel^ swapChainPanel );
	virtual void CreateDeviceResources() override;
	virtual void CreateWindowSizeDependentResources() override;
	//virtual void UpdateForWindowSizeChange() override;
	virtual void Render() override;

protected private:
	bool													m_loadingComplete;
	bool													m_initialized;

	Windows::UI::Xaml::Controls::SwapChainBackgroundPanel^	m_swapChainPanel;

	Microsoft::WRL::ComPtr<ID3D11InputLayout>				m_inputLayout;
	Microsoft::WRL::ComPtr<ID3D11Buffer>					m_vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer>					m_indexBuffer;
	Microsoft::WRL::ComPtr<ID3D11VertexShader>				m_vertexShader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader>				m_pixelShader;
	Microsoft::WRL::ComPtr<ID3D11Buffer>					m_constantBuffer;

	uint32													m_indexCount;
	ModelViewProjectionConstantBuffer						m_constantBufferData;

};

