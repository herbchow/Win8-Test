#include "pch.h"
#include "RenderEngine.h"

using namespace Microsoft::WRL;
using namespace Windows::Foundation;
using namespace Windows::UI::Core;
using namespace DirectX;

RenderEngine::RenderEngine(void) : m_loadingComplete(false), m_initialized(false), m_indexCount(0)
{
}

void RenderEngine::Initialize(_In_ CoreWindow^ window, _In_ SwapChainBackgroundPanel^ swapChainPanel )
{
	m_swapChainPanel = swapChainPanel;

	if (m_initialized)
	{
		// Being restarted probably as a result of the device was removed either by a disconnect
		// or a driver upgradeTDR so make sure the rendering state has been released.
		m_renderTargetView = nullptr;
		m_depthStencilView = nullptr;
		m_swapChain = nullptr;
	}
	m_initialized = true;

	DirectXBase::Initialize(window);
}

void RenderEngine::CreateDeviceResources()
{
	DirectXBase::CreateDeviceResources();

	auto loadVSTask = DX::ReadDataAsync("SimpleVertexShader.cso");
	auto loadPSTask = DX::ReadDataAsync("SimplePixelShader.cso");

	auto createVSTask = loadVSTask.then([this](DX::ByteArray ba) {
		auto bytecodeVS = ba.data;
		DX::ThrowIfFailed(
			m_d3dDevice->CreateVertexShader(
			bytecodeVS->Data,
			bytecodeVS->Length,
			nullptr,
			&m_vertexShader
			)
			);

		const D3D11_INPUT_ELEMENT_DESC vertexDesc[] = 
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "COLOR",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};

		DX::ThrowIfFailed(
			m_d3dDevice->CreateInputLayout(
			vertexDesc,
			ARRAYSIZE(vertexDesc),
			bytecodeVS->Data,
			bytecodeVS->Length,
			&m_inputLayout
			)
			);
	});

	auto createPSTask = loadPSTask.then([this](DX::ByteArray ba) {
		auto bytecodePS = ba.data;
		DX::ThrowIfFailed(
			m_d3dDevice->CreatePixelShader(
			bytecodePS->Data,
			bytecodePS->Length,
			nullptr,
			&m_pixelShader
			)
			);

		DX::ThrowIfFailed(
			m_d3dDevice->CreateBuffer(
			&CD3D11_BUFFER_DESC(sizeof(ModelViewProjectionConstantBuffer), D3D11_BIND_CONSTANT_BUFFER),
			nullptr,
			&m_constantBuffer
			)
			);
	});

	auto createCubeTask = (createPSTask && createVSTask).then([this] () {
		VertexPositionColor cubeVertices[] = 
		{
			{XMFLOAT3(-0.5f, -0.5f, -0.5f), XMFLOAT3(0.0f, 0.0f, 0.0f)},
			{XMFLOAT3(-0.5f, -0.5f,  0.5f), XMFLOAT3(0.0f, 0.0f, 1.0f)},
			{XMFLOAT3(-0.5f,  0.5f, -0.5f), XMFLOAT3(0.0f, 1.0f, 0.0f)},
			{XMFLOAT3(-0.5f,  0.5f,  0.5f), XMFLOAT3(0.0f, 1.0f, 1.0f)},
			{XMFLOAT3( 0.5f, -0.5f, -0.5f), XMFLOAT3(1.0f, 0.0f, 0.0f)},
			{XMFLOAT3( 0.5f, -0.5f,  0.5f), XMFLOAT3(1.0f, 0.0f, 1.0f)},
			{XMFLOAT3( 0.5f,  0.5f, -0.5f), XMFLOAT3(1.0f, 1.0f, 0.0f)},
			{XMFLOAT3( 0.5f,  0.5f,  0.5f), XMFLOAT3(1.0f, 1.0f, 1.0f)},
		};

		D3D11_SUBRESOURCE_DATA vertexBufferData = {0};
		vertexBufferData.pSysMem = cubeVertices;
		vertexBufferData.SysMemPitch = 0;
		vertexBufferData.SysMemSlicePitch = 0;
		DX::ThrowIfFailed(
			m_d3dDevice->CreateBuffer(
			&CD3D11_BUFFER_DESC(sizeof(cubeVertices), D3D11_BIND_VERTEX_BUFFER),
			&vertexBufferData,
			&m_vertexBuffer
			)
			);

		unsigned short cubeIndices[] = 
		{
			0,2,1, // -x
			1,2,3,

			4,5,6, // +x
			5,7,6,

			0,1,5, // -y
			0,5,4,

			2,6,7, // +y
			2,7,3,

			0,4,6, // -z
			0,6,2,

			1,3,7, // +z
			1,7,5,
		};

		m_indexCount = ARRAYSIZE(cubeIndices);

		D3D11_SUBRESOURCE_DATA indexBufferData = {0};
		indexBufferData.pSysMem = cubeIndices;
		indexBufferData.SysMemPitch = 0;
		indexBufferData.SysMemSlicePitch = 0;
		DX::ThrowIfFailed(
			m_d3dDevice->CreateBuffer(
			&CD3D11_BUFFER_DESC(sizeof(cubeVertices), D3D11_BIND_INDEX_BUFFER),
			&indexBufferData,
			&m_indexBuffer
			)
			);
	});

	createCubeTask.then([this] () {
		m_loadingComplete = true;
	});
}

void RenderEngine::CreateWindowSizeDependentResources()
{
	// *** NOTE ***
	// This function replaces the DirectXBase::CreateWindowSizeDependentResources()
	// method completely and does not call it.  It is specialized to interface with XAML
	// *** END NOTE ***

	// Store the window bounds so the next time we get a SizeChanged event we can
	// avoid rebuilding everything if the size is identical.
	m_windowBounds = m_window->Bounds;

	if (m_swapChain != nullptr)
	{
		// Existing swap chain needs to be resized
		// make sure that the depend objects have been released.
		m_d3dContext->OMSetRenderTargets(0, nullptr, nullptr);
		m_renderTargetView = nullptr;
		m_depthStencilView = nullptr;

		DX::ThrowIfFailed(
			m_swapChain->ResizeBuffers(
			2, 
			static_cast<UINT>(m_window->Bounds.Width* m_dpi / 96.0f),
			static_cast<UINT>(m_window->Bounds.Height* m_dpi / 96.0f),
			DXGI_FORMAT_B8G8R8A8_UNORM,
			0
			)
		);
	}
	else
	{
		// m_swapChain is nullptr either because it has never been created or because it has been
		// invalidated. Make sure that the dependent objects are also released.
		m_d3dContext->OMSetRenderTargets(0, nullptr, nullptr);
		m_renderTargetView = nullptr;
		m_depthStencilView = nullptr;
		m_d3dContext->Flush();

		DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {0};
		swapChainDesc.Width = static_cast<UINT>(m_window->Bounds.Width * m_dpi / 96.0f);    // Can not use 0 to get the default on Composition SwapChain
		swapChainDesc.Height = static_cast<UINT>(m_window->Bounds.Height * m_dpi / 96.0f);
		swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		swapChainDesc.Stereo = false;
		swapChainDesc.SampleDesc.Count = 1;                                                 // don't use multi-sampling
		swapChainDesc.SampleDesc.Quality = 0;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.BufferCount = 2;                                                      // use two buffers to enable flip effect
		swapChainDesc.Scaling = DXGI_SCALING_STRETCH;                                       // Required to be STRETCH for Composition 
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
		swapChainDesc.Flags = 0;

		// once the desired swap chain description is configured, it must be created on the same adapter as our D3D Device

		// first, retrieve the underlying DXGI Device from the D3D Device
		ComPtr<IDXGIDevice1> dxgiDevice;
		DX::ThrowIfFailed(
			m_d3dDevice.As(&dxgiDevice)
			);

		// next, get the associated adapter from the DXGI Device
		ComPtr<IDXGIAdapter> dxgiAdapter;
		DX::ThrowIfFailed(
			dxgiDevice->GetAdapter(&dxgiAdapter)
			);

		DX::ThrowIfFailed(
			dxgiDevice->SetMaximumFrameLatency(1)
			);

		// Next, get the parent factory from the DXGI adapter.
		ComPtr<IDXGIFactory2> dxgiFactory;
		DX::ThrowIfFailed(
			dxgiAdapter->GetParent(IID_PPV_ARGS(&dxgiFactory))
			);

		// Create the swap chain and then associate it with the SwapChainBackgroundPanel
		DX::ThrowIfFailed(
			dxgiFactory->CreateSwapChainForComposition(
			m_d3dDevice.Get(),
			&swapChainDesc,
			nullptr,
			&m_swapChain
			)
		);

		ComPtr<ISwapChainBackgroundPanelNative> dxRootPanelAsNative;

		// set the swap chain on the SwapChainBackgroundPanel
		reinterpret_cast<IUnknown*>(m_swapChainPanel)->QueryInterface(__uuidof(ISwapChainBackgroundPanelNative), (void**)&dxRootPanelAsNative);

		DX::ThrowIfFailed(
			dxRootPanelAsNative->SetSwapChain(m_swapChain.Get())
			);
	}

	// Obtain the backbuffer for this window which will be the final 3D rendertarget.
    ComPtr<ID3D11Texture2D> backBuffer;
    DX::ThrowIfFailed(
        m_swapChain->GetBuffer(
            0,
            __uuidof(ID3D11Texture2D),
            &backBuffer
            )
        );

    // Create a view interface on the rendertarget to use on bind.
    DX::ThrowIfFailed(
        m_d3dDevice->CreateRenderTargetView(
            backBuffer.Get(),
            nullptr,
            &m_renderTargetView
            )
        );

    // Cache the rendertarget dimensions in our helper class for convenient use.
    D3D11_TEXTURE2D_DESC backBufferDesc;
    backBuffer->GetDesc(&backBufferDesc);
    m_renderTargetSize.Width  = static_cast<float>(backBufferDesc.Width);
    m_renderTargetSize.Height = static_cast<float>(backBufferDesc.Height);

    // Create a descriptor for the depth/stencil buffer.
    CD3D11_TEXTURE2D_DESC depthStencilDesc(
        DXGI_FORMAT_D24_UNORM_S8_UINT, 
        backBufferDesc.Width,
        backBufferDesc.Height,
        1,
        1,
        D3D11_BIND_DEPTH_STENCIL);

    // Allocate a 2-D surface as the depth/stencil buffer.
    ComPtr<ID3D11Texture2D> depthStencil;
    DX::ThrowIfFailed(
        m_d3dDevice->CreateTexture2D(
            &depthStencilDesc,
            nullptr,
            &depthStencil
            )
        );

    // Create a DepthStencil view on this surface to use on bind.
    DX::ThrowIfFailed(
        m_d3dDevice->CreateDepthStencilView(
            depthStencil.Get(),
            &CD3D11_DEPTH_STENCIL_VIEW_DESC(D3D11_DSV_DIMENSION_TEXTURE2D),
            &m_depthStencilView
            )
        );

    // Create a viewport descriptor of the full window size.
    CD3D11_VIEWPORT viewPort(
        0.0f,
        0.0f,
        static_cast<float>(backBufferDesc.Width),
        static_cast<float>(backBufferDesc.Height)
        );
        
    // Set the current viewport using the descriptor.
    m_d3dContext->RSSetViewports(1, &viewPort);

	float aspectRatio = m_windowBounds.Width / m_windowBounds.Height;
    float fovAngleY = 70.0f * XM_PI / 180.0f;
    if (aspectRatio < 1.0f)
    {
        fovAngleY /= aspectRatio;
    }

    m_constantBufferData.projection = XMMatrixTranspose(XMMatrixPerspectiveFovRH(
        fovAngleY,
        aspectRatio,
        0.01f,
        100.0f
        ));
}

void RenderEngine::Render()
{
	// clear
	m_d3dContext->OMSetRenderTargets(
		1,
		m_renderTargetView.GetAddressOf(),
		m_depthStencilView.Get()
		);

	const float midnightBlue[] = { 0.098f, 0.098f, 0.439f, 1.000f };
	m_d3dContext->ClearRenderTargetView(
		m_renderTargetView.Get(),
		midnightBlue
		);

	m_d3dContext->ClearDepthStencilView(
		m_depthStencilView.Get(),
		D3D11_CLEAR_DEPTH,
		1.0f,
		0
		);

	// only draw the cube once it's loaded (this is async)
	if (!m_loadingComplete)
		return;

	m_d3dContext->UpdateSubresource(
		m_constantBuffer.Get(),
		0,
		NULL,
		&m_constantBufferData,
		0,
		0
		);

	UINT stride = sizeof(VertexPositionColor);
	UINT offset = 0;
	m_d3dContext->IASetVertexBuffers(
		0,
		1,
		m_vertexBuffer.GetAddressOf(),
		&stride,
		&offset
		);

	m_d3dContext->IASetIndexBuffer(
		m_indexBuffer.Get(),
		DXGI_FORMAT_R16_UINT,
		0
		);

	m_d3dContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	m_d3dContext->IASetInputLayout(m_inputLayout.Get());

	m_d3dContext->VSSetShader(
		m_vertexShader.Get(),
		nullptr,
		0
		);

	m_d3dContext->VSSetConstantBuffers(
		0,
		1,
		m_constantBuffer.GetAddressOf()
		);

	m_d3dContext->PSSetShader(
		m_pixelShader.Get(),
		nullptr,
		0
		);

	m_d3dContext->DrawIndexed(
		m_indexCount,
		0,
		0
		);
}
