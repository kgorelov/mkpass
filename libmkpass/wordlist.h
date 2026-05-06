#pragma once

#include <string>
#include <vector>
#include <map>

#include "word_classes.h"


typedef const char* (*GetWordFuncType)(int);
typedef int (*GetWordCountFuncType)();


class Wordlist
{
public:
  Wordlist(GetWordFuncType get_word, GetWordCountFuncType get_word_count)
    : get_word_(get_word)
    , get_word_count_(get_word_count)
  {
  }

  int length() const {
      return get_word_count_();
  }

  std::string operator[](int idx) const {
      return get_word_(idx);
  }
private:
  GetWordFuncType get_word_;
  GetWordCountFuncType get_word_count_;
};
