#include "Windows.h"
#include "dwmapi.h"
#include "d3d11.h"

#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_dx11.h"
#include "ImGui/imgui_impl_win32.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT WINAPI WndProc(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam)
{
    if (ImGui_ImplWin32_WndProcHandler(Window, Message, WParam, LParam))
		return 0L;
    
    if (Message == WM_DESTROY)
    {
        PostQuitMessage(0);
		return 0L;
    }

    return DefWindowProc(Window, Message, WParam, LParam);
}

INT APIENTRY WinMain(HINSTANCE Instance, HINSTANCE, PSTR, INT ShowCMD)
{
    WNDCLASSEXW WC{};

    WC.cbSize = sizeof(WNDCLASSEXW);
    WC.style = CS_HREDRAW || CS_VREDRAW;
    WC.lpfnWndProc = WndProc;
    WC.hInstance = Instance;
    WC.lpszClassName = L"Overlook";

    RegisterClassExW(&WC);

    const HWND Window = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED,
        WC.lpszClassName,
        L"Overlook",
        WS_POPUP,
        0,
        0,
        1920,
        1080,
        nullptr,
        nullptr,
        WC.hInstance,
        nullptr
    );

    SetLayeredWindowAttributes(Window, RGB(0, 0, 0), BYTE(255), LWA_ALPHA);

    {
        RECT client_area{};
        GetClientRect(Window, &client_area);

        RECT window_area{};
        GetWindowRect(Window, &window_area);

        POINT diff{};
        ClientToScreen(Window, &diff);

        const MARGINS margins{
            window_area.left + (diff.x - window_area.left),
            window_area.top + (diff.y - window_area.top),
            client_area.right,
            client_area.bottom
        };

        DwmExtendFrameIntoClientArea(Window, &margins);
    }

    DXGI_SWAP_CHAIN_DESC SwapChainDesc{};
    SwapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
    SwapChainDesc.BufferDesc.RefreshRate.Denominator = 1U;
    SwapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    SwapChainDesc.SampleDesc.Count = 1U;
    SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    SwapChainDesc.BufferCount = 2U;
    SwapChainDesc.OutputWindow = Window;
    SwapChainDesc.Windowed = TRUE;
    SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    SwapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    constexpr D3D_FEATURE_LEVEL FeatureLevels[]{
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_0
	};

    ID3D11Device* Device{nullptr};
    ID3D11DeviceContext* DeviceContext{nullptr};
    IDXGISwapChain* SwapChain{nullptr};
    ID3D11RenderTargetView* RenderTargetView{nullptr};
    D3D_FEATURE_LEVEL FeatureLevel{};

    D3D11CreateDeviceAndSwapChain(
		nullptr,
		D3D_DRIVER_TYPE_HARDWARE,
		nullptr,
		0U,
		FeatureLevels,
		2U,
		D3D11_SDK_VERSION,
		&SwapChainDesc,
		&SwapChain,
		&Device,
		&FeatureLevel,
		&DeviceContext
	);

    ID3D11Texture2D* BackBuffer{nullptr};
    SwapChain->GetBuffer(0U, IID_PPV_ARGS(&BackBuffer));

    if (BackBuffer)
    {
        Device->CreateRenderTargetView(BackBuffer, nullptr, &RenderTargetView);
        BackBuffer->Release();
    }
    else
        return 1;

    ShowWindow(Window, ShowCMD);
    UpdateWindow(Window);

    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    ImGui_ImplWin32_Init(Window);
    ImGui_ImplDX11_Init(Device, DeviceContext);

    bool Running = true;
    bool Menu = false;
    bool Crosshair = true;
    int CrosshairX = GetSystemMetrics(SM_CXSCREEN) / 2;
    int CrosshairY = GetSystemMetrics(SM_CYSCREEN) / 2;
    int CrosshairRadius = 10;
    const char* Crosshairs[] = { "Circle", "Rectangle", "-", "+", "*"};
    static int CurrentCrosshairIndex = 0;
    bool VSync = true;

    while (Running)
    {
       MSG Message;
       while (PeekMessage(&Message, nullptr, 0U, 0U, PM_REMOVE))
       {
			TranslateMessage(&Message);
			DispatchMessage(&Message);

			if (Message.message == WM_QUIT)
				Running = false;
		}

       if (!Running)
		   break;   

       ImGui_ImplDX11_NewFrame();
	   ImGui_ImplWin32_NewFrame();

	   ImGui::NewFrame();

       if (GetAsyncKeyState(VK_INSERT))
       {
           Menu = !Menu;
           Sleep(150);

           if (!Menu)
               SetWindowLongPtr(Window, GWL_EXSTYLE, GetWindowLongPtr(Window, GWL_EXSTYLE) | WS_EX_LAYERED); // Add WS_EX_LAYERED
           else
               SetWindowLongPtr(Window, GWL_EXSTYLE, GetWindowLongPtr(Window, GWL_EXSTYLE) & ~WS_EX_LAYERED); // Remove WS_EX_LAYERED

           SetWindowPos(Window, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);
       }


       if (Menu)
       {
           ImGui::Begin("Overlook", &Menu, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
           
           ImGuiStyle& style = ImGui::GetStyle();
           style.WindowRounding = 0.0f;
           style.Colors[ImGuiCol_TitleBg] = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);
           style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.15f, 0.15f, 0.15f, 1.0f);
           style.Colors[ImGuiCol_Text] = ImVec4(0.8f, 0.8f, 0.8f, 1.0f);
           style.Colors[ImGuiCol_WindowBg] = ImVec4(0.05f, 0.05f, 0.05f, 0.7f);

           style.Colors[ImGuiCol_Button] = ImVec4(0.02f, 0.02f, 0.02f, 0.7f);
           style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.6f, 0.6f, 0.6f, 0.7f);
           style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.12f, 0.12f, 0.12f, 0.7f);

           style.Colors[ImGuiCol_CheckMark] = ImVec4(0.7f, 0.7f, 0.7f, 0.7f);
           style.Colors[ImGuiCol_FrameBg] = ImVec4(0.01f, 0.01f, 0.01f, 0.7f);
           style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.6f, 0.6f, 0.6f, 0.7f);
           style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.12f, 0.12f, 0.12f, 0.7f);

           style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.6f, 0.6f, 0.6f, 0.7f);
           style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.12f, 0.12f, 0.12f, 0.7f);

           ImGui::Checkbox("Active", &Crosshair);

           if (ImGui::BeginCombo("Style", Crosshairs[CurrentCrosshairIndex]))
           {
               for (int i = 0; i < IM_ARRAYSIZE(Crosshairs); i++)
               {
                   const bool isSelected = (CurrentCrosshairIndex == i);
                   if (ImGui::Selectable(Crosshairs[i], isSelected))
                       CurrentCrosshairIndex = i;

                   if (isSelected)
                       ImGui::SetItemDefaultFocus();
               }

               ImGui::EndCombo();
           }

           ImGui::SliderInt("X", &CrosshairX, 0, GetSystemMetrics(SM_CXSCREEN));
           ImGui::SliderInt("Y", &CrosshairY, GetSystemMetrics(SM_CYSCREEN), 0);
           ImGui::SliderInt("Radius", &CrosshairRadius, 1, 100);
           ImGui::Checkbox("VSync |", &VSync);
           ImGui::SameLine();
           ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);

           if (ImGui::Button("Reset", { 100.f, 20.f }))
           {
			   CrosshairX = GetSystemMetrics(SM_CXSCREEN) / 2;
			   CrosshairY = GetSystemMetrics(SM_CYSCREEN) / 2;
               CrosshairRadius = 10;
		   }

           if (ImGui::Button("Exit", { 100.f, 20.f }))
               Running = false;
           
           ImGui::SetWindowSize({ 225.f, 230.f });
           ImGui::End();
       }

       if (Crosshair)
       {
           switch (CurrentCrosshairIndex)
           {
               case 0:
				   ImGui::GetBackgroundDrawList()->AddCircleFilled({ CrosshairX + 0.f, CrosshairY + 0.f }, CrosshairRadius, ImColor(1.f, 0.f, 0.f));
				   break;
               case 1:
                   ImGui::GetBackgroundDrawList()->AddRectFilled({ CrosshairX - (CrosshairRadius + 0.f / 2), CrosshairY - (CrosshairRadius + 0.f / 2) }, { CrosshairX + (CrosshairRadius + 0.f / 2), CrosshairY + (CrosshairRadius + 0.f / 2) }, ImColor(1.f, 0.f, 0.f) );
                   break;
               case 2:
                   ImGui::GetBackgroundDrawList()->AddText({ CrosshairX - ImGui::CalcTextSize("-").x / 2.0f, CrosshairY - ImGui::CalcTextSize("-").y / 2.0f }, ImColor(1.f, 0.f, 0.f), "-");
                   break;
               case 3:
                   ImGui::GetBackgroundDrawList()->AddText({ CrosshairX - ImGui::CalcTextSize("+").x / 2.0f, CrosshairY - ImGui::CalcTextSize("+").y / 2.0f}, ImColor(1.f, 0.f, 0.f), "+");
                   break;
               case 4:
                   ImGui::GetBackgroundDrawList()->AddText({ CrosshairX - ImGui::CalcTextSize("*").x / 2.0f, CrosshairY - ImGui::CalcTextSize("*").y / 2.0f}, ImColor(1.f, 0.f, 0.f), "*");
				   break;
               default:
                   ImGui::GetBackgroundDrawList()->AddCircleFilled({ CrosshairX + 0.f, CrosshairY + 0.f }, CrosshairRadius, ImColor(1.f, 0.f, 0.f));
                   break;

           }
       }

       ImGui::Render();

       constexpr float Colour[4]{0.f, 0.f, 0.f, 0.f};

	   DeviceContext->OMSetRenderTargets(1U, &RenderTargetView, nullptr);
	   DeviceContext->ClearRenderTargetView(RenderTargetView, Colour);

	   ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

       if (VSync)
           SwapChain->Present(1U, 0U);
	   else
	       SwapChain->Present(0U, 0U);
    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    if (SwapChain)
		SwapChain->Release();
    if (DeviceContext)
        DeviceContext->Release();
    if (Device)
        Device->Release();
    if (RenderTargetView)
        RenderTargetView->Release();

    DestroyWindow(Window);
    UnregisterClassW(WC.lpszClassName, WC.hInstance);

    return 0;
}
