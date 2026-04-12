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

class WordsSeparator {
public:
  std::string operator()(const std::string& word) {
    std::string result(word);
    if (result.length() > 0) {
      result[0] = std::toupper(result[0]);
    }
    return result;
  }
};


std::string ComposePassPhrase(
    GeneratorInterface& generator,
    const Wordlist& wordlist,
    size_t length)
{
  std::string result;
  UniformDistribution distibution(0, wordlist.length()-1);
  WordsSeparator separator;
  std::set<int> used_idxs;

  while (length > 0) {
    auto idx = distibution(generator);
    if (used_idxs.find(idx) != used_idxs.end()) {
      continue;
    }
    used_idxs.insert(idx);
    result += separator(wordlist[idx]);
    --length;
  }
  return result;
}


std::string ComposePassPhrase(
    GeneratorInterface& generator,
    std::map<WordClasses, Wordlist> &&wordlists,
    std::vector<WordClasses> &&pattern)
{
    std::string result;
    std::map<WordClasses, UniformDistribution<int>> distributions;
    std::map<WordClasses, std::set<int>> used_idxs;

    WordsSeparator separator;

    for (auto& [wc, wl]: wordlists) {
        distributions.emplace(wc, UniformDistribution<int>(0, wl.length()-1));
    }

    for (auto& wc: pattern) {
        auto idx = distributions.at(wc)(generator);
        if (used_idxs[wc].find(idx) != used_idxs[wc].end()) {
            continue;
        }
        used_idxs[wc].insert(idx);
        result += separator(wordlists.at(wc)[idx]);
    }

    return result;
}


// Implement Composing passphares using patterns/formulas instead of length
// Formula: ADJ NOUN VERB NOUN
// FormulaL ADJ NOUN ADV VERB ADJ NOUN
// Separator: '-' ' ' CAPITILIZE UPPER

