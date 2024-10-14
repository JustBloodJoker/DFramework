void AlphaClipping(float alpha)
{
    clip(alpha < 0.1f ? -1.0f : 1.0f);
}