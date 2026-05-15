/*
 * PROJECT:   Registry Library
 * LICENSE:   See LICENSE in the top level directory
 * COPYRIGHT: Copyright 2024
 * FILE:      exceptions.hpp
 * PURPOSE:   Modern C++20 exception handling
 */

#pragma once

#include <stdexcept>
#include <string>
#include <system_error>
#include "types.hpp"

namespace registry {

// Registry error codes
enum class RegistryError : i32 {
    Success = 0,
    KeyNotFound = 2,
    AccessDenied = 5,
    OutOfMemory = 8,
    InvalidParameter = 87,
    NoMoreItems = 259
};

// Exception base class
class RegistryException : public std::runtime_error {
    RegistryError error_;
public:
    explicit RegistryException(RegistryError err, const char* msg)
        : std::runtime_error(msg), error_(err) {}

    [[nodiscard]] RegistryError error() const noexcept { return error_; }
};

// Specific exceptions
class KeyNotFoundException : public RegistryException {
public:
    explicit KeyNotFoundException(const char* key)
        : RegistryException(RegistryError::KeyNotFound, key) {}
};

class AccessDeniedException : public RegistryException {
public:
    explicit AccessDeniedException(const char* msg = "Access denied")
        : RegistryException(RegistryError::AccessDenied, msg) {}
};

} // namespace registry
