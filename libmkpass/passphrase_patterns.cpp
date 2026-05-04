#include <map>
#include "passphrase_patterns.h"

using E = WordClasses;


std::map<int, PatternsList> NaturalPatterns = {
    {
        1, // 1-word patterns
        {
            { E::Noun }, // "Bird"
            { E::Verb }, // "Fly"
            { E::Adj },  // "Huge"
            { E::Adv }   // "Quickly"
        }
    },
    {
        2, // 2-word patterns
        {
            { E::Noun, E::Verb }, // "Birds fly"
            { E::Verb, E::Noun }, // "Eat vegetables"
            { E::Adj,  E::Noun }, // "Big problem"
            { E::Verb, E::Adv }   // "Come quickly"
        }
    },
    {
        3, // 3-word patterns
        {
            { E::Adj,  E::Adj,  E::Noun }, // "Big red car"
            { E::Noun, E::Adv,  E::Verb }, // "Birds often sing"
            { E::Noun, E::Verb, E::Noun }, // "Cats chase mice"
            { E::Verb, E::Adj,  E::Noun }, // "Open heavy door"
            { E::Noun, E::Verb, E::Adj }   // "Sky looks blue"
        }
    },
    {
        4, // 4-word patterns
        {
            { E::Adj,  E::Adj,  E::Noun, E::Verb }, // "Small brown dog barked"
            { E::Noun, E::Adv,  E::Verb, E::Noun }, // "Birds quickly chase mice"
            { E::Verb, E::Adj,  E::Noun, E::Adv },  // "Open heavy door slowly"
            { E::Noun, E::Verb, E::Adj,  E::Noun }, // "Life brings good fortune"
            { E::Noun, E::Verb, E::Adv,  E::Adj }   // "Food tastes very good"
        }
    },
    {
        5, // 5-word patterns
        {
            { E::Adj,  E::Adj,  E::Noun, E::Verb, E::Adv },  // "Brave young soldier fought valiantly"
            { E::Noun, E::Verb, E::Adv,  E::Adj,  E::Adv },  // "Coffee tastes very strong today"
            { E::Noun, E::Verb, E::Adj,  E::Noun, E::Adv },  // "Cat chased small mouse swiftly"
            { E::Verb, E::Adj,  E::Noun, E::Adv,  E::Adv },  // "Open heavy door very slowly"
            { E::Noun, E::Adv,  E::Verb, E::Adj,  E::Noun }  // "Birds often sing sweet songs"
        }
    },
    {
        6, // 6-word patterns
        {
            { E::Adj,  E::Adj,  E::Noun, E::Verb, E::Adj,  E::Noun }, // "Hungry clever fox chased small rabbit"
            { E::Noun, E::Adv,  E::Verb, E::Adj,  E::Noun, E::Adv },  // "She quickly bought red dress yesterday"
            { E::Adj,  E::Noun, E::Verb, E::Adv,  E::Adj,  E::Adv },  // "Warm soup tastes very good tonight"
            { E::Verb, E::Adj,  E::Noun, E::Verb, E::Adj,  E::Noun }, // "Carry old books fill empty shelves"
            { E::Noun, E::Verb, E::Noun, E::Verb, E::Adj,  E::Noun }  // "Children build houses create happy memories"
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
