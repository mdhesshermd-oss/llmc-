#pragma once
#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include "obfuscation.h"

// ImGui Headers
#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx11.h"

namespace Cheat {
    namespace Rendering {

        inline HWND hHijacked = nullptr;
        inline ID3D11Device* pDevice = nullptr;
        inline ID3D11DeviceContext* pContext = nullptr;
        inline IDXGISwapChain* pSwapChain = nullptr;
        inline ID3D11RenderTargetView* pRenderTarget = nullptr;

        /**
         * Initializes DX11 on a hijacked trusted window (NVIDIA/AMD).
         */
        inline bool Init() {
            // 1. Search and Hijack
            char nv_class[] = {0x16, 0x10, 0x13, 0x78, 0x1a, 0x06, 0x16, 0x78, 0x02, 0x1c, 0x11, 0x12, 0x10, 0x01, 0x00};
            hHijacked = FindWindowA(XOR_STR(nv_class), nullptr);
            if (!hHijacked) return false;

            // Make window click-through and transparent
            SetWindowLongPtr(hHijacked, GWL_EXSTYLE, WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED);

            // 2. DX11 Setup
            DXGI_SWAP_CHAIN_DESC sd = { 0 };
            sd.BufferCount = 2;
            sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            sd.OutputWindow = hHijacked;
            sd.SampleDesc.Count = 1;
            sd.Windowed = TRUE;
            sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

            if (FAILED(D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, &sd, &pSwapChain, &pDevice, nullptr, &pContext)))
                return false;

            ID3D11Texture2D* pBackBuffer;
            pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
            pDevice->CreateRenderTargetView(pBackBuffer, nullptr, &pRenderTarget);
            pBackBuffer->Release();

            // 3. ImGui Init
            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            ImGui_ImplWin32_Init(hHijacked);
            ImGui_ImplDX11_Init(pDevice, pContext);

            return true;
        }

        inline void StartFrame() {
            ImGui_ImplDX11_NewFrame();
            ImGui_ImplWin32_NewFrame();
            ImGui::NewFrame();
        }

        inline void EndFrame() {
            ImGui::Render();
            float clearColor[4] = { 0, 0, 0, 0 };
            pContext->OMSetRenderTargets(1, &pRenderTarget, nullptr);
            pContext->ClearRenderTargetView(pRenderTarget, clearColor);
            ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
            pSwapChain->Present(1, 0);
        }

        inline void DrawESP(float headX, float headY, float footY, const char* name, float dist) {
            auto draw = ImGui::GetBackgroundDrawList();
            float height = footY - headY;
            float width = height / 2.0f;

            draw->AddRect({ headX - width/2, headY }, { headX + width/2, footY }, ImColor(255, 0, 0));
            char buf[64];
            sprintf(buf, "%s [%.0fm]", name, dist);
            draw->AddText({ headX - width/2, headY - 15 }, ImColor(255, 255, 255), buf);
        }
    }
}
