/*
 * PROJECT:   Registry Library
 * LICENSE:   See LICENSE in the top level directory
 * COPYRIGHT: Copyright 2024
 * FILE:      hive_init.hpp
 * PURPOSE:   Modern C++20 hive initialization system
 */

#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include "types.hpp"
#include "exceptions.hpp"
#include "registry_api.hpp"

namespace registry {

// Hive initialization configuration
struct HiveConfig {
    std::wstring name;              // SYSTEM, SOFTWARE, etc.
    std::filesystem::path inf_file; // Path to INF file
    std::filesystem::path output_dir; // Output directory
    bool uppercase = false;         // Uppercase filename
};

// Modern hive initializer
class HiveInitializer {
public:
    // Initialize from INF files
    static void initialize_from_inf(const std::vector<HiveConfig>& configs);

    // Load existing hive
    static void load_hive(std::wstring_view hive_name, const std::filesystem::path& hive_path);

    // Save hive to file
    static void save_hive(std::wstring_view hive_name, const std::filesystem::path& output_path);

    // Quick initialization with default settings
    static void quick_init(const std::filesystem::path& reginit_dir,
                          const std::filesystem::path& output_dir);
};

} // namespace registry
