#pragma once
#include "pch.h"
#include "UI/Core/UIElementBase.h"

namespace FD2DW {

    class UIElementBaseWithInput : public UIElementBase {

    public:
        UIElementBaseWithInput() = default;
        virtual ~UIElementBaseWithInput() = default;

    public:
        virtual bool OnInput(UINT msg, WPARAM wParam, LPARAM lParam) = 0;
    
    };

}