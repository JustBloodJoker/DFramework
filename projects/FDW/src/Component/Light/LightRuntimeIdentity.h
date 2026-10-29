#pragma once

class LightRuntimeIdentity {
  public:
    LightRuntimeIdentity();
    LightRuntimeIdentity(const LightRuntimeIdentity &);
    LightRuntimeIdentity &operator=(const LightRuntimeIdentity &);
    std::uint64_t Value() const;

  private:
    static std::uint64_t Next();
    std::uint64_t m_uValue;
};
