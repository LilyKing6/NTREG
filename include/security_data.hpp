/*
 * PROJECT:   Registry Library
 * FILE:      security_data.hpp
 * PURPOSE:   Pre-defined security descriptors for registry hives
 */

#pragma once

#include <cstddef>

extern unsigned char BcdSecurity[];
extern const std::size_t BcdSecuritySize;

extern unsigned char SoftwareSecurity[];
extern const std::size_t SoftwareSecuritySize;

extern unsigned char SystemSecurity[];
extern const std::size_t SystemSecuritySize;
