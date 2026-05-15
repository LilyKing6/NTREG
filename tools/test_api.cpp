#include <iostream>
#include <cassert>
#include "registry_api.hpp"

int main() {
    int passed = 0, failed = 0;

    auto check = [&](bool cond, const char* name) {
        if (cond) { ++passed; std::cout << "  PASS: " << name << "\n"; }
        else      { ++failed; std::cout << "  FAIL: " << name << "\n"; }
    };

    try {
        // --- Init ---
        std::cout << "[1] Initialize registry\n";
        registry::Registry::initialize("SYSTEM");
        check(true, "Registry::initialize");

        // --- Create key ---
        std::cout << "\n[2] Create key\n";
        auto key = registry::Registry::create_key(L"\\NTReg\\Local\\SYSTEM\\TestAPI");
        check(key.valid(), "key is valid");

        // --- Set values ---
        std::cout << "\n[3] Set values\n";
        key.set_string(L"StringVal", L"Hello C++20");
        key.set_dword(L"DwordVal", 12345);
        key.set_binary(L"BinVal", reinterpret_cast<const uint8_t*>("\xDE\xAD\xBE\xEF"), 4);
        check(true, "set_string / set_dword / set_binary");

        // --- Read values back ---
        std::cout << "\n[4] Read values back\n";
        auto sv = key.get_string(L"StringVal");
        check(sv.has_value() && *sv == L"Hello C++20", "get_string matches");

        auto dv = key.get_dword(L"DwordVal");
        check(dv.has_value() && *dv == 12345, "get_dword matches");

        auto bv = key.get_binary(L"BinVal");
        check(bv.has_value() && bv->size() == 4, "get_binary size matches");

        // --- Missing value returns nullopt ---
        std::cout << "\n[5] Missing value\n";
        auto missing = key.get_string(L"Nonexistent");
        check(!missing.has_value(), "missing value is nullopt");

        // --- Subkey create/open ---
        std::cout << "\n[6] Subkey create/open\n";
        auto sub = key.create_subkey(L"SubKey1");
        sub.set_dword(L"InnerVal", 99);
        auto opened = key.open_subkey(L"SubKey1");
        auto iv = opened.get_dword(L"InnerVal");
        check(iv.has_value() && *iv == 99, "subkey value round-trip");

        // --- Enumerate keys ---
        std::cout << "\n[7] Enumerate subkeys\n";
        int subkey_count = 0;
        key.enum_keys([&](std::wstring_view name) {
            subkey_count++;
            return true;
        });
        check(subkey_count >= 1, "enum_keys finds subkeys");

        // --- Enumerate values ---
        std::cout << "\n[8] Enumerate values\n";
        int val_count = 0;
        key.enum_values([&](std::wstring_view name, registry::ValueType type, registry::usize size) {
            val_count++;
            return true;
        });
        check(val_count == 3, "enum_values finds 3 values");

        // --- Delete value ---
        std::cout << "\n[9] Delete value\n";
        key.delete_value(L"BinVal");
        auto gone = key.get_binary(L"BinVal");
        check(!gone.has_value(), "deleted value is gone");

        // --- Delete subkey ---
        std::cout << "\n[10] Delete subkey\n";
        key.delete_subkey(L"SubKey1");
        int after_del = 0;
        key.enum_keys([&](std::wstring_view) { after_del++; return true; });
        check(after_del == 0, "deleted subkey is gone");

        // --- Shutdown ---
        std::cout << "\n[11] Shutdown\n";
        registry::Registry::shutdown();
        check(true, "Registry::shutdown");

    } catch (const registry::RegistryException& e) {
        std::cerr << "Registry error: " << e.what() << "\n";
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    std::cout << "\n==============================\n";
    std::cout << "Results: " << passed << " passed, " << failed << " failed\n";
    return failed > 0 ? 1 : 0;
}
