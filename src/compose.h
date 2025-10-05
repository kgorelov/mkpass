#pragma once
#include "generator.h"

const std::string LowercaseLetters = "abcdefghijklmnopqrstuvwxyz";
const std::string UppercaseLetters = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
const std::string Digits = "0123456789";
const std::string Symbols = "!@#$%^&*()-_=+[]{};:,.<>?/";


std::string ComposePassword(
    Generator& generator,
    std::vector<std::string>&& char_classes,
    size_t length);
