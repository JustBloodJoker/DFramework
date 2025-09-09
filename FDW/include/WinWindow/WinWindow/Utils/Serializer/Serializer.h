#pragma once
#include <string>
#include <map>
#include <list>
#include <vector>
#include <unordered_map>
#include <cstring>
#include <type_traits>
#include "Concepts.h"
#include "DynamicSerializerRegistry.h"
#include "PolymorphicFactory.h"
#include "../Macroses.h"


void SerializeObject(IArchive& ar, void* baseObj, const std::vector<FieldInfo>& fields);
void SerializeFieldName(IArchive& ar, const char* name);
void SkipFieldName(IArchive& ar);

template<typename T>
    requires std::is_trivially_copyable_v<T>
IArchive& operator<<(IArchive& ar, T& val) {
    ar.Serialize(&val, sizeof(T));
    return ar;
}

namespace registers_Serializer {

    using String = std::string;
    static StaticSerializerRegister<String> _reg_serializer_String([](IArchive& ar, void* ptr) {
        std::string& str = *static_cast<std::string*>(ptr);
        if (ar.IsOutput()) {
            uint32_t size = static_cast<uint32_t>(str.size());
            ar << size;
            ar.Serialize(str.data(), size);
        }
        else {
            uint32_t size;
            ar << size;
            str.resize(size);
            ar.Serialize(str.data(), size);
        }
    });

}


template<typename T>
inline void SerializeAny(IArchive& ar, T& val) {
    std::type_index type  = typeid(val);
    auto* serializer = DynamicSerializerRegistry::GetInstance()->Get(type);
    if (serializer) {
        (*serializer)(ar, &val);
    }
    else {
        SAFE_ASSERT(false, std::string("No serializer registered for type: ") + type.name());
    }
}

template<typename T>
    requires (std::is_arithmetic_v<T>)
void SerializeAny(IArchive& ar, T& val) {
    ar << val;
}

template<typename T>
    requires (std::is_enum_v<T>)
void SerializeAny(IArchive& ar, T& val) {
    using Underlying = std::underlying_type_t<T>;
    ar << reinterpret_cast<Underlying&>(val);
}


template<MapContainer Map>
void SerializeAny(IArchive& ar, Map& m) {
    if (ar.IsOutput()) {
        uint32_t count = static_cast<uint32_t>(m.size());
        ar << count;
        for (auto& [key, val] : m) {
            SerializeAny(ar, const_cast<typename Map::key_type&>(key));
            SerializeAny(ar, val);
        }
    }
    else {
        uint32_t count;
        ar << count;
        m.clear();
        for (uint32_t i = 0; i < count; ++i) {
            typename Map::key_type key{};
            typename Map::mapped_type val{};
            SerializeAny(ar, key);
            SerializeAny(ar, val);
            m.emplace(std::move(key), std::move(val));
        }
    }
}

template<Container Cont>
    requires (!MapContainer<Cont>)
void SerializeAny(IArchive& ar, Cont& c) {
    if (ar.IsOutput()) {
        uint32_t count = static_cast<uint32_t>(c.size());
        ar << count;
        for (auto& el : c) SerializeAny(ar, el);
    }
    else {
        uint32_t count;
        ar << count;
        c.clear();
        for (uint32_t i = 0; i < count; ++i) {
            typename Cont::value_type val{};
            SerializeAny(ar, val);
            if constexpr (requires { c.emplace_back(val); }) {
                c.emplace_back(std::move(val));
            }
            else if constexpr (requires { c.insert(c.end(), val); }) {
                c.insert(c.end(), std::move(val));
            }
        }
    }
}

template<typename PtrT>
void SerializePointer(IArchive& ar, PtrT& ptr)
{
    using T = typename std::pointer_traits<PtrT>::element_type;
    auto& ctx = ar.GetContext();

    if constexpr (std::is_same_v<PtrT, std::shared_ptr<T>>)
    {
        uint32_t id = 0;

        if (ar.IsOutput())
        {
            if (ptr)
            {
                std::string typeName = typeid(*ptr).name();
                SerializeAny(ar, typeName);

                void* raw = ptr.get();
                auto it = ctx.PtrToID.find(raw);
                if (it == ctx.PtrToID.end())
                {
                    id = ctx.NextID++;
                    ctx.PtrToID[raw] = id;
                    ar << id;
                    SerializeAny(ar, *ptr);
                }
                else
                {
                    id = it->second;
                    ar << id;
                }
            }
            else
            {
                std::string empty{};
                SerializeAny(ar, empty);
                ar << id;
            }
        }
        else
        {
            std::string typeName;
            SerializeAny(ar, typeName);
            ar << id;

            if (id == 0)
            {
                ptr.reset();
                return;
            }

            auto it = ctx.IDToPtrShared.find(id);
            if (it != ctx.IDToPtrShared.end())
            {
                ptr = std::static_pointer_cast<T>(it->second);
            }
            else
            {
                void* raw = PolymorphicFactory::GetInstance()->Create(typeName);
                assert(raw && "Unknown polymorphic type");

                auto sp = std::shared_ptr<T>(static_cast<T*>(raw));
                ctx.IDToPtrShared[id] = sp;
                ctx.IDToPtr[id] = raw;
                SerializeAny(ar, *sp);
                ptr = sp;
            }
        }
    }

    else if constexpr (std::is_same_v<PtrT, std::unique_ptr<T>>)
    {
        uint32_t id = 0;

        if (ar.IsOutput())
        {
            if (ptr)
            {
                std::string typeName = typeid(*ptr).name();
                SerializeAny(ar, typeName);

                void* raw = ptr.get();
                auto it = ctx.PtrToID.find(raw);
                if (it == ctx.PtrToID.end())
                {
                    id = ctx.NextID++;
                    ctx.PtrToID[raw] = id;
                    ar << id;
                    SerializeAny(ar, *ptr);
                }
                else
                {
                    id = it->second;
                    ar << id;
                }
            }
            else
            {
                std::string empty{};
                SerializeAny(ar, empty);
                ar << id;
            }
        }
        else
        {
            std::string typeName;
            SerializeAny(ar, typeName);
            ar << id;

            if (id == 0)
            {
                ptr.reset();
                return;
            }

            auto it = ctx.IDToPtr.find(id);
            if (it != ctx.IDToPtr.end())
            {
                ptr.reset(static_cast<T*>(it->second));
            }
            else
            {
                void* raw = PolymorphicFactory::GetInstance()->Create(typeName);
                assert(raw && "Unknown polymorphic type");

                T* obj = static_cast<T*>(raw);
                ctx.IDToPtr[id] = obj;
                SerializeAny(ar, *obj);
                ptr.reset(obj);
            }
        }
    }

    else if constexpr (std::is_pointer_v<PtrT>)
    {
        uint32_t id = 0;

        if (ar.IsOutput())
        {
            if (ptr)
            {
                std::string typeName = typeid(*ptr).name();
                SerializeAny(ar, typeName);

                auto it = ctx.PtrToID.find(ptr);
                if (it == ctx.PtrToID.end())
                {
                    id = ctx.NextID++;
                    ctx.PtrToID[ptr] = id;
                    ar << id;
                    SerializeAny(ar, *ptr);
                }
                else
                {
                    id = it->second;
                    ar << id;
                }
            }
            else
            {
                std::string empty{};
                SerializeAny(ar, empty);
                ar << id;
            }
        }
        else
        {
            std::string typeName;
            SerializeAny(ar, typeName);
            ar << id;

            if (id == 0)
            {
                ptr = nullptr;
                return;
            }

            auto it = ctx.IDToPtr.find(id);
            if (it != ctx.IDToPtr.end())
            {
                ptr = static_cast<T*>(it->second);
            }
            else
            {
                void* raw = PolymorphicFactory::GetInstance()->Create(typeName);
                assert(raw && "Unknown polymorphic type");

                ptr = static_cast<T*>(raw);
                ctx.IDToPtr[id] = ptr;
                SerializeAny(ar, *ptr);
            }
        }
    }

    else if constexpr (std::is_same_v<PtrT, std::weak_ptr<T>>)
    {
        if (ar.IsOutput())
        {
            std::shared_ptr<T> tmp = ptr.lock();
            SerializePointer(ar, tmp);
        }
        else
        {
            std::shared_ptr<T> tmp;
            SerializePointer(ar, tmp);
            ptr = tmp;
        }
    }
}


template<typename T>
void SerializeAny(IArchive& ar, std::shared_ptr<T>& ptr) {
    SerializePointer(ar, ptr);
}

template<typename T>
void SerializeAny(IArchive& ar, std::weak_ptr<T>& ptr) {
    SerializePointer(ar, ptr);
}

template<typename T>
void SerializeAny(IArchive& ar, std::unique_ptr<T>& ptr) {
    SerializePointer(ar, ptr);
}

template<typename T>
void SerializeAny(IArchive& ar, T*& ptr) {
    SerializePointer(ar, ptr);
}

inline void SerializeFieldName(IArchive& ar, const char* name) {
    uint32_t len = static_cast<uint32_t>(strlen(name));
    ar << len;
    ar.Serialize((void*)name, len);
}

inline void SkipFieldName(IArchive& ar) {
    uint32_t len;
    ar << len;
    std::string dummy(len, ' ');
    ar.Serialize(dummy.data(), len);
}

inline void SerializeObject(IArchive& ar, void* baseObj, const std::vector<FieldInfo>& fields) {
    for (const auto& f : fields) {
        void* fieldPtr = static_cast<void*>(static_cast<char*>(baseObj) + f.Offset);
        if (ar.IsOutput()) {
            SerializeFieldName(ar, f.Name.c_str());
        }
        else {
            SkipFieldName(ar);
        }
        f.Serializer(ar, fieldPtr);
    }
}
