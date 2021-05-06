#pragma once
#ifndef CLASS_ONLY
#include "d3dUtil.h"
#include "expamle.h"
#endif

struct RenderObject
{
public:
	DirectX::XMFLOAT4X4 World = MathHelper::Identity4x4();
public:
	int NumFrameDirty = App::FrameBufferCount;
	UINT ObjCBIndex = -1;
public:
	MeshGeometry* Geo = nullptr;
	D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
public:
	UINT IndexCount = 0;
	UINT StartIndexLocation = 0;
	UINT BaseVertexLocation = 0;
public:
	RenderObject() = default;
	~RenderObject() = default;
	void SetDrawArgs(std::string name);
};

