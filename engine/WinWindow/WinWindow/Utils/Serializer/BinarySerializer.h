#pragma once
#include "IArchive.h"
#include "Serializer.h"
#include <fstream>

class BinarySerializer {

public:
    BinarySerializer() = default;

    bool LoadFromFile(const std::string& filename) {
        std::ifstream in(filename, std::ios::binary | std::ios::ate);
        if (!in) return false;

        std::streamsize size = in.tellg();
        in.seekg(0, std::ios::beg);
        m_xBuffer.resize(size);
        return in.read(m_xBuffer.data(), size).good();
    }

    bool SaveToFile(const std::string& filename) const {
        std::ofstream out(filename, std::ios::binary);
        if (!out) return false;
        return out.write(m_xBuffer.data(), m_xBuffer.size()).good();
    }

    template<typename... Args>
    void LoadFromObjects(const Args&... args) {
        m_xBuffer.clear();
        MemoryArchive ar(true);
        (SerializeAny(ar, const_cast<Args&>(args)), ...);
        m_xBuffer = ar.GetBuffer();
    }

    template<typename... Args>
    void DeserializeToObjects(Args&... args) const {
        MemoryArchive ar(false, m_xBuffer);
        (SerializeAny(ar, args), ...);
    }


protected:
    std::vector<char> m_xBuffer;
};
