#ifndef BUNKERBUILDER_NAMEGEN_H
#define BUNKERBUILDER_NAMEGEN_H
#include <string>
#include <vector>
#include "random.h"

using namespace std;

namespace bb {
    namespace namegen {
        vector<string> prefixes = {"", "bel", "nar", "xan", "bell", "natr", "ev"};
        vector<string> stems = {"adur", "aes", "anim", "apoll", "imac", "educ", "equis", "extr", "guius", "hann",
                                "equi", "amora", "hum", "iace", "ille", "inept", "iuv", "obe", "ocul", "orbis"};
        vector<string> suffixes = {"", "us", "ix", "ox", "ith", "ath", "um", "ator", "or", "axia", "imus", "ais",
                                   "itur", "orex", "o", "y"};

        string choice(const vector<string> &choices) {
            uint64_t index = random::u32() % choices.size();
            return choices[index];
        }

        string gen() {
            string name = choice(prefixes) + choice(stems) + choice(suffixes);
            name[0] = (char)toupper(name[0]);
            return name;
        }
    }
}

#endif //BUNKERBUILDER_NAMEGEN_H
