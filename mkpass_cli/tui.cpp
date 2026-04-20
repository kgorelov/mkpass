#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <limits>
#include <stdexcept>
#include <algorithm>
#include <set>
#include <optional>

#if defined(_WIN32)
#include "win32_term.h"
#else
#include "posix_term.h"
#endif

#include "mkpass.h"
#include "tui.h"
#include "compose.h"
#include "context.h"
#include "character_classes.h"
#include "db.h"
#include "platform_utils.h"
#include "linenoise.h"

namespace {

std::set<std::string> service_names;

void completion(const char *buf, linenoiseCompletions *lc) {
    for (const auto &s : service_names) {
        if (s.rfind(buf, 0) == 0) {
            linenoiseAddCompletion(lc, s.c_str());
        }
    }
}

std::string AskForMasterPassword() {
    std::cerr << "Enter Master Password: ";
    std::string pwd = InputPassword();
    std::cerr << "\n";
    if (pwd.empty()) {
        throw std::runtime_error("Master password must not be empty");
    }

    std::cerr << "Repeat Master Password: ";
    std::string pwd2 = InputPassword();
    std::cerr << "\n";

    if (pwd2.empty()) {
        std::cerr << "[NO CHECK]\n";
    } else if (pwd2 == pwd) {
        std::cerr << "[CORRECT]\n";
    } else {
        throw std::runtime_error("Passwords don't match");
    }
    return pwd;
}

std::string AskForService() {
    linenoiseSetCompletionCallback(completion);
    char *service_c_str = linenoise("Service name: ");
    if (service_c_str == nullptr) {
        throw std::exception(); // Will be caught in run_tui_safe and return 130
    }
    std::string service(service_c_str);
    free(service_c_str);
    return service;
}

Algorithm AskForAlgorithm(Algorithm default_algorithm) {
    std::map<char, Algorithm> choices = {
        {'1', Algorithm::Argon2},
        {'2', Algorithm::SlowSha512},
        {'3', Algorithm::Old},
        {'4', Algorithm::Passphrase_Diceware_EFF_Large},
        {'5', Algorithm::Passphrase_Wordnet_Pattern}
    };
    std::map<Algorithm, char> algo_to_char = {
        {Algorithm::Argon2, '1'},
        {Algorithm::SlowSha512, '2'},
        {Algorithm::Old, '3'},
        {Algorithm::Passphrase_Diceware_EFF_Large, '4'},
        {Algorithm::Passphrase_Wordnet_Pattern, '5'}
    };

    std::cerr << "Choose algorithm:\n";
    std::cerr << "1. Password (Argon2)\n";
    std::cerr << "2. Password (SHA512 HMAC)\n";
    std::cerr << "3. OldPassword\n";
    std::cerr << "4. Passphrase Diceware EFF Large (Argon2)\n";
    std::cerr << "5. Passphrase Wordnet Pattern (Argon2)\n";
    std::cerr << "Your choice (e.g. 1) [" << algo_to_char[default_algorithm] << "]: ";
    std::string choice;
    std::getline(std::cin, choice);

    if (choice.empty()) {
        return default_algorithm;
    }

    if (choices.count(choice[0])) {
        return choices[choice[0]];
    }

    return Algorithm::Argon2;
}

std::vector<CharacterClass> AskForCharClasses(const std::vector<CharacterClass>& default_char_classes) {
    struct Choice {
        std::string name;
        CharacterClass value;
    };
    std::map<char, Choice> choices = {
        {'1', {"Lowercase Letters", CharacterClass::LOWERCASE}},
        {'2', {"Uppercase Letters", CharacterClass::UPPERCASE}},
        {'3', {"Digits", CharacterClass::DIGITS}},
        {'4', {"Symbols", CharacterClass::SYMBOLS}},
        {'5', {"Custom", CharacterClass::CUSTOM}}
    };
    std::map<CharacterClass, char> cc_to_char = {
        {CharacterClass::LOWERCASE, '1'},
        {CharacterClass::UPPERCASE, '2'},
        {CharacterClass::DIGITS, '3'},
        {CharacterClass::SYMBOLS, '4'},
        {CharacterClass::CUSTOM, '5'}
    };

    std::string default_choice_str;
    for (const auto& cc : default_char_classes) {
        if (cc_to_char.count(cc)) {
            default_choice_str += cc_to_char[cc];
        }
    }

    std::cerr << "Choose character classes:\n";
    for (auto const& [key, val] : choices) {
        std::cerr << key << ". " << val.name << "\n";
    }
    std::cerr << "Your choice (e.g. 123) [" << default_choice_str << "]: ";
    std::string choice;
    std::getline(std::cin, choice);

    if (choice.empty()) {
        return default_char_classes;
    }

    std::vector<CharacterClass> result;
    for (char c : choice) {
        if (choices.count(c)) {
            result.push_back(choices[c].value);
        }
    }
    return result;
}

std::optional<std::string> AskForCustomChars(const std::optional<std::string>& default_custom_chars) {
    std::string prompt = "Custom characters";
    if (default_custom_chars) {
        prompt += " [" + *default_custom_chars + "]";
    }
    prompt += ": ";

    char *custom_chars_c_str = linenoise(prompt.c_str());
    if (custom_chars_c_str == nullptr) {
        throw std::exception();
    }
    std::string custom_chars_str(custom_chars_c_str);
    free(custom_chars_c_str);

    if (custom_chars_str.empty()) {
        return default_custom_chars;
    }
    return custom_chars_str;
}

PassphraseSeparator AskForSeparator(PassphraseSeparator default_separator) {
    std::map<char, PassphraseSeparator> choices = {
        {'1', PassphraseSeparator::CamelCase},
        {'2', PassphraseSeparator::KebabCase}
    };
    std::map<PassphraseSeparator, char> sep_to_char = {
        {PassphraseSeparator::CamelCase, '1'},
        {PassphraseSeparator::KebabCase, '2'}
    };

    std::cerr << "Choose separator:\n";
    std::cerr << "1. CamelCase\n";
    std::cerr << "2. kebab-case\n";
    std::cerr << "Your choice (1 or 2) [" << sep_to_char[default_separator] << "]: ";
    std::string choice;
    std::getline(std::cin, choice);

    if (choice.empty()) {
        return default_separator;
    }

    if (choices.count(choice[0])) {
        return choices[choice[0]];
    }

    return PassphraseSeparator::KebabCase;
}

unsigned AskForLength(unsigned default_length) {
    std::string prompt = "Length";
    if (default_length > 0) {
        prompt += " [" + std::to_string(default_length) + "]";
    }
    prompt += ": ";

    char *length_c_str = linenoise(prompt.c_str());
    if (length_c_str == nullptr) {
        throw std::exception();
    }
    std::string length_str(length_c_str);
    free(length_c_str);

    if (length_str.empty()) {
        if (default_length == 0) {
            throw std::runtime_error("Length must be specified");
        }
        return default_length;
    }
    try {
        return std::stoul(length_str);
    } catch (...) {
        throw std::runtime_error("Invalid length: " + length_str);
    }
}

bool IsPasswordAlgo(Algorithm a) {
    return a == Algorithm::Argon2 || a == Algorithm::SlowSha512;
}

void HandlePasswordAlgo(Context& ctx, const std::optional<mkpass::ServiceEntry>& db_entry) {
    std::vector<CharacterClass> default_char_classes = {
        CharacterClass::LOWERCASE,
        CharacterClass::UPPERCASE,
        CharacterClass::DIGITS,
        CharacterClass::SYMBOLS
    };
    if (db_entry && IsPasswordAlgo(db_entry->algorithm) && !db_entry->char_classes.empty()) {
        default_char_classes = db_entry->char_classes;
    }
    ctx.char_classes = AskForCharClasses(default_char_classes);

    if (std::find(ctx.char_classes.begin(), ctx.char_classes.end(), CharacterClass::CUSTOM) != ctx.char_classes.end()) {
        std::optional<std::string> default_custom_chars;
        if (db_entry && IsPasswordAlgo(db_entry->algorithm)) {
            default_custom_chars = db_entry->custom_chars;
        }
        ctx.custom_chars = AskForCustomChars(default_custom_chars);
    }

    unsigned default_length = 16;
    if (db_entry && IsPasswordAlgo(db_entry->algorithm) && db_entry->length > 0) {
        default_length = db_entry->length;
    }
    ctx.length = AskForLength(default_length);
}

void HandlePassphraseDicewareAlgo(Context& ctx, const std::optional<mkpass::ServiceEntry>& db_entry) {
    unsigned default_length = 6;
    if (db_entry && db_entry->algorithm == Algorithm::Passphrase_Diceware_EFF_Large && db_entry->length > 0) {
        default_length = db_entry->length;
    }
    ctx.length = AskForLength(default_length);

    PassphraseSeparator default_separator = PassphraseSeparator::KebabCase;
    if (db_entry && db_entry->algorithm == Algorithm::Passphrase_Diceware_EFF_Large) {
        default_separator = db_entry->separator;
    }
    ctx.separator = AskForSeparator(default_separator);
}

void HandlePassphraseWordnetPatternAlgo(Context& ctx, const std::optional<mkpass::ServiceEntry>& db_entry) {
    // Wordnet Pattern currently doesn't use configurable length
    ctx.length = 0;

    PassphraseSeparator default_separator = PassphraseSeparator::KebabCase;
    if (db_entry && db_entry->algorithm == Algorithm::Passphrase_Wordnet_Pattern) {
        default_separator = db_entry->separator;
    }
    ctx.separator = AskForSeparator(default_separator);
}

void HandleOldAlgo(Context& ctx, const std::optional<mkpass::ServiceEntry>& db_entry) {
    unsigned default_length = 8;
    if (db_entry && db_entry->algorithm == Algorithm::Old && db_entry->length > 0) {
        default_length = db_entry->length;
    }
    ctx.length = AskForLength(default_length);
}
} // namespace

int run_tui_safe() {
    try {
        return run_tui();
    } catch (const std::runtime_error &e) {
        std::cerr << "ERROR! " << e.what() << std::endl;
        return 1;
    } catch (const std::exception &e) {
        // This happens on Ctrl+C (when we throw std::exception())
        return 130;
    }
}

int run_tui() {
    mkpass::ConfigDB db(GetConfigDBPath());
    service_names = db.get_all_service_names();

    std::string pwd = AskForMasterPassword();
    std::string service = AskForService();

    auto db_entry = db.get_service_entry(service);

    Algorithm default_algorithm = db_entry ? db_entry->algorithm : Algorithm::Argon2;
    Algorithm algorithm = AskForAlgorithm(default_algorithm);

    Context ctx = {
        .password = pwd,
        .service = service,
        .algorithm = algorithm
    };

    switch (algorithm) {
        case Algorithm::Argon2:
        case Algorithm::SlowSha512:
            HandlePasswordAlgo(ctx, db_entry);
            break;
        case Algorithm::Old:
            HandleOldAlgo(ctx, db_entry);
            break;
        case Algorithm::Passphrase_Diceware_EFF_Large:
            HandlePassphraseDicewareAlgo(ctx, db_entry);
            break;
        case Algorithm::Passphrase_Wordnet_Pattern:
            HandlePassphraseWordnetPatternAlgo(ctx, db_entry);
            break;
    }

    std::cout << MkPass(ctx) << std::endl;

    db.save_service_entry({service, ctx.algorithm, ctx.length, ctx.char_classes, ctx.custom_chars, ctx.separator});

    return 0;
}
