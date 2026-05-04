#pragma once
#include <string>
#include <vector>
#include <map>
#include "character_classes.h"
#include "wordlist.h"

#include "algorithms.h"

class GeneratorInterface;

std::string ComposePassword(
    GeneratorInterface& generator,
    std::vector<std::string>&& char_classes,
    size_t length);

std::string ComposeOldMkpass1Password(
    const std::string& master_password,
    const std::string& service_name,
    size_t length);

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
