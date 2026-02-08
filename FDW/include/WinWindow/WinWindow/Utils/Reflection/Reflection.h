#pragma once
#include "../../pch.h"
#include "ReflectionTemplates.h"
#include "../Serializer/Serializer.h"

#define REFLECT_BODY(ClassName)                                                                     \
    friend struct ReflectionRegistrar_##ClassName;                                                  \
public:                                                                                             \
    static const ClassInfo& GetStaticClassInfo() { \
        return *ReflectionRegistry::GetInstance()->GetReflectedClassInfo<ClassName>();              \
    }                                                                                               \
    virtual const ClassInfo& GetClassInfo() const {                                                 \
        return GetStaticClassInfo();                                                                \
    }                                                                                               \
private:

#define REFLECT_STRUCT(ClassName)                                                                   \
    friend struct ReflectionRegistrar_##ClassName;                                                  \
public:                                                                                             \
    static const ClassInfo& GetStaticClassInfo() {                                                  \
        return *ReflectionRegistry::GetInstance()->GetReflectedClassInfo<ClassName>();              \
    }                                                                                               \
private:


#define BEGIN_REFLECT(Type, ...)                                                                    \
    struct ReflectionRegistrar_##Type {                                                             \
        static void Register(ClassInfo& info) {                                                     \
            using Self = Type;                                                                      \
            ParentRegistrar<Self, ##__VA_ARGS__>::Register(info);


#define REFLECT_PROPERTY(Name)                                                                      \
            info.AddProperty(PropertyInfo::Create<Self, decltype(Self::Name)>(#Name, offsetof(Self, Name)));

#define REFLECT_METHOD(Name)                                                                        \
            info.AddMethod(CreateMethodWrapper(#Name, &Self::Name));

#define END_REFLECT(Type)                                                                           \
        }                                                                                           \
        inline static AutoRegisterReflection<Type> _registrar{#Type, Register};                     \
    };

#define REGISTER_SERIALIZER(Type, SerializerFunc)                                                   \
    inline static StaticSerializerRegister<Type> _reg_serializer_##Type{                            \
        SerializerFunc                                                                              \
    };
