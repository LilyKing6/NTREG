/*
 * PROJECT:   Registry Library
 * LICENSE:   See LICENSE in the top level directory
 * COPYRIGHT: Copyright 2024
 * FILE:      registry_api.cpp
 * PURPOSE:   Modern C++20 registry API implementation
 */

#include "registry_api.hpp"
#include "reg.hpp"
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

std::optional<u32> Key::get_dword(std::wstring_view name) const {
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

std::optional<std::wstring> Key::get_string(std::wstring_view name) const {
    ensure_initialized();
    ULONG type, size = 0;
    if (g_reg->QueryValue(static_cast<REGHKEY>(handle_), name.data(), nullptr,
                          &type, nullptr, &size) != ERROR_SUCCESS || type != REG_SZ) {
        return std::nullopt;
    }
    std::wstring result(size / sizeof(wchar_t), L'\0');
    if (g_reg->QueryValue(static_cast<REGHKEY>(handle_), name.data(), nullptr,
                          &type, reinterpret_cast<unsigned char*>(result.data()), &size) == ERROR_SUCCESS) {
        result.resize(std::wcslen(result.c_str()));
        return result;
    }
    return std::nullopt;
}

std::optional<std::vector<u8>> Key::get_binary(std::wstring_view name) const {
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

void Key::set_dword(std::wstring_view name, u32 value) {
    ensure_initialized();
    throw_on_error(g_reg->SetValue(static_cast<REGHKEY>(handle_), name.data(), 0, REG_DWORD,
                        reinterpret_cast<const unsigned char*>(&value), sizeof(u32)),
                   "Failed to set DWORD value");
}

void Key::set_string(std::wstring_view name, std::wstring_view value) {
    ensure_initialized();
    ULONG size = (value.length() + 1) * sizeof(wchar_t);
    throw_on_error(g_reg->SetValue(static_cast<REGHKEY>(handle_), name.data(), 0, REG_SZ,
                        reinterpret_cast<const unsigned char*>(value.data()), size),
                   "Failed to set string value");
}

void Key::set_binary(std::wstring_view name, const u8* data, usize size) {
    ensure_initialized();
    throw_on_error(g_reg->SetValue(static_cast<REGHKEY>(handle_), name.data(), 0, REG_BINARY,
                        data, size),
                   "Failed to set binary value");
}

void Key::delete_value(std::wstring_view name) {
    ensure_initialized();
    throw_on_error(g_reg->DeleteValue(static_cast<REGHKEY>(handle_), name.data()),
                   "Failed to delete value");
}

Key Key::create_subkey(std::wstring_view name) {
    ensure_initialized();
    REGHKEY handle;
    throw_on_error(g_reg->CreateKey(static_cast<REGHKEY>(handle_), name.data(), &handle),
                   "Failed to create subkey");
    return Key(handle);
}

Key Key::open_subkey(std::wstring_view name) {
    ensure_initialized();
    REGHKEY handle;
    if (g_reg->OpenKey(static_cast<REGHKEY>(handle_), name.data(), &handle) != ERROR_SUCCESS) {
        throw RegistryException(RegistryError::KeyNotFound, "Failed to open subkey");
    }
    return Key(handle);
}

void Key::delete_subkey(std::wstring_view name) {
    ensure_initialized();
    throw_on_error(g_reg->DeleteKey(static_cast<REGHKEY>(handle_), name.data()),
                   "Failed to delete subkey");
}

void Key::enum_keys(std::function<bool(std::wstring_view)> callback) const {
    ensure_initialized();
    WCHAR name[1024];
    ULONG nameSize;
    DWORD index = 0;
    while (true) {
        nameSize = 1024;
        if (g_reg->EnumKey(static_cast<REGHKEY>(handle_), index++, name, &nameSize, nullptr) != ERROR_SUCCESS)
            break;
        if (!callback(std::wstring_view(name, nameSize)))
            break;
    }
}

void Key::enum_values(std::function<bool(std::wstring_view, ValueType, usize)> callback) const {
    ensure_initialized();
    WCHAR name[1024];
    ULONG nameSize, type, dataSize;
    DWORD index = 0;
    while (true) {
        nameSize = 1024;
        if (g_reg->EnumValue(static_cast<REGHKEY>(handle_), index++, name, &nameSize,
                             &type, nullptr, &dataSize) != ERROR_SUCCESS)
            break;
        if (!callback(std::wstring_view(name, nameSize), static_cast<ValueType>(type), dataSize))
            break;
    }
}

// Registry implementation
void Registry::initialize(const char* hive_list) {
    if (!g_reg) {
        g_reg = InitNTReg();
        if (!RegInitializeRegistry(hive_list, FALSE)) {
            throw RegistryException(RegistryError::InvalidParameter, "Failed to initialize registry");
        }
        if (!g_reg->Connect()) {
            throw RegistryException(RegistryError::InvalidParameter, "Failed to connect to registry");
        }
    }
}

void Registry::shutdown() {
    if (g_reg) {
        g_reg->Disconnect();
        free(g_reg);
        g_reg = nullptr;
    }
}

Key Registry::open_key(std::wstring_view path) {
    ensure_initialized();
    REGHKEY handle;
    if (g_reg->OpenKey(nullptr, path.data(), &handle) != ERROR_SUCCESS) {
        throw RegistryException(RegistryError::KeyNotFound, "Failed to open key");
    }
    return Key(handle);
}

Key Registry::create_key(std::wstring_view path) {
    ensure_initialized();
    REGHKEY handle;
    throw_on_error(g_reg->CreateKey(nullptr, path.data(), &handle),
                   "Failed to create key");
    return Key(handle);
}

void Registry::delete_key(std::wstring_view path) {
    ensure_initialized();
    throw_on_error(g_reg->DeleteKey(nullptr, path.data()),
                   "Failed to delete key");
}

} // namespace registry
