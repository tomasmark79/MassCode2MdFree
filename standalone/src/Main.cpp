// MIT License
// Copyright (c) 2024-2025 Tomáš Mark

#include <cxxopts.hpp>
#include <fstream>
#include <iostream>
#include <memory>

#include "Logger/Logger.hpp"
#include "Mc2MdParser/Mc2MdParser.hpp"
#include "Utils/Utils.hpp"

constexpr char standaloneName[] = "MassCode2Md";

std::string standaloneExecutableDirectory =
    FileSystemManager::getExecutableDirectory();

/// @brief Main Standalone entry point
int main(int argc, const char *argv[]) {
  // ---basic-information------is-safe-to-delete 👇🏻
  LOG_I << standaloneName << " / C++ " << __cplusplus << std::endl;
  LOG_D << "Executable Directory: " << standaloneExecutableDirectory
        << std::endl;
  // ----------------------------------delete me 👆🏻

  // ---assets-testing---------is-safe-to-delete 👇🏻
  std::string assetFp = standaloneExecutableDirectory + "/" +
                        static_cast<std::string>(UTILS_ASSET_PATH) + "/" +
                        static_cast<std::string>(UTILS_FIRST_ASSET_FILE);
  std::ifstream file(assetFp);
  try {
    if (file.is_open()) {
      LOG_DEBUG("Opened first asset file: " + assetFp);
    } else {
      LOG_ERROR("No assets found: " + assetFp);
    }
  } catch (const std::exception &e) {
    LOG_ERROR("Error opening first asset file: " + assetFp);
  }
  // ----------------------------------delete me 👆🏻

  // ---argument-parsing-------is-safe-to-delete 👇🏻
  try {
    std::string in = "";
    std::string out = "";

    auto options = std::make_unique<cxxopts::Options>(argv[0], standaloneName);
    options->positional_help("[optional args]").show_positional_help();
    options->set_width(70)
        .set_tab_expansion()
        .allow_unrecognised_options()
        .add_options()("h,help", "Show help")(
            "m,omit", "Omit library loading",
            cxxopts::value<bool>()->default_value("false"))(
            "i,inputfile", "Input file", cxxopts::value<std::string>())(
            "o,outputfile", "Output file", cxxopts::value<std::string>());

    options->parse_positional({"inputfile", "outputfile"});
    const auto result = options->parse(argc, argv);

    
    if (result.count("inputfile") && result.count("outputfile")) {
      in = result["inputfile"].as<std::string>();
      out = result["outputfile"].as<std::string>();
      LOG_I << "Input file: " << result["inputfile"].as<std::string>()
      << std::endl;
      LOG_I << "Output file: " << result["outputfile"].as<std::string>()
      << std::endl;
    }
    
    if (result.count("help") || in.empty()) {
      LOG_I << options->help({"", "Group"}) << std::endl;
      return 0;
    }

    if (!result.count("omit")) {
      const std::string assetsPath = standaloneExecutableDirectory + "/" +
                                     static_cast<std::string>(UTILS_ASSET_PATH);
      std::unique_ptr<library::Mc2MdParser> lib =
          std::make_unique<library::Mc2MdParser>(assetsPath, in, out);
    } else {
      LOG_W << "Loading library omitted [-o]" << std::endl;
    }
  } catch (const cxxopts::exceptions::exception &e) {
    LOG_E << "error parsing options: " << e.what() << std::endl;
    return 1;
  }
  // ----------------------------------delete me 👆🏻

  return 0;
}