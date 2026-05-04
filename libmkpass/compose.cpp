#include <stdexcept>
#include <numeric>
#include <algorithm>
#include <iostream>
#include <set>

#include <stdexcept>
#include <numeric>
#include <algorithm>
#include <iostream>
#include <set>
#include <cctype>
#include <memory>

#include "compose.h"
#include "generator.h"
#include "uniform.h"
#include "sha1.hpp"
#include "base64.hpp"
#include "passphrase_patterns.h"


template <typename Container>
int total_length(const Container& strings) {
    return std::accumulate(
        strings.begin(),
        strings.end(),
        0,
        [](int sum, const std::string& s) {
            return sum + s.size();
        }
        );
}


struct combined_string  {
    std::vector<std::string> parts;
    using value_type = std::string::value_type;

    combined_string(std::vector<std::string>&& strings)
        : parts(std::move(strings))
    {
    }

    std::pair<size_t, value_type> at(size_t pos) {
        size_t part_no = 0;
        while (1) {
            if (part_no >= parts.size()) {
                throw std::out_of_range("");
            }
            if (pos >= parts[part_no].length()) {
                pos -= parts[part_no].length();
                ++part_no;
                continue;
            }
            break;
        };

        return {part_no, parts[part_no][pos]};
    }

    int length() {
        return total_length(parts);
    }
};


std::string ComposePassword(
    GeneratorInterface& generator,
    std::vector<std::string>&& char_classes,
    size_t length)
{
    std::string result;
    std::set<size_t> used_classes;
    combined_string combo(std::move(char_classes));
    UniformDistribution distibution(0, combo.length()-1);

    while (length > 0) {
        size_t n_unused_classes = combo.parts.size() - used_classes.size();

        if (length < n_unused_classes) {
            throw std::runtime_error("Can not generate password: too short");
        }

        if (length == n_unused_classes) {
            // There's still a chance to save it, remove all used classes
            for (auto it = used_classes.rbegin(); it != used_classes.rend(); ++it) {
                auto char_class_id = *it;
                combo.parts.erase(combo.parts.begin() + char_class_id);
            }
            used_classes.clear();
            distibution.param({0, combo.length()-1});
        }

        // Generate a random number within [0, total_length)
        auto pos = distibution(generator);
        auto [char_class_no, ch] = combo.at(pos);
        used_classes.insert(char_class_no);
        result += ch;

        --length;
    }

    return result;
}


std::string ComposeOldMkpass1Password(
    const std::string& master_password,
    const std::string& service_name,
    size_t length)
{
    SHA1 sha1;
    sha1.update(master_password + service_name);
    const std::vector<uint8_t> hash = sha1.final_bytes();

    return czkz::base64_encode(hash).substr(0, length);
}


class WordSeparator {
public:
    WordSeparator(const std::string& separator)
        : separator_(separator)
    {
    }

    std::string operator()(const std::string& word)
    {
        if (first_word) {
            first_word = false;
            return word;
        }
        return separator_ + word;
    }

private:
    std::string separator_;
    bool first_word = true;
};


class WordModifierBase {
public:
    virtual std::string operator()(const std::string& word) = 0;
    virtual ~WordModifierBase() = default;
};


class WordCapitalizer: public WordModifierBase {
public:
    std::string operator()(const std::string& word) {
        std::string result(word);
        if (result.length() > 0) {
            result[0] = std::toupper(result[0]);
        }
        return result;
    }
};


class WordModifier: public WordModifierBase {
public:
    WordModifier(
        GeneratorInterface& generator,
        const std::string& char_class_string,
        const CharMap& substitution_map,
        bool allow_substitutions)
        : generator_(generator)
        , char_class_string_(char_class_string)
        , substitution_map_(substitution_map)
        , distibution_(0, char_class_string.length()-1)
        , allow_substitutions_(allow_substitutions)
    {
    }

    std::string operator()(const std::string& word)
    {
        if (allow_substitutions_) {
            std::string sword = word;
            auto substituions = MakeSubstitutionsList(sword);
            if (substituions.size() > 0) {
                UniformDistribution<int> d(0, substituions.size() - 1);
                auto subst = substituions[d(generator_)];
                (*subst.first) = subst.second;
                return sword;
            }
        }

        return word + char_class_string_[distibution_(generator_)];
    }

private:
    using StrSubtitition = std::pair<std::string::iterator, char>;
    using SubtititionsList = std::vector<StrSubtitition>;

    SubtititionsList MakeSubstitutionsList(std::string &word)
    {
        auto first = word.begin();
        auto last = word.end();
        SubtititionsList result;
        while (first != last) {
            auto subit = substitution_map_.find(*first);
            if (subit == substitution_map_.end()) {
                subit = substitution_map_.find(std::tolower(*first));
            }
            if (subit != substitution_map_.end()) {
                result.push_back({first, subit->second});
            }
            ++first;
        }
        return result;
    }

private:
    GeneratorInterface& generator_;
    std::string char_class_string_;
    CharMap substitution_map_;
    UniformDistribution<int> distibution_;
    bool allow_substitutions_;
};


std::multimap<int, CharacterClass> GetModWordPositions(
    GeneratorInterface& generator,
    const std::vector<CharacterClass>& char_classes,
    int passprhase_length)
{
    UniformDistribution distibution(0, passprhase_length - 1);
    std::multimap<int, CharacterClass> result;

    for (auto& cls: char_classes) {
        auto word_idx = distibution(generator);
        result.insert({word_idx, cls});
    }
    return result;
}


std::vector<CharacterClass> GetWordCharClasses(
    std::multimap<int, CharacterClass> wtmmap,
    int word_pos)
{
    auto [first, last] = wtmmap.equal_range(word_pos);
    std::vector<CharacterClass> result;
    std::transform(first, last, std::back_inserter(result),
                   [](const auto& kv) { return kv.second; });
    return result;
}


using ModifiersList = std::vector<std::unique_ptr<WordModifierBase>>;


ModifiersList GetWordModifiers(
    GeneratorInterface& generator,
    const std::vector<CharacterClass>& char_classes,
    bool allow_substitutions,
    bool capitalize_words)
{
    ModifiersList result;

    if (capitalize_words) {
        auto uptr = std::make_unique<WordCapitalizer>();
        result.push_back(std::move(uptr));
    }

    for (auto& cls: char_classes) {
        auto chars_cls_str = GetCharClassString(cls);
        auto substitution_map = GetCharClassSubstitutions(cls);
        auto uptr = std::make_unique<WordModifier>(
            generator,
            chars_cls_str,
            substitution_map,
            allow_substitutions);
        result.push_back(std::move(uptr));
    }
    return result;
}


template <typename T>
class UniqueRandom
{
public:
    UniqueRandom(GeneratorInterface& generator,
                 UniformDistribution<T> distibution)
        : generator_(generator)
        , distibution_(distibution)
    {
    }

    T operator()() {
        while (true) {
            T value = distibution_(generator_);
            auto [it, inserted] = seen_.insert(value);
            if (inserted) {
                return value;
            }
        }
    }
private:
    GeneratorInterface& generator_;
    UniformDistribution<T> distibution_;
    std::set<T> seen_;
};


std::string ComposePassPhrase(
    GeneratorInterface& generator,
    const Wordlist& wordlist,
    size_t length,
    const std::string& separator_str,
    const std::vector<CharacterClass>& char_classes,
    bool allow_substitutions,
    bool capitalize_words)
{

    std::string result;
    UniformDistribution distibution(0, wordlist.length()-1);
    WordSeparator separator(separator_str);

    auto words_to_modify = GetModWordPositions(
        generator, char_classes, length);

    UniqueRandom urandom(generator, distibution);

    for (size_t pos = 0; pos < length; ++pos) {
        auto idx = urandom();
        auto word = wordlist[idx];
        auto modifiers = GetWordModifiers(
            generator,
            GetWordCharClasses(words_to_modify, pos),
            allow_substitutions,
            capitalize_words);
        for (auto& modifier: modifiers) {
            word = (*modifier)(word);
        }
        result += separator(word);
    }
    return result;
}


std::string ComposePassPhrase(
    GeneratorInterface& generator,
    std::map<WordClasses, Wordlist> wordlists,
    std::vector<WordClasses> pattern,
    size_t length,
    const std::string& separator_str,
    const std::vector<CharacterClass>& char_classes,
    bool allow_substitutions,
    bool capitalize_words)
{
    if (pattern.empty()) {
        PatternsList patterns = GetPassphrasePatterns(length);
        if (patterns.empty()) {
            throw std::runtime_error("No patterns for length " + std::to_string(length));
        }
        UniformDistribution<int> d(0, patterns.size() - 1);
        pattern = patterns[d(generator)];
    }

    std::string result;
    std::map<WordClasses, UniqueRandom<int>> urandoms;

    WordSeparator separator(separator_str);
    auto words_to_modify = GetModWordPositions(
        generator, char_classes, pattern.size());

    for (auto& [wc, wl]: wordlists) {
        auto d = UniformDistribution<int>(0, wl.length()-1);
        urandoms.emplace(wc, UniqueRandom<int>(generator, d));
    }

    for (size_t pos = 0; pos < pattern.size(); ++pos) {
        auto& wc = pattern[pos];
        auto idx = urandoms.at(wc)();
        auto word = wordlists.at(wc)[idx];
        auto modifiers = GetWordModifiers(
            generator,
            GetWordCharClasses(words_to_modify, pos),
            allow_substitutions,
            capitalize_words);
        for (auto& modifier: modifiers) {
            word = (*modifier)(word);
        }
        result += separator(word);
    }

    return result;
}
