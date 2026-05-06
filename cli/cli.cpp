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
#include "cli.h"
#include "compose_password.h"
#include "context.h"
#include "character_classes.h"
#include "db.h"
#include "platform_utils.h"
#include "linenoise.h"
#include "passphrase_patterns.h"
#include "CLI11.hpp"

namespace {

struct CliOptions {
    std::optional<std::string> password;
    std::optional<std::string> service;
    std::optional<std::string> algorithm;
    std::optional<std::string> char_classes;
    std::optional<std::string> custom_chars;
    std::optional<unsigned int> length;
    std::optional<std::string> separator;
    std::optional<std::string> passphrase_pattern;
    std::optional<bool> digits;
    std::optional<bool> symbols;
    std::optional<bool> substitutions;
    std::optional<bool> capitalize;
};

CliOptions global_options;

std::optional<std::string> GetEnv(const std::string& name) {
    const char* val = std::getenv(name.c_str());
    if (val && *val) {
        return std::string(val);
    }
    return std::nullopt;
}

bool ParseBoolEnv(const std::string& val) {
    std::string s = val;
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s == "y" || s == "yes" || s == "1" || s == "true" || s == "on";
}

std::set<std::string> service_names;

void completion(const char *buf, linenoiseCompletions *lc) {
    for (const auto &s : service_names) {
        if (s.rfind(buf, 0) == 0) {
            linenoiseAddCompletion(lc, s.c_str());
        }
    }
}

std::string AskForMasterPassword() {
    if (global_options.password) {
        return *global_options.password;
    }

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
    if (global_options.service) {
        return *global_options.service;
    }

    linenoiseSetCompletionCallback(completion);
    char *service_c_str = linenoise("Service name: ");
    if (service_c_str == nullptr) {
        throw std::exception(); // Will be caught in run_cli_safe and return 130
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

    if (global_options.algorithm) {
        if (choices.count((*global_options.algorithm)[0])) {
            return choices[(*global_options.algorithm)[0]];
        }
    }

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

    std::string choice;
    if (global_options.char_classes) {
        choice = *global_options.char_classes;
    } else {
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
        std::getline(std::cin, choice);
    }

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
    if (global_options.custom_chars) {
        return global_options.custom_chars;
    }

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

std::string AskForSeparator(const std::string& default_separator) {
    std::map<char, std::string> choices = {
        {'1', ""},
        {'2', "-"},
        {'3', " "},
        {'4', "/"}
    };
    std::map<std::string, char> sep_to_char = {
        {"", '1'},
        {"-", '2'},
        {" ", '3'},
        {"/", '4'}
    };

    std::string choice;
    if (global_options.separator) {
        choice = *global_options.separator;
    } else {
        std::cerr << "Choose separator:\n";
        std::cerr << "1. None\n";
        std::cerr << "2. Hyphen (-)\n";
        std::cerr << "3. Space ( )\n";
        std::cerr << "4. Slash (/)\n";

        char dflt_char = '1'; // Default to None
        if (sep_to_char.count(default_separator)) {
            dflt_char = sep_to_char[default_separator];
        }

        std::cerr << "Your choice (1-4) [" << dflt_char << "]: ";
        std::getline(std::cin, choice);
    }

    if (choice.empty()) {
        if (default_separator.empty() && (!global_options.separator || (global_options.separator && (*global_options.separator).empty()))) return "";
        return default_separator.empty() ? "" : default_separator;
    }

    if (choices.count(choice[0])) {
        return choices[choice[0]];
    }

    if (global_options.separator) {
        return *global_options.separator;
    }

    return "-";
}

std::vector<WordClasses> AskForPassphrasePattern(int length, const std::vector<WordClasses>& default_pattern) {
    PatternsList patterns = GetPassphrasePatterns(length);

    std::string choice;
    if (global_options.passphrase_pattern) {
        choice = *global_options.passphrase_pattern;
    } else {
        std::cerr << "Choose pattern:\n";
        std::cerr << "1. Random\n";

        for (size_t i = 0; i < patterns.size(); ++i) {
            std::cerr << (i + 2) << ". " << mkpass::PatternToString(patterns[i]) << "\n";
        }
        std::cerr << "c. Custom pattern (e.g. 'navrn')\n";

        std::string default_choice_str = "1";
        if (!default_pattern.empty()) {
            for (size_t i = 0; i < patterns.size(); ++i) {
                if (patterns[i] == default_pattern) {
                    default_choice_str = std::to_string(i + 2);
                    break;
                }
            }
            if (default_choice_str == "1") {
                default_choice_str = "c (" + mkpass::PatternToString(default_pattern) + ")";
            }
        }

        std::cerr << "Your choice (1-" << (patterns.size() + 1) << " or c) [" << default_choice_str << "]: ";
        std::getline(std::cin, choice);
    }

    if (choice.empty()) {
        return default_pattern;
    }

    if (choice == "1") {
        return {};
    }

    try {
        size_t idx = std::stoul(choice);
        if (idx >= 2 && idx <= patterns.size() + 1) {
            return patterns[idx - 2];
        }
    } catch (...) {
    }

    if (choice[0] == 'c') {
        std::string custom_pattern;
        if (global_options.passphrase_pattern && choice.length() > 1) {
            custom_pattern = choice.substr(1);
        } else {
            std::cerr << "Enter custom pattern (n:noun, v:verb, a:adj, r:adv): ";
            std::getline(std::cin, custom_pattern);
        }

        if (custom_pattern.empty()) {
            return default_pattern;
        }
        return mkpass::StringToPattern(custom_pattern);
    }

    if (global_options.passphrase_pattern) {
        return mkpass::StringToPattern(*global_options.passphrase_pattern);
    }

    return default_pattern;
}

unsigned AskForLength(unsigned default_length) {
    std::string length_str;
    if (global_options.length) {
        return *global_options.length;
    } else {
        std::string prompt = "Length";
        if (default_length > 0) {
            prompt += " [" + std::to_string(default_length) + "]";
        }
        prompt += ": ";

        char *length_c_str = linenoise(prompt.c_str());
        if (length_c_str == nullptr) {
            throw std::exception();
        }
        length_str = std::string(length_c_str);
        free(length_c_str);
    }

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

bool AskYesNoQuestion(const std::string& question, bool dflt, const std::optional<bool>& opt) {
    if (opt) {
        return *opt;
    }

    std::cerr << question
              << " (y/n) [" << (dflt ? "y" : "n") << "]: ";
    std::string choice;
    std::getline(std::cin, choice);
    if (choice.empty()) {
        return dflt;
    }
    if (std::tolower(choice[0]) == 'y') {
        return true;
    }
    return false;
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
    bool same_algo = db_entry && db_entry->algorithm == ctx.algorithm;

    ctx.length = AskForLength(same_algo && db_entry->length > 0 ? db_entry->length : 3);

    auto ask_and_add = [&](const std::string& question, CharacterClass cls, const std::optional<bool>& opt) {
        bool dflt = same_algo ? std::ranges::count(db_entry->char_classes, cls) > 0 : false;
        if (AskYesNoQuestion(question, dflt, opt)) {
            ctx.char_classes.push_back(cls);
        }
    };

    ask_and_add("Include digits?", CharacterClass::DIGITS, global_options.digits);
    ask_and_add("Include symbols?", CharacterClass::SYMBOLS, global_options.symbols);

    bool has_digits_or_symbols = false;
    for (auto cc : ctx.char_classes) {
        if (cc == CharacterClass::DIGITS || cc == CharacterClass::SYMBOLS) {
            has_digits_or_symbols = true;
            break;
        }
    }

    if (has_digits_or_symbols) {
        ctx.allow_substitutions = AskYesNoQuestion(
            "Allow character substitutions (e.g. a -> 4, s -> $)?",
            same_algo ? db_entry->allow_substitutions : false,
            global_options.substitutions);
    } else {
        ctx.allow_substitutions = false;
    }

    ctx.capitalize_words = AskYesNoQuestion(
        "Capitalize words?",
        same_algo ? db_entry->capitalize_words : true,
        global_options.capitalize);

    ctx.separator = AskForSeparator(same_algo ? db_entry->separator : "");
}

void HandlePassphraseWordnetPatternAlgo(
    Context& ctx,
    const std::optional<mkpass::ServiceEntry>& db_entry)
{
    bool same_algo = db_entry && db_entry->algorithm == ctx.algorithm;

    unsigned default_length = same_algo && db_entry->length > 0 ? db_entry->length : 3;
    ctx.length = AskForLength(default_length);

    if (ctx.length > GetMaxPassphrasePatternLength()) {
        ctx.length = GetMaxPassphrasePatternLength();
        std::cerr << "Length capped to " << ctx.length << "\n";
    }

    if (ctx.length < 1) {
        ctx.length = 1;
        std::cerr << "Length set to 1\n";
    }

    std::vector<WordClasses> default_pattern = same_algo ? db_entry->passphrase_pattern : std::vector<WordClasses>{};

    ctx.passphrase_pattern = AskForPassphrasePattern(ctx.length, default_pattern);

    auto ask_and_add = [&](const std::string& question, CharacterClass cls, const std::optional<bool>& opt) {
        bool dflt = same_algo ? std::ranges::count(db_entry->char_classes, cls) > 0 : false;
        if (AskYesNoQuestion(question, dflt, opt)) {
            ctx.char_classes.push_back(cls);
        }
    };

    ask_and_add("Include digits?", CharacterClass::DIGITS, global_options.digits);
    ask_and_add("Include symbols?", CharacterClass::SYMBOLS, global_options.symbols);

    bool has_digits_or_symbols = false;
    for (auto cc : ctx.char_classes) {
        if (cc == CharacterClass::DIGITS || cc == CharacterClass::SYMBOLS) {
            has_digits_or_symbols = true;
            break;
        }
    }

    if (has_digits_or_symbols) {
        ctx.allow_substitutions = AskYesNoQuestion(
            "Allow character substitutions (e.g. a -> 4, s -> $)?",
            same_algo ? db_entry->allow_substitutions : false,
            global_options.substitutions);
    } else {
        ctx.allow_substitutions = false;
    }

    ctx.capitalize_words = AskYesNoQuestion(
        "Capitalize words?",
        same_algo ? db_entry->capitalize_words : true,
        global_options.capitalize);

    ctx.separator = AskForSeparator(same_algo ? db_entry->separator : "");
}

void HandleOldAlgo(
    Context& ctx,
    const std::optional<mkpass::ServiceEntry>& db_entry)
{
    unsigned default_length = 8;
    if (db_entry && db_entry->algorithm == Algorithm::Old && db_entry->length > 0) {
        default_length = db_entry->length;
    }
    ctx.length = AskForLength(default_length);
}
} // namespace

int run_cli_safe(int argc, char *argv[]) {
    try {
        return run_cli(argc, argv);
    } catch (const std::runtime_error &e) {
        std::cerr << "ERROR! " << e.what() << std::endl;
        return 1;
    } catch (const std::exception &e) {
        // This happens on Ctrl+C (when we throw std::exception())
        return 130;
    }
}

int run_cli(int argc, char *argv[]) {
    CLI::App app{"mkpass - Command-line password generator"};

    app.add_option("-p,--password", global_options.password, "Master password")->envname("MKPASS_PASSWORD");
    app.add_option("-s,--service", global_options.service, "Service name")->envname("MKPASS_SERVICE");
    app.add_option("-a,--algorithm", global_options.algorithm, "Algorithm (1-5)")->envname("MKPASS_ALGORITHM");
    app.add_option("-c,--char-classes", global_options.char_classes, "Character classes (e.g. 12345)")->envname("MKPASS_CHAR_CLASSES");
    app.add_option("--custom-chars", global_options.custom_chars, "Custom characters")->envname("MKPASS_CUSTOM_CHARS");
    app.add_option("-l,--length", global_options.length, "Password length")->envname("MKPASS_LENGTH");
    app.add_option("--separator", global_options.separator, "Separator for passphrases")->envname("MKPASS_SEPARATOR");
    app.add_option("--pattern", global_options.passphrase_pattern, "Passphrase pattern")->envname("MKPASS_PASSPHRASE_PATTERN");
    app.add_option("--digits", global_options.digits, "Include digits in passphrase (y/n)")->envname("MKPASS_DIGITS");
    app.add_option("--symbols", global_options.symbols, "Include symbols in passphrase (y/n)")->envname("MKPASS_SYMBOLS");
    app.add_option("--substitutions", global_options.substitutions, "Allow character substitutions (y/n)")->envname("MKPASS_SUBSTITUTIONS");
    app.add_option("--capitalize", global_options.capitalize, "Capitalize words (y/n)")->envname("MKPASS_CAPITALIZE");

    try {
        app.parse(argc, argv);
    } catch (const CLI::ParseError &e) {
        return app.exit(e);
    }

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

    db.save_service_entry({service, ctx.algorithm, ctx.length, ctx.char_classes, ctx.custom_chars, ctx.separator, ctx.passphrase_pattern, ctx.allow_substitutions, ctx.capitalize_words});

    return 0;
}
