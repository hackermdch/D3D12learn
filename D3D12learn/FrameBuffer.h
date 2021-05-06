#pragma once
#include "d3dUtil.h"
#include "UploadBuffer.h"
#include "ConstantBuffers.h"

class FrameBuffer
{
protected:
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;
public:
	ComPtr<ID3D12CommandAllocator> pCmdAllocator;
	std::unique_ptr<UploadBuffer<PassConstants>> PassCB;
	std::unique_ptr<UploadBuffer<ObjectConstants>> ObjCB;
	UINT64 Fence = 0;
public:
	FrameBuffer(ID3D12Device* device, UINT passCount, UINT objCount);
	~FrameBuffer();
};

