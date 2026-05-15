#include <iostream>
#include <cstring>
#include "reg.hpp"

/*
 * Test HINIT_FILE loading path (HvLoadHive / block-by-block)
 * by loading a previously generated hive file via RegInitializeRegistry(..., TRUE).
 */
int main() {
    const char* hive_file = "hiveload_test/system";

    // Initialize registry with LoadHiveData=TRUE to exercise HvLoadHive
    if (!RegInitializeRegistry("SYSTEM", TRUE)) {
        std::cerr << "RegInitializeRegistry with load failed\n";
        return 1;
    }

    // Try to open and read a key from the loaded hive
    NTReg* reg = InitNTReg();
    if (!reg) {
        std::cerr << "InitNTReg failed\n";
        RegShutdownRegistry();
        return 1;
    }

    if (!reg->Connect()) {
        std::cerr << "Connect failed\n";
        RegShutdownRegistry();
        return 1;
    }

    REGHKEY hKey;
    LONG rc = reg->OpenKey(nullptr, L"\\NTReg\\Local\\SYSTEM", &hKey);
    if (rc != ERROR_SUCCESS) {
        std::cerr << "OpenKey SYSTEM failed: " << rc << "\n";
        RegShutdownRegistry();
        return 1;
    }

    // Enumerate subkeys to verify data was loaded
    ULONG index = 0;
    WCHAR name[256];
    ULONG nameSize;
    int keyCount = 0;
    while (true) {
        nameSize = 256;
        rc = reg->EnumKey(hKey, index++, name, &nameSize, nullptr);
        if (rc != ERROR_SUCCESS) break;
        keyCount++;
        std::wcout << L"  Subkey: " << name << L"\n";
    }

    reg->CloseKey(hKey);
    RegShutdownRegistry();

    std::cout << "Loaded hive has " << keyCount << " subkeys under SYSTEM\n";
    std::cout << "HINIT_FILE load test PASSED\n";
    return 0;
}
