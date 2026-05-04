#include <map>
#include "passphrase_patterns.h"


using E = WordClasses;


std::map<int, PatternsList> NaturalPatterns = {
    {
        1, // 1-word patterns
        {
            { E::Noun }, // bird
            { E::Verb }, // fly
            { E::Adj },  // huge
            { E::Adv }   // quickly
        }
    },
    {
        2, // 2-word patterns
        {
            { E::Verb, E::Noun }, // eat apple
            { E::Adj,  E::Noun }, // red car
            { E::Noun, E::Verb }, // bird fly
            { E::Verb, E::Adv }   // come quickly
        }
    },
    {
        3, // 3-word patterns
        {
            { E::Verb, E::Adj,  E::Noun }, // open heavy door
            { E::Verb, E::Noun, E::Adv },  // run marathon daily
            { E::Adj,  E::Adj,  E::Noun }, // big red car
            { E::Noun, E::Adv,  E::Verb }, // bird quickly fly (stylistic / lower naturalness)
            { E::Noun, E::Verb, E::Noun }  // cat chase mouse (telegraphic style)
        }
    },
    {
        4, // 4-word patterns
        {
            { E::Verb, E::Adj,  E::Noun, E::Adv },  // open heavy door slowly
            { E::Verb, E::Adj,  E::Adj,  E::Noun }, // choose bright red apple
            { E::Adj,  E::Adj,  E::Noun, E::Verb }, // small black bird sing
            { E::Noun, E::Verb, E::Adj,  E::Noun }, // life bring good fortune
            { E::Noun, E::Adv,  E::Verb, E::Noun }  // cat quickly chase mouse
        }
    },
    {
        5, // 5-word patterns
        {
            { E::Verb, E::Adj,  E::Noun, E::Adv,  E::Adv },  // open heavy door very slowly
            { E::Verb, E::Adj,  E::Adj,  E::Noun, E::Adv },  // choose bright red apple wisely
            { E::Adj,  E::Adj,  E::Noun, E::Verb, E::Adv },  // brave young soldier fight valiantly
            { E::Noun, E::Verb, E::Adj,  E::Noun, E::Adv },  // cat chase small mouse swiftly
            { E::Noun, E::Adv,  E::Verb, E::Adj,  E::Noun }  // bird often sing sweet song
        }
    },
    {
        6, // 6-word patterns
        {
            { E::Verb, E::Adj,  E::Noun, E::Verb, E::Adj,  E::Noun }, // carry old book fill empty shelf
            { E::Verb, E::Adj,  E::Adj,  E::Noun, E::Adv,  E::Adv },  // choose bright red apple very wisely
            { E::Adj,  E::Adj,  E::Noun, E::Verb, E::Adj,  E::Noun }, // hungry clever fox chase small rabbit
            { E::Noun, E::Verb, E::Noun, E::Verb, E::Adj,  E::Noun }, // child build house create happy memory
            { E::Noun, E::Adv,  E::Verb, E::Adj,  E::Noun, E::Adv }   // bird quickly chase small insect daily
        }
    }
};


PatternsList GetPassphrasePatterns(int length)
{
    auto it = NaturalPatterns.find(length);
    if (it == NaturalPatterns.end()) {
        return {};
    }
    return it->second;
}

int GetMaxPassphrasePatternLength()
{
    return NaturalPatterns.size();
}
