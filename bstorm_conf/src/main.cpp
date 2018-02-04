#include <windows.h>
#include <string>
#include <cstdio>
#include <clocale>
#include <fstream>
#include <d3d9.h>
#include <imgui.h>

#include <bstorm/input_device.hpp>
#include <bstorm/key_name_map.hpp>
#include <bstorm/config.hpp>

#include "../../imgui/examples/directx9_example/imgui_impl_dx9.h"
#include "../../IconFontCppHeaders/IconsFontAwesome.h"
#include "../../glyph_ranges_ja.hpp"
#include "../../version.hpp"
#include "../resource.h"

using namespace bstorm;

/* window procedure */
extern IMGUI_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
static LRESULT WINAPI windowProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp) {
  if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wp, lp)) return true;
  switch (msg) {
    case WM_SYSCOMMAND:
      if ((wp & 0xfff0) == SC_KEYMENU)
        return 0;
      break;
    case WM_DESTROY:
      PostQuitMessage(0);
      return 0;
  }
  return DefWindowProc(hWnd, msg, wp, lp);
}

constexpr float configWindowWidth = 480;
constexpr float configWindowHeight = 360;
#ifdef _DEBUG
constexpr DWORD windowWidth = 800;
constexpr DWORD windowHeight = 600;
#else
constexpr DWORD windowWidth = configWindowWidth;
constexpr DWORD windowHeight = configWindowHeight;
#endif

constexpr wchar_t* windowTitle = L"bstorm config " BSTORM_VERSION_W;
constexpr char* jaFontPath = "fonts/ja/ipagp.ttf";
constexpr char* iconFontPath = "fonts/fa/fontawesome-webfont.ttf";

constexpr bool useBinaryFormat = true;
constexpr char* configFilePath = useBinaryFormat ? "config.dat" : "config.json";

enum class Tab {
  WINDOW,
  KEY,
  OPTION
};

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int) {
  _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
  std::setlocale(LC_ALL, "C");

  /* create window */
  HWND hWnd = NULL;
  WNDCLASSEX windowClass;
  {
    windowClass.cbSize = sizeof(WNDCLASSEX);
    windowClass.style = 0;
    windowClass.lpfnWndProc = windowProc;
    windowClass.cbClsExtra = 0;
    windowClass.cbWndExtra = 0;
    windowClass.hInstance = GetModuleHandle(NULL);
    windowClass.hIcon = LoadIcon(NULL, IDC_ICON);
    windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    windowClass.hbrBackground = NULL;
    windowClass.lpszMenuName = NULL;
    windowClass.lpszClassName = L"BSTORM CONFIG";
    windowClass.hIconSm = NULL;

    RegisterClassEx(&windowClass);

    RECT windowRect = { 0, 0, windowWidth, windowHeight };
    DWORD windowStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU;
    AdjustWindowRect(&windowRect, windowStyle, FALSE);
    hWnd = CreateWindowEx(0, windowClass.lpszClassName, windowTitle, windowStyle, CW_USEDEFAULT, CW_USEDEFAULT, windowRect.right - windowRect.left, windowRect.bottom - windowRect.top, NULL, NULL, GetModuleHandle(NULL), NULL);
    ValidateRect(hWnd, NULL);
    ShowWindow(hWnd, SW_RESTORE);
    UpdateWindow(hWnd);
  }

  /* get direct3D device */
  IDirect3D9* d3D = Direct3DCreate9(D3D_SDK_VERSION);
  IDirect3DDevice9* d3DDevice = NULL;
  D3DPRESENT_PARAMETERS presentParams = {
    windowWidth,
    windowHeight,
    D3DFMT_UNKNOWN,
    1,
    D3DMULTISAMPLE_NONE,
    0,
    D3DSWAPEFFECT_DISCARD,
    hWnd,
    TRUE,
    TRUE,
    D3DFMT_D24S8,
    0,
    D3DPRESENT_RATE_DEFAULT,
    D3DPRESENT_INTERVAL_ONE
  };

  if (FAILED(d3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_FPU_PRESERVE, &presentParams, &d3DDevice))) {
    if (FAILED(d3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_FPU_PRESERVE, &presentParams, &d3DDevice))) {
      if (FAILED(d3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_REF, hWnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_FPU_PRESERVE, &presentParams, &d3DDevice))) {
        d3D->Release();
        return 1;
      }
    }
  }

  MSG msg;
  try {
    InputDevice inputDevice(hWnd, nullptr, nullptr, windowWidth, windowHeight, nullptr, nullptr);

    {
      /* init imgui */
      ImGui_ImplDX9_Init(hWnd, d3DDevice);
      ImGuiIO& io = ImGui::GetIO();

      // prevent imgui.ini generation
      io.IniFilename = NULL;

      // imgui font setting
      ImFontConfig config;
      config.MergeMode = true;
      io.Fonts->AddFontDefault();
      if (FILE* fp = fopen(jaFontPath, "rb")) {
        fclose(fp);
        io.Fonts->AddFontFromFileTTF(jaFontPath, 12, &config, glyphRangesJapanese);
      }
      if (FILE* fp = fopen(iconFontPath, "rb")) {
        fclose(fp);
        static const ImWchar icon_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
        io.Fonts->AddFontFromFileTTF("fonts/fa/fontawesome-webfont.ttf", 13.0f, &config, icon_ranges);
      }

      // imgui style
      {
        auto& style = ImGui::GetStyle();
        style.FrameRounding = 3.0f;
        ImGui::StyleColorsDark();
      }
    }

    conf::BstormConfig config = loadBstormConfig(configFilePath, useBinaryFormat, IDR_HTML1);

    /* message loop */
    while (true) {
      if (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)) {
        BOOL result = GetMessage(&msg, NULL, 0, 0);
        if ((!result) || (!(~result))) {
          // exit or error
          break;
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        continue;
      }

      inputDevice.updateInputState();

      // draw
      if (SUCCEEDED(d3DDevice->BeginScene())) {
        ImGui_ImplDX9_NewFrame();
        d3DDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(114, 144, 154), 1.0f, 0);
        ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiSetCond_Once);
        ImGui::SetNextWindowSize(ImVec2(configWindowWidth, configWindowHeight), ImGuiSetCond_Once);
        ImGuiWindowFlags windowFlag = ImGuiWindowFlags_NoTitleBar |
          ImGuiWindowFlags_NoResize |
          ImGuiWindowFlags_NoMove;
        ImGui::Begin("Config", NULL, windowFlag);
        ImGui::Columns(3, "tabs", false);
        static Tab selectedTab = Tab::KEY;
        if (ImGui::Selectable("Window", selectedTab == Tab::WINDOW)) {
          selectedTab = Tab::WINDOW;
        }
        ImGui::NextColumn();
        if (ImGui::Selectable("Key", selectedTab == Tab::KEY)) {
          selectedTab = Tab::KEY;
        }
        ImGui::NextColumn();
        if (ImGui::Selectable("Option", selectedTab == Tab::OPTION)) {
          selectedTab = Tab::OPTION;
        }
        ImGui::NextColumn();
        ImGui::Columns(1);
        if (selectedTab == Tab::WINDOW) {

        } else if (selectedTab == Tab::KEY) {
          ImGui::BeginChild("KeyConfig");
          conf::KeyConfig& keyConfig = config.keyConfig;
          static int selectedKeyMapIdx = 0;

          // list keymaps
          ImGui::Columns(3, "key maps");
          ImGui::Separator();
          ImGui::Text("Action"); ImGui::NextColumn(); ImGui::Text("Keyboard"); ImGui::NextColumn(); ImGui::Text("Pad"); ImGui::NextColumn();
          ImGui::Separator();
          for (int i = 0; i < keyConfig.keyMaps.size(); i++) {
            const auto& keyMap = keyConfig.keyMaps[i];
            bool validKey = DIKeyNameMap.count(keyMap.key) != 0;
            ImGui::Separator();
            if (ImGui::Selectable((keyMap.actionName + "##" + std::to_string(i)).c_str(), selectedKeyMapIdx == i, ImGuiSelectableFlags_SpanAllColumns)) {
              selectedKeyMapIdx = i;
            }
            ImGui::NextColumn();
            ImGui::Text(validKey ? DIKeyNameMap.at(keyMap.key) : "invalid key");
            ImGui::NextColumn();
            ImGui::Text("%02d", keyMap.pad);
            ImGui::NextColumn();
          }
          ImGui::Columns(1);
          ImGui::Separator();
          ImGui::EndChild();

          // readinput & setting
          if (selectedKeyMapIdx >= 0 && selectedKeyMapIdx < keyConfig.keyMaps.size()) {
            auto& keyMap = keyConfig.keyMaps[selectedKeyMapIdx];
            for (int k = 0; k <= InputDevice::MaxKey; k++) {
              if (inputDevice.getKeyState(k) == KEY_PUSH) {
                keyMap.key = k;
                selectedKeyMapIdx++;
                selectedKeyMapIdx %= keyConfig.keyMaps.size();
                break;
              }
            }

            for (int k = 0; k <= InputDevice::MaxPadButton; k++) {
              if (inputDevice.getPadButtonState(k) == KEY_PUSH) {
                keyMap.pad = k;
                selectedKeyMapIdx++;
                selectedKeyMapIdx %= keyConfig.keyMaps.size();
                break;
              }
            }
          }
        } else if (selectedTab == Tab::OPTION) {

        }
        ImGui::End();
#ifdef _DEBUG
        ImGui::ShowDemoWindow();
        ImGui::ShowMetricsWindow();
        ImGui::ShowStyleEditor();
        ImGui::ShowStyleSelector("style selector");
        ImGui::ShowFontSelector("font selector");
        ImGui::ShowUserGuide();
#endif
        ImGui::Render();
        d3DDevice->EndScene();
        switch (d3DDevice->Present(NULL, NULL, NULL, NULL)) {
          case D3DERR_DEVICELOST:
            if (d3DDevice->TestCooperativeLevel() == D3DERR_DEVICENOTRESET) {
              ImGui_ImplDX9_InvalidateDeviceObjects();
              if (FAILED(d3DDevice->Reset(&presentParams))) {
                PostQuitMessage(0);
              }
              ImGui_ImplDX9_CreateDeviceObjects();
            }
            break;
          case D3DERR_DRIVERINTERNALERROR:
            PostQuitMessage(0);
            break;
        }
      }
    }
    /* save config */
    saveBstormConfig(configFilePath, useBinaryFormat, config);
  } catch (const std::exception& e) {
    MessageBoxA(hWnd, e.what(), "Error", MB_OK);
  }

  // clean
  ImGui_ImplDX9_Shutdown();
  d3DDevice->Release();
  d3D->Release();
  UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
  return msg.wParam;
}