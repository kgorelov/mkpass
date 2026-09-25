#pragma once

#include <string>
#include <vector>
#include <optional>

#include "algorithms.h"
#include "character_classes.h"
#include "word_classes.h"

namespace mkpass {

struct ConfigOptions {
    std::optional<Algorithm> algorithm;
    std::optional<std::vector<CharacterClass>> char_classes;
    std::optional<std::string> custom_chars;
    std::optional<size_t> length;
    std::optional<std::string> separator;
    std::optional<std::vector<WordClasses>> passphrase_pattern;
    std::optional<bool> digits;
    std::optional<bool> symbols;
    std::optional<bool> substitutions;
    std::optional<bool> capitalize;
    std::optional<bool> enable_old_algorithm;
};

class Config {
public:
    explicit Config(std::string file_path = "");

    bool load();
    bool save();

    std::optional<std::string> get_raw(const std::string& key) const;
    void set_raw(const std::string& key, const std::string& value);
    bool unset_raw(const std::string& key);
    bool is_set(const std::string& key) const;
    std::string print(bool all = false) const;

    const ConfigOptions& options() const { return options_; }
    void set_options(const ConfigOptions& opts) { options_ = opts; }

    const std::string& path() const { return path_; }
    void set_path(const std::string& path) { path_ = path; }
    bool exists() const;

    static std::string get_default_config_path();
    static bool is_valid_key(const std::string& key);
    static std::vector<std::string> get_all_keys();
    static std::string get_built_in_default(const std::string& key);

private:
    std::string path_;
    ConfigOptions options_;
};

bool IsOldAlgorithmEnabled(const Config& config);

} // namespace mkpass
