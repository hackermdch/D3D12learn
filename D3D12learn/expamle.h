#pragma once
#define CLASS_ONLY
#include "d3dUtil.h"
#include "dx.h"
#include "FrameBuffer.h"
using namespace DirectX;

class App :public DXApp {
public:
	static constexpr int FrameBufferCount = 3;
protected:
	ComPtr<ID3D12RootSignature> roots;
	ComPtr<ID3D12PipelineState> pState;
	ComPtr<ID3DBlob> vs, ps;
	ComPtr<ID3D12Resource> consts;
	ComPtr<ID3D12DescriptorHeap> cbvh;
protected:
	std::unique_ptr<FrameBuffer> mFrameBuffers[FrameBufferCount];
	std::vector<std::unique_ptr<struct RenderObject>> objs;
	std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> mGeos;
	FrameBuffer* mCurrentFrame;
	int mCurrentFrameIndex = 0;
	PassConstants mMainPassCB;
	int passCbvOffset = 0;
protected:
	XMFLOAT4X4 mView;
	XMFLOAT4X4 mProj;
	XMFLOAT3 mEyePos;
private:
	RECT mouseRect;
	float pitch;
	float yaw;
	union
	{
		BYTE states;
		struct {
			bool A_Pressed : 1;
			bool W_Pressed : 1;
			bool S_Pressed : 1;
			bool D_Pressed : 1;
			bool Space_Pressed : 1;
			bool LShift_Pressed : 1;
		}KeyStates;
	};
public:
	App();
	void Render() override;
	void Update(float delta) override;
	bool OnRawInput(RAWINPUT*) override;
	void UpdateObjectCBs();
	void UpdateMainPassCB();
	void UpdateInput();
protected:
	LRESULT WndProc(HWND, UINT, WPARAM, LPARAM);
	void DrawObjects();
private:
	void __OnRawKeyDown(UINT code, UINT type = 0);
	void __OnRawKeyUp(UINT code, UINT type = 0);
	void OnMouseButtonChange(RAWINPUT*);
	void OnMouseMove(RAWINPUT*);
	XMVECTOR GetForward();
};

#include "RenderObject.h"
#undef CLASS_ONLY
