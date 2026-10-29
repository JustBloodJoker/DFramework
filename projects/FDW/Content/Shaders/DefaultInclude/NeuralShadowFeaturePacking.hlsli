#ifndef NEURAL_SHADOW_FEATURE_PACKING_HLSLI
#define NEURAL_SHADOW_FEATURE_PACKING_HLSLI

static const uint NS_FEATURE_PATCH_WIDTH = 16u;
static const uint NS_FEATURE_PATCH_HEIGHT = 16u;
static const uint NS_FEATURE_PIXELS_PER_PATCH = 256u;
static const uint NS_FEATURE_PIXEL_CHANNEL_COUNT = 10u;
static const uint NS_FEATURE_LAYOUT_VERSION = 2u;
static const uint NS_FEATURE_LIGHT_CHANNEL_COUNT = 14u;
static const float NS_FEATURE_MAX_DIRECTIONAL_ANGULAR_RADIUS = 0.5f;
static const float NS_FEATURE_MAX_SHADOW_SOFTNESS = 4.0f;
static const uint NS_FEATURE_TENSOR_LAYOUT_NCHW = 1u;
static const uint NS_FEATURE_STORAGE_IEEE_FLOAT16 = 1u;
static const float NS_FEATURE_NORMALIZATION_EPSILON = 1.0e-6f;
static const float NS_FEATURE_CPU_HLSL_TOLERANCE = 1.0e-5f;
static const float NS_FEATURE_FP16_STORAGE_TOLERANCE = 1.0e-3f;

static const uint NS_FEATURE_PIXEL_HARD_VISIBILITY = 0u;
static const uint NS_FEATURE_PIXEL_RECEIVER_LOG_DEPTH = 1u;
static const uint NS_FEATURE_PIXEL_RECEIVER_NORMAL_X = 2u;
static const uint NS_FEATURE_PIXEL_RECEIVER_NORMAL_Y = 3u;
static const uint NS_FEATURE_PIXEL_RECEIVER_NORMAL_Z = 4u;
static const uint NS_FEATURE_PIXEL_DIRECTION_TO_LIGHT_X = 5u;
static const uint NS_FEATURE_PIXEL_DIRECTION_TO_LIGHT_Y = 6u;
static const uint NS_FEATURE_PIXEL_DIRECTION_TO_LIGHT_Z = 7u;
static const uint NS_FEATURE_PIXEL_NORMALIZED_LIGHT_DISTANCE = 8u;
static const uint NS_FEATURE_PIXEL_RECEIVER_VALID = 9u;

static const uint NS_FEATURE_LIGHT_TYPE_POINT = 0u;
static const uint NS_FEATURE_LIGHT_TYPE_SPOT = 1u;
static const uint NS_FEATURE_LIGHT_TYPE_DIRECTIONAL = 2u;
static const uint NS_FEATURE_LIGHT_TYPE_RECTANGLE = 3u;
static const uint NS_FEATURE_LIGHT_ATTENUATION_OVER_CAMERA_FAR = 4u;
static const uint NS_FEATURE_LIGHT_POINT_RADIUS_OVER_ATTENUATION = 5u;
static const uint NS_FEATURE_LIGHT_RECT_WIDTH_OVER_ATTENUATION = 6u;
static const uint NS_FEATURE_LIGHT_RECT_HEIGHT_OVER_ATTENUATION = 7u;
static const uint NS_FEATURE_LIGHT_RECT_ROTATION_QUATERNION_X = 8u;
static const uint NS_FEATURE_LIGHT_RECT_ROTATION_QUATERNION_Y = 9u;
static const uint NS_FEATURE_LIGHT_RECT_ROTATION_QUATERNION_Z = 10u;
static const uint NS_FEATURE_LIGHT_RECT_ROTATION_QUATERNION_W = 11u;
static const uint NS_FEATURE_LIGHT_DIRECTIONAL_ANGULAR_RADIUS = 12u;
static const uint NS_FEATURE_LIGHT_SHADOW_SOFTNESS = 13u;

static const int NS_FEATURE_LIGHT_TYPE_POINT_ENUM = 0;
static const int NS_FEATURE_LIGHT_TYPE_SPOT_ENUM = 1;
static const int NS_FEATURE_LIGHT_TYPE_DIRECTIONAL_ENUM = 2;
static const int NS_FEATURE_LIGHT_TYPE_RECTANGLE_ENUM = 3;

struct NSPixelFeatureInput
{
    float HardVisibility;
    float ReceiverLinearDepth;
    float CameraNear;
    float CameraFar;
    float3 ReceiverNormal;
    float3 ReceiverWorldPosition;
    uint ReceiverValid;
};

struct NSLightFeatureInput
{
    int LightType;
    float3 Position;
    float AttenuationRadius;
    float SourceRadius;
    float3 Direction;
    float RectangleWidth;
    float RectangleHeight;
    float3 Rotation;
    float DirectionalAngularRadius;
    float ShadowSoftness;
};

struct NSPixelFeatures
{
    float Values[NS_FEATURE_PIXEL_CHANNEL_COUNT];
};

struct NSLightFeatures
{
    float Values[NS_FEATURE_LIGHT_CHANNEL_COUNT];
};

bool NSFeatureIsFinite3(float3 value)
{
    return all(isfinite(value));
}

bool NSFeatureTryNormalize(float3 value, out float3 result)
{
    float lengthSquared = dot(value, value);
    if (!isfinite(lengthSquared) || lengthSquared <= NS_FEATURE_NORMALIZATION_EPSILON * NS_FEATURE_NORMALIZATION_EPSILON)
    {
        result = 0.0f;
        return false;
    }
    result = value * rsqrt(lengthSquared);
    return NSFeatureIsFinite3(result);
}

float NSFeatureNormalizeReceiverDepth(float linearDepth, float cameraNear, float cameraFar)
{
    if (!isfinite(linearDepth) || !isfinite(cameraNear) || !isfinite(cameraFar) ||
        cameraNear <= NS_FEATURE_NORMALIZATION_EPSILON ||
        cameraFar <= cameraNear + NS_FEATURE_NORMALIZATION_EPSILON)
    {
        return 0.0f;
    }
    float clampedDepth = clamp(linearDepth, cameraNear, cameraFar);
    return saturate(log2(clampedDepth / cameraNear) / log2(cameraFar / cameraNear));
}

float4 NSFeatureMultiplyQuaternion(float4 left, float4 right)
{
    return float4(
        left.w * right.x + left.x * right.w + left.y * right.z - left.z * right.y,
        left.w * right.y - left.x * right.z + left.y * right.w + left.z * right.x,
        left.w * right.z + left.x * right.y - left.y * right.x + left.z * right.w,
        left.w * right.w - left.x * right.x - left.y * right.y - left.z * right.z);
}

float4 NSFeatureRectangleRotationQuaternion(float3 rotation)
{
    if (!NSFeatureIsFinite3(rotation)) {
        return 0.0f;
    }

    
    float3 halfRotation = rotation * 0.5f;
    
    float4 pitch = float4(sin(halfRotation.x), 0.0f, 0.0f, cos(halfRotation.x));
    
    float4 yaw = float4(0.0f, sin(halfRotation.y), 0.0f, cos(halfRotation.y));
    
    float4 roll = float4(0.0f, 0.0f, sin(halfRotation.z), cos(halfRotation.z));
    
    float4 quaternion = NSFeatureMultiplyQuaternion(yaw, NSFeatureMultiplyQuaternion(roll, pitch));
    
    float lengthSquared = dot(quaternion, quaternion);
    if (!isfinite(lengthSquared) || lengthSquared <= NS_FEATURE_NORMALIZATION_EPSILON * NS_FEATURE_NORMALIZATION_EPSILON) {
        return 0.0f;
    }
    
    quaternion *= rsqrt(lengthSquared);
    return quaternion.w < 0.0f ? -quaternion : quaternion;
}

bool NSFeatureIsSupportedLightType(int lightType)
{
    return lightType >= NS_FEATURE_LIGHT_TYPE_POINT_ENUM &&
        lightType <= NS_FEATURE_LIGHT_TYPE_RECTANGLE_ENUM;
}

NSPixelFeatures NSFeatureExtractPixel(NSPixelFeatureInput pixel, NSLightFeatureInput light)
{
    NSPixelFeatures output = (NSPixelFeatures)0;
    if (pixel.ReceiverValid == 0u || !NSFeatureIsSupportedLightType(light.LightType) ||
        !isfinite(pixel.HardVisibility) || !isfinite(pixel.ReceiverLinearDepth) ||
        !isfinite(pixel.CameraNear) || !isfinite(pixel.CameraFar) ||
        pixel.CameraNear <= NS_FEATURE_NORMALIZATION_EPSILON ||
        pixel.CameraFar <= pixel.CameraNear + NS_FEATURE_NORMALIZATION_EPSILON ||
        !NSFeatureIsFinite3(pixel.ReceiverWorldPosition)) {
        return output;
    }

    float3 receiverNormal;
    if (!NSFeatureTryNormalize(pixel.ReceiverNormal, receiverNormal)) {
        return output;
    }

    float3 directionToLight;
    float normalizedDistance = 0.0f;
    if (light.LightType == NS_FEATURE_LIGHT_TYPE_DIRECTIONAL_ENUM) {
        if (!NSFeatureTryNormalize(-light.Direction, directionToLight)) {
            return output;
        }
    
        normalizedDistance = 1.0f;
    }
    else {
        if (!NSFeatureIsFinite3(light.Position) || !isfinite(light.AttenuationRadius) ||
            light.AttenuationRadius <= NS_FEATURE_NORMALIZATION_EPSILON) {
            return output;
        }
    
        float3 toLight = light.Position - pixel.ReceiverWorldPosition;
        float distanceSquared = dot(toLight, toLight);
        if (!isfinite(distanceSquared) ||
            distanceSquared <= NS_FEATURE_NORMALIZATION_EPSILON * NS_FEATURE_NORMALIZATION_EPSILON) {
            return output;
        }
        
        float distanceToLight = sqrt(distanceSquared);
        directionToLight = toLight / distanceToLight;
        normalizedDistance = saturate(distanceToLight / light.AttenuationRadius);
    }

    float depth = NSFeatureNormalizeReceiverDepth(pixel.ReceiverLinearDepth, pixel.CameraNear, pixel.CameraFar);
    output.Values[NS_FEATURE_PIXEL_HARD_VISIBILITY] = saturate(pixel.HardVisibility);
    output.Values[NS_FEATURE_PIXEL_RECEIVER_LOG_DEPTH] = depth;
    output.Values[NS_FEATURE_PIXEL_RECEIVER_NORMAL_X] = receiverNormal.x;
    output.Values[NS_FEATURE_PIXEL_RECEIVER_NORMAL_Y] = receiverNormal.y;
    output.Values[NS_FEATURE_PIXEL_RECEIVER_NORMAL_Z] = receiverNormal.z;
    output.Values[NS_FEATURE_PIXEL_DIRECTION_TO_LIGHT_X] = directionToLight.x;
    output.Values[NS_FEATURE_PIXEL_DIRECTION_TO_LIGHT_Y] = directionToLight.y;
    output.Values[NS_FEATURE_PIXEL_DIRECTION_TO_LIGHT_Z] = directionToLight.z;
    output.Values[NS_FEATURE_PIXEL_NORMALIZED_LIGHT_DISTANCE] = normalizedDistance;
    output.Values[NS_FEATURE_PIXEL_RECEIVER_VALID] = 1.0f;
    
    return output;
}

NSLightFeatures NSFeatureExtractLight(NSLightFeatureInput light, float cameraFar)
{
    NSLightFeatures output = (NSLightFeatures)0;
    if (!NSFeatureIsSupportedLightType(light.LightType) || !isfinite(cameraFar) ||
        cameraFar <= NS_FEATURE_NORMALIZATION_EPSILON) {
        return output;
    }
    
    output.Values[NS_FEATURE_LIGHT_TYPE_POINT + (uint)light.LightType] = 1.0f;

    if (light.LightType != NS_FEATURE_LIGHT_TYPE_DIRECTIONAL_ENUM &&
        isfinite(light.AttenuationRadius) && light.AttenuationRadius > NS_FEATURE_NORMALIZATION_EPSILON) {
        float range = light.AttenuationRadius;
        output.Values[NS_FEATURE_LIGHT_ATTENUATION_OVER_CAMERA_FAR] = saturate(range / cameraFar);
        if ((light.LightType == NS_FEATURE_LIGHT_TYPE_POINT_ENUM ||
            light.LightType == NS_FEATURE_LIGHT_TYPE_SPOT_ENUM) && isfinite(light.SourceRadius)) {
            output.Values[NS_FEATURE_LIGHT_POINT_RADIUS_OVER_ATTENUATION] =
                saturate(max(light.SourceRadius, 0.0f) / range);
        }
      
        if (light.LightType == NS_FEATURE_LIGHT_TYPE_RECTANGLE_ENUM) {
            if (isfinite(light.RectangleWidth)) {
                output.Values[NS_FEATURE_LIGHT_RECT_WIDTH_OVER_ATTENUATION] = saturate(max(light.RectangleWidth, 0.0f) / range);
            }
      
            if (isfinite(light.RectangleHeight)) {
                output.Values[NS_FEATURE_LIGHT_RECT_HEIGHT_OVER_ATTENUATION] = saturate(max(light.RectangleHeight, 0.0f) / range);
            }
        }
    }

    if (light.LightType == NS_FEATURE_LIGHT_TYPE_RECTANGLE_ENUM) {
        float4 quaternion = NSFeatureRectangleRotationQuaternion(light.Rotation);
        output.Values[NS_FEATURE_LIGHT_RECT_ROTATION_QUATERNION_X] = quaternion.x;
        output.Values[NS_FEATURE_LIGHT_RECT_ROTATION_QUATERNION_Y] = quaternion.y;
        output.Values[NS_FEATURE_LIGHT_RECT_ROTATION_QUATERNION_Z] = quaternion.z;
        output.Values[NS_FEATURE_LIGHT_RECT_ROTATION_QUATERNION_W] = quaternion.w;
    }
    if (light.LightType == NS_FEATURE_LIGHT_TYPE_DIRECTIONAL_ENUM && isfinite(light.DirectionalAngularRadius)) {
        output.Values[NS_FEATURE_LIGHT_DIRECTIONAL_ANGULAR_RADIUS] =
            saturate(max(light.DirectionalAngularRadius, 0.0f) / NS_FEATURE_MAX_DIRECTIONAL_ANGULAR_RADIUS);
    }
    
    float softness = isfinite(light.ShadowSoftness) ? light.ShadowSoftness : 1.0f;
    
    output.Values[NS_FEATURE_LIGHT_SHADOW_SOFTNESS] =
        saturate(max(softness, 0.0f) / NS_FEATURE_MAX_SHADOW_SOFTNESS);
    
    return output;
}

uint NSFeatureComputePixelTensorOffset(uint batchIndex, uint channel, uint2 localPixel)
{
    return ((batchIndex * NS_FEATURE_PIXEL_CHANNEL_COUNT + channel) * NS_FEATURE_PATCH_HEIGHT + localPixel.y) *
        NS_FEATURE_PATCH_WIDTH + localPixel.x;
}

uint NSFeatureComputeLightTensorOffset(uint batchIndex, uint channel)
{
    return batchIndex * NS_FEATURE_LIGHT_CHANNEL_COUNT + channel;
}

#endif
