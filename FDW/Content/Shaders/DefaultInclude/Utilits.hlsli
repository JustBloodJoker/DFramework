#ifndef _UTILITS_HLSLI_
#define _UTILITS_HLSLI_

void AlphaClipping(float alpha)
{
    clip(alpha < 0.1f ? -1.0f : 1.0f);
}

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

float4x4 Inverse(float4x4 m) {
    float n11 = m[0][0], n12 = m[1][0], n13 = m[2][0], n14 = m[3][0];
    float n21 = m[0][1], n22 = m[1][1], n23 = m[2][1], n24 = m[3][1];
    float n31 = m[0][2], n32 = m[1][2], n33 = m[2][2], n34 = m[3][2];
    float n41 = m[0][3], n42 = m[1][3], n43 = m[2][3], n44 = m[3][3];

    float t11 = n23 * n34 * n42 - n24 * n33 * n42 + n24 * n32 * n43 - n22 * n34 * n43 - n23 * n32 * n44 + n22 * n33 * n44;
    float t12 = n14 * n33 * n42 - n13 * n34 * n42 - n14 * n32 * n43 + n12 * n34 * n43 + n13 * n32 * n44 - n12 * n33 * n44;
    float t13 = n13 * n24 * n42 - n14 * n23 * n42 + n14 * n22 * n43 - n12 * n24 * n43 - n13 * n22 * n44 + n12 * n23 * n44;
    float t14 = n14 * n23 * n32 - n13 * n24 * n32 - n14 * n22 * n33 + n12 * n24 * n33 + n13 * n22 * n34 - n12 * n23 * n34;

    float det = n11 * t11 + n21 * t12 + n31 * t13 + n41 * t14;
    float idet = 1.0f / det;

    float4x4 ret;

    ret[0][0] = t11 * idet;
    ret[0][1] = (n24 * n33 * n41 - n23 * n34 * n41 - n24 * n31 * n43 + n21 * n34 * n43 + n23 * n31 * n44 - n21 * n33 * n44) * idet;
    ret[0][2] = (n22 * n34 * n41 - n24 * n32 * n41 + n24 * n31 * n42 - n21 * n34 * n42 - n22 * n31 * n44 + n21 * n32 * n44) * idet;
    ret[0][3] = (n23 * n32 * n41 - n22 * n33 * n41 - n23 * n31 * n42 + n21 * n33 * n42 + n22 * n31 * n43 - n21 * n32 * n43) * idet;

    ret[1][0] = t12 * idet;
    ret[1][1] = (n13 * n34 * n41 - n14 * n33 * n41 + n14 * n31 * n43 - n11 * n34 * n43 - n13 * n31 * n44 + n11 * n33 * n44) * idet;
    ret[1][2] = (n14 * n32 * n41 - n12 * n34 * n41 - n14 * n31 * n42 + n11 * n34 * n42 + n12 * n31 * n44 - n11 * n32 * n44) * idet;
    ret[1][3] = (n12 * n33 * n41 - n13 * n32 * n41 + n13 * n31 * n42 - n11 * n33 * n42 - n12 * n31 * n43 + n11 * n32 * n43) * idet;

    ret[2][0] = t13 * idet;
    ret[2][1] = (n14 * n23 * n41 - n13 * n24 * n41 - n14 * n21 * n43 + n11 * n24 * n43 + n13 * n21 * n44 - n11 * n23 * n44) * idet;
    ret[2][2] = (n12 * n24 * n41 - n14 * n22 * n41 + n14 * n21 * n42 - n11 * n24 * n42 - n12 * n21 * n44 + n11 * n22 * n44) * idet;
    ret[2][3] = (n13 * n22 * n41 - n12 * n23 * n41 - n13 * n21 * n42 + n11 * n23 * n42 + n12 * n21 * n43 - n11 * n22 * n43) * idet;

    ret[3][0] = t14 * idet;
    ret[3][1] = (n13 * n24 * n31 - n14 * n23 * n31 + n14 * n21 * n33 - n11 * n24 * n33 - n13 * n21 * n34 + n11 * n23 * n34) * idet;
    ret[3][2] = (n14 * n22 * n31 - n12 * n24 * n31 - n14 * n21 * n32 + n11 * n24 * n32 + n12 * n21 * n34 - n11 * n22 * n34) * idet;
    ret[3][3] = (n12 * n23 * n31 - n13 * n22 * n31 + n13 * n21 * n32 - n11 * n23 * n32 - n12 * n21 * n33 + n11 * n22 * n33) * idet;

    return ret;
}


/////////////////////////////////////////////////////////////////////////////
//EULER ROTATIONS
float3 RotateYaw(float3 axis, float angle)
{
    float3 r;
    r.x = axis.x * cos(angle) + axis.z * sin(angle);
    r.y = axis.y;
    r.z = -axis.x * sin(angle) + axis.z * cos(angle);
    return r;
}

float3 RotateRoll(float3 axis, float angle)
{
    float3 r;
    r.x = axis.x * cos(angle) - axis.y * sin(angle);
    r.y = axis.x * sin(angle) + axis.y * cos(angle);
    r.z = axis.z;
    return r;
}

float3 RotatePitch(float3 axis, float angle)
{
    float3 r;
    r.x = axis.x;
    r.y = cos(angle) * axis.y - axis.z * sin(angle);
    r.z = sin(angle) * axis.y + cos(angle) * axis.z;
    return r;
}

// Not recommended: causes Gimbal Lock due to sequential Euler rotations.
float3 RotateAxis(float3 axis, float pitch, float yaw, float roll)
{
    return RotatePitch(RotateRoll(RotateYaw(axis, yaw), roll), pitch);
}
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//QUAT ROTATIONS
float4 AxisAngleQuat(float3 axis, float angle)
{
    float halfAngle = angle * 0.5;
    float s = sin(halfAngle);
    return float4(axis * s, cos(halfAngle));
}

float4 MulQuat(float4 q, float4 r)
{
    return float4(
        q.w * r.x + q.x * r.w + q.y * r.z - q.z * r.y,
        q.w * r.y - q.x * r.z + q.y * r.w + q.z * r.x,
        q.w * r.z + q.x * r.y - q.y * r.x + q.z * r.w,
        q.w * r.w - q.x * r.x - q.y * r.y - q.z * r.z
    );
}

float3 RotateByQuat(float3 v, float4 q)
{
    float3 t = 2.0 * cross(q.xyz, v);
    return v + q.w * t + cross(q.xyz, t);
}

float3 RotateAxisQuat(float3 v, float pitch, float yaw, float roll)
{
    float4 qPitch = AxisAngleQuat(float3(1, 0, 0), pitch);
    float4 qYaw   = AxisAngleQuat(float3(0, 1, 0), yaw);
    float4 qRoll  = AxisAngleQuat(float3(0, 0, 1), roll);
    float4 q = MulQuat(qYaw, MulQuat(qRoll, qPitch));
    return RotateByQuat(v, q);
}
/////////////////////////////////////////////////////////////////////////////

#endif