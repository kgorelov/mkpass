#pragma once

#include <cstddef>
#include <vector>
#include "word_classes.h"

using PassphrasePattern = std::vector<WordClasses>;
using PatternsList = std::vector<PassphrasePattern>;

PatternsList GetPassphrasePatterns(size_t length);
size_t GetMaxPassphrasePatternLength();
