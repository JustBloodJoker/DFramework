#define PI 3.14159265359

void AlphaClipping(float alpha)
{
    clip(alpha < 0.1f ? -1.0f : 1.0f);
}

/// IEC 61966-2-1
float3 ToSRGB(float3 x) {
    float3 result;
    result.r = (x.r <= 0.0031308) ? (x.r * 12.92) : (1.055 * pow(x.r, 1.0 / 2.4) - 0.055);
    result.g = (x.g <= 0.0031308) ? (x.g * 12.92) : (1.055 * pow(x.g, 1.0 / 2.4) - 0.055);
    result.b = (x.b <= 0.0031308) ? (x.b * 12.92) : (1.055 * pow(x.b, 1.0 / 2.4) - 0.055);
    return result;
}

float3 ToLinear(float3 sRGB) {
    float3 result;
    result.r = (sRGB.r <= 0.04045) ? sRGB.r / 12.92 : pow((sRGB.r + 0.055) / 1.055, 2.4);
    result.g = (sRGB.g <= 0.04045) ? sRGB.g / 12.92 : pow((sRGB.g + 0.055) / 1.055, 2.4);
    result.b = (sRGB.b <= 0.04045) ? sRGB.b / 12.92 : pow((sRGB.b + 0.055) / 1.055, 2.4);
    return result;
}
////////////////////

float3 GammaCorrection(float3 rgb, float gamma) {
    float igm = 1.0/gamma;
    return pow(rgb, float3(igm,igm,igm));
}

bool IsZeroMatrix(matrix m)
{
    return all(m[0] == 0) &&
           all(m[1] == 0) &&
           all(m[2] == 0) &&
           all(m[3] == 0);
}

matrix MakeZeroMatrix()
{
    return matrix(
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0
    );
}

matrix MakeIdentityMatrix()
{
    return matrix(
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    );
}