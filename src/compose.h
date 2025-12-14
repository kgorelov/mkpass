#pragma once
#include <string>
#include <vector>

class GeneratorInterface;

const std::string LowercaseLetters = "abcdefghijklmnopqrstuvwxyz";
const std::string UppercaseLetters = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
const std::string Digits = "0123456789";
const std::string Symbols = "!@#$%^&*()-_=+[]{};:,.<>?/";


std::string ComposePassword(
    GeneratorInterface& generator,
    std::vector<std::string>&& char_classes,
    size_t length);

std::string ComposeOldMkpass1Password(
    const std::string& master_password,
    const std::string& service_name,
    size_t length);
