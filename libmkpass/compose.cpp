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

#include "compose.h"
#include "generator.h"
#include "uniform.h"
#include "sha1.hpp"
#include "base64.hpp"

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

// Separator algorithm modifies the current selected word:
// either adding '-' at the front if it's not the fist word
// the state is in the separator class itself
// or capitilizing the first letter.
// These must be different separator classes.

class CamelWordsSeparator {
public:
  std::string operator()(const std::string& word) {
    std::string result(word);
    if (result.length() > 0) {
      result[0] = std::toupper(result[0]);
    }
    return result;
  }
};

class KebabWordsSeparator {
public:
  std::string operator()(const std::string& word) {
    if (first_word) {
      first_word = false;
      return word;
    }
    return std::string("-") + word;
  }
private:
  bool first_word = true;
};


class WordModifier {
public:
    WordModifier(
        GeneratorInterface& generator,
        int word_position,
        const std::string& char_class_string,
        const CharMap& substitution_map,
        bool allow_substitutions)
        : generator_(generator)
        , word_position_(word_position)
        , char_class_string_(char_class_string)
        , substitution_map_(substitution_map)
        , distibution_(0, char_class_string.length()-1)
        , allow_substitutions_(allow_substitutions)
    {
    }

    std::string operator()(const std::string& word, int pos)
    {
        if (pos != word_position_) {
            return word;
        }
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
    int word_position_;
    std::string char_class_string_;
    CharMap substitution_map_;
    UniformDistribution<int> distibution_;
    bool allow_substitutions_;
};

std::vector<WordModifier> GetWordModifiers(
    GeneratorInterface& generator,
    const std::vector<CharacterClass>& char_classes,
    int passprhase_length,
    bool allow_substitutions)
{
    UniformDistribution distibution(0, passprhase_length - 1);
    std::vector<WordModifier> result;
    for (auto& cls: char_classes) {
        auto word_idx = distibution(generator);
        auto chars_cls_str = GetCharClassString(cls);
        auto substitution_map = GetCharClassSubstitutions(cls);
        result.push_back(WordModifier(
            generator, word_idx, chars_cls_str,
            substitution_map, allow_substitutions));
    }
    return result;
}

template <typename Separator>
std::string ComposePassPhraseWithSeparator(
    GeneratorInterface& generator,
    const Wordlist& wordlist,
    size_t length,
    const std::vector<CharacterClass>& char_classes,
    bool allow_substitutions)
{
  std::string result;
  UniformDistribution distibution(0, wordlist.length()-1);
  Separator separator;
  auto modifiers = GetWordModifiers(generator, char_classes, length, allow_substitutions);
  std::set<int> used_idxs;

  for (size_t pos = 0; pos < length; ++pos) {
    auto idx = distibution(generator);
    if (used_idxs.find(idx) != used_idxs.end()) {
      continue;
    }
    used_idxs.insert(idx);
    auto word = wordlist[idx];
    for (auto& modifier: modifiers) {
        word = modifier(word, pos);
    }
    result += separator(word);
  }
  return result;
}

std::string ComposePassPhrase(
    GeneratorInterface& generator,
    const Wordlist& wordlist,
    size_t length,
    PassphraseSeparator separator_type,
    const std::vector<CharacterClass>& char_classes,
    bool allow_substitutions)
{
  if (separator_type == PassphraseSeparator::CamelCase) {
      return ComposePassPhraseWithSeparator<CamelWordsSeparator>(generator, wordlist, length, char_classes, allow_substitutions);
  } else {
      return ComposePassPhraseWithSeparator<KebabWordsSeparator>(generator, wordlist, length, char_classes, allow_substitutions);
  }
}

template <typename Separator>
std::string ComposePassPhraseWithSeparator(
    GeneratorInterface& generator,
    std::map<WordClasses, Wordlist> &&wordlists,
    std::vector<WordClasses> &&pattern,
    const std::vector<CharacterClass>& char_classes,
    bool allow_substitutions)
{
    std::string result;
    std::map<WordClasses, UniformDistribution<int>> distributions;
    std::map<WordClasses, std::set<int>> used_idxs;
    Separator separator;
    auto modifiers = GetWordModifiers(generator, char_classes, pattern.size(), allow_substitutions);

    for (auto& [wc, wl]: wordlists) {
        distributions.emplace(wc, UniformDistribution<int>(0, wl.length()-1));
    }

    for (size_t pos = 0; pos < pattern.size(); ++pos) {
        auto& wc = pattern[pos];
        auto idx = distributions.at(wc)(generator);
        if (used_idxs[wc].find(idx) != used_idxs[wc].end()) {
            continue;
        }
        used_idxs[wc].insert(idx);
        auto word = separator(wordlists.at(wc)[idx]);
        for (auto& modifier: modifiers) {
            word = modifier(word, pos);
        }
        result += word;
    }

    return result;
}

std::string ComposePassPhrase(
    GeneratorInterface& generator,
    std::map<WordClasses, Wordlist> &&wordlists,
    std::vector<WordClasses> &&pattern,
    PassphraseSeparator separator_type,
    const std::vector<CharacterClass>& char_classes,
    bool allow_substitutions)
{
    if (separator_type == PassphraseSeparator::CamelCase) {
        return ComposePassPhraseWithSeparator<CamelWordsSeparator>(generator, std::move(wordlists), std::move(pattern), char_classes, allow_substitutions);
    } else {
        return ComposePassPhraseWithSeparator<KebabWordsSeparator>(generator, std::move(wordlists), std::move(pattern), char_classes, allow_substitutions);
    }
}


// Implement Composing passphares using patterns/formulas instead of length
// Formula: ADJ NOUN VERB NOUN
// FormulaL ADJ NOUN ADV VERB ADJ NOUN
// Separator: '-' ' ' CAPITILIZE UPPER

