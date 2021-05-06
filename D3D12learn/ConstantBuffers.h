#pragma once
#include "d3dUtil.h"

__interface Consts {
};

class ObjectConstants : public Consts {
public:
	DirectX::XMFLOAT4X4 world = MathHelper::Identity4x4();
};

class PassConstants : public Consts {
	using float4x4 = DirectX::XMFLOAT4X4;
	using float3 = DirectX::XMFLOAT3;
	using float2 = DirectX::XMFLOAT2;
public:
	float4x4 gView;
	float4x4 gInvView;
	float4x4 gProj;
	float4x4 gInvProj;
	float4x4 gViewProj;
	float4x4 gInvViewProj;
	float3 gEyePosW;
	float cbPerObjectPad1;
	float2 gRenderTargetSize;
	float2 gInvRenderTargetSize;
	float gNearZ;
	float gFarZ;
	float gTotalTime;
	float gDeltaTime;
};

