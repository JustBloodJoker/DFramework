#pragma once
#include <cstddef>
#include "Concepts.h"
#include "FieldInfo.h"
#include "BaseFieldCollector.h"
#include "DynamicSerializerRegistry.h"

#define FIELDS_NAME s_sFields

#define BEGIN_FIELD_REGISTRATION(SelfType, ...) \
    using Self = SelfType; \
    static const std::vector<FieldInfo>& GetFieldList() { \
        static std::vector<FieldInfo> FIELDS_NAME; \
        if (FIELDS_NAME.empty()) { \
            BaseFieldCollector<SelfType, __VA_ARGS__>::Collect(FIELDS_NAME);

#define REGISTER_FIELD(name) \
    FIELDS_NAME.push_back(FieldInfo::Make<decltype(Self::name)>(#name, offsetof(Self, name)));

#define END_FIELD_REGISTRATION() } return FIELDS_NAME; }



#define REGISTER_SERIALIZER(Type, SerializerFunc)           \
static StaticSerializerRegister<Type> _reg_serializer_##Type(SerializerFunc);
