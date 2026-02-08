#include "ScriptValue.h"

ScriptValue::ScriptValue() : Data(0) {}
ScriptValue::ScriptValue(int v) : Data(v) {}
ScriptValue::ScriptValue(float v) : Data(v) {}
ScriptValue::ScriptValue(std::string v) : Data(v) {}
ScriptValue::ScriptValue(void* v) : Data(v) {}
ScriptValue::ScriptValue(const char* v) : Data(std::string(v)) {}

bool ScriptValue::IsInt() const { return std::holds_alternative<int>(Data); }
bool ScriptValue::IsFloat() const { return std::holds_alternative<float>(Data); }
bool ScriptValue::IsString() const { return std::holds_alternative<std::string>(Data); }
bool ScriptValue::IsObject() const { return std::holds_alternative<void*>(Data); }

int ScriptValue::AsInt() const {
    if (IsInt()) return std::get<int>(Data);
    if (IsFloat()) return static_cast<int>(std::get<float>(Data));
    return 0;
}

float ScriptValue::AsFloat() const {
    if (IsFloat()) return std::get<float>(Data);
    if (IsInt()) return static_cast<float>(std::get<int>(Data));
    return 0.0f;
}

std::string ScriptValue::AsString() const {
    if (IsString()) return std::get<std::string>(Data);
    if (IsInt()) return std::to_string(std::get<int>(Data));
    if (IsFloat()) return std::to_string(std::get<float>(Data));
    return "";
}

void* ScriptValue::AsObject() const {
    if (IsObject()) return std::get<void*>(Data);
    return nullptr;
}

bool ScriptValue::AsBool() const {
    if (IsInt()) return std::get<int>(Data) != 0;
    if (IsFloat()) return std::get<float>(Data) != 0.0f;
    if (IsString()) return !std::get<std::string>(Data).empty();
    if (IsObject()) return std::get<void*>(Data) != nullptr;
    return false;
}

ScriptValue ScriptValue::operator+(const ScriptValue& other) const {
    if (IsString() || other.IsString()) return ScriptValue(AsString() + other.AsString());
    if (IsFloat() || other.IsFloat()) return ScriptValue(AsFloat() + other.AsFloat());
    return ScriptValue(AsInt() + other.AsInt());
}

ScriptValue ScriptValue::operator-(const ScriptValue& other) const {
    if (IsFloat() || other.IsFloat()) return ScriptValue(AsFloat() - other.AsFloat());
    return ScriptValue(AsInt() - other.AsInt());
}

ScriptValue ScriptValue::operator*(const ScriptValue& other) const {
    if (IsFloat() || other.IsFloat()) return ScriptValue(AsFloat() * other.AsFloat());
    return ScriptValue(AsInt() * other.AsInt());
}

ScriptValue ScriptValue::operator/(const ScriptValue& other) const {
    float div = other.AsFloat();
    if (div == 0.0f) return ScriptValue(0);
    return ScriptValue(AsFloat() / div);
}

bool ScriptValue::operator==(const ScriptValue& other) const {
    if (IsString() && other.IsString()) return AsString() == other.AsString();
    if (IsObject() || other.IsObject()) return AsObject() == other.AsObject(); // Ptr comparison
    return std::abs(AsFloat() - other.AsFloat()) < 0.0001f;
}

bool ScriptValue::operator>(const ScriptValue& other) const {
    return AsFloat() > other.AsFloat();
}

bool ScriptValue::operator<(const ScriptValue& other) const {
    return AsFloat() < other.AsFloat();
}

