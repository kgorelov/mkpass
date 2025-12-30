#pragma once
#include <string>
#include <vector>
#include "character_classes.h"

class GeneratorInterface;

std::string ComposePassword(
    GeneratorInterface& generator,
    std::vector<std::string>&& char_classes,
    size_t length);

std::string ComposeOldMkpass1Password(
    const std::string& master_password,
    const std::string& service_name,
    size_t length);
