#pragma once
#include <iostream>
#include <vector>
#include <cassert>
#include <unordered_map>

struct PointerTrackingContext {
    std::unordered_map<const void*, uint32_t> PtrToID;
    std::unordered_map<uint32_t, void*> IDToPtr;
    std::unordered_map<uint32_t, std::shared_ptr<void>> IDToPtrShared; 
    uint32_t NextID = 1;
};

class IArchive {
public:
    virtual void Serialize(void* data, size_t size) = 0;
    virtual bool IsOutput() const = 0;
    virtual ~IArchive() = default;
    PointerTrackingContext& GetContext() { return m_xContext; }

private:
    PointerTrackingContext m_xContext;

};

class OutputArchive : public IArchive {
public:
    explicit OutputArchive(std::ostream& s) : m_xOut(s) {}
    void Serialize(void* data, size_t size) override { m_xOut.write(static_cast<const char*>(data), size); }
    bool IsOutput() const override { return true; }

private:
    std::ostream& m_xOut;

};

class InputArchive : public IArchive {
public:
    explicit InputArchive(std::istream& s) : m_xIn(s) {}
    void Serialize(void* data, size_t size) override { m_xIn.read(static_cast<char*>(data), size); }
    bool IsOutput() const override { return false; }

private:
    std::istream& m_xIn;
};

class MemoryArchive : public IArchive {
public:
    explicit MemoryArchive(bool output, std::vector<char> existing = {})
        : m_vBuffer(std::move(existing)), m_bIsOutput(output) {
    }

    void Serialize(void* data, size_t size) override {
        if (m_bIsOutput) {
            m_vBuffer.insert(m_vBuffer.end(), static_cast<char*>(data), static_cast<char*>(data) + size);
        }
        else {
            assert(m_uPosition + size <= m_vBuffer.size());
            std::memcpy(data, m_vBuffer.data() + m_uPosition, size);
            m_uPosition += size;
        }
    }

    bool IsOutput() const override { return m_bIsOutput; }

    const std::vector<char>& GetBuffer() const { return m_vBuffer; }
    void LoadBuffer(std::vector<char> newBuffer) {
        m_vBuffer = std::move(newBuffer);
        m_uPosition = 0;
    }

private:
    std::vector<char> m_vBuffer;
    size_t m_uPosition = 0;
    bool m_bIsOutput;

};