#pragma once

#include <string>
#include <vector>
#include <map>

#include "eff_large_words.h"

class Wordlist
{
public:
  Wordlist() {
  }

  int length() const {
      return eff_large_get_word_count();
  }

  std::string operator[](int idx) const {
      return eff_large_get_word(idx);
  }
};
