/*
 * PROJECT:   Registry Library
 * LICENSE:   See LICENSE in the top level directory
 * COPYRIGHT: Copyright 2024
 * FILE:      regexplorer.cpp
 * PURPOSE:   Interactive registry browser using modern C++20 API
 */

#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <optional>
#include "registry_api.hpp"

using registry::Key;
using registry::Registry;
using registry::ValueType;

static Key currentKey = Key(nullptr, false);
static std::u16string currentPath = u"\\NTReg\\Local";

void printHelp() {
    std::cout << "Commands:\n";
    std::cout << "  ls              - List subkeys and values\n";
    std::cout << "  cd <path>       - Change to subkey\n";
    std::cout << "  pwd             - Print current path\n";
    std::cout << "  cat <value>     - Read value\n";
    std::cout << "  mkdir <key>     - Create subkey\n";
    std::cout << "  rm <key/value>  - Delete key or value\n";
    std::cout << "  set <name> <type> <data> - Set value (type: sz/dword)\n";
    std::cout << "  exit            - Exit\n";
}

static std::string u16_to_narrow(const std::u16string& s) {
    std::string result;
    result.reserve(s.size());
    for (char16_t c : s) result.push_back(static_cast<char>(c & 0x7F));
    return result;
}

void ls() {
    std::cout << "\nSubkeys:\n";
    currentKey.enum_keys([](std::u16string_view name) -> bool {
        std::cout << "  [" << u16_to_narrow(std::u16string(name)) << "]\n";
        return true;
    });

    std::cout << "\nValues:\n";
    currentKey.enum_values([](std::u16string_view name, ValueType type, registry::usize size) -> bool {
        const char* typeName = "?";
        switch (type) {
            case ValueType::String:    typeName = "REG_SZ"; break;
            case ValueType::Dword:     typeName = "REG_DWORD"; break;
            case ValueType::Binary:    typeName = "REG_BINARY"; break;
            case ValueType::MultiString:   typeName = "REG_MULTI_SZ"; break;
            case ValueType::Qword:     typeName = "REG_QWORD"; break;
            default: break;
        }
        std::cout << "  " << u16_to_narrow(std::u16string(name)) << " (" << typeName << ", size=" << size << ")\n";
        return true;
    });
}

void cd(const std::u16string& path) {
    try {
        std::u16string fullPath;
        if (path == u"..") {
            size_t pos = currentPath.rfind(u'\\');
            if (pos != std::u16string::npos && pos > 0) {
                fullPath = currentPath.substr(0, pos);
            } else {
                std::cout << "Already at root\n";
                return;
            }
        } else if (path[0] == u'\\') {
            fullPath = path;
        } else {
            fullPath = currentPath + u"\\" + path;
        }

        auto newKey = Registry::open_key(fullPath);
        currentKey = std::move(newKey);
        currentPath = fullPath;
    } catch (const registry::RegistryException&) {
        std::cout << "Failed to open key\n";
    }
}

void cat(const std::u16string& valueName) {
    auto strVal = currentKey.get_string(valueName);
    if (strVal) {
        std::cout << u16_to_narrow(*strVal) << "\n";
        return;
    }

    auto dwordVal = currentKey.get_dword(valueName);
    if (dwordVal) {
        std::cout << *dwordVal << "\n";
        return;
    }

    auto binVal = currentKey.get_binary(valueName);
    if (binVal) {
        for (auto byte : *binVal)
            std::cout << std::hex << (byte < 16 ? "0" : "") << (int)byte << " ";
        std::cout << std::dec << "\n";
        return;
    }

    std::cout << "Value not found\n";
}

void mkdir(const std::u16string& keyName) {
    try {
        auto subkey = currentKey.create_subkey(keyName);
        std::cout << "Key created\n";
    } catch (const registry::RegistryException&) {
        std::cout << "Failed to create key\n";
    }
}

void rm(const std::u16string& name) {
    try {
        currentKey.delete_subkey(name);
        std::cout << "Deleted key\n";
    } catch (const registry::RegistryException&) {
        try {
            currentKey.delete_value(name);
            std::cout << "Deleted value\n";
        } catch (const registry::RegistryException&) {
            std::cout << "Failed to delete\n";
        }
    }
}

void setValue(const std::u16string& name, const std::u16string& typeStr, const std::u16string& data) {
    try {
        if (typeStr == u"sz") {
            currentKey.set_string(name, data);
            std::cout << "Value set\n";
        } else if (typeStr == u"dword") {
            std::string narrow(data.begin(), data.end());
            uint32_t val = std::stoul(narrow);
            currentKey.set_dword(name, val);
            std::cout << "Value set\n";
        } else {
            std::cout << "Unsupported type\n";
        }
    } catch (const registry::RegistryException&) {
        std::cout << "Failed to set value\n";
    }
}

static std::u16string to_wstring(const std::string& s) {
    if (s.empty()) return u"";
    std::u16string ws(s.size(), u'\0');
    for (size_t i = 0; i < s.size(); i++)
        ws[i] = static_cast<char16_t>(static_cast<unsigned char>(s[i]));
    return ws;
}

int main() {
    try {
        Registry::initialize("SYSTEM");
    } catch (const registry::RegistryException& e) {
        std::cerr << "Failed to initialize registry: " << e.what() << "\n";
        return 1;
    }

    try {
        currentKey = Registry::open_key(currentPath);
    } catch (const registry::RegistryException&) {
        std::cerr << "Failed to open initial key\n";
        Registry::shutdown();
        return 1;
    }

    std::cout << "Registry Browser - Type 'help' for commands\n";

    std::string line;
    while (true) {
        std::cout << "\n" << u16_to_narrow(currentPath) << "> ";
        if (!std::getline(std::cin, line))
            break;

        std::istringstream iss(line);
        std::string cmd;
        iss >> cmd;

        if (cmd == "exit" || cmd == "quit") {
            break;
        } else if (cmd == "help") {
            printHelp();
        } else if (cmd == "ls") {
            ls();
        } else if (cmd == "pwd") {
            std::cout << u16_to_narrow(currentPath) << "\n";
        } else if (cmd == "cd") {
            std::string path;
            iss >> path;
            cd(to_wstring(path));
        } else if (cmd == "cat") {
            std::string name;
            iss >> name;
            cat(to_wstring(name));
        } else if (cmd == "mkdir") {
            std::string name;
            iss >> name;
            mkdir(to_wstring(name));
        } else if (cmd == "rm") {
            std::string name;
            iss >> name;
            rm(to_wstring(name));
        } else if (cmd == "set") {
            std::string name, type, data;
            iss >> name >> type >> data;
            setValue(to_wstring(name), to_wstring(type), to_wstring(data));
        } else if (!cmd.empty()) {
            std::cout << "Unknown command. Type 'help' for commands\n";
        }
    }

    currentKey.close();
    Registry::shutdown();
    return 0;
}
