#pragma once
#include "../../pch.h"

class ScriptValue {
public:
    std::variant<int, float, std::string, void*> Data;

    ScriptValue();
    ScriptValue(int v);
    ScriptValue(float v);
    ScriptValue(std::string v);
    ScriptValue(void* v);
    ScriptValue(const char* v);
    
    template<typename T, typename = std::enable_if_t<std::is_arithmetic_v<T> && !std::is_same_v<T, int> && !std::is_same_v<T, float>>>
    ScriptValue(T v) {
        if constexpr (std::is_floating_point_v<T>) {
            Data = static_cast<float>(v);
        } else {
            Data = static_cast<int>(v);
        }
    }

    bool IsInt() const;
    bool IsFloat() const;
    bool IsString() const;
    bool IsObject() const;

    int AsInt() const;
    float AsFloat() const;
    std::string AsString() const;
    void* AsObject() const;
    bool AsBool() const;

    ScriptValue operator+(const ScriptValue& other) const;
    ScriptValue operator-(const ScriptValue& other) const;
    ScriptValue operator*(const ScriptValue& other) const;
    ScriptValue operator/(const ScriptValue& other) const;
    
    bool operator==(const ScriptValue& other) const;
    bool operator>(const ScriptValue& other) const;
    bool operator<(const ScriptValue& other) const;

    template<typename T>
    T As() const {
        if constexpr (std::is_same_v<T, int>) return AsInt();
        else if constexpr (std::is_same_v<T, float>) return AsFloat();
        else if constexpr (std::is_same_v<T, std::string>) return AsString();
        else if constexpr (std::is_pointer_v<T>) return static_cast<T>( AsObject() );
        else return T{};
    }

};
