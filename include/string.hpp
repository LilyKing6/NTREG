/*
 * PROJECT:   Registry Library
 * LICENSE:   See LICENSE in the top level directory
 * COPYRIGHT: Copyright 2024
 * FILE:      string.hpp
 * PURPOSE:   Modern C++20 string utilities for registry operations
 */

#pragma once

#include <string>
#include <string_view>
#include <algorithm>
#include <cctype>

#include "types.hpp"

namespace registry {

// Modern replacement for UNICODE_STRING with automatic memory management
class RegistryString {
private:
    std::u16string data_;

public:
    // Constructors
    constexpr RegistryString() noexcept = default;

    RegistryString(const char16_t* str) : data_(str ? str : u"") {}

    RegistryString(std::u16string_view sv) : data_(sv) {}

    RegistryString(const std::u16string& str) : data_(str) {}

    RegistryString(std::u16string&& str) noexcept : data_(std::move(str)) {}

    // Accessors
    [[nodiscard]] constexpr std::u16string_view view() const noexcept {
        return data_;
    }

    [[nodiscard]] constexpr const char16_t* c_str() const noexcept {
        return data_.c_str();
    }

    [[nodiscard]] constexpr const char16_t* data() const noexcept {
        return data_.data();
    }

    [[nodiscard]] constexpr usize size() const noexcept {
        return data_.size();
    }

    [[nodiscard]] constexpr usize length() const noexcept {
        return data_.length();
    }

    [[nodiscard]] constexpr bool empty() const noexcept {
        return data_.empty();
    }

    // Modifiers
    void clear() noexcept {
        data_.clear();
    }

    void reserve(usize capacity) {
        data_.reserve(capacity);
    }

    // Comparison (case-insensitive for registry keys)
    [[nodiscard]] i32 compare_insensitive(std::u16string_view other) const noexcept {
        const auto len = std::min(data_.size(), other.size());

        for (usize i = 0; i < len; ++i) {
            const auto c1 = _to_upper_char16(data_[i]);
            const auto c2 = _to_upper_char16(other[i]);
            if (c1 != c2) {
                return static_cast<i32>(c1 - c2);
            }
        }

        return static_cast<i32>(data_.size() - other.size());
    }

    [[nodiscard]] bool equals_insensitive(std::u16string_view other) const noexcept {
        if (data_.size() != other.size()) {
            return false;
        }

        return std::equal(data_.begin(), data_.end(), other.begin(),
            [](char16_t a, char16_t b) {
                return _to_upper_char16(a) == _to_upper_char16(b);
            });
    }

    // Conversion operators
    [[nodiscard]] operator std::u16string_view() const noexcept {
        return data_;
    }

    [[nodiscard]] const std::u16string& str() const noexcept {
        return data_;
    }

    // Comparison operators
    [[nodiscard]] auto operator<=>(const RegistryString& other) const noexcept = default;

    // String operations
    [[nodiscard]] bool starts_with(std::u16string_view prefix) const noexcept {
        return data_.starts_with(prefix);
    }

    [[nodiscard]] bool ends_with(std::u16string_view suffix) const noexcept {
        return data_.ends_with(suffix);
    }

    [[nodiscard]] bool contains(std::u16string_view substr) const noexcept {
        return data_.find(substr) != std::u16string::npos;
    }
};

// String utilities
namespace string_utils {

// Case-insensitive comparison
[[nodiscard]] inline i32 compare_insensitive(std::u16string_view a, std::u16string_view b) noexcept {
    const auto len = std::min(a.size(), b.size());

    for (usize i = 0; i < len; ++i) {
        const auto c1 = _to_upper_char16(a[i]);
        const auto c2 = _to_upper_char16(b[i]);
        if (c1 != c2) {
            return static_cast<i32>(c1 - c2);
        }
    }

    return static_cast<i32>(a.size() - b.size());
}

// Case-insensitive equality
[[nodiscard]] inline bool equals_insensitive(std::u16string_view a, std::u16string_view b) noexcept {
    if (a.size() != b.size()) {
        return false;
    }

    return std::equal(a.begin(), a.end(), b.begin(),
        [](char16_t c1, char16_t c2) {
            return _to_upper_char16(c1) == _to_upper_char16(c2);
        });
}

// Convert to uppercase
[[nodiscard]] inline std::u16string to_upper(std::u16string_view str) {
    std::u16string result;
    result.reserve(str.size());

    std::transform(str.begin(), str.end(), std::back_inserter(result),
        [](char16_t c) { return _to_upper_char16(c); });

    return result;
}

// Convert to lowercase
[[nodiscard]] inline std::u16string to_lower(std::u16string_view str) {
    std::u16string result;
    result.reserve(str.size());

    std::transform(str.begin(), str.end(), std::back_inserter(result),
        [](char16_t c) { return _to_lower_char16(c); });

    return result;
}

} // namespace string_utils

} // namespace registry
