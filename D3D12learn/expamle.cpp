#include "resource.h"
#include "expamle.h"
#include "GeometryGenerator.h"
#include "rawinput.h"
#include "ScanCodeSet.h"
using namespace std;

#define T(M) ::XMMatrixTranspose(M)
#define R(d) d * (XM_PI / 180)
#define D(r) r * (180 / XM_PI)

struct Vertex {
	XMFLOAT4 pos;
	XMFLOAT4 color;
};

App::App() : mouseRect({})
{
#pragma region init
	{
		ThrowIfFailed(pCommandAllocator->Reset());
		ThrowIfFailed(pCommandList->Reset(pCommandAllocator.Get(), nullptr));
	}
#pragma endregion
#pragma region Create Constant Descriptor Heap
	{
		D3D12_DESCRIPTOR_HEAP_DESC cds = {};
		cds.NumDescriptors = 1;
		cds.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		cds.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		cds.NodeMask = 0;
		ThrowIfFailed(device->CreateDescriptorHeap(&cds, IID_PPV_ARGS(&cbvh)));
#pragma endregion 
#pragma region Create Root Parameter
		CD3DX12_ROOT_PARAMETER rps[2];
		CD3DX12_DESCRIPTOR_RANGE cbt1(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
		CD3DX12_DESCRIPTOR_RANGE cbt2(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1);
		rps[0].InitAsDescriptorTable(1, &cbt1);
		rps[1].InitAsDescriptorTable(1, &cbt2);
#pragma endregion
#pragma region Create Root Signature
		CD3DX12_ROOT_SIGNATURE_DESC rs(2, rps, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		ComPtr<ID3DBlob> root;
		ThrowIfFailed(D3D12SerializeRootSignature(&rs, D3D_ROOT_SIGNATURE_VERSION_1, &root, nullptr));
		ThrowIfFailed(device->CreateRootSignature(0, root->GetBufferPointer(), root->GetBufferSize(), IID_PPV_ARGS(&roots)));
#pragma endregion
#pragma region Create PSO
		vs = d3dUtil::FromMemory(IDR_SHADER1);
		ps = d3dUtil::FromMemory(IDR_SHADER2);

		D3D12_INPUT_ELEMENT_DESC il[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};

		D3D12_GRAPHICS_PIPELINE_STATE_DESC sd = {};
		sd.InputLayout = { il,_countof(il) };
		sd.pRootSignature = roots.Get();
		sd.VS = CD3DX12_SHADER_BYTECODE(vs.Get());
		sd.PS = CD3DX12_SHADER_BYTECODE(ps.Get());
		sd.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		sd.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		sd.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		sd.SampleMask = UINT_MAX;
		sd.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		sd.NumRenderTargets = 1;
		sd.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		sd.SampleDesc.Count = 1;
		sd.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		ThrowIfFailed(device->CreateGraphicsPipelineState(&sd, IID_PPV_ARGS(&pState)));
	}
#pragma endregion
#pragma region Create Geometry
	{
		using MeshData = GeometryGenerator::MeshData;
		GeometryGenerator geoGen;
		MeshData box = geoGen.CreateBox(1.5, 0.5, 1.5, 3);
		MeshData grid = geoGen.CreateGrid(20, 30, 60, 40);
		MeshData sphere = geoGen.CreateSphere(0.5, 20, 20);
		MeshData cylinder = geoGen.CreateCylinder(0.5, 0.3, 3.0, 20, 20);

		UINT boxOffset = 0;
		UINT gridOffset = (UINT)box.Vertices.size();
		UINT sphereOffset = gridOffset + (UINT)grid.Vertices.size();
		UINT cylinderOffset = sphereOffset + (UINT)sphere.Vertices.size();

		UINT boxOffset2 = 0;
		UINT gridOffset2 = (UINT)box.Indices32.size();
		UINT sphereOffset2 = gridOffset2 + (UINT)grid.Indices32.size();
		UINT cylinderOffset2 = sphereOffset2 + (UINT)sphere.Indices32.size();

		SubmeshGeometry boxSub;
		boxSub.IndexCount = (UINT)box.Indices32.size();
		boxSub.StartIndexLocation = boxOffset2;
		boxSub.BaseVertexLocation = boxOffset;

		SubmeshGeometry gridSub;
		gridSub.IndexCount = (UINT)grid.Indices32.size();
		gridSub.StartIndexLocation = gridOffset2;
		gridSub.BaseVertexLocation = gridOffset;

		SubmeshGeometry sphereSub;
		sphereSub.IndexCount = (UINT)sphere.Indices32.size();
		sphereSub.StartIndexLocation = sphereOffset2;
		sphereSub.BaseVertexLocation = sphereOffset;

		SubmeshGeometry cylinderSub;
		cylinderSub.IndexCount = (UINT)cylinder.Indices32.size();
		cylinderSub.StartIndexLocation = cylinderOffset2;
		cylinderSub.BaseVertexLocation = cylinderOffset;

		auto totalVertexCount = box.Vertices.size() + grid.Vertices.size() + sphere.Vertices.size() + cylinder.Vertices.size();

		vector<Vertex> vertices(totalVertexCount);

		UINT k = 0;
		for (int i = 0; i < box.Vertices.size(); i++, k++) {
			memcpy(&vertices[k].pos, &box.Vertices[i].Position, sizeof(XMFLOAT3));
			vertices[k].pos.w = 1;
			vertices[k].color = XMFLOAT4(Colors::DarkGreen);
		}
		for (int i = 0; i < grid.Vertices.size(); i++, k++) {
			memcpy(&vertices[k].pos, &grid.Vertices[i].Position, sizeof(XMFLOAT3));
			vertices[k].pos.w = 1;
			vertices[k].color = XMFLOAT4(Colors::ForestGreen);
		}
		for (int i = 0; i < sphere.Vertices.size(); i++, k++) {
			memcpy(&vertices[k].pos, &sphere.Vertices[i].Position, sizeof(XMFLOAT3));
			vertices[k].pos.w = 1;
			vertices[k].color = XMFLOAT4(Colors::Crimson);
		}
		for (int i = 0; i < cylinder.Vertices.size(); i++, k++) {
			memcpy(&vertices[k].pos, &cylinder.Vertices[i].Position, sizeof(XMFLOAT3));
			vertices[k].pos.w = 1;
			vertices[k].color = XMFLOAT4(Colors::SteelBlue);
		}

		vector<uint16_t> indices;
		indices.insert(indices.end(), box.GetIndices16().begin(), box.GetIndices16().end());
		indices.insert(indices.end(), grid.GetIndices16().begin(), grid.GetIndices16().end());
		indices.insert(indices.end(), sphere.GetIndices16().begin(), sphere.GetIndices16().end());
		indices.insert(indices.end(), cylinder.GetIndices16().begin(), cylinder.GetIndices16().end());

		const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
		const UINT ibByteSize = (UINT)indices.size() * sizeof(uint16_t);

		auto geo = make_unique<MeshGeometry>();
		geo->Name = "ShapeGeo";

		ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
		memcpy(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

		ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
		memcpy(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

		geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(device.Get(), pCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);
		geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(device.Get(), pCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

		geo->VertexByteStride = sizeof(Vertex);
		geo->VertexBufferByteSize = vbByteSize;
		geo->IndexFormat = DXGI_FORMAT_R16_UINT;
		geo->IndexBufferByteSize = ibByteSize;

		geo->DrawArgs["box"] = boxSub;
		geo->DrawArgs["grid"] = gridSub;
		geo->DrawArgs["sphere"] = sphereSub;
		geo->DrawArgs["cylinder"] = cylinderSub;
		mGeos[geo->Name] = move(geo);
	}
#pragma endregion
#pragma region Create Render Object
	{
		auto box = make_unique<RenderObject>();
		XMStoreFloat4x4(&box->World, XMMatrixScaling(2, 2, 2) * XMMatrixTranslation(0, 0.5, 0));
		box->ObjCBIndex = 0;
		box->Geo = mGeos["ShapeGeo"].get();
		box->SetDrawArgs("box");
		objs.push_back(move(box));

		auto grid = make_unique<RenderObject>();
		grid->World = MathHelper::Identity4x4();
		grid->ObjCBIndex = 1;
		grid->Geo = mGeos["ShapeGeo"].get();
		grid->SetDrawArgs("grid");
		objs.push_back(move(grid));
	}
	{
		UINT obji = 2;
		for (int i = 0; i < 5; i++) {
			auto lcyl = make_unique<RenderObject>();
			auto rcyl = make_unique<RenderObject>();
			auto lsph = make_unique<RenderObject>();
			auto rsph = make_unique<RenderObject>();

			XMMATRIX&& lcylW = XMMatrixTranslation(-5, 1.5, -10 + i * 5);
			XMMATRIX&& rcylW = XMMatrixTranslation(5, 1.5, -10 + i * 5);

			XMMATRIX&& lsphW = XMMatrixTranslation(-5, 3.5, -10 + i * 5);
			XMMATRIX&& rsphW = XMMatrixTranslation(5, 3.5, -10 + i * 5);

			XMStoreFloat4x4(&lcyl->World, lcylW);
			lcyl->ObjCBIndex = obji++;
			lcyl->Geo = mGeos["ShapeGeo"].get();
			lcyl->SetDrawArgs("cylinder");

			XMStoreFloat4x4(&rcyl->World, rcylW);
			rcyl->ObjCBIndex = obji++;
			rcyl->Geo = mGeos["ShapeGeo"].get();
			rcyl->SetDrawArgs("cylinder");

			XMStoreFloat4x4(&lsph->World, lsphW);
			lsph->ObjCBIndex = obji++;
			lsph->Geo = mGeos["ShapeGeo"].get();
			lsph->SetDrawArgs("sphere");

			XMStoreFloat4x4(&rsph->World, rsphW);
			rsph->ObjCBIndex = obji++;
			rsph->Geo = mGeos["ShapeGeo"].get();
			rsph->SetDrawArgs("sphere");

			objs.push_back(move(lcyl));
			objs.push_back(move(rcyl));
			objs.push_back(move(lsph));
			objs.push_back(move(rsph));
		}
	}
#pragma endregion
#pragma region Create Frame Buffer
	{
		for (int i = 0; i < FrameBufferCount; i++) {
			mFrameBuffers[i] = make_unique<FrameBuffer>(device.Get(), 1, objs.size());
		}
	}
#pragma endregion
#pragma region Create CBVs
	{
		UINT objsize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
		UINT objc = (UINT)objs.size();
		UINT numd = (objc + 1) * FrameBufferCount;
		{	/*Create CBV Heap*/
			passCbvOffset = objc * FrameBufferCount;
			D3D12_DESCRIPTOR_HEAP_DESC cbvd = {};
			cbvd.NumDescriptors = numd;
			cbvd.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			cbvd.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
			cbvd.NodeMask = 0;
			ThrowIfFailed(device->CreateDescriptorHeap(&cbvd, IID_PPV_ARGS(&cbvh)));
		}
		for (UINT64 i = 0; i < FrameBufferCount; i++) {
			auto obCB = mFrameBuffers[i]->ObjCB->Resource();
			for (UINT64 j = 0; j < objc; j++) {
				auto cbAddr = obCB->GetGPUVirtualAddress();
				cbAddr += j * objsize;

				int hi = i * objc + j;
				auto hand = CD3DX12_CPU_DESCRIPTOR_HANDLE(cbvh->GetCPUDescriptorHandleForHeapStart());
				hand.Offset(hi, device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));

				D3D12_CONSTANT_BUFFER_VIEW_DESC cbd = {};
				cbd.BufferLocation = cbAddr;
				cbd.SizeInBytes = objsize;

				device->CreateConstantBufferView(&cbd, hand);
			}
		}
		UINT passsize = d3dUtil::CalcConstantBufferByteSize(sizeof(PassConstants));
		for (int i = 0; i < FrameBufferCount; i++) {
			auto passCV = mFrameBuffers[i]->PassCB->Resource();
			auto cbAddr = passCV->GetGPUVirtualAddress();

			int hi = passCbvOffset + i;
			auto hand = CD3DX12_CPU_DESCRIPTOR_HANDLE(cbvh->GetCPUDescriptorHandleForHeapStart());
			hand.Offset(hi, device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));

			D3D12_CONSTANT_BUFFER_VIEW_DESC cd = {};
			cd.BufferLocation = cbAddr;
			cd.SizeInBytes = passsize;

			device->CreateConstantBufferView(&cd, hand);
		}
	}
#pragma endregion
#pragma region post init
	ThrowIfFailed(pCommandList->Close());
	pCommandQueue->ExecuteCommandLists(1, (ID3D12CommandList**)pCommandList.GetAddressOf());

	mEyePos = { 0 , 3 , -10 };

	XMVECTOR pos = XMVectorSet(mEyePos.x, mEyePos.y, mEyePos.z, 1.0f);
	XMVECTOR target = XMVectorZero();
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	XMMATRIX&& view = ::XMMatrixLookAtLH(pos, target, up);
	XMStoreFloat4x4(&mView, view);

	XMMATRIX&& proj = ::XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, 800.0f / 600, 1.0f, 1000.0f);
	XMStoreFloat4x4(&mProj, proj);
	ShowCursor(false);
	mouseRect.left = 200 + 400;
	mouseRect.top = 200 + 300;
	mouseRect.right = 200 + 400;
	mouseRect.bottom = 200 + 300;
#pragma endregion
}

void App::Render()
{
	if (!closed) {
#pragma region Reset command list and allocator
		ThrowIfFailed(mCurrentFrame->pCmdAllocator->Reset());
		ThrowIfFailed(pCommandList->Reset(mCurrentFrame->pCmdAllocator.Get(), pState.Get()));
#pragma endregion
#pragma region Create vars
		static const float color[] = { 0,1,1,1 };
		CD3DX12_RESOURCE_BARRIER&& b1 = CD3DX12_RESOURCE_BARRIER::Transition(buffers[mCurrBackBuffer].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
		CD3DX12_RESOURCE_BARRIER&& b2 = CD3DX12_RESOURCE_BARRIER::Transition(buffers[mCurrBackBuffer].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		D3D12_CPU_DESCRIPTOR_HANDLE&& cbv = CurrentBackBufferView();
		D3D12_CPU_DESCRIPTOR_HANDLE&& dsv = DepthStencilView();
		auto passHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(cbvh->GetGPUDescriptorHandleForHeapStart());
		{
			int i = passCbvOffset + mCurrentFrameIndex;
			passHandle.Offset(i, device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
		}
#pragma endregion 
#pragma region Rendering
		pCommandList->ResourceBarrier(1, &b1);
		pCommandList->RSSetViewports(1, &mScreenViewport);
		pCommandList->RSSetScissorRects(1, &mScissorRect);
		pCommandList->ClearRenderTargetView(cbv, color, 0, nullptr);
		pCommandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1, 0, 0, nullptr);
		pCommandList->OMSetRenderTargets(1, &cbv, true, &dsv);
		pCommandList->SetDescriptorHeaps(1, cbvh.GetAddressOf());
		pCommandList->SetGraphicsRootSignature(roots.Get());
		pCommandList->SetGraphicsRootDescriptorTable(1, passHandle);
		DrawObjects();
		pCommandList->ResourceBarrier(1, &b2);
		ThrowIfFailed(pCommandList->Close());
		pCommandQueue->ExecuteCommandLists(1, (ID3D12CommandList**)pCommandList.GetAddressOf());
		pSwapChain->Present(0, DXGI_PRESENT_ALLOW_TEARING);
#pragma endregion
#pragma region Update fence
		++mCurrBackBuffer %= BUFF_COUNT;
		mCurrentFrame->Fence = ++mCurrentFence;
		pCommandQueue->Signal(fence.Get(), mCurrentFence);
#pragma endregion
	}
}

void App::Update(float delta)
{
	render = true;
	++mCurrentFrameIndex %= FrameBufferCount;
	mCurrentFrame = mFrameBuffers[mCurrentFrameIndex].get();
#pragma region Wait Fence
	if (mCurrentFrame->Fence != 0 && fence->GetCompletedValue() < mCurrentFrame->Fence) {
		HANDLE event = CreateEventEx(nullptr, TEXT("fence"), 0, EVENT_ALL_ACCESS);
		ThrowIfFailed(fence->SetEventOnCompletion(mCurrentFrame->Fence, event));
		assert(event != NULL);
		WaitForSingleObject(event, INFINITE);
		CloseHandle(event);
	}
#pragma endregion
	UpdateInput();
	UpdateObjectCBs();
	UpdateMainPassCB();
}

bool App::OnRawInput(RAWINPUT* raw)
{
	if (raw->header.dwType == RIM_TYPEKEYBOARD)
	{
		UINT code = raw->data.keyboard.MakeCode;
		UINT flags = raw->data.keyboard.Flags;
		UINT exInfo = raw->data.keyboard.ExtraInformation;
		UINT m = raw->data.keyboard.Message;
		switch (flags)
		{
		case RI_KEY_BREAK:
			__OnRawKeyUp(code);
			break;
		case RI_KEY_MAKE:
			__OnRawKeyDown(code);
			break;
		case RI_KEY_E0 | RI_KEY_BREAK:
			__OnRawKeyUp(code, 1);
			break;
		case RI_KEY_E0 | RI_KEY_MAKE:
			__OnRawKeyDown(code, 1);
			break;
		case RI_KEY_E1 | RI_KEY_BREAK:
			__OnRawKeyUp(code, 2);
			break;
		case RI_KEY_E1 | RI_KEY_MAKE:
			__OnRawKeyDown(code, 2);
			break;
		}
	}
	else if (raw->header.dwType == RIM_TYPEMOUSE)
	{
		if (raw->data.mouse.ulButtons != 0) {
			OnMouseButtonChange(raw);
		}
		OnMouseMove(raw);
	}
	return false;
}

void App::UpdateObjectCBs()
{
	auto cobj = mCurrentFrame->ObjCB.get();
	for (auto& e : objs) {
		if (e->NumFrameDirty > 0) {
			XMMATRIX world = XMLoadFloat4x4(&e->World);
			ObjectConstants co;
			XMStoreFloat4x4(&co.world, T(world));
			cobj->CopyData(e->ObjCBIndex, co);
			e->NumFrameDirty--;
		}
	}
}

void App::UpdateMainPassCB()
{
	XMVECTOR pos = XMVectorSet(mEyePos.x, mEyePos.y, mEyePos.z, 1.0f);
	XMVECTOR&& dir = GetForward();
	XMVECTOR target = pos + dir;
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMMATRIX view = ::XMMatrixLookAtLH(pos, target, up);

	XMMATRIX proj = XMLoadFloat4x4(&mProj);
	XMMATRIX viewProj = view * proj;
	XMVECTOR&& v1 = ::XMMatrixDeterminant(view);
	XMMATRIX invView = ::XMMatrixInverse(&v1, view);
	XMVECTOR&& v2 = ::XMMatrixDeterminant(proj);
	XMMATRIX invProj = ::XMMatrixInverse(&v2, proj);
	XMVECTOR&& v3 = ::XMMatrixDeterminant(viewProj);
	XMMATRIX invViewProj = ::XMMatrixInverse(&v3, viewProj);
#pragma region Update
	XMStoreFloat4x4(&mMainPassCB.gView, T(view));
	XMStoreFloat4x4(&mMainPassCB.gInvView, T(invView));
	XMStoreFloat4x4(&mMainPassCB.gProj, T(proj));
	XMStoreFloat4x4(&mMainPassCB.gInvProj, T(invProj));
	XMStoreFloat4x4(&mMainPassCB.gViewProj, T(viewProj));
	XMStoreFloat4x4(&mMainPassCB.gInvViewProj, T(invViewProj));
	mMainPassCB.gEyePosW = mEyePos;
	mMainPassCB.gRenderTargetSize = { 800.0f,600.0f };
	mMainPassCB.gInvRenderTargetSize = { 1.0f / 800,1.0f / 600 };
	mMainPassCB.gNearZ = 1;
	mMainPassCB.gFarZ = 1000;
	mMainPassCB.gTotalTime = timer.TotalTime();
	mMainPassCB.gDeltaTime = timer.DeltaTime();
	mCurrentFrame->PassCB->CopyData(0, mMainPassCB);
#pragma endregion
}

LRESULT App::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_ACTIVATE:
		if (wParam != WA_INACTIVE) {
			SetCapture(hwnd);
			ClipCursor(&mouseRect);
		}
		else {
			ReleaseCapture();
			ClipCursor(nullptr);
		}
		break;
	}
	return __super::WndProc(hwnd, msg, wParam, lParam);
}

void App::DrawObjects()
{
	UINT objsize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
	auto obCB = mCurrentFrame->ObjCB->Resource();

	for (int i = 0; i < objs.size(); i++) {
		auto it = objs[i].get();
		auto&& vbv = it->Geo->VertexBufferView();
		auto&& ibv = it->Geo->IndexBufferView();
		pCommandList->IASetVertexBuffers(0, 1, &vbv);
		pCommandList->IASetIndexBuffer(&ibv);
		pCommandList->IASetPrimitiveTopology(it->PrimitiveType);

		UINT cbi = mCurrentFrameIndex * objs.size() + it->ObjCBIndex;
		auto cbh = CD3DX12_GPU_DESCRIPTOR_HANDLE(cbvh->GetGPUDescriptorHandleForHeapStart());
		cbh.Offset(cbi, device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));

		pCommandList->SetGraphicsRootDescriptorTable(0, cbh);
		pCommandList->DrawIndexedInstanced(it->IndexCount, 1, it->StartIndexLocation, it->BaseVertexLocation, 0);
	}
}

void App::UpdateInput()
{
	if (states != 0) {
		static const XMVECTOR up = { 0, 1, 0, 0 };
		static const float speed = 0.02f;
		XMVECTOR&& pos = XMLoadFloat3(&mEyePos);
		XMVECTOR&& look = GetForward();
		XMVECTOR&& right = XMVector3Normalize(XMVector3Cross(up, look));
		XMVECTOR&& front = XMVector3Normalize(XMVector3Cross(right, up));
		if (KeyStates.A_Pressed) {
			pos -= right * speed;
		}
		if (KeyStates.S_Pressed) {
			pos -= front * speed;
		}
		if (KeyStates.D_Pressed) {
			pos += right * speed;
		}
		if (KeyStates.W_Pressed) {
			pos += front * speed;
		}
		if (KeyStates.Space_Pressed) {
			pos += up * speed;
		}
		if (KeyStates.LShift_Pressed) {
			pos -= up * speed;
		}
		XMStoreFloat3(&mEyePos, pos);
	}
}

void App::__OnRawKeyDown(UINT code, UINT type)
{
	switch (code)
	{
	case A:
		KeyStates.A_Pressed = true;
		break;
	case S:
		KeyStates.S_Pressed = true;
		break;
	case D:
		KeyStates.D_Pressed = true;
		break;
	case W:
		KeyStates.W_Pressed = true;
		break;
	case SPACE:
		KeyStates.Space_Pressed = true;
		break;
	case LSHIFT:
		KeyStates.LShift_Pressed = true;
		break;
	case ESC:
		closed = true;
		DestroyWindow(hWnd);
		return;
	}
}

void App::__OnRawKeyUp(UINT code, UINT type)
{
	switch (code)
	{
	case A:
		KeyStates.A_Pressed = false;
		break;
	case S:
		KeyStates.S_Pressed = false;
		break;
	case D:
		KeyStates.D_Pressed = false;
		break;
	case W:
		KeyStates.W_Pressed = false;
		break;
	case SPACE:
		KeyStates.Space_Pressed = false;
		break;
	case LSHIFT:
		KeyStates.LShift_Pressed = false;
		break;
	}
}

void App::OnMouseButtonChange(RAWINPUT* raw)
{
	UINT flags = raw->data.mouse.ulButtons;
}

void App::OnMouseMove(RAWINPUT* raw)
{
	int x = raw->data.mouse.lLastX;
	int y = raw->data.mouse.lLastY;
	if (raw->data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE) {
	}
	else if (!(raw->data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE)) {
		yaw -= x;
		if (yaw >= 180)
			yaw -= 360;
		if (yaw <= -180)
			yaw += 360;
		if (pitch - y < 90 && pitch - y>-90)
			pitch -= y;
	}
}

DirectX::XMVECTOR App::GetForward()
{
	auto const ry = R(yaw);
	auto const rp = R(pitch);
	auto const cy = cos(ry);
	auto const cp = cos(rp);
	auto const sp = sin(rp);
	auto const sy = sin(ry);
	return XMVector3Normalize(XMVectorSet(cy * cp, sp, cp * sy, 0));
}
