#pragma once
#include "pch.h"
#include "D2DFWStandalone.h"
#include "UI/Core/UIElementBase.h"
#include "UI/Core/UIElementBaseWithInput.h"

namespace FD2DW {

// Currently using D2DFWStandalone for testing.
// In the future, inherit from D2DFWDXGI to enable Direct3D integration.

    class D2DFWUI : public D2DFWStandalone {
    public:
        D2DFWUI() = default;
        virtual ~D2DFWUI() = default;

        void AddUIElement(std::shared_ptr<UIElementBase> element);

    protected:
        void UserD2DLoop() override;

        void ProcessInput(UINT msg, WPARAM wParam, LPARAM lParam);

        void ChildMOUSEUP(WPARAM btnState, int x, int y) override;
        void ChildMOUSEDOWN(WPARAM btnState, int x, int y) override;
        void ChildMOUSEMOVE(WPARAM btnState, int x, int y) override;
        void ChildKeyPressed(WPARAM key) override;

        virtual void UserAfterD2DInit() override = 0;
        virtual void UserD2DClose() override {}

    private:
        std::vector<std::shared_ptr<UIElementBase>> m_vUIElements;
    };

} // namespace FD2DW 