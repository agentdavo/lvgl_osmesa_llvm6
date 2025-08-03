#ifndef DX8GL_GUID_UTILS_H
#define DX8GL_GUID_UTILS_H

#include "d3d8_types.h"
#include <functional>

namespace std {

// Hash specialization for GUID to allow use in unordered_map
template<>
struct hash<GUID> {
    size_t operator()(const GUID& guid) const noexcept {
        // Simple hash combining all GUID components
        size_t h1 = std::hash<unsigned long>{}(guid.Data1);
        size_t h2 = std::hash<unsigned short>{}(guid.Data2);
        size_t h3 = std::hash<unsigned short>{}(guid.Data3);
        size_t h4 = 0;
        
        // Hash the 8 bytes of Data4
        for (int i = 0; i < 8; ++i) {
            h4 = h4 * 31 + std::hash<unsigned char>{}(guid.Data4[i]);
        }
        
        // Combine all hashes
        return h1 ^ (h2 << 1) ^ (h3 << 2) ^ (h4 << 3);
    }
};

// Equality operator for GUID
template<>
struct equal_to<GUID> {
    bool operator()(const GUID& a, const GUID& b) const noexcept {
        return a.Data1 == b.Data1 &&
               a.Data2 == b.Data2 &&
               a.Data3 == b.Data3 &&
               std::equal(std::begin(a.Data4), std::end(a.Data4), std::begin(b.Data4));
    }
};

} // namespace std

#endif // DX8GL_GUID_UTILS_H