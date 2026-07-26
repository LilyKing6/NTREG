/*
 * PROJECT:   Registry Library
 * LICENSE:   See LICENSE in the top level directory
 * COPYRIGHT: Copyright 2024
 * FILE:      types.hpp
 * PURPOSE:   Modern C++20 type definitions
 */

#pragma once

#include <cstdint>
#include <cstddef>

namespace registry {

// Modern integer types (replacing ULONG, DWORD, etc.)
using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

using i8 = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;

using usize = std::size_t;
using isize = std::ptrdiff_t;

// Platform-specific types (keep for Windows API compatibility)
using HANDLE = void*;
using HCELL_INDEX = u32;

// Large integer type
struct LargeInteger {
    union {
        struct {
            u32 low_part;
            i32 high_part;
        };
        i64 quad_part;
    };

    constexpr LargeInteger() noexcept : quad_part(0) {}
    constexpr explicit LargeInteger(i64 value) noexcept : quad_part(value) {}

    constexpr operator i64() const noexcept { return quad_part; }
};

// Storage type enumeration
enum class StorageType : u32 {
    Stable = 0,
    Volatile = 1
};

// File type enumeration
enum class FileType : u32 {
    Primary = 0,
    Log = 1,
    External = 2,
    Max = 3
};

// Registry value types
enum class ValueType : u32 {
    None = 0,
    String = 1,
    ExpandString = 2,
    Binary = 3,
    Dword = 4,
    DwordBigEndian = 5,
    Link = 6,
    MultiString = 7,
    ResourceList = 8,
    FullResourceDescriptor = 9,
    ResourceRequirementsList = 10,
    Qword = 11
};

// Constants
inline constexpr u32 HCELL_NIL = 0xFFFFFFFF;
inline constexpr u32 HCELL_TYPE_MASK = 0x80000000;
inline constexpr u32 HCELL_BLOCK_MASK = 0x7FFFFFFF;

inline constexpr usize PAGE_SIZE = 0x1000;
inline constexpr usize HBLOCK_SIZE = 0x1000;
inline constexpr usize HSECTOR_SIZE = 0x200;
inline constexpr usize HSECTOR_COUNT = 8;

// Helper functions
[[nodiscard]] constexpr bool is_cell_nil(HCELL_INDEX cell) noexcept {
    return cell == HCELL_NIL;
}

[[nodiscard]] constexpr StorageType get_cell_storage_type(HCELL_INDEX cell) noexcept {
    return (cell & HCELL_TYPE_MASK) ? StorageType::Volatile : StorageType::Stable;
}

[[nodiscard]] constexpr u32 get_cell_block_offset(HCELL_INDEX cell) noexcept {
    return cell & HCELL_BLOCK_MASK;
}

} // namespace registry

// char16_t helper functions (registry binary format uses 2-byte characters)
inline unsigned int _wcs_len_char16(const char16_t* str) {
    const char16_t* s = str;
    while (*s) ++s;
    return static_cast<unsigned int>(s - str);
}
inline int _wcs_cmp_char16(const char16_t* a, const char16_t* b) {
    while (*a && (*a == *b)) { ++a; ++b; }
    return static_cast<int>(*a) - static_cast<int>(*b);
}
inline char16_t _to_upper_char16(char16_t c) {
    return (c >= u'a' && c <= u'z') ? c - (u'a' - u'A') : c;
}
inline char16_t _to_lower_char16(char16_t c) {
    return (c >= u'A' && c <= u'Z') ? c + (u'a' - u'A') : c;
}
