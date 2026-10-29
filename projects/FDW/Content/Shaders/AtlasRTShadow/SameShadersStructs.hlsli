// ShadowPayload.hlsl
struct ShadowPayload {
    float Visible; // 1.0 - not occluded, 0.0 - occluded
    float BlockerDistance; // world-space first-hit T, or the light distance on miss
};
