// ImGui Win32 + DirectX10 binding
// In this binding, ImTextureID is used to store a 'ID3D10ShaderResourceView*' texture identifier. Read the FAQ about ImTextureID in imgui.cpp.

// You can copy and use unmodified imgui_impl_* files in your project. See main.cpp for an example of using this.
// If you use this binding you'll need to call 4 functions: ImGui_ImplXXXX_Init(), ImGui_ImplXXXX_NewFrame(), ImGui::Render() and ImGui_ImplXXXX_Shutdown().
// If you are new to ImGui, see examples/README.txt and documentation at the top of imgui.cpp.
// https://github.com/ocornut/imgui

struct ID3D10Device;

bool        ImGui_ImplDX10_Init(void* hwnd, ID3D10Device* device);
void        ImGui_ImplDX10_Shutdown();
void        ImGui_ImplDX10_NewFrame();

// Use if you want to reset your rendering device without losing ImGui state.
void        ImGui_ImplDX10_InvalidateDeviceObjects();
bool        ImGui_ImplDX10_CreateDeviceObjects();

// Handler for Win32 messages, update mouse/keyboard data.
// You may or not need this for your implementation, but it can serve as reference for handling inputs.
// Commented out to avoid dragging dependencies on <windows.h> types. You can copy the extern declaration in your code.
/*
IMGUI_API LRESULT   ImGui_ImplDX10_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
*/