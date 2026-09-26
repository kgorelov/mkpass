#include "config.h"
#include "platform_utils.h"
#include "db.h"
#include "toml.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <array>
#include <cctype>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace mkpass {

namespace {

std::string_view Trim(std::string_view str) {
    auto start = std::find_if_not(str.begin(), str.end(), [](unsigned char ch) { return std::isspace(ch); });
    auto end = std::find_if_not(str.rbegin(), str.rend(), [](unsigned char ch) { return std::isspace(ch); }).base();
    return (start < end) ? std::string_view(&*start, static_cast<size_t>(end - start)) : std::string_view{};
}

std::string ToLower(std::string_view str) {
    std::string s(str);
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
    return s;
}

std::optional<bool> ParseBool(std::string_view str) {
    std::string s = ToLower(Trim(str));
    if (s == "true" || s == "1" || s == "yes" || s == "y") return true;
    if (s == "false" || s == "0" || s == "no" || s == "n") return false;
    return std::nullopt;
}

std::optional<size_t> ParseLength(std::string_view val) {
    std::string_view v = Trim(val);
    if (v.empty() || v[0] == '-' || !std::all_of(v.begin(), v.end(), [](unsigned char c) { return std::isdigit(c); })) {
        return std::nullopt;
    }
    try {
        size_t pos = 0;
        unsigned long len = std::stoul(std::string(v), &pos);
        if (pos != v.size() || len == 0) {
            return std::nullopt;
        }
        return static_cast<size_t>(len);
    } catch (...) {
        return std::nullopt;
    }
}

std::optional<std::vector<WordClasses>> ParsePassphrasePattern(std::string_view val) {
    std::string v = ToLower(Trim(val));
    if (v.empty() || v == "random" || v == "1") {
        return std::vector<WordClasses>{};
    }
    for (char c : v) {
        if (c != 'n' && c != 'v' && c != 'a' && c != 'r') {
            return std::nullopt;
        }
    }
    return StringToPattern(v);
}

struct Property {
    std::string_view name;
    std::string_view alias = "";
    std::string_view default_str;

    constexpr bool matches(std::string_view key) const {
        return key == name || (!alias.empty() && key == alias);
    }

    std::optional<std::string> (*get_raw)(const ConfigOptions& opts);
    void (*set_raw)(ConfigOptions& opts, std::string_view key, const std::string& val);
    bool (*unset_raw)(ConfigOptions& opts);
    bool (*has_value)(const ConfigOptions& opts);
    void (*load)(const Property& prop, ConfigOptions& opts, const toml::table& tbl);
    void (*save)(const Property& prop, const ConfigOptions& opts, toml::table& tbl);
    void (*save_default)(const Property& prop, toml::table& tbl);
};

template <auto Member>
bool UnsetField(ConfigOptions& opts) {
    bool was_set = (opts.*Member).has_value();
    opts.*Member = std::nullopt;
    return was_set;
}

template <auto Member>
bool HasField(const ConfigOptions& opts) {
    return (opts.*Member).has_value();
}

template <std::optional<bool> ConfigOptions::*Member>
std::optional<std::string> GetBool(const ConfigOptions& opts) {
    const auto& val = opts.*Member;
    return val ? std::optional<std::string>(*val ? "true" : "false") : std::nullopt;
}

template <std::optional<bool> ConfigOptions::*Member>
void SetBool(ConfigOptions& opts, std::string_view key, const std::string& val) {
    auto b = ParseBool(val);
    if (!b) {
        throw std::invalid_argument("Invalid boolean for " + std::string(key) + ": " + val);
    }
    opts.*Member = *b;
}

template <std::optional<bool> ConfigOptions::*Member>
void LoadBool(const Property& prop, ConfigOptions& opts, const toml::table& tbl) {
    if (auto val = tbl[prop.name].value<bool>()) {
        opts.*Member = *val;
    }
}

template <std::optional<bool> ConfigOptions::*Member>
void SaveBool(const Property& prop, const ConfigOptions& opts, toml::table& tbl) {
    if (auto val = opts.*Member) {
        tbl.insert_or_assign(prop.name, *val);
    }
}

template <bool DefaultVal>
void SaveDefaultBool(const Property& prop, toml::table& tbl) {
    tbl.insert_or_assign(prop.name, DefaultVal);
}

template <std::optional<bool> ConfigOptions::*Member, bool DefaultVal>
constexpr Property MakeBoolProp(std::string_view name) {
    return Property{
        .name = name,
        .alias = "",
        .default_str = DefaultVal ? "true" : "false",
        .get_raw = GetBool<Member>,
        .set_raw = SetBool<Member>,
        .unset_raw = UnsetField<Member>,
        .has_value = HasField<Member>,
        .load = LoadBool<Member>,
        .save = SaveBool<Member>,
        .save_default = SaveDefaultBool<DefaultVal>,
    };
}

template <std::optional<std::string> ConfigOptions::*Member>
std::optional<std::string> GetString(const ConfigOptions& opts) {
    return opts.*Member;
}

template <std::optional<std::string> ConfigOptions::*Member>
void SetString(ConfigOptions& opts, std::string_view, const std::string& val) {
    opts.*Member = val;
}

template <std::optional<std::string> ConfigOptions::*Member>
void LoadString(const Property& prop, ConfigOptions& opts, const toml::table& tbl) {
    if (auto val = tbl[prop.name].value<std::string>()) {
        opts.*Member = *val;
    }
}

template <std::optional<std::string> ConfigOptions::*Member>
void SaveString(const Property& prop, const ConfigOptions& opts, toml::table& tbl) {
    if (auto val = opts.*Member) {
        tbl.insert_or_assign(prop.name, *val);
    }
}

inline void SaveDefaultString(const Property& prop, toml::table& tbl) {
    tbl.insert_or_assign(prop.name, "");
}

template <std::optional<std::string> ConfigOptions::*Member>
constexpr Property MakeStringProp(std::string_view name) {
    return Property{
        .name = name,
        .alias = "",
        .default_str = "",
        .get_raw = GetString<Member>,
        .set_raw = SetString<Member>,
        .unset_raw = UnsetField<Member>,
        .has_value = HasField<Member>,
        .load = LoadString<Member>,
        .save = SaveString<Member>,
        .save_default = SaveDefaultString,
    };
}

std::optional<std::string> GetAlgorithm(const ConfigOptions& opts) {
    return opts.algorithm ? std::optional<std::string>(AlgorithmToIdentifier(*opts.algorithm)) : std::nullopt;
}

void SetAlgorithm(ConfigOptions& opts, std::string_view, const std::string& val) {
    auto algo = ParseAlgorithm(val);
    if (!algo) {
        throw std::invalid_argument("Invalid algorithm: " + val + ". Valid values: password/argon2, password/sha512, passphrase/diceware, passphrase/wordnet, password/old");
    }
    opts.algorithm = algo;
}

void LoadAlgorithm(const Property& prop, ConfigOptions& opts, const toml::table& tbl) {
    if (auto val = tbl[prop.name].value<std::string>()) {
        opts.algorithm = ParseAlgorithm(*val);
    } else if (auto val_int = tbl[prop.name].value<int64_t>()) {
        opts.algorithm = ParseAlgorithm(std::to_string(*val_int));
    }
}

void SaveAlgorithm(const Property& prop, const ConfigOptions& opts, toml::table& tbl) {
    if (opts.algorithm) {
        tbl.insert_or_assign(prop.name, AlgorithmToIdentifier(*opts.algorithm));
    }
}

void SaveDefaultAlgorithm(const Property& prop, toml::table& tbl) {
    tbl.insert_or_assign(prop.name, AlgorithmToIdentifier(Algorithm::Argon2));
}

std::optional<std::string> GetCharClasses(const ConfigOptions& opts) {
    return opts.char_classes ? std::optional<std::string>(CharacterClassesToIdentifierString(*opts.char_classes)) : std::nullopt;
}

void SetCharClasses(ConfigOptions& opts, std::string_view, const std::string& val) {
    auto cc = ParseCharacterClasses(val);
    if (cc.empty()) {
        throw std::invalid_argument("Invalid char_classes: " + val + ". Valid tokens: lowercase, uppercase, digits, symbols, custom");
    }
    opts.char_classes = cc;
}

void LoadCharClasses(const Property& prop, ConfigOptions& opts, const toml::table& tbl) {
    if (auto val = tbl[prop.name].value<std::string>()) {
        opts.char_classes = ParseCharacterClasses(*val);
    } else if (auto val_int = tbl[prop.name].value<int64_t>()) {
        opts.char_classes = ParseCharacterClasses(std::to_string(*val_int));
    }
}

void SaveCharClasses(const Property& prop, const ConfigOptions& opts, toml::table& tbl) {
    if (opts.char_classes) {
        tbl.insert_or_assign(prop.name, CharacterClassesToIdentifierString(*opts.char_classes));
    }
}

void SaveDefaultCharClasses(const Property& prop, toml::table& tbl) {
    tbl.insert_or_assign(prop.name, "lowercase,uppercase,digits,symbols");
}

std::optional<std::string> GetLength(const ConfigOptions& opts) {
    return opts.length ? std::optional<std::string>(std::to_string(*opts.length)) : std::nullopt;
}

void SetLength(ConfigOptions& opts, std::string_view, const std::string& val) {
    auto len = ParseLength(val);
    if (!len) {
        throw std::invalid_argument("Invalid length: " + val + ". Must be a positive integer.");
    }
    opts.length = len;
}

void LoadLength(const Property& prop, ConfigOptions& opts, const toml::table& tbl) {
    if (auto val = tbl[prop.name].value<int64_t>()) {
        if (*val > 0) {
            opts.length = static_cast<size_t>(*val);
        }
    }
}

void SaveLength(const Property& prop, const ConfigOptions& opts, toml::table& tbl) {
    if (opts.length) {
        tbl.insert_or_assign(prop.name, static_cast<int64_t>(*opts.length));
    }
}

void SaveDefaultLength(const Property& prop, toml::table& tbl) {
    tbl.insert_or_assign(prop.name, static_cast<int64_t>(16));
}

std::optional<std::string> GetPassphrasePattern(const ConfigOptions& opts) {
    return opts.passphrase_pattern ? std::optional<std::string>(PatternToString(*opts.passphrase_pattern)) : std::nullopt;
}

void SetPassphrasePattern(ConfigOptions& opts, std::string_view, const std::string& val) {
    auto pat = ParsePassphrasePattern(val);
    if (!pat) {
        throw std::invalid_argument("Invalid passphrase pattern: " + val + ". Allowed characters: n (noun), v (verb), a (adj), r (adv).");
    }
    opts.passphrase_pattern = pat;
}

void LoadPassphrasePattern(const Property& prop, ConfigOptions& opts, const toml::table& tbl) {
    if (auto val = tbl[prop.name].value<std::string>()) {
        opts.passphrase_pattern = StringToPattern(*val);
    } else if (!prop.alias.empty()) {
        if (auto val2 = tbl[prop.alias].value<std::string>()) {
            opts.passphrase_pattern = StringToPattern(*val2);
        }
    }
}

void SavePassphrasePattern(const Property& prop, const ConfigOptions& opts, toml::table& tbl) {
    if (opts.passphrase_pattern) {
        tbl.insert_or_assign(prop.name, PatternToString(*opts.passphrase_pattern));
    }
}

void SaveDefaultPassphrasePattern(const Property& prop, toml::table& tbl) {
    tbl.insert_or_assign(prop.name, "");
}

inline constexpr std::array kProperties = {
    Property{
        .name = "algorithm",
        .alias = "",
        .default_str = "password/argon2",
        .get_raw = GetAlgorithm,
        .set_raw = SetAlgorithm,
        .unset_raw = UnsetField<&ConfigOptions::algorithm>,
        .has_value = HasField<&ConfigOptions::algorithm>,
        .load = LoadAlgorithm,
        .save = SaveAlgorithm,
        .save_default = SaveDefaultAlgorithm,
    },
    Property{
        .name = "char_classes",
        .alias = "",
        .default_str = "lowercase,uppercase,digits,symbols",
        .get_raw = GetCharClasses,
        .set_raw = SetCharClasses,
        .unset_raw = UnsetField<&ConfigOptions::char_classes>,
        .has_value = HasField<&ConfigOptions::char_classes>,
        .load = LoadCharClasses,
        .save = SaveCharClasses,
        .save_default = SaveDefaultCharClasses,
    },
    MakeStringProp<&ConfigOptions::custom_chars>("custom_chars"),
    Property{
        .name = "length",
        .alias = "",
        .default_str = "16",
        .get_raw = GetLength,
        .set_raw = SetLength,
        .unset_raw = UnsetField<&ConfigOptions::length>,
        .has_value = HasField<&ConfigOptions::length>,
        .load = LoadLength,
        .save = SaveLength,
        .save_default = SaveDefaultLength,
    },
    MakeStringProp<&ConfigOptions::separator>("separator"),
    Property{
        .name = "passphrase_pattern",
        .alias = "pattern",
        .default_str = "",
        .get_raw = GetPassphrasePattern,
        .set_raw = SetPassphrasePattern,
        .unset_raw = UnsetField<&ConfigOptions::passphrase_pattern>,
        .has_value = HasField<&ConfigOptions::passphrase_pattern>,
        .load = LoadPassphrasePattern,
        .save = SavePassphrasePattern,
        .save_default = SaveDefaultPassphrasePattern,
    },
    MakeBoolProp<&ConfigOptions::digits, false>("digits"),
    MakeBoolProp<&ConfigOptions::symbols, false>("symbols"),
    MakeBoolProp<&ConfigOptions::substitutions, false>("substitutions"),
    MakeBoolProp<&ConfigOptions::capitalize, true>("capitalize"),
    MakeBoolProp<&ConfigOptions::enable_old_algorithm, false>("enable_old_algorithm"),
};

const Property* FindProperty(std::string_view key) {
    for (const auto& prop : kProperties) {
        if (prop.matches(key)) {
            return &prop;
        }
    }
    return nullptr;
}

} // namespace

Config::Config(std::string file_path) : path_(std::move(file_path)) {
    if (path_.empty()) {
        path_ = get_default_config_path();
    }
}

std::string Config::get_default_config_path() {
    return GetConfigFilePath();
}

bool Config::is_valid_key(const std::string& key) {
    return FindProperty(key) != nullptr;
}

std::vector<std::string> Config::get_all_keys() {
    std::vector<std::string> keys;
    keys.reserve(kProperties.size());
    for (const auto& prop : kProperties) {
        keys.emplace_back(prop.name);
    }
    return keys;
}

std::string Config::get_built_in_default(const std::string& key) {
    if (const auto* prop = FindProperty(key)) {
        return std::string(prop->default_str);
    }
    throw std::invalid_argument("Unknown configuration key: " + key);
}

bool Config::exists() const {
    std::string p = path_.empty() ? get_default_config_path() : path_;
    return std::filesystem::exists(p);
}

bool Config::load() {
    options_ = ConfigOptions{};
    if (path_.empty()) {
        path_ = get_default_config_path();
    }
    if (!std::filesystem::exists(path_)) {
        return true;
    }

    try {
        auto tbl = toml::parse_file(path_);
        for (const auto& prop : kProperties) {
            prop.load(prop, options_, tbl);
        }
        return true;
    } catch (...) {
        return false;
    }
}

bool Config::save() {
    try {
        if (path_.empty()) {
            path_ = get_default_config_path();
        }
        std::filesystem::path p(path_);
        if (p.has_parent_path()) {
            std::filesystem::create_directories(p.parent_path());
        }

        toml::table tbl;
        for (const auto& prop : kProperties) {
            prop.save(prop, options_, tbl);
        }

        std::ofstream out(path_);
        if (!out.is_open()) {
            return false;
        }
        out << "# mkpass configuration file\n\n";
        out << tbl << "\n";
        return true;
    } catch (...) {
        return false;
    }
}

std::optional<std::string> Config::get_raw(const std::string& key) const {
    if (const auto* prop = FindProperty(key)) {
        return prop->get_raw(options_);
    }
    throw std::invalid_argument("Unknown configuration key: " + key);
}

void Config::set_raw(const std::string& key, const std::string& value) {
    if (const auto* prop = FindProperty(key)) {
        prop->set_raw(options_, key, value);
        return;
    }
    throw std::invalid_argument("Unknown configuration key: " + key);
}

bool Config::unset_raw(const std::string& key) {
    if (const auto* prop = FindProperty(key)) {
        return prop->unset_raw(options_);
    }
    throw std::invalid_argument("Unknown configuration key: " + key);
}

bool Config::is_set(const std::string& key) const {
    return get_raw(key).has_value();
}

std::string Config::print(bool all) const {
    if (!all && exists()) {
        std::ifstream in(path_);
        if (in.is_open()) {
            std::stringstream ss;
            ss << in.rdbuf();
            return ss.str();
        }
    }

    if (!all) {
        toml::table tbl;
        for (const auto& prop : kProperties) {
            prop.save(prop, options_, tbl);
        }
        std::stringstream ss;
        ss << tbl << "\n";
        return ss.str();
    }

    toml::table explicit_tbl;
    toml::table default_tbl;

    for (const auto& prop : kProperties) {
        if (prop.has_value(options_)) {
            prop.save(prop, options_, explicit_tbl);
        } else {
            prop.save_default(prop, default_tbl);
        }
    }

    std::stringstream ss;
    if (!explicit_tbl.empty()) {
        ss << "# Explicit options\n\n" << explicit_tbl << "\n";
        if (!default_tbl.empty()) {
            ss << "\n";
        }
    }
    if (!default_tbl.empty()) {
        ss << "# Default options\n\n" << default_tbl << "\n";
    }

    return ss.str();
}

bool IsOldAlgorithmEnabled(const Config& config) {
    if (const char* env1 = std::getenv("MKPASS_ENABLE_OLD_ALGORITHM")) {
        auto b = ParseBool(env1);
        if (b.has_value()) return *b;
    }
    if (const char* env2 = std::getenv("enable_old_algorithm")) {
        auto b = ParseBool(env2);
        if (b.has_value()) return *b;
    }
    return config.options().enable_old_algorithm.value_or(false);
}

} // namespace mkpass
