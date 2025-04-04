// MIT License
// Copyright (c) 2024-2025 Tomáš Mark

#ifndef __MC2MDPARSER_HPP
#define __MC2MDPARSER_HPP

#include <Mc2MdParser/version.h>
#include <filesystem>
#include <string>

// Public API

namespace dotname {

  class Mc2MdParser {

    const std::string libName_ = std::string ("Mc2MdParser v.") + MC2MDPARSER_VERSION;
    std::filesystem::path assetsPath_;
    std::string input_;
    std::string output_;

  public:
    Mc2MdParser ();
    Mc2MdParser (const std::filesystem::path& assetsPath, std::string in, std::string out);
    ~Mc2MdParser ();

    const std::filesystem::path getAssetsPath () const {
      return assetsPath_;
    }
    void setAssetsPath (const std::filesystem::path& assetsPath) {
      assetsPath_ = assetsPath;
    }
  };

} // namespace dotname

#endif // __MC2MDPARSER_HPP