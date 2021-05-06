#include "FrameBuffer.h"
using namespace std;

FrameBuffer::FrameBuffer(ID3D12Device* device, UINT passCount, UINT objCount)
{
	ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&pCmdAllocator)));
	PassCB = make_unique<UploadBuffer<PassConstants>>(device, passCount);
	ObjCB = make_unique<UploadBuffer<ObjectConstants>>(device, objCount);
}

FrameBuffer::~FrameBuffer() = default;
