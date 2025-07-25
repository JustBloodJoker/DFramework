#pragma once
#include "Concepts.h"
#include "IArchive.h"
#include <string>
#include <map>
#include <list>
#include <vector>
#include <unordered_map>
#include <cstring>
#include <type_traits>

void SerializeObject(IArchive& ar, void* baseObj, const std::vector<FieldInfo>& fields);
void SerializeFieldName(IArchive& ar, const char* name);
void SkipFieldName(IArchive& ar);

template<typename T> void SerializeAny(IArchive& ar, T& val);

template<typename T>
    requires std::is_trivially_copyable_v<T>
IArchive& operator<<(IArchive& ar, T& val) {
    ar.Serialize(&val, sizeof(T));
    return ar;
}

template<typename T>
    requires (std::is_arithmetic_v<T> && !Reflectable<T>)
void SerializeAny(IArchive& ar, T& val) {
    ar << val;
}

template<>
inline void SerializeAny<std::string>(IArchive& ar, std::string& str) {
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
}

template<typename T>
    requires Reflectable<T>
void SerializeAny(IArchive& ar, T& val) {
    SerializeObject(ar, &val, T::GetFieldList());
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
                ar << id;
            }
        }
        else 
        {
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
                auto sp = std::make_shared<T>();
                ctx.IDToPtrShared[id] = sp;
                ctx.IDToPtr[id] = sp.get();
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
                ar << id;
            }
        }
        else
        {
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
                auto raw = new T();
                ctx.IDToPtr[id] = raw;
                SerializeAny(ar, *raw);
                ptr.reset(raw);
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
    else if constexpr (std::is_pointer_v<PtrT>) 
    {
        uint32_t id = 0;

        if (ar.IsOutput()) 
        {
            if (ptr) 
            {
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
                ar << id;
            }
        }
        else 
        {
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
                ptr = new T();
                ctx.IDToPtr[id] = ptr;
                SerializeAny(ar, *ptr);
            }
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
