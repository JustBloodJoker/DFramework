#pragma once
#include "pch.h"
#include "UI/Core/UIElementBaseWithInput.h"

namespace FD2DW {

    class UIComboBox : public UIElementBaseWithInput {
    public:
        UIComboBox(std::shared_ptr<BaseShape> shape);
        virtual ~UIComboBox() = default;

        virtual void Draw(ID2D1RenderTarget* pRenderTarget, ID2D1SolidColorBrush* brush) override;

        virtual void SetPosition(float x, float y) override;
        virtual void SetSize(float width, float height) override;
        
        void AddItem(std::shared_ptr<BaseShape> itemShape);
        
        void SetSelectedIndex(size_t index);
        size_t GetSelectedIndex() const;
        
        void SetOnSelect(std::function<void(size_t)> handler);
        
        virtual bool OnInput(UINT msg, WPARAM wParam, LPARAM lParam) override;
        
        void SetDropOffset(float x, float y);
        
        bool IsOpen() const;
        void Close();

    protected:
        std::vector<std::shared_ptr<BaseShape>> m_vItems;

        size_t m_uSelectedIndex = 0;
        
        std::function<void(size_t)> m_hOnSelect;
        
        bool m_bIsOpen = false;
        
        float m_fDropOffsetX = 0.0f;
        float m_fDropOffsetY = 0.0f;
    };

}
