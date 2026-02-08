#pragma once
#include "../../pch.h"

struct MethodInfo {
    std::string Name;
    std::function<void(void* obj, const std::vector<std::variant<int, float, std::string, void*>>& args, std::variant<int, float, std::string, void*>& ret)> Invoke;
};
