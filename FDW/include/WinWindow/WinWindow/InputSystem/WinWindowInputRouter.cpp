#include "WinWindowInputRouter.h"

namespace FDWWIN {

    void WinWindowInputRouter::AddLayer(WinWindowInputLayer* layer) {
        layer->AddToRouter(this);
        m_vInputLayers.push_back(layer);
    }

    void WinWindowInputRouter::RemoveLayer(WinWindowInputLayer* layer)
    {
        auto it = std::find_if(m_vInputLayers.begin(), m_vInputLayers.end(),
            [layer](WinWindowInputLayer* ptr) {
                return ptr == layer;
            });

        if (it != m_vInputLayers.end()) 
        {
            m_vInputLayers.erase(it);
        }
    }
    
    bool WinWindowInputRouter::RouteInput(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        bool handled = false;

        for (auto& layer : m_vInputLayers) {
            if (layer->ProcessInput(hwnd, msg, wParam, lParam)) {
                handled = true;
                break;
            }
        }

       
            switch (msg) {

            if (!handled) {
                case WM_KEYDOWN:
                    if (!(lParam & (1 << 30))) m_aKeyStates[wParam] = true;
                break;
            }

            case WM_KEYUP:
                m_aKeyStates[wParam] = false;
                break;
            }
        

        return handled;
    }

    void WinWindowInputRouter::PreTickUpdate(float DT) {
        for (auto& layer : m_vInputLayers) {
            layer->PreTickUpdate(DT);
        }
    }

    bool WinWindowInputRouter::IsKeyDown(uint8_t key) const
    {
        return m_aKeyStates[key];
    }

}


