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
        for (auto& layer : m_vInputLayers) {
            if (layer->ProcessInput(hwnd, msg, wParam, lParam))
                return true;
        }
        return false;
    }

}


