#include "dx.h"
#include "d3dUtil.h"
#include "rawinput.h"
#define WM_CALC_FPS 0x401

using namespace std;

DXApp::DXApp(LPCTSTR title) :
	mCurrentFence(0),
	mCurrBackBuffer(0),
	title(title),
	closed(false),
	pause(false),
	render(true),
	hWnd(CreateWindowEx(0, _static::GetClassName(), title, WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU, 200, 200, 800, 600, nullptr, nullptr, _static::GetInstance()->GetHInstance(), this))
{
#if defined(DEBUG) || defined(_DEBUG) 
	{
		ComPtr<ID3D12Debug> debugController;
		ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
		debugController->EnableDebugLayer();
	}
#endif
	ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&factory)));
	ThrowIfFailed(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&device)));
	ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
	D3D12_COMMAND_QUEUE_DESC qd = {};
	qd.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	qd.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	ThrowIfFailed(device->CreateCommandQueue(&qd, IID_PPV_ARGS(&pCommandQueue)));
	ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&pCommandAllocator)));
	ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, pCommandAllocator.Get(), nullptr, IID_PPV_ARGS(&pCommandList)));
	pCommandList->Close();
	DXGI_SWAP_CHAIN_DESC sd = {};
	sd.BufferDesc.Width = 800;
	sd.BufferDesc.Height = 600;
	sd.BufferDesc.RefreshRate.Numerator = 0;
	sd.BufferDesc.RefreshRate.Denominator = 0;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.BufferCount = BUFF_COUNT;
	sd.OutputWindow = hWnd;
	sd.Windowed = true;
	sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH | DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
	ThrowIfFailed(factory->CreateSwapChain(pCommandQueue.Get(), &sd, &pSwapChain));
	D3D12_DESCRIPTOR_HEAP_DESC rd = {};
	rd.NumDescriptors = BUFF_COUNT;
	rd.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rd.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rd.NodeMask = 0;
	ThrowIfFailed(device->CreateDescriptorHeap(&rd, IID_PPV_ARGS(&mRTVHeap)));
	D3D12_DESCRIPTOR_HEAP_DESC dd = {};
	dd.NumDescriptors = 1;
	dd.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dd.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dd.NodeMask = 0;
	ThrowIfFailed(device->CreateDescriptorHeap(&dd, IID_PPV_ARGS(&mDSVHeap)));
	FlushCommandQueue();
	ThrowIfFailed(pCommandList->Reset(pCommandAllocator.Get(), nullptr));
	CD3DX12_CPU_DESCRIPTOR_HANDLE rh(mRTVHeap->GetCPUDescriptorHandleForHeapStart());
	UINT rs = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	for (UINT i = 0; i < BUFF_COUNT; i++)
	{
		ThrowIfFailed(pSwapChain->GetBuffer(i, IID_PPV_ARGS(&buffers[i])));
		device->CreateRenderTargetView(buffers[i].Get(), nullptr, rh);
		rh.Offset(1, rs);
	}
	D3D12_RESOURCE_DESC dsd = {};
	dsd.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	dsd.Alignment = 0;
	dsd.Width = 800;
	dsd.Height = 600;
	dsd.DepthOrArraySize = 1;
	dsd.MipLevels = 1;
	dsd.Format = DXGI_FORMAT_R24G8_TYPELESS;
	dsd.SampleDesc.Count = 1;
	dsd.SampleDesc.Quality = 0;
	dsd.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	dsd.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	D3D12_CLEAR_VALUE oc;
	oc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	oc.DepthStencil.Depth = 1.0f;
	oc.DepthStencil.Stencil = 0;
	CD3DX12_HEAP_PROPERTIES a(D3D12_HEAP_TYPE_DEFAULT);
	ThrowIfFailed(device->CreateCommittedResource(&a, D3D12_HEAP_FLAG_NONE, &dsd, D3D12_RESOURCE_STATE_COMMON, &oc, IID_PPV_ARGS(&pDepthStencilBuffer)));
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvd;
	dsvd.Flags = D3D12_DSV_FLAG_NONE;
	dsvd.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvd.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvd.Texture2D.MipSlice = 0;
	device->CreateDepthStencilView(pDepthStencilBuffer.Get(), &dsvd, DepthStencilView());
	CD3DX12_RESOURCE_BARRIER&& b = CD3DX12_RESOURCE_BARRIER::Transition(pDepthStencilBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE);
	pCommandList->ResourceBarrier(1, &b);
	ThrowIfFailed(pCommandList->Close());
	pCommandQueue->ExecuteCommandLists(1, (ID3D12CommandList* const*)pCommandList.GetAddressOf());
	FlushCommandQueue();
	mScreenViewport.TopLeftX = 0;
	mScreenViewport.TopLeftY = 0;
	mScreenViewport.Width = 800;
	mScreenViewport.Height = 600;
	mScreenViewport.MinDepth = 0.0f;
	mScreenViewport.MaxDepth = 1.0f;
	mScissorRect = { 0, 0, 800, 600 };
	factory->MakeWindowAssociation(hWnd, DXGI_MWA_NO_WINDOW_CHANGES | DXGI_MWA_NO_ALT_ENTER | DXGI_MWA_NO_PRINT_SCREEN);
	RAWINPUTDEVICE Rid[2];

	Rid[0].usUsagePage = HID_USAGE_PAGE_GENERIC;
	Rid[0].usUsage = HID_USAGE_GENERIC_MOUSE;
	Rid[0].dwFlags = 0;    // adds mouse and also ignores legacy mouse messages
	Rid[0].hwndTarget = 0;

	Rid[1].usUsagePage = HID_USAGE_PAGE_GENERIC;
	Rid[1].usUsage = HID_USAGE_GENERIC_KEYBOARD;
	Rid[1].dwFlags = 0;    // adds keyboard and also ignores legacy keyboard messages
	Rid[1].hwndTarget = 0;

	if (RegisterRawInputDevices(Rid, 2, sizeof(Rid[0])) == FALSE)
	{
#ifdef _DEBUG
		MessageBox(hWnd, L"error register raw input", L"message", 0);
#endif
		exit(-1);
	}
}

int DXApp::Run()
{
	ShowWindow(hWnd, SW_SHOW);
	CreateThread(NULL, 0, Timer, this, 0, NULL);
	timer.Reset();
	return MsgLoop();
}

DXApp::~DXApp() = default;

LRESULT DXApp::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_CLOSE:
		closed = true;
		break;
	case WM_CALC_FPS:
		CalculateFrameStats();
		return 0;
	}
	return DefWindowProc(hWnd, msg, wParam, lParam);
}

int DXApp::MsgLoop()
{
	MSG msg;
	while (GetMessage(&msg, hWnd, 0, 0) > 0)
	{
		if (PreTranslateMessage(&msg)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	return (int)msg.wParam;
}

void DXApp::Render()
{

}

void DXApp::Update(float delta)
{

}

bool DXApp::PreTranslateMessage(MSG * msg)
{
	{
		bool flag1 = msg->message >= WM_MOUSEFIRST && msg->message <= WM_MOUSELAST;
		bool flag2 = msg->message >= WM_KEYFIRST && msg->message <= WM_KEYLAST;
		if (lastIgnoreInput && (flag1 || flag2))
			return false;
	}
	switch (msg->message)
	{
	case WM_INPUT:
	{
		UINT dwSize;

		GetRawInputData((HRAWINPUT)msg->lParam, RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER));
		LPBYTE lpb = new BYTE[dwSize];
		if (lpb == NULL)
		{
			return 0;
		}

		if (GetRawInputData((HRAWINPUT)msg->lParam, RID_INPUT, lpb, &dwSize, sizeof(RAWINPUTHEADER)) != dwSize)
			OutputDebugString(TEXT("GetRawInputData does not return correct size !\n"));

		RAWINPUT* raw = (RAWINPUT*)lpb;
		bool ret = OnRawInput(raw);
		delete[] lpb;
		return raw;
	}
	}
	return true;
}

bool DXApp::OnRawInput(RAWINPUT * raw)
{
	return true;
}

void DXApp::FlushCommandQueue()
{
	mCurrentFence++;
	ThrowIfFailed(pCommandQueue->Signal(fence.Get(), mCurrentFence));
	if (fence->GetCompletedValue() < mCurrentFence) {
		HANDLE eventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);
		ThrowIfFailed(fence->SetEventOnCompletion(mCurrentFence, eventHandle));
		WaitForSingleObject(_Notnull_(eventHandle), INFINITE);
		CloseHandle(_Notnull_(eventHandle));
	}
}

DWORD WINAPI DXApp::Timer(LPVOID lpParam)
{
	DXApp* t = (DXApp*)lpParam;
	while (!t->closed) {
		t->timer.Tick();
		if (!t->pause)
		{
			SendMessage(t->hWnd, WM_CALC_FPS, 0, (LPARAM)0);
			t->Update(t->timer.DeltaTime());
			if (t->render) {
				t->Render();
				t->render = false;
			}
		}
		else
		{
			Sleep(100);
		}
	}
	return 0;
}

void DXApp::CalculateFrameStats()
{
	static int frameCnt = 0;
	static float timeElapsed = 0.0f;
	frameCnt++;
	if ((timer.TotalTime() - timeElapsed) >= 1.0f)
	{
		float fps = (float)frameCnt;
		float mspf = 1000.0f / fps;
		wostringstream outs;
		outs.precision(6);
		outs << title << L"    "
			<< L"FPS: " << fps << L"    "
			<< L"Frame Time: " << mspf << L" (ms)";
		SetWindowText(hWnd, outs.str().c_str());
		frameCnt = 0;
		timeElapsed += 1.0f;
	}
}

DXApp::_static::_static() :hInstance(GetModuleHandle(nullptr))
{
	WNDCLASSEX wc = { 0 };
	wc.cbSize = sizeof(wc);
	wc.style = CS_OWNDC;
	wc.lpfnWndProc = _static::WndProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = className;
	wc.hbrBackground = nullptr;
	wc.hCursor = nullptr;
	wc.hIcon = nullptr;
	wc.hIconSm = nullptr;
	wc.lpszMenuName = nullptr;
	RegisterClassEx(&wc);
}

LRESULT WINAPI DXApp::_static::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (msg == WM_NCCREATE) {
		CREATESTRUCT* ps = (CREATESTRUCT*)lParam;
		DXApp* wnd = (DXApp*)ps->lpCreateParams;
		SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)wnd);
	}
	DXApp* wnd = (DXApp*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	return wnd == nullptr ? DefWindowProc(hWnd, msg, wParam, lParam) : wnd->WndProc(hWnd, msg, wParam, lParam);
}

unique_ptr<DXApp::_static> DXApp::_static::instance = nullptr;

constexpr LPCTSTR DXApp::_static::GetClassName()
{
	return className;
}

DXApp::_static* DXApp::_static::GetInstance()
{
	if (instance == nullptr)
		instance = make_unique<DXApp::_static>();
	return instance.get();
}

HINSTANCE DXApp::_static::GetHInstance()
{
	return hInstance;
}
