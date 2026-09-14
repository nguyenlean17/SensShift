#pragma once
#include <cstddef>

#if defined(_MSC_VER)
namespace stdext {
    template <typename T>
    constexpr inline T make_checked_array_iterator(T it, size_t) noexcept {
        return it;
    }

    template <typename T>
    constexpr inline T make_unchecked_array_iterator(T it) noexcept {
        return it;
    }
}
#endif
