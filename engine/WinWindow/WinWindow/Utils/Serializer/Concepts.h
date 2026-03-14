#pragma once
#include <type_traits>
#include <iterator>

template<typename T, typename = void>
constexpr bool has_getfieldlist_v = false;

template<typename T>
constexpr bool has_getfieldlist_v<T, std::void_t<decltype(T::GetFieldList())>> = true;

template<typename T>
concept Reflectable = has_getfieldlist_v<T>;

template<typename T>
concept Container = requires(T a) {
    std::begin(a);
    std::end(a);
    typename T::value_type;
};

template<typename T>
concept MapContainer = requires {
    typename T::key_type;
    typename T::mapped_type;
        requires Container<T>;
};