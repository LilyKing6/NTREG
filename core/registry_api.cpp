/*
 * PROJECT:   Registry Library
 * LICENSE:   See LICENSE in the top level directory
 * COPYRIGHT: Copyright 2024
 * FILE:      registry_api.cpp
 * PURPOSE:   Modern C++20 registry API implementation
 */

#include "registry_api.hpp"
#include "reg.hpp"
#include "reg_internal.hpp"
#include <cstring>
#include <utility>

namespace registry {

static NTReg* g_reg = nullptr;

static void ensure_initialized() {
    if (!g_reg) {
        throw RegistryException(RegistryError::InvalidParameter, "Registry not initialized");
    }
}

static RegistryError map_error(LONG rc) {
    switch (rc) {
        case ERROR_FILE_NOT_FOUND:    return RegistryError::KeyNotFound;
        case ERROR_ACCESS_DENIED:     return RegistryError::AccessDenied;
        case ERROR_INVALID_PARAMETER: return RegistryError::InvalidParameter;
        case ERROR_NOT_ENOUGH_MEMORY: return RegistryError::OutOfMemory;
        default:                      return RegistryError::InvalidParameter;
    }
}

static void throw_on_error(LONG rc, const char* msg) {
    if (rc != ERROR_SUCCESS)
        throw RegistryException(map_error(rc), msg);
}

// Key implementation
Key::~Key() noexcept {
    close();
}

void Key::close() noexcept {
    if (handle_ && owns_handle_ && g_reg) {
        g_reg->CloseKey(static_cast<REGHKEY>(handle_));
        handle_ = nullptr;
    }
}

std::optional<u32> Key::get_dword(std::u16string_view name) const {
    ensure_initialized();
    ULONG type, size = sizeof(u32);
    u32 value;
    if (g_reg->QueryValue(static_cast<REGHKEY>(handle_), name.data(), nullptr,
                          &type, reinterpret_cast<unsigned char*>(&value), &size) == ERROR_SUCCESS
        && type == REG_DWORD) {
        return value;
    }
    return std::nullopt;
}

std::optional<std::u16string> Key::get_string(std::u16string_view name) const {
    ensure_initialized();
    ULONG type, size = 0;
    if (g_reg->QueryValue(static_cast<REGHKEY>(handle_), name.data(), nullptr,
                          &type, nullptr, &size) != ERROR_SUCCESS || type != REG_SZ) {
        return std::nullopt;
    }
    std::u16string result(size / sizeof(WCHAR), u'\0');
    if (g_reg->QueryValue(static_cast<REGHKEY>(handle_), name.data(), nullptr,
                          &type, reinterpret_cast<unsigned char*>(result.data()), &size) == ERROR_SUCCESS) {
        result.resize(_wcs_len_char16(result.c_str()));
        return result;
    }
    return std::nullopt;
}

std::optional<std::vector<u8>> Key::get_binary(std::u16string_view name) const {
    ensure_initialized();
    ULONG type, size = 0;
    if (g_reg->QueryValue(static_cast<REGHKEY>(handle_), name.data(), nullptr,
                          &type, nullptr, &size) != ERROR_SUCCESS) {
        return std::nullopt;
    }
    std::vector<u8> result(size);
    if (g_reg->QueryValue(static_cast<REGHKEY>(handle_), name.data(), nullptr,
                          &type, result.data(), &size) == ERROR_SUCCESS) {
        return result;
    }
    return std::nullopt;
}

std::optional<std::vector<std::u16string>> Key::get_multi_string(std::u16string_view name) const {
    ensure_initialized();
    ULONG type, size = 0;
    if (g_reg->QueryValue(static_cast<REGHKEY>(handle_), name.data(), nullptr,
                          &type, nullptr, &size) != ERROR_SUCCESS ||
        (type != REG_MULTI_SZ && type != REG_SZ)) {
        return std::nullopt;
    }
    if (size == 0) return std::vector<std::u16string>{};

    std::vector<u8> buf(size);
    if (g_reg->QueryValue(static_cast<REGHKEY>(handle_), name.data(), nullptr,
                          &type, buf.data(), &size) != ERROR_SUCCESS) {
        return std::nullopt;
    }

    std::vector<std::u16string> result;
    const char16_t* p = reinterpret_cast<const char16_t*>(buf.data());
    const char16_t* end = reinterpret_cast<const char16_t*>(buf.data() + size);
    while (p < end && *p != u'\0') {
        std::u16string s(p);
        result.push_back(std::move(s));
        p += result.back().length() + 1;
    }
    return result;
}

void Key::set_multi_string(std::u16string_view name, const std::vector<std::u16string>& value) {
    ensure_initialized();
    usize total = 0;
    for (const auto& s : value) total += s.length() + 1;
    total += 1;
    std::vector<char16_t> buf(total, u'\0');
    WCHAR* p = buf.data();
    for (const auto& s : value) {
        std::memcpy(p, s.data(), s.length() * sizeof(WCHAR));
        p += s.length();
        *p++ = u'\0';
    }
    *p = u'\0';
    throw_on_error(g_reg->SetValue(static_cast<REGHKEY>(handle_), name.data(), 0, REG_MULTI_SZ,
                        reinterpret_cast<const unsigned char*>(buf.data()),
                        total * sizeof(WCHAR)),
                   "Failed to set MULTI_SZ value");
}

std::optional<u64> Key::get_qword(std::u16string_view name) const {
    ensure_initialized();
    ULONG type, size = sizeof(u64);
    u64 value;
    if (g_reg->QueryValue(static_cast<REGHKEY>(handle_), name.data(), nullptr,
                          &type, reinterpret_cast<unsigned char*>(&value), &size) == ERROR_SUCCESS
        && type == REG_QWORD) {
        return value;
    }
    return std::nullopt;
}

void Key::set_qword(std::u16string_view name, u64 value) {
    ensure_initialized();
    throw_on_error(g_reg->SetValue(static_cast<REGHKEY>(handle_), name.data(), 0, REG_QWORD,
                        reinterpret_cast<const unsigned char*>(&value), sizeof(u64)),
                   "Failed to set QWORD value");
}

std::optional<std::u16string> Key::get_link(std::u16string_view name) const {
    ensure_initialized();
    ULONG type, size = 0;
    if (g_reg->QueryValue(static_cast<REGHKEY>(handle_), name.data(), nullptr,
                          &type, nullptr, &size) != ERROR_SUCCESS || type != REG_LINK) {
        return std::nullopt;
    }
    if (size == 0) return std::u16string{};
    std::u16string result(size / sizeof(WCHAR), u'\0');
    if (g_reg->QueryValue(static_cast<REGHKEY>(handle_), name.data(), nullptr,
                          &type, reinterpret_cast<unsigned char*>(result.data()), &size) == ERROR_SUCCESS) {
        result.resize(_wcs_len_char16(result.c_str()));
        return result;
    }
    return std::nullopt;
}

std::optional<std::u16string> Key::get_expand_string(std::u16string_view name) const {
    ensure_initialized();
    ULONG type, size = 0;
    if (g_reg->QueryValue(static_cast<REGHKEY>(handle_), name.data(), nullptr,
                          &type, nullptr, &size) != ERROR_SUCCESS || type != REG_EXPAND_SZ) {
        return std::nullopt;
    }
    std::u16string result(size / sizeof(WCHAR), u'\0');
    if (g_reg->QueryValue(static_cast<REGHKEY>(handle_), name.data(), nullptr,
                          &type, reinterpret_cast<unsigned char*>(result.data()), &size) == ERROR_SUCCESS) {
        result.resize(_wcs_len_char16(result.c_str()));
        return result;
    }
    return std::nullopt;
}

void Key::set_link(std::u16string_view name, std::u16string_view value) {
    ensure_initialized();
    ULONG size = (value.length() + 1) * sizeof(WCHAR);
    throw_on_error(g_reg->SetValue(static_cast<REGHKEY>(handle_), name.data(), 0, REG_LINK,
                        reinterpret_cast<const unsigned char*>(value.data()), size),
                   "Failed to set LINK value");
}

void Key::set_expand_string(std::u16string_view name, std::u16string_view value) {
    ensure_initialized();
    ULONG size = (value.length() + 1) * sizeof(WCHAR);
    throw_on_error(g_reg->SetValue(static_cast<REGHKEY>(handle_), name.data(), 0, REG_EXPAND_SZ,
                        reinterpret_cast<const unsigned char*>(value.data()), size),
                   "Failed to set EXPAND_SZ value");
}

void Key::set_dword(std::u16string_view name, u32 value) {
    ensure_initialized();
    throw_on_error(g_reg->SetValue(static_cast<REGHKEY>(handle_), name.data(), 0, REG_DWORD,
                        reinterpret_cast<const unsigned char*>(&value), sizeof(u32)),
                   "Failed to set DWORD value");
}

void Key::set_string(std::u16string_view name, std::u16string_view value) {
    ensure_initialized();
    ULONG size = (value.length() + 1) * sizeof(WCHAR);
    throw_on_error(g_reg->SetValue(static_cast<REGHKEY>(handle_), name.data(), 0, REG_SZ,
                        reinterpret_cast<const unsigned char*>(value.data()), size),
                   "Failed to set string value");
}

void Key::set_binary(std::u16string_view name, const u8* data, usize size) {
    ensure_initialized();
    throw_on_error(g_reg->SetValue(static_cast<REGHKEY>(handle_), name.data(), 0, REG_BINARY,
                        data, size),
                   "Failed to set binary value");
}

void Key::delete_value(std::u16string_view name) {
    ensure_initialized();
    throw_on_error(g_reg->DeleteValue(static_cast<REGHKEY>(handle_), name.data()),
                   "Failed to delete value");
}

Key Key::create_subkey(std::u16string_view name) {
    ensure_initialized();
    REGHKEY handle;
    throw_on_error(g_reg->CreateKey(static_cast<REGHKEY>(handle_), name.data(), &handle),
                   "Failed to create subkey");
    return Key(handle);
}

Key Key::open_subkey(std::u16string_view name) {
    ensure_initialized();
    REGHKEY handle;
    if (g_reg->OpenKey(static_cast<REGHKEY>(handle_), name.data(), &handle) != ERROR_SUCCESS) {
        throw RegistryException(RegistryError::KeyNotFound, "Failed to open subkey");
    }
    return Key(handle);
}

void Key::delete_subkey(std::u16string_view name) {
    ensure_initialized();
    throw_on_error(g_reg->DeleteKey(static_cast<REGHKEY>(handle_), name.data()),
                   "Failed to delete subkey");
}

void Key::enum_keys(std::function<bool(std::u16string_view)> callback) const {
    ensure_initialized();
    WCHAR name[1024];
    ULONG nameSize;
    DWORD index = 0;
    while (true) {
        nameSize = 1024;
        if (g_reg->EnumKey(static_cast<REGHKEY>(handle_), index++, name, &nameSize, nullptr) != ERROR_SUCCESS)
            break;
        if (!callback(std::u16string_view(name)))
            break;
    }
}

void Key::enum_values(std::function<bool(std::u16string_view, ValueType, usize)> callback) const {
    ensure_initialized();
    WCHAR name[1024];
    ULONG nameSize, type, dataSize;
    DWORD index = 0;
    while (true) {
        nameSize = 1024;
        if (g_reg->EnumValue(static_cast<REGHKEY>(handle_), index++, name, &nameSize,
                             &type, nullptr, &dataSize) != ERROR_SUCCESS)
            break;
        if (!callback(std::u16string_view(name), static_cast<ValueType>(type), dataSize))
            break;
    }
}

// Registry implementation
// Note: RegInitializeRegistry is called inside NTRegConnect.
// To avoid double-initialization, we skip the direct call and let NTRegConnect
// handle the full init. We also need to pass the user's hive list through.
extern const char* const HiveList;

void Registry::initialize(const char* hive_list) {
    if (!g_reg) {
        g_reg = InitNTReg();

        // Store the hive list for NTRegConnect's use
        // (HiveList is const char* const — we use g_ActiveHiveList as a
        // mutable override mechanism)
        g_ActiveHiveList = hive_list;

        // Rewire NTRegConnect to use our hive list by patching the local copy
        // Since HiveList is const, we use g_ActiveHiveList in the connect path.
        // RegInitializeRegistry in NTRegConnect uses HiveList still,
        // so we do the full init here and skip NTRegConnect's re-init.
        if (!RegInitializeRegistry(hive_list, TRUE)) {
            throw RegistryException(RegistryError::InvalidParameter, "Failed to initialize registry");
        }

        // Set up root key handles (normally done in NTRegConnect after re-init)
        NTRegOpenKeyW(nullptr, const_cast<PWSTR>(u"NTReg"), &REGHKEY_ROOT);
        NTRegOpenKeyW(nullptr, const_cast<PWSTR>(u"NTReg\\Local"), &REGHKEY_LOCAL);
        NTRegOpenKeyW(nullptr, const_cast<PWSTR>(u"NTReg\\Local\\SYSTEM"), &REGHKEY_SYSTEM);
        NTRegOpenKeyW(nullptr, const_cast<PWSTR>(u"NTReg\\Local\\SOFTWARE"), &REGHKEY_SOFTWARE);
        NTRegOpenKeyW(nullptr, const_cast<PWSTR>(u"NTReg\\Local\\SYSTEM\\NTSoft\\NTReg\\CurrentVersion"), &REGHKEY_CURRENT_VERSION);
    }
}

void Registry::shutdown() {
    if (g_reg) {
        RegShutdownRegistry();
        free(g_reg);
        g_reg = nullptr;
    }
    REGHKEY_ROOT = nullptr;
    REGHKEY_LOCAL = nullptr;
    REGHKEY_SYSTEM = nullptr;
    REGHKEY_SOFTWARE = nullptr;
    REGHKEY_CURRENT_VERSION = nullptr;
}

Key Registry::open_key(std::u16string_view path) {
    ensure_initialized();
    REGHKEY handle;
    if (g_reg->OpenKey(nullptr, path.data(), &handle) != ERROR_SUCCESS) {
        throw RegistryException(RegistryError::KeyNotFound, "Failed to open key");
    }
    return Key(handle);
}

Key Registry::create_key(std::u16string_view path) {
    ensure_initialized();
    REGHKEY handle;
    throw_on_error(g_reg->CreateKey(nullptr, path.data(), &handle),
                   "Failed to create key");
    return Key(handle);
}

void Registry::delete_key(std::u16string_view path) {
    ensure_initialized();
    throw_on_error(g_reg->DeleteKey(nullptr, path.data()),
                   "Failed to delete key");
}

} // namespace registry
