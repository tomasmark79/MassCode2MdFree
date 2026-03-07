// MIT License
// Copyright (c) 2024-2025 Tomáš Mark
//
// mc2md.cpp — standalone single-file MassCode → Markdown exporter
//
// Compile (requires nlohmann/json in include path):
//   g++ -std=c++17 -o mc2md mc2md.cpp
//   # or with explicit header path:
//   g++ -std=c++17 -I/path/to/nlohmann -o mc2md mc2md.cpp
//
// Usage:
//   ./mc2md -i db.json -o output_dir
//   ./mc2md --help

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

// nlohmann/json — single-header, available at https://github.com/nlohmann/json
#include <nlohmann/json.hpp>

namespace fs = std::filesystem;

// ---------------------------------------------------------------------------
// Minimal logger helpers (replace full Logger framework)
// ---------------------------------------------------------------------------
#define LOG_I(msg) std::cout  << "[INFO]  " << msg << "\n"
#define LOG_W(msg) std::cout  << "[WARN]  " << msg << "\n"
#define LOG_E(msg) std::cerr  << "[ERROR] " << msg << "\n"

// ---------------------------------------------------------------------------
// Core conversion logic
// ---------------------------------------------------------------------------

/**
 * Parse a MassCode db.json file and export each snippet fragment as a
 * Markdown code-block file under outputDir/<FolderName>/<SnippetName>.md
 *
 * @param inputFile  path to MassCode db.json
 * @param outputDir  root output directory (created if absent)
 * @return 0 on success, non-zero on error
 */
int convert (const std::string& inputFile, const std::string& outputDir) {

  // --- Read JSON -----------------------------------------------------------
  std::ifstream ifs (inputFile);
  if (!ifs) {
    LOG_E ("Cannot open input file: " << inputFile);
    return 1;
  }

  std::string jsonContent ((std::istreambuf_iterator<char> (ifs)),
                            std::istreambuf_iterator<char> ());
  ifs.close ();

  nlohmann::json db;
  try {
    db = nlohmann::json::parse (jsonContent);
  } catch (const std::exception& e) {
    LOG_E ("JSON parse error: " << e.what ());
    return 1;
  }

  // --- Ensure output root exists -------------------------------------------
  if (!fs::exists (outputDir)) {
    if (!fs::create_directories (outputDir)) {
      LOG_E ("Failed to create output directory: " << outputDir);
      return 1;
    }
  }

  // --- Build folderId → sanitised folderName map ---------------------------
  // pair: (sanitisedName, rawId)
  std::vector<std::pair<std::string, std::string>> folderData;

  for (const auto& folder : db["folders"]) {
    std::string name = folder["name"].get<std::string> ();
    std::string id   = folder["id"].get<std::string> ();

    LOG_I ("Folder: " << name << "  (id=" << id << ")");

    // Sanitise: keep alphanumeric, '_', '-', '+'
    std::replace_if (name.begin (), name.end (),
                     [] (char c) {
                       return !std::isalnum (static_cast<unsigned char> (c))
                              && c != '_' && c != '-' && c != '+';
                     },
                     '_');

    std::string folderPath = outputDir + "/" + name;
    if (!fs::exists (folderPath)) {
      if (!fs::create_directory (folderPath)) {
        LOG_E ("Failed to create folder: " << folderPath);
        return 1;
      }
    }

    folderData.emplace_back (name, id);
  }

  // --- Process snippets ----------------------------------------------------
  int writtenFiles   = 0;
  int skippedFiles   = 0;

  for (const auto& snippet : db["snippets"]) {

    // Skip deleted snippets
    if (snippet.value ("isDeleted", false))
      continue;

    std::string folderId    = snippet["folderId"].get<std::string> ();
    std::string snippetName = snippet["name"].get<std::string> ();

    LOG_I ("Snippet: " << snippetName << "  (folder=" << folderId << ")");

    // Resolve folder name
    std::string folderName;
    for (const auto& fd : folderData) {
      if (fd.second == folderId) {
        folderName = fd.first;
        break;
      }
    }

    if (folderName.empty ()) {
      LOG_E ("No matching folder for snippet \"" << snippetName
             << "\" (folderId=" << folderId << ") — skipping");
      continue;
    }

    // Sanitise snippet name
    std::replace_if (snippetName.begin (), snippetName.end (),
                     [] (char c) {
                       return !std::isalnum (static_cast<unsigned char> (c))
                              && c != '_' && c != '-' && c != '+';
                     },
                     '_');

    // Each snippet may have multiple content fragments
    const auto& contents      = snippet["content"];
    const int   fragmentCount = static_cast<int> (contents.size ());
    int         fragmentIdx   = 0;

    for (const auto& content : contents) {

      std::string label    = content.value ("label", "");
      std::string value    = content.value ("value", "");
      std::string language = content.value ("language", "");

      if (label.empty ()) {
        LOG_W ("Empty label in snippet \"" << snippetName << "\" — skipping fragment");
        ++fragmentIdx;
        continue;
      }

      if (value.empty ()) {
        LOG_W ("Empty value in snippet \"" << snippetName << "\" — skipping fragment");
        ++fragmentIdx;
        continue;
      }

      // Build output file path
      std::string filePath;
      if (fragmentCount > 1) {
        filePath = outputDir + "/" + folderName + "/" + snippetName
                   + "_" + std::to_string (fragmentIdx + 1)
                   + "_from_" + std::to_string (fragmentCount) + ".md";
      } else {
        filePath = outputDir + "/" + folderName + "/" + snippetName + ".md";
      }

      if (fs::exists (filePath)) {
        LOG_W ("File already exists, skipping: " << filePath);
        ++skippedFiles;
        ++fragmentIdx;
        continue;
      }

      // Normalise language identifier
      if (language == "c_cpp")
        language = "cpp";

      // Write Markdown code-block
      std::ofstream out (filePath);
      if (!out) {
        LOG_E ("Failed to create file: " << filePath);
        ++fragmentIdx;
        continue;
      }

      out << "```" << language << "\n";
      out << value;
      out << "\n```\n";
      out.close ();

      LOG_I ("  Written: " << filePath);
      ++writtenFiles;
      ++fragmentIdx;
    }
  }

  LOG_I ("---");
  LOG_I ("Written : " << writtenFiles << " file(s)");
  LOG_I ("Skipped : " << skippedFiles << " file(s)  (already existed)");
  LOG_I ("Done.");
  return 0;
}

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------

static void printHelp (const char* prog) {
  std::cout
      << "Usage: " << prog << " -i <db.json> -o <output_dir>\n"
      << "\n"
      << "Options:\n"
      << "  -i, --input   <file>   MassCode database file (db.json)\n"
      << "  -o, --output  <dir>    Output directory for Markdown files\n"
      << "  -h, --help             Show this help and exit\n";
}

int main (int argc, char* argv[]) {

  std::string inputFile;
  std::string outputDir;

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];

    if (arg == "-h" || arg == "--help") {
      printHelp (argv[0]);
      return 0;
    } else if ((arg == "-i" || arg == "--input") && i + 1 < argc) {
      inputFile = argv[++i];
    } else if ((arg == "-o" || arg == "--output") && i + 1 < argc) {
      outputDir = argv[++i];
    } else {
      std::cerr << "Unknown option: " << arg << "\n";
      printHelp (argv[0]);
      return 1;
    }
  }

  if (inputFile.empty () || outputDir.empty ()) {
    std::cerr << "Both -i and -o are required.\n\n";
    printHelp (argv[0]);
    return 1;
  }

  return convert (inputFile, outputDir);
}
