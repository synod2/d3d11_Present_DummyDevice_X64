#pragma once
#include <windows.h>
#include <d3d11.h>
#include <stdio.h>
#include <iostream>
#include <iomanip>

#define DLogs(x,...) { if(x) {printf("========"); printf(__VA_ARGS__); printf("=========\n"); } else {printf(__VA_ARGS__); printf("\n");} }

// d3d11 related object ptrs
ID3D11Device* pDevice = NULL;
IDXGISwapChain* pSwapchain = NULL;
ID3D11DeviceContext* pContext = NULL;
ID3D11RenderTargetView* pRenderTargetView = NULL;
ID3D11VertexShader* pVertexShader = NULL;
ID3D11InputLayout* pVertexLayout = NULL;
ID3D11PixelShader* pPixelShader = NULL;
ID3D11Buffer* pVertexBuffer = NULL;
ID3D11Buffer* pIndexBuffer = NULL;
ID3D11Buffer* pConstantBuffer = NULL;

void* dtable[18];
using namespace std;

typedef HRESULT(APIENTRY* tPresent)(IDXGISwapChain* pThis, UINT SyncInterval, UINT Flags);
tPresent oPresent = NULL;

namespace d3dhelper {
	bool GetD3D11Device(void** pTable, size_t Size) {
		DLogs(1, "Initialize Dummy Device");
		
		//CreateDevice Options
		D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
		DXGI_SWAP_CHAIN_DESC sd;
		sd.BufferCount = 1;
		sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		sd.OutputWindow = GetForegroundWindow();
		sd.SampleDesc.Count = 1;
		sd.Windowed = TRUE;
		sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
		sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
		/*sd.SampleDesc.Quality = 0;
		sd.BufferDesc.RefreshRate = { 60,};*/

		//Create Dummy Device
		HRESULT ddc = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, NULL, &featureLevel, 1, D3D11_SDK_VERSION,
			&sd, &pSwapchain, &pDevice, NULL, &pContext);
		if (FAILED(ddc)) {
			sd.Windowed = FALSE;
			DLogs(0, "try Window mode..");
			ddc = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_REFERENCE, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, 
				&sd, &pSwapchain, &pDevice, NULL, &pContext);
			if (FAILED(ddc))
			{
				DLogs(0, "CreateDevice Error %0lx", ddc);
				return false;
			}
		}
			

		memcpy(pTable, *reinterpret_cast<void***>(pSwapchain), Size);

		DLogs(1, "Dummy Device Initializing success");
		
		pSwapchain->Release();
		pDevice->Release();

		return true;
	}
}

namespace hook {
	HRESULT APIENTRY hEndScene(IDXGISwapChain* pThis, UINT SyncInterval, UINT Flags) {
		DLogs(0, "Hook Successful!");

		return oPresent(pThis, SyncInterval, Flags);
	}
	//minimum length size is 12
	PVOID hookTramp(DWORD64 src, DWORD64 target, DWORD len) {
		DLogs(1, "Make Trampoline");
		if (len < 12) {
			DLogs(0, "too small overwrite length");
			return 0;
		}
		DWORD protect = 0;
		PVOID tmp = 0;
		VirtualProtect((PVOID)src, len, PAGE_EXECUTE_READWRITE, &protect);

		//make trampoline
		// push rax
		// movabs rax, 0xCCCCCCCCCCCCCCCC
		// xchg rax, [rsp]
		// ret
		BYTE stub[] = {
			0x50, 0x48, 0xB8, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0x48, 0x87, 0x04, 0x24, 0xC3
		};

		//Allocate Trampoline memory
		PVOID tramp = VirtualAlloc(0, len + sizeof(stub), MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);

		//Set Original Address in Trampoline Function
		tmp = &(stub[3]);
		*((DWORD64*)tmp) = src + len;

		if (tramp) {
			//Copy source's original Codes to trampoline
			memcpy(tramp, (PVOID)src, len);
			memcpy((PBYTE)tramp + len, stub, sizeof(stub));
		}
		else
			return 0;

		//Overwrite Original Function to Jumped to hook function 
		//mov rax
		*((LPWORD)src) = 0xB848;
		tmp = (LPBYTE)src + 2;

		//Target address
		*((DWORD64*)tmp) = target;
		tmp = (LPBYTE)tmp + 8;

		//jmp rax
		*((LPWORD)tmp) = 0xE0FF;
		tmp = (LPBYTE)tmp + 2;

		//overwrite 0x90 to remaining area
		for (int i = 0; i != len - 12; i++, tmp = (LPBYTE)tmp + 1)
		{
			*((LPBYTE)tmp) = 0x90;
		}

		return tramp;
	}
}