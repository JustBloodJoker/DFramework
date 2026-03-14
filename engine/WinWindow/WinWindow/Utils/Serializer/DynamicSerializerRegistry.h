#pragma once

#include <unordered_map>
#include <typeindex>
#include <functional>
#include "IArchive.h"
#include "../CreativeSingleton.h"

class DynamicSerializerRegistry : public FDWWIN::CreativeSingleton<DynamicSerializerRegistry>{
public:
    using SerializerFunc = std::function<void(IArchive&, void*)>;

    void Register(std::type_index type, SerializerFunc func) {
        m_vRegistry[type] = std::move(func);
    }

    SerializerFunc* Get(std::type_index type) {
        auto it = m_vRegistry.find(type);
        return it != m_vRegistry.end() ? &it->second : nullptr;
    }

    template<typename T>
    void RegisterCustom(std::function<void(IArchive&, void*)> func) {
        Register(typeid(T), std::move(func));
    }

private:
    std::unordered_map<std::type_index, SerializerFunc> m_vRegistry;
};


template<typename T>
struct StaticSerializerRegister {
    StaticSerializerRegister(std::function<void(IArchive&, void*)> fn) {
        DynamicSerializerRegistry::GetInstance()->RegisterCustom<T>(std::move(fn));
    }
};
