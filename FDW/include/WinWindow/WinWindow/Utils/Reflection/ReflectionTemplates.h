#pragma once
#include "../../pch.h"
#include "ReflectionRegistry.h"
#include "../Serializer/PolymorphicFactory.h"
#include "../Serializer/DynamicSerializerRegistry.h"


template<typename T>
struct AutoRegisterReflection {

    AutoRegisterReflection(const char* name, std::function<void(ClassInfo&)> registerFunc) {
        ClassInfo& info = ReflectionRegistry::GetInstance()->RegisterReflectedClass<T>(name);
        registerFunc(info);

        DynamicSerializerRegistry::GetInstance()->RegisterCustom<T>(
            [](IArchive& ar, void* ptr) {
                auto* info = ReflectionRegistry::GetInstance()->GetReflectedClassInfo<T>();
                SerializeObject(ar, ptr, info);
            }
        );

        if constexpr (std::is_default_constructible_v<T> && !std::is_abstract_v<T>) {
            PolymorphicFactory::GetInstance()->RegisterType<T>(name);
            PolymorphicFactory::GetInstance()->RegisterType<T>(typeid(T).name());
        }
    }

};

template<typename T>
T UnwrapScriptValue(const std::variant<int, float, std::string, void*>& val) {
    if constexpr (std::is_same_v<T, int>) return std::holds_alternative<int>(val) ? std::get<int>(val) : (int)std::get<float>(val);
    else if constexpr (std::is_same_v<T, float>) return std::holds_alternative<float>(val) ? std::get<float>(val) : (float)std::get<int>(val);
    else if constexpr (std::is_same_v<T, std::string>) return std::holds_alternative<std::string>(val) ? std::get<std::string>(val) : "";
    else if constexpr (std::is_pointer_v<T>) {
        if (std::holds_alternative<void*>(val)) return static_cast<T>(std::get<void*>(val));
        if (std::holds_alternative<int>(val) && std::get<int>(val) == 0) return nullptr;
        return nullptr;
    }
    else return T{};
}

template<typename T>
void WrapReturnValue(T ret, std::variant<int, float, std::string, void*>& outVal) {
    if constexpr (std::is_same_v<T, int>) outVal = ret;
    else if constexpr (std::is_same_v<T, float>) outVal = ret;
    else if constexpr (std::is_same_v<T, std::string>) outVal = ret;
    else if constexpr (std::is_pointer_v<T>) outVal = static_cast<void*>(ret);
}

template<typename Class, typename Ret, typename... Args>
MethodInfo CreateMethodWrapper(const char* name, Ret(Class::* method)(Args...)) {
    return MethodInfo{
        name,
        [method](void* obj, const std::vector<std::variant<int, float, std::string, void*>>& args, std::variant<int, float, std::string, void*>& retVal) {
            Class* instance = static_cast<Class*>(obj);
            InvokeMethod(instance, method, args, retVal, std::make_index_sequence<sizeof...(Args)>{});
        }
    };
}

template <typename Class, typename Ret, typename... Args, size_t... I>
void InvokeMethod(Class* obj, Ret(Class::* method)(Args...), const std::vector<std::variant<int, float, std::string, void*>>& args, std::variant<int, float, std::string, void*>& retVal, std::index_sequence<I...>) {
    if constexpr (std::is_void_v<Ret>) {
        (obj->*method)(UnwrapScriptValue<std::decay_t<Args>>(args[I])...);
    }
    else {
        WrapReturnValue((obj->*method)(UnwrapScriptValue<std::decay_t<Args>>(args[I])...), retVal);
    }
}

template<typename Class, typename... Bases>
struct ParentRegistrar;

template<typename Class, typename First, typename... Rest>
struct ParentRegistrar<Class, First, Rest...> {
    static void Register(ClassInfo& info) {
        // Calculate offset from Derived (Class) to Base (First)
        // Using common trick: (Base*)((Derived*)1) - (Derived*)1
        // but vtable is creating in runtime .....
        ptrdiff_t offset = (char*)(static_cast<First*>((Class*)1)) - (char*)((Class*)1);
        info.Parents.push_back({ typeid(First), offset });
        ParentRegistrar<Class, Rest...>::Register(info);
    }
};

template<typename Class>
struct ParentRegistrar<Class> {
    static void Register(ClassInfo&) {}
};

