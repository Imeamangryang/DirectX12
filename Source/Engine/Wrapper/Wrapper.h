#pragma once
#include "Common/Common.h"

#include "Window/MainWindow.h"
#include "Render/Renderer.h"

namespace Engine
{
    /*
        Renderer와 window를 관리하는 wrapper클래스
    */
    class Wrapper final
    {
    public:
        Wrapper(_In_ PCWSTR pszWrapperName, _In_ INT nWidth, _In_ INT nHeight);
        Wrapper(const Wrapper& other) = delete;
        Wrapper(Wrapper&& other) = delete;
        Wrapper& operator=(const Wrapper& other) = delete;
        Wrapper& operator=(Wrapper&& other) = delete;
        ~Wrapper() = default;

        HRESULT Initialize(_In_ HINSTANCE hInstance, _In_ INT nCmdShow);
        INT Run();

        PCWSTR GetWrapperName() const;
        std::unique_ptr<MainWindow>& GetWindow();
    private:
        PCWSTR m_pszWrapperName;
        std::unique_ptr<MainWindow> m_mainWindow;
        std::unique_ptr<Renderer> m_renderer;
    };
}