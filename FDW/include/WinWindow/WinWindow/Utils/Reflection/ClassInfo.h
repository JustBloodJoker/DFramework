#pragma once
#include "../../pch.h"
#include "PropertyInfo.h"
#include "MethodInfo.h"

struct ClassInfo {

public:
    std::string Name;
    size_t Size;
    std::type_index Type;
    std::vector<std::pair<std::type_index, ptrdiff_t>> Parents;
    std::vector<PropertyInfo> Properties;
    std::unordered_map<std::string, size_t> PropertyMap;
    std::unordered_map<std::string, MethodInfo> Methods;

public:
    void AddProperty(PropertyInfo prop);
    void AddMethod(MethodInfo method);
    const PropertyInfo* GetProperty(const std::string& name) const;
    const MethodInfo* GetMethod(const std::string& name) const;
    
    const PropertyInfo* FindProperty(const std::string& name, void*& obj) const;
    const MethodInfo* FindMethod(const std::string& name, void*& obj) const;
};
