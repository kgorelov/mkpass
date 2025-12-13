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

#include "compose.h"
#include "generator.h"
#include "uniform.h"

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
