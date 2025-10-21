#pragma once
#include <pch.h>

struct Rect {
    int X;
    int Y;
    int W;
    int H;
    int ID;
    Rect() = default;
    Rect(int _x, int _y, int _w = 0, int _h = 0, int _id = -1);
};

struct UpdateResult {
    std::vector<Rect> Added;
    std::vector<Rect> Removed;
};

class RectsPacker {
public:
    RectsPacker(int width, int height, int padding = 1);
    int InsertRects(std::vector<Rect>& rects); 
    UpdateResult SyncRects(std::vector<Rect>& rects);
    int GetUsedWidth();
    int GetUsedHeight();

protected:
    struct FreeRect {
        int X;
        int Y;
        int W;
        int H;
    };
    
    Rect InsertBest(const Rect& r);
    void SplitFreeNode(const FreeRect& freeNode, const Rect& usedNode);
    void PruneFreeList();
    bool IsContainedIn(const FreeRect& a, const FreeRect& b) const;

private:
    int m_iBinWidth;
    int m_iBinHeight;
    int m_iPadding;
    std::vector<FreeRect> m_vFreeRects;
    std::vector<Rect> m_vUsedRects;
    int m_iUsedWidth;
    int m_iUsedHeight;

};
