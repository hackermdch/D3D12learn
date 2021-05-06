#pragma once
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <memory>
#include <wrl.h>
#include "GameTimer.h"
#include "d3dx12.h"

class DXApp
{
protected:
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;
private:
	class _static {
		friend std::unique_ptr<_static> std::make_unique<_static>();
	private:
		constexpr static LPCTSTR className = TEXT("HackerWindow");
		static std::unique_ptr<_static> instance;
		HINSTANCE hInstance;
	public:
		static constexpr LPCTSTR GetClassName();
		static _static* GetInstance();
		HINSTANCE GetHInstance();
	private:
		static LRESULT WINAPI WndProc(HWND, UINT, WPARAM, LPARAM);
		_static();
	};
private:
	bool lastIgnoreInput = true;
protected:
	static constexpr int BUFF_COUNT = 2;
	const HWND hWnd;
	LPCTSTR title;
	GameTimer timer;
	ComPtr<ID3D12Device> device;
	ComPtr<IDXGIFactory4> factory;
	ComPtr<ID3D12Fence> fence;
	ComPtr<ID3D12CommandQueue> pCommandQueue;
	ComPtr<ID3D12CommandAllocator> pCommandAllocator;
	ComPtr<ID3D12GraphicsCommandList> pCommandList;
	ComPtr<IDXGISwapChain> pSwapChain;
	ComPtr<ID3D12DescriptorHeap> mRTVHeap;
	ComPtr<ID3D12DescriptorHeap> mDSVHeap;
	ComPtr<ID3D12Resource> buffers[BUFF_COUNT];
	ComPtr<ID3D12Resource> pDepthStencilBuffer;
	int mCurrentFence;
	int mCurrBackBuffer;
	D3D12_VIEWPORT mScreenViewport;
	D3D12_RECT mScissorRect;
	bool closed;
	bool render;
	bool pause;
public:
	DXApp(LPCTSTR = TEXT("D3D12 Window"));
	virtual ~DXApp();
	DXApp(const DXApp& ref) = delete;
	DXApp& operator=(const DXApp& ref) = delete;
	int Run();
protected:
	virtual LRESULT WndProc(HWND, UINT, WPARAM, LPARAM);
	virtual int MsgLoop();
	virtual void Render();
	virtual void Update(float);
	virtual bool PreTranslateMessage(MSG*);
	virtual bool OnRawInput(RAWINPUT*);
	void FlushCommandQueue();
	inline D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView() const;
	inline D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView() const;
private:
	static DWORD WINAPI Timer(LPVOID);
	void CalculateFrameStats();
};

inline D3D12_CPU_DESCRIPTOR_HANDLE DXApp::CurrentBackBufferView() const
{
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(mRTVHeap->GetCPUDescriptorHandleForHeapStart(), mCurrBackBuffer, device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV));
}

inline D3D12_CPU_DESCRIPTOR_HANDLE DXApp::DepthStencilView() const
{
	return mDSVHeap->GetCPUDescriptorHandleForHeapStart();
}
