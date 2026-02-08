#include "ClassInfo.h"
#include "ReflectionRegistry.h"

void ClassInfo::AddProperty(PropertyInfo prop) {
    PropertyMap[prop.Name] = Properties.size();
    Properties.push_back(std::move(prop));
}

void ClassInfo::AddMethod(MethodInfo method) {
    Methods[method.Name] = std::move(method);
}

const PropertyInfo* ClassInfo::GetProperty(const std::string& name) const {
    auto it = PropertyMap.find(name);
    if (it != PropertyMap.end()) {
        return &Properties[it->second];
    }
    return nullptr;
}

const MethodInfo* ClassInfo::GetMethod(const std::string& name) const {
    auto it = Methods.find(name);
    if (it != Methods.end()) {
        return &it->second;
    }
    return nullptr;
}

const PropertyInfo* ClassInfo::FindProperty(const std::string& name, void*& obj) const {
    if (auto* prop = GetProperty(name)) return prop;

    for (const auto& [type, offset] : Parents) {
        if (auto* parentInfo = ReflectionRegistry::GetInstance()->GetReflectedClassInfo(type)) {
            void* parentObj = static_cast<char*>(obj) + offset;
            if (auto* prop = parentInfo->FindProperty(name, parentObj)) {
                obj = parentObj;
                return prop;
            }
        }
    }
    return nullptr;
}

const MethodInfo* ClassInfo::FindMethod(const std::string& name, void*& obj) const {
    if (auto* method = GetMethod(name)) return method;

    for (const auto& [type, offset] : Parents) {
        if (auto* parentInfo = ReflectionRegistry::GetInstance()->GetReflectedClassInfo(type)) {
            void* parentObj = static_cast<char*>(obj) + offset;
            if (auto* method = parentInfo->FindMethod(name, parentObj)) {
                obj = parentObj;
                return method;
            }
        }
    }
    return nullptr;
}