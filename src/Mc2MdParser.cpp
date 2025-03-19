// MIT License
// Copyright (c) 2024-2025 Tomáš Mark

#include <Logger/Logger.hpp>
#include <Mc2MdParser/Mc2MdParser.hpp>
#include <Mc2MdParser/version.h>
#include <Utils/Utils.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <regex>
#include <string>
#include <utility>
#include <vector>

namespace library {

Mc2MdParser::Mc2MdParser(const std::string &assetsPath, std::string in,
                         std::string out)
    : m_assetsPath(assetsPath), m_input(in), m_output(out) {
  LOG_IMPORTANT("Mc2MdParser v." + std::string(MC2MDPARSER_VERSION) +
                " constructed.");
  LOG_DEBUG("Assets Path: " + this->m_assetsPath);

  // Check if the input file exists
  if (!std::filesystem::exists(m_input)) {
    LOG_E << "Input file does not exist: " << m_input << std::endl;
    return;
  }

  // parser(m_input, m_output);

  int totalWrittenFolders = 0;
  int totalWrittenFiles = 0;

  std::ifstream ifsJsonBuffer(in);
  std::string strJsonContent((std::istreambuf_iterator<char>(ifsJsonBuffer)),
                             std::istreambuf_iterator<char>());
  nlohmann::json masscodeJsonReparsed;
  try {
    masscodeJsonReparsed = nlohmann::json::parse(strJsonContent);
  } catch (const std::exception &e) {
    LOG_E << e.what() << std::endl;
    return;
  }

  // Create Folders
  std::vector<std::pair<std::string, std::string>> folderData;
  for (const auto &folders : masscodeJsonReparsed["folders"]) {
    std::string folderName = folders["name"];
    std::string folderId = folders["id"];

    folderData.push_back(std::make_pair(folderName, folderId));

    // Create the folder using the folderName
    LOG_I << "Folder Name: " << folderName << ", Folder ID: " << folderId
          << std::endl;

    // Replace invalid characters in label with a valid character
    std::replace_if(
        folderName.begin(), folderName.end(),
        [](char c) {
          return !std::isalnum(c) && c != '_' && c != '-' && c != '+';
        },
        '_');

    std::string folderPath = out + "/" + folderName;
    if (std::filesystem::exists(folderPath)) {
      LOG_I << "Folder already exists: " << folderPath << std::endl;
      // Update the folderName in the folderData vector
      folderData.back().first = folderName;
      continue;
    } else {
      LOG_I << "Creating folder: " << folderPath << std::endl;
      if (!std::filesystem::create_directory(folderPath)) {
        LOG_E << "Failed to create folder: " << folderPath << std::endl;
        return;
      }
      totalWrittenFolders++;
    }

    // Update the folderName in the folderData vector
    folderData.back().first = folderName;
  }

  // Process the Snippets --------------------------------------------------
  for (const auto &snippet : masscodeJsonReparsed["snippets"]) {
    // skip deleted snippets
    if (snippet["isDeleted"])
      continue;

    std::string folderId = snippet["folderId"];
    std::string name = snippet["name"];

    // Print the snippet data
    LOG_I << "Snippet: " << name << ", Folder ID: " << folderId << std::endl;

    // Find the folderName for this folderId
    std::string folderName;
    for (const auto &folder : folderData) {
      if (folder.second == folderId) {
        folderName = folder.first;
        break;
      }
    }

    if (folderName.empty()) {
      LOG_E << "Failed to find folder for snippet: "
            << snippet["label"].get<std::string>() << std::endl;
      continue;
    }

    // Process each content in the snippet and add id of the content to the
    // end of the file name
    int id = 0;
    int fragmentId = 0;
    for (const auto &content : snippet["content"]) {
      int fragmentCount = snippet["content"].size();

      std::string label = content["label"];
      std::string value = content["value"];
      std::string language = content["language"];

      if (label.empty()) {
        LOG_E << "Empty label in snippet - skipping: "
              << snippet["name"].get<std::string>() << std::endl;
        continue;
      }

      if (value.empty()) {
        LOG_W << "Empty content in snippet - skipping: "
              << snippet["name"].get<std::string>() << std::endl;
        continue;
      }

      // Replace invalid characters in label with a valid character
      std::replace_if(
          name.begin(), name.end(),
          [](char c) {
            return !std::isalnum(c) && c != '_' && c != '-' && c != '+';
          },
          '_');

      // Create the file name depending on the id
      if (id > 0) {
        name = name + "_" + std::to_string(id);
      }

      std::string filePath = "";
      if (fragmentCount > 1) {
        filePath = out + "/" + folderName + "/" + name + "_" +
                   std::to_string(fragmentId + 1) + "_from_" +
                   std::to_string(fragmentCount) + ".md";
        fragmentId++;
      } else {
        filePath = out + "/" + folderName + "/" + name + ".md";
      }

      // check if file is existing
      if (std::filesystem::exists(filePath)) {
        LOG_E << "File already exists: " << filePath << std::endl;
        continue;
      } else {
        LOG_I << "Creating file: " << filePath << std::endl;
        std::ofstream outFile(filePath); // Open file
        if (!outFile) {
          LOG_E << "Failed to create file: " << filePath << std::endl;
          continue;
        }
        totalWrittenFiles++;

        // Change the language to cpp if it is c_cpp to match the
        // markdown language
        if (language == "c_cpp") {
          language = "cpp";
        }

        outFile << "```" << language << '\n';
        outFile << value;
        outFile << '\n' << "```" << '\n';

        outFile.close();
      }
    }
  }

  LOG_I << "Total written folders:\t" << totalWrittenFolders << std::endl;
  LOG_I << "Total written files:\t" << totalWrittenFiles << std::endl;
  LOG_I << "Parsing done!" << std::endl;
  return;
}

Mc2MdParser::~Mc2MdParser() { LOG_DEBUG("Mc2MdParser deconstructed."); }

} // namespace library