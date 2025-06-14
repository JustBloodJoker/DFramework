#pragma once
#include "pch.h"
#include "UI/Core/UIElementBase.h"
#include "UI/Core/UIElementBaseWithInput.h"

namespace FD2DW {

    class D2DFW_UIController {
    public:
        D2DFW_UIController() = default;
        virtual ~D2DFW_UIController() = default;

        void AddUIElement(std::shared_ptr<UIElementBase> element);

    protected:

        void D2DUILoop();
        void ProcessInput(UINT msg, WPARAM wParam, LPARAM lParam);
        virtual void UserUICreateFunction() = 0;
        
    protected:
        virtual ID2D1RenderTarget* GetRenderTarget() const = 0;
        virtual ID2D1SolidColorBrush* GetBrush() const = 0;

    private:
        std::vector<std::shared_ptr<UIElementBase>> m_vUIElements;
    };

} // namespace FD2DW 