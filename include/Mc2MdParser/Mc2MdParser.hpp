#ifndef __MC2MDPARSER_H__
#define __MC2MDPARSER_H__

// MIT License
// Copyright (c) 2024-2025 Tomáš Mark

#include <string>

// Public API

namespace library {

class Mc2MdParser {
public:
  Mc2MdParser(const std::string &assetsPath, std::string in, std::string out);
  ~Mc2MdParser();

  // alternatively, you can use a getter function
  const std::string getAssetsPath() const { return m_assetsPath; }

private:
  std::string m_assetsPath;
  std::string m_input;
  std::string m_output;
  
};

} // namespace library

#endif // __MC2MDPARSER_H__
