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

#include <algorithm>
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
// Transliterate common Czech/Slovak (and wider Latin-extended) diacritics
// to ASCII base equivalents. Processes UTF-8 input.
// ---------------------------------------------------------------------------
static std::string transliterate (const std::string& s) {
  struct Entry { unsigned char b1, b2; char rep; };
  static const Entry table[] = {
    // a
    {0xC3,0xA1,'a'},{0xC3,0x81,'A'}, // á Á
    {0xC3,0xA0,'a'},{0xC3,0x80,'A'}, // à À
    {0xC3,0xA2,'a'},{0xC3,0x82,'A'}, // â Â
    {0xC3,0xA4,'a'},{0xC3,0x84,'A'}, // ä Ä
    {0xC3,0xA3,'a'},{0xC3,0x83,'A'}, // ã Ã
    {0xC3,0xA5,'a'},{0xC3,0x85,'A'}, // å Å
    // c
    {0xC4,0x8D,'c'},{0xC4,0x8C,'C'}, // č Č
    {0xC4,0x87,'c'},{0xC4,0x86,'C'}, // ć Ć
    {0xC3,0xA7,'c'},{0xC3,0x87,'C'}, // ç Ç
    // d
    {0xC4,0x8F,'d'},{0xC4,0x8E,'D'}, // ď Ď
    // e
    {0xC3,0xA9,'e'},{0xC3,0x89,'E'}, // é É
    {0xC3,0xA8,'e'},{0xC3,0x88,'E'}, // è È
    {0xC3,0xAA,'e'},{0xC3,0x8A,'E'}, // ê Ê
    {0xC3,0xAB,'e'},{0xC3,0x8B,'E'}, // ë Ë
    {0xC4,0x9B,'e'},{0xC4,0x9A,'E'}, // ě Ě
    // i
    {0xC3,0xAD,'i'},{0xC3,0x8D,'I'}, // í Í
    {0xC3,0xAC,'i'},{0xC3,0x8C,'I'}, // ì Ì
    {0xC3,0xAE,'i'},{0xC3,0x8E,'I'}, // î Î
    {0xC3,0xAF,'i'},{0xC3,0x8F,'I'}, // ï Ï
    // l
    {0xC4,0xBE,'l'},{0xC4,0xBD,'L'}, // ľ Ľ
    {0xC4,0xBA,'l'},{0xC4,0xB9,'L'}, // ĺ Ĺ
    {0xC5,0x82,'l'},{0xC5,0x81,'L'}, // ł Ł
    // n
    {0xC5,0x88,'n'},{0xC5,0x87,'N'}, // ň Ň
    {0xC3,0xB1,'n'},{0xC3,0x91,'N'}, // ñ Ñ
    // o
    {0xC3,0xB3,'o'},{0xC3,0x93,'O'}, // ó Ó
    {0xC3,0xB2,'o'},{0xC3,0x92,'O'}, // ò Ò
    {0xC3,0xB4,'o'},{0xC3,0x94,'O'}, // ô Ô
    {0xC3,0xB6,'o'},{0xC3,0x96,'O'}, // ö Ö
    {0xC3,0xB5,'o'},{0xC3,0x95,'O'}, // õ Õ
    {0xC5,0x91,'o'},{0xC5,0x90,'O'}, // ő Ő
    // r
    {0xC5,0x99,'r'},{0xC5,0x98,'R'}, // ř Ř
    // s
    {0xC5,0xA1,'s'},{0xC5,0xA0,'S'}, // š Š
    {0xC5,0x9B,'s'},{0xC5,0x9A,'S'}, // ś Ś
    // t
    {0xC5,0xA5,'t'},{0xC5,0xA4,'T'}, // ť Ť
    // u
    {0xC3,0xBA,'u'},{0xC3,0x9A,'U'}, // ú Ú
    {0xC3,0xB9,'u'},{0xC3,0x99,'U'}, // ù Ù
    {0xC3,0xBB,'u'},{0xC3,0x9B,'U'}, // û Û
    {0xC3,0xBC,'u'},{0xC3,0x9C,'U'}, // ü Ü
    {0xC5,0xAF,'u'},{0xC5,0xAE,'U'}, // ů Ů
    {0xC5,0xB1,'u'},{0xC5,0xB0,'U'}, // ű Ű
    // y
    {0xC3,0xBD,'y'},{0xC3,0x9D,'Y'}, // ý Ý
    // z
    {0xC5,0xBE,'z'},{0xC5,0xBD,'Z'}, // ž Ž
    {0xC5,0xBA,'z'},{0xC5,0xB9,'Z'}, // ź Ź
    {0xC5,0xBC,'z'},{0xC5,0xBB,'Z'}, // ż Ż
  };

  std::string result;
  result.reserve (s.size ());
  for (size_t i = 0; i < s.size (); ) {
    const auto c = static_cast<unsigned char> (s[i]);
    if ((c & 0xE0) == 0xC0 && i + 1 < s.size ()) {
      const auto c2 = static_cast<unsigned char> (s[i + 1]);
      char rep = '\0';
      for (const auto& e : table)
        if (e.b1 == c && e.b2 == c2) { rep = e.rep; break; }
      if (rep) { result += rep; i += 2; continue; }
    }
    result += s[i++];
  }
  return result;
}

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

    // Transliterate diacritics → ASCII before sanitising
    name = transliterate (name);

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

    // Transliterate diacritics → ASCII before sanitising
    snippetName = transliterate (snippetName);

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
