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
    const std::vector<CharacterClass>& char_classes,
    size_t length,
    PassphraseSeparator separator_type);

std::string ComposePassPhrase(
    GeneratorInterface& generator,
    std::map<WordClasses, Wordlist> &&wordlists,
    std::vector<WordClasses> &&pattern,
    PassphraseSeparator separator_type);
