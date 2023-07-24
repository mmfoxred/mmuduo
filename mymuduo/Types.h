#pragma once

#include <stdint.h>
#include <ext/vstring.h>
#include <ext/vstring_fwd.h>
#include <string>
using std::string;

template <typename To, typename From>
inline To implicit_cast(From const& f) {
    return f;
}

template <typename To, typename From>
inline To down_cast(From* f) {
    if (false) {
        implicit_cast<From*, To>(0);
    }
#if !defined(NDEBUG) && !defined(GOOGLE_PROTOBUF_NO_RTTI)
    assert(f == NULL || dynamic_cast<To>(f) != NULL);  // RTTI: debug mode only!
#endif

    return static_cast<To>(f);
}