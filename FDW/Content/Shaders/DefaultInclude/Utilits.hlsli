void AlphaClipping(float alpha)
{
    clip(alpha < 0.1f ? -1.0f : 1.0f);
}

bool IsZeroMatrix(matrix m)
{
    return all(m[0] == 0) &&
           all(m[1] == 0) &&
           all(m[2] == 0) &&
           all(m[3] == 0);
}