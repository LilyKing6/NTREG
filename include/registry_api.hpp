/*
 * PROJECT:   Registry Library
 * LICENSE:   See LICENSE in the top level directory
 * COPYRIGHT: Copyright 2024
 * FILE:      registry_api.hpp
 * PURPOSE:   Modern C++20 registry API
 */

#pragma once

#include <string>
#include <string_view>
#include <optional>
#include <vector>
#include <functional>
#include <utility>
#include "types.hpp"
#include "exceptions.hpp"

namespace registry {

class Key {
    void* handle_;
    bool owns_handle_;

public:
    explicit Key(void* handle, bool owns = true) noexcept
        : handle_(handle), owns_handle_(owns) {}

    ~Key() noexcept;

    Key(Key&& other) noexcept
        : handle_(std::exchange(other.handle_, nullptr))
        , owns_handle_(std::exchange(other.owns_handle_, false)) {}

    Key& operator=(Key&& other) noexcept {
        if (this != &other) {
            close();
            handle_ = std::exchange(other.handle_, nullptr);
            owns_handle_ = std::exchange(other.owns_handle_, false);
        }
        return *this;
    }

    Key(const Key&) = delete;
    Key& operator=(const Key&) = delete;

    [[nodiscard]] void* native_handle() const noexcept { return handle_; }
    [[nodiscard]] bool valid() const noexcept { return handle_ != nullptr; }

    void close() noexcept;

    // Value operations
    [[nodiscard]] std::optional<u32> get_dword(std::u16string_view name) const;
    [[nodiscard]] std::optional<std::u16string> get_string(std::u16string_view name) const;
    [[nodiscard]] std::optional<std::vector<u8>> get_binary(std::u16string_view name) const;
    [[nodiscard]] std::optional<std::vector<std::u16string>> get_multi_string(std::u16string_view name) const;
    [[nodiscard]] std::optional<u64> get_qword(std::u16string_view name) const;
    [[nodiscard]] std::optional<std::u16string> get_link(std::u16string_view name) const;

    void set_dword(std::u16string_view name, u32 value);
    void set_string(std::u16string_view name, std::u16string_view value);
    void set_binary(std::u16string_view name, const u8* data, usize size);
    void set_multi_string(std::u16string_view name, const std::vector<std::u16string>& value);
    void set_qword(std::u16string_view name, u64 value);
    void set_link(std::u16string_view name, std::u16string_view value);

    void delete_value(std::u16string_view name);

    // Key operations
    [[nodiscard]] Key create_subkey(std::u16string_view name);
    [[nodiscard]] Key open_subkey(std::u16string_view name);
    void delete_subkey(std::u16string_view name);

    // Enumeration
    void enum_keys(std::function<bool(std::u16string_view)> callback) const;
    void enum_values(std::function<bool(std::u16string_view, ValueType, usize)> callback) const;
};

class Registry {
public:
    static void initialize(const char* hive_list = "SYSTEM");
    static void shutdown();

    [[nodiscard]] static Key open_key(std::u16string_view path);
    [[nodiscard]] static Key create_key(std::u16string_view path);
    static void delete_key(std::u16string_view path);
};

} // namespace registry
