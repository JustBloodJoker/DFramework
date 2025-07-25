#pragma once
#include <string>
#include <vector>
#include <functional>
#include "IArchive.h"

struct FieldInfo {
    std::string Name;
    size_t Offset;
    std::function<void(IArchive&, void*)> Serializer;

    template<typename FieldT>
    static FieldInfo Make(const char* name, size_t offset) {
        return FieldInfo{
            name,
            offset,
            [](IArchive& ar, void* ptr) {
                SerializeAny(ar, *static_cast<FieldT*>(ptr));
            }
        };
    }
};