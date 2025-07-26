#pragma once
#include <vector>
#include "Concepts.h"
#include "FieldInfo.h"

template<typename Derived, typename Base>
constexpr size_t BaseOffset() {
    constexpr uintptr_t dummy = 0x1000;
    return reinterpret_cast<uintptr_t>(static_cast<Base*>(reinterpret_cast<Derived*>(dummy))) - dummy;
}

template<typename Derived, typename... Bases>
struct BaseFieldCollector {
    static void Collect(std::vector<FieldInfo>& fields) {
        (CollectOne<Derived, Bases>(fields), ...);
    }
private:
    template<typename Derived, typename Base>
    static void CollectOne(std::vector<FieldInfo>& fields) {
        if constexpr (Reflectable<Base>) {
            if (Base::GetFieldList().empty()) return;

            size_t baseOffset = BaseOffset<Derived, Base>();
            for (const auto& f : Base::GetFieldList()) {
                fields.push_back({ f.Name, baseOffset + f.Offset, f.Serializer });
            }
        }
    }
};
