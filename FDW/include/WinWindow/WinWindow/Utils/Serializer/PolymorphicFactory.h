#pragma once

#include <string>
#include <unordered_map>
#include <functional>
#include <typeindex>
#include <memory>
#include <cassert>
#include "../CreativeSingleton.h"

class PolymorphicFactory : public FDWWIN::CreativeSingleton<PolymorphicFactory> {
public:
    PolymorphicFactory() {
        m_mFactories[typeid(int).name()] = []() { return new int(); };
        m_mFactories[typeid(float).name()] = []() { return new float(); };
        m_mFactories[typeid(double).name()] = []() { return new double(); };
        m_mFactories[typeid(bool).name()] = []() { return new bool(); };
        m_mFactories[typeid(std::string).name()] = []() { return new std::string(); };
    }

    template<typename T>
    void RegisterType(const std::string& name) {
        m_mFactories[name] = []() -> void* { return new T(); };
    }

    void* Create(const std::string& name) const {
        auto it = m_mFactories.find(name);
        if (it != m_mFactories.end())
            return it->second();
        return nullptr;
    }

private:
    std::unordered_map<std::string, std::function<void* ()>> m_mFactories;
};

template<typename T>
struct AutoRegisterInFactory {
    AutoRegisterInFactory() {
        Register();
    }

private:
    template<typename U = T>
    typename std::enable_if<!std::is_abstract<U>::value>::type
        Register() {
        PolymorphicFactory::GetInstance()->RegisterType<T>(typeid(T).name());
    }

    template<typename U = T>
    typename std::enable_if<std::is_abstract<U>::value>::type
        Register() {
    }
};