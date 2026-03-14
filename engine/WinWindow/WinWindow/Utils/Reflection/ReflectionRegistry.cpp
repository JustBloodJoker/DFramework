#include "ReflectionRegistry.h"

const ClassInfo* ReflectionRegistry::GetReflectedClassInfo(const std::string& name) {
    auto it = m_mNameToType.find(name);
    if (it != m_mNameToType.end()) {
        auto it2 = m_mRegistry.find(it->second);
        if (it2 != m_mRegistry.end()) {
            return &it2->second;
        }
    }
    return nullptr;
}

const ClassInfo* ReflectionRegistry::GetReflectedClassInfo(std::type_index type) {
    auto it = m_mRegistry.find(type);
    if (it != m_mRegistry.end()) {
        return &it->second;
    }
    return nullptr;
}