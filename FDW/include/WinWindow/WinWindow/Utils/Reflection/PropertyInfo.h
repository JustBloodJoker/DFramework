#pragma once
#include "../../pch.h"
#include "../Scripting/ScriptValue.h"
#include "../Serializer/IArchive.h"

template<typename T>
void SerializeAny(IArchive& ar, T& val);

struct PropertyInfo {
    
    std::string Name;
    size_t Offset;
    std::type_index Type;
    std::function<void(IArchive&, void*)> Serializer;
    std::function<ScriptValue(void*)> Getter;
    std::function<void(void*, ScriptValue)> Setter;

    template<typename ClassType, typename MemberType>
    static PropertyInfo Create(const char* name, size_t offset) {
        
        PropertyInfo info{
            name,
            offset,
            typeid(MemberType),
            [offset](IArchive& ar, void* obj) {
                MemberType* member = reinterpret_cast<MemberType*>(static_cast<char*>(obj) + offset);
                SerializeAny(ar, *member); 
            }
        };

        if constexpr (std::is_constructible_v<ScriptValue, MemberType> || std::is_pointer_v<MemberType> || std::is_convertible_v<MemberType, std::string> || std::is_arithmetic_v<MemberType>) {
             info.Getter = [offset](void* obj) -> ScriptValue {
                 MemberType* member = reinterpret_cast<MemberType*>(static_cast<char*>(obj) + offset);
                 return ScriptValue(*member);
             };
             info.Setter = [offset](void* obj, ScriptValue v) {
                 MemberType* member = reinterpret_cast<MemberType*>(static_cast<char*>(obj) + offset);
                 *member = v.As<MemberType>();
             };
        }

        return info;

    }
};
