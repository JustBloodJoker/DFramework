#include "pch.h"

#include <System/RectsPacker.h>

RectsPacker::RectsPacker(int width, int height, int padding_) : m_iBinWidth(width), m_iBinHeight(height), m_iPadding(padding_)
{
    m_vFreeRects.clear();
    m_vFreeRects.push_back({ 0,0,m_iBinWidth,m_iBinHeight });
    m_vUsedRects.clear();
}

UpdateResult RectsPacker::SyncRects(std::vector<Rect>& rects) {
    UpdateResult result;

    std::unordered_map<int, Rect> inputMap;
    for (auto& r : rects) {
        inputMap[r.ID] = r;
    }

    for (auto it = m_vUsedRects.begin(); it != m_vUsedRects.end(); ) {
        auto found = inputMap.find(it->ID);
        if (found == inputMap.end()) {
            result.Removed.push_back(*it);
            it = m_vUsedRects.erase(it);
        }
        else {
            ++it;
        }
    }

    for (auto& [id, r] : inputMap) {
        auto it = std::find_if(m_vUsedRects.begin(), m_vUsedRects.end(), [&](const Rect& u) { return u.ID == id; });

        if (it != m_vUsedRects.end()) {
            if (it->W == r.W && it->H == r.H) {
                r.X = it->X;
                r.Y = it->Y;
            }
            else {
                result.Removed.push_back(m_vUsedRects.at(std::distance(m_vUsedRects.begin(), it)));
                m_vUsedRects.erase(it);

                Rect res = InsertBest(r);
                if (res.W > 0) {
                    r.X = res.X;
                    r.Y = res.Y;
                    m_vUsedRects.push_back(res);
                    result.Added.push_back(r);
                }
                else {
                    r.X = -1;
                    r.Y = -1;
                }
            }
        }
        else {
            Rect res = InsertBest(r);
            if (res.W > 0) {
                r.X = res.X;
                r.Y = res.Y;
                m_vUsedRects.push_back(res);
                result.Added.push_back(r);
            }
            else {
                r.X = -1;
                r.Y = -1;
            }
        }
    }

    return result;
}

Rect RectsPacker::InsertBest(const Rect& inRect)
{
    int bestScore1 = std::numeric_limits<int>::max();
    int bestIndex = -1;
    
    Rect bestNode{ 0,0,0,0,-1 };
    int rw = inRect.W + m_iPadding * 2;
    int rh = inRect.H + m_iPadding * 2;

    for (auto i = 0; i < m_vFreeRects.size(); ++i) {
        const FreeRect& fr = m_vFreeRects[i];
        if (fr.W < rw || fr.H < rh) continue;

        int leftoverHoriz = abs(fr.W - rw);
        int leftoverVert = abs(fr.H - rh);
        int score = std::min(leftoverHoriz, leftoverVert);
        if (score < bestScore1) {
            bestScore1 = score;
            bestIndex = (int)i;
            bestNode = { fr.X + m_iPadding, fr.Y + m_iPadding, inRect.W, inRect.H, inRect.ID };
        }
    }

    if (bestIndex == -1) return Rect{ 0,0,0,0,-1 };

    FreeRect usedFree = m_vFreeRects[bestIndex];
   
    SplitFreeNode(usedFree, bestNode);
   
    m_vFreeRects.erase(m_vFreeRects.begin() + bestIndex);
   
    PruneFreeList();
    
    return bestNode;
}

void RectsPacker::SplitFreeNode(const FreeRect& freeNode, const Rect& usedNode)
{
    int u1 = usedNode.X - m_iPadding;
    int top = usedNode.Y - m_iPadding;
    int u2 = usedNode.X + usedNode.W + m_iPadding;
    int bottom = usedNode.Y + usedNode.H + m_iPadding;

    if (u1 >= freeNode.X + freeNode.W || u2 <= freeNode.X || top >= freeNode.Y + freeNode.H || bottom <= freeNode.Y) {
        m_vFreeRects.push_back(freeNode);
        return;
    }

    if (u1 > freeNode.X && u1 < freeNode.X + freeNode.W) {
        m_vFreeRects.push_back({ freeNode.X, freeNode.Y, u1 - freeNode.X, freeNode.H });
    }
    if (u2 > freeNode.X && u2 < freeNode.X + freeNode.W) {
        m_vFreeRects.push_back({ u2, freeNode.Y, freeNode.X + freeNode.W - u2, freeNode.H });
    }
    if (top > freeNode.Y && top < freeNode.Y + freeNode.H) {
        m_vFreeRects.push_back({ freeNode.X, freeNode.Y, freeNode.W, top - freeNode.Y });
    }
    if (bottom > freeNode.Y && bottom < freeNode.Y + freeNode.H) {
        m_vFreeRects.push_back({ freeNode.X, bottom, freeNode.W, freeNode.Y + freeNode.H - bottom });
    }
}

void RectsPacker::PruneFreeList()
{
    for (size_t i = 0; i < m_vFreeRects.size(); ++i) {
        for (size_t j = i + 1; j < m_vFreeRects.size();) {
            if (IsContainedIn(m_vFreeRects[i], m_vFreeRects[j])) {
                m_vFreeRects.erase(m_vFreeRects.begin() + i);
                --i;
                break;
            }
            else if (IsContainedIn(m_vFreeRects[j], m_vFreeRects[i])) {
                m_vFreeRects.erase(m_vFreeRects.begin() + j);
            }
            else ++j;
        }
    }
}

bool RectsPacker::IsContainedIn(const FreeRect& a, const FreeRect& b) const
{
    return a.X >= b.X && a.Y >= b.Y && a.X + a.W <= b.X + b.W && a.Y + a.H <= b.Y + b.H;
}

Rect::Rect(int _x, int _y, int _w, int _h, int _id) :X(_x), Y(_y), W(_w), H(_h), ID(_id){}
