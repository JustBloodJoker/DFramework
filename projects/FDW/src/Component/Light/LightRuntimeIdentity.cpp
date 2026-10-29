#include "pch.h"

#include <Component/Light/LightRuntimeIdentity.h>

LightRuntimeIdentity::LightRuntimeIdentity() : m_uValue(Next()) { }

LightRuntimeIdentity::LightRuntimeIdentity(const LightRuntimeIdentity &) : LightRuntimeIdentity() { }

LightRuntimeIdentity &LightRuntimeIdentity::operator=(const LightRuntimeIdentity &) { return *this; }

std::uint64_t LightRuntimeIdentity::Value() const { return m_uValue; }

std::uint64_t LightRuntimeIdentity::Next() {
    static std::atomic<std::uint64_t> next{1u};
    return next.fetch_add(1u, std::memory_order_relaxed);
}
