/*
 * PROJECT:   Registry Library
 * LICENSE:   See LICENSE in the top level directory
 * COPYRIGHT: Copyright 2024
 * FILE:      inithive_modern.cpp
 * PURPOSE:   Modern hive initialization with version info
 */

#include <iostream>
#include <ctime>
#include "registry_api.hpp"
#include "version.h"

void write_version_info() {
    try {
        auto key = registry::Registry::create_key(u"\\NTReg\\Local\\SYSTEM\\CurrentVersion");

        // Product information
        key.set_string(u"ProductName", u"" SOFTNAME);
        key.set_string(u"CodeName", u"" CODENAME);
        key.set_dword(u"MajorVersion", VER_PRODUCTMAJORVERSION);
        key.set_dword(u"MinorVersion", VER_PRODUCTMINORVERSION);
        key.set_dword(u"BuildNumber", VER_PRODUCTBUILD);

        // Platform
        key.set_string(u"Platform", u"" _PLATFORM);

        // Install date (current time)
        key.set_dword(u"InstallDate", static_cast<uint32_t>(std::time(nullptr)));

        std::cout << "Version information written successfully\n";
        std::cout << "  Product: " SOFTNAME " (" CODENAME ")\n";
        std::cout << "  Version: " VERSION_PRODUCT "\n";
        std::cout << "  Platform: " _PLATFORM "\n";

    } catch (const registry::RegistryException& e) {
        std::cerr << "Failed to write version info: " << e.what() << std::endl;
        throw;
    }
}

int main(int argc, char* argv[]) {
    try {
        std::cout << "Registry Initialization Tool\n";
        std::cout << "============================\n\n";

        const char* hive_list = "SYSTEM";

        if (argc > 1) {
            if (std::string(argv[1]) == "load") {
                std::cout << "Loading existing registry...\n";
                registry::Registry::initialize(hive_list);
                std::cout << "Registry loaded successfully!\n";
                registry::Registry::shutdown();
                return 0;
            }
        }

        // Initialize new registry
        std::cout << "Initializing new registry...\n";
        registry::Registry::initialize(hive_list);

        // Write version information
        write_version_info();

        // Create test key and value
        auto test_key = registry::Registry::create_key(u"\\NTReg\\Local\\SYSTEM\\Test");
        test_key.set_string(u"TestValue", u"Hello from modern C++20!");
        test_key.set_dword(u"Counter", 42);

        std::cout << "\nTest key created with sample values\n";
        std::cout << "Registry initialization complete!\n";

        registry::Registry::shutdown();
        return 0;

    } catch (const registry::RegistryException& e) {
        std::cerr << "Registry error: " << e.what() << std::endl;
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
