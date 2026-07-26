#include <string>
#include <vector>
#include <filesystem>
#include <stdexcept>

#include "hive_init.hpp"
#include "reg.hpp"
#include "reg_internal.hpp"
#include "reginf.hpp"
#include "binhive.hpp"

namespace registry {

static std::string wide_to_narrow(std::u16string_view ws) {
    std::string result;
    result.reserve(ws.size());
    for (auto c : ws)
        result.push_back(static_cast<char>(c));
    return result;
}

void HiveInitializer::initialize_from_inf(const std::vector<HiveConfig>& configs) {
    if (configs.empty())
        throw RegistryException(RegistryError::InvalidParameter, "No hive configurations provided");

    // Build comma-separated hive list: "SYSTEM,SOFTWARE,DEFAULT"
    std::string hive_list;
    for (size_t i = 0; i < configs.size(); ++i) {
        if (i > 0) hive_list += ',';
        hive_list += wide_to_narrow(configs[i].name);
    }

    // Verify all output directories are the same
    const auto& first_output = configs[0].output_dir;
    bool uppercase = configs[0].uppercase;
    for (const auto& cfg : configs) {
        if (cfg.output_dir != first_output)
            throw RegistryException(RegistryError::InvalidParameter, "All configs must share the same output directory");
    }

    // Single initialization with all hive names
    if (!RegInitializeRegistry(hive_list.c_str(), FALSE))
        throw RegistryException(RegistryError::InvalidParameter, "Failed to initialize registry");

    // Import each INF file
    for (const auto& cfg : configs) {
        std::string inf_path = cfg.inf_file.string();
        if (!ImportRegistryFile(inf_path.data()))
            throw RegistryException(RegistryError::InvalidParameter, "Failed to import INF");
    }

    // Single save with all hives
    std::string output_dir = first_output.string();
    if (!SaveRegistryIntoHive(output_dir.data(), hive_list.c_str(), uppercase))
        throw RegistryException(RegistryError::InvalidParameter, "Failed to save hive");

    // Single shutdown
    RegShutdownRegistry();
}

void HiveInitializer::load_hive(std::u16string_view hive_name, const std::filesystem::path& hive_path) {
    std::string name = wide_to_narrow(hive_name);

    // If hive_path has a parent directory, use it as the HivePath search base
    if (!hive_path.empty() && hive_path.has_parent_path()) {
        char saved_hive_path[260];
        std::strncpy(saved_hive_path, HivePath, sizeof(saved_hive_path));
        saved_hive_path[sizeof(saved_hive_path) - 1] = '\0';

        std::string dir = hive_path.parent_path().string();
        std::strncpy(HivePath, dir.c_str(), sizeof(HivePath));
        HivePath[sizeof(HivePath) - 1] = '\0';

        if (!RegInitializeRegistry(name.c_str(), TRUE)) {
            std::strncpy(HivePath, saved_hive_path, sizeof(HivePath));
            throw RegistryException(RegistryError::KeyNotFound, "Failed to load hive");
        }

        std::strncpy(HivePath, saved_hive_path, sizeof(HivePath));
    } else {
        if (!RegInitializeRegistry(name.c_str(), TRUE))
            throw RegistryException(RegistryError::KeyNotFound, "Failed to load hive");
    }
}

void HiveInitializer::save_hive(std::u16string_view hive_name, const std::filesystem::path& output_path) {
    std::string name = wide_to_narrow(hive_name);
    std::string path = output_path.string();

    if (!SaveRegistryIntoHive(path.data(), name.c_str(), false))
        throw RegistryException(RegistryError::InvalidParameter, "Failed to save hive");
}

void HiveInitializer::quick_init(const std::filesystem::path& reginit_dir,
                                  const std::filesystem::path& output_dir) {
    std::vector<HiveConfig> configs;

    if (std::filesystem::exists(reginit_dir / "hivesys.inf"))
        configs.push_back({u"SYSTEM", reginit_dir / "hivesys.inf", output_dir, false});

    if (std::filesystem::exists(reginit_dir / "hivesft.inf"))
        configs.push_back({u"SOFTWARE", reginit_dir / "hivesft.inf", output_dir, false});

    if (std::filesystem::exists(reginit_dir / "hivedef.inf"))
        configs.push_back({u"DEFAULT", reginit_dir / "hivedef.inf", output_dir, false});

    if (configs.empty())
        throw RegistryException(RegistryError::KeyNotFound, "No INF files found");

    initialize_from_inf(configs);
}

} // namespace registry
