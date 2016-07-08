#ifndef BUNKERBUILDER_RANDOM_H
#define BUNKERBUILDER_RANDOM_H
#include <stdint.h>
namespace bb {
    namespace random {
        uint32_t x = 0x9d546264u, y = 0x13664b81u, z = 0x1be129e9u, w = 0x1686d9f3u;

        uint32_t u32(void) {
            uint32_t t = x;
            t ^= t << 11;
            t ^= t >> 8;
            x = y;
            y = z;
            z = w;
            w ^= w >> 19;
            w ^= t;
            return w;
        }

        int32_t i32(void) {
            return (int32_t) u32();
        }
    }
}

#endif //BUNKERBUILDER_RANDOM_H
