#pragma once
#include "../../pch.h"
#include "PropertyInfo.h"
#include "ClassInfo.h"
#include "../CreativeSingleton.h"

class ReflectionRegistry : public FDWWIN::CreativeSingleton<ReflectionRegistry> {
public:
    template<typename T>
    ClassInfo& RegisterReflectedClass(const std::string& name) {
        std::type_index type = typeid(T);
        auto it = m_mRegistry.find(type);
        if (it == m_mRegistry.end()) {
            auto result = m_mRegistry.emplace(type, ClassInfo{ name, sizeof(T), type });
            m_mNameToType.emplace(name, type);
            return result.first->second;
        }
        return it->second;
    }

    template<typename T>
    const ClassInfo* GetReflectedClassInfo() {
        auto it = m_mRegistry.find(typeid(T));
        if (it != m_mRegistry.end()) {
            return &it->second;
        }
        return nullptr;
    }

    const ClassInfo* GetReflectedClassInfo(const std::string& name);
    const ClassInfo* GetReflectedClassInfo(std::type_index type);

private:
    std::unordered_map<std::type_index, ClassInfo> m_mRegistry;
    std::unordered_map<std::string, std::type_index> m_mNameToType;
};
