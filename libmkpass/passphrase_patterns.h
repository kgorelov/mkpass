#pragma once

#include <vector>
#include "word_classes.h"

using PassphrasePattern = std::vector<WordClasses>;
using PatternsList = std::vector<PassphrasePattern>;

PatternsList GetPassphrasePatterns(int length);
int GetMaxPassphrasePatternLength();
