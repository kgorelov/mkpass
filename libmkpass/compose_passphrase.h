#pragma once

#include <string>
#include <vector>
#include <map>
#include "character_classes.h"
#include "wordlist.h"
#include "generator.h"


std::string ComposePassPhrase(
    GeneratorInterface& generator,
    const Wordlist& wordlist,
    size_t length,
    const std::string& separator_str,
    const std::vector<CharacterClass>& char_classes,
    bool allow_substitutions,
    bool capitalize_words);

std::string ComposePassPhrase(
    GeneratorInterface& generator,
    std::map<WordClasses, Wordlist> wordlists,
    std::vector<WordClasses> pattern,
    size_t length,
    const std::string& separator_str,
    const std::vector<CharacterClass>& char_classes,
    bool allow_substitutions,
    bool capitalize_words);
