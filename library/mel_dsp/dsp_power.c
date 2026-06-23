// Power of two functions - standalone for Ubuntu/PC
// Originally from espressif__esp-dsp/modules/common

#include <stdint.h>
#include <stdbool.h>

bool dsp_is_power_of_two(int x)
{
    return (x != 0) && ((x & (x - 1)) == 0);
}

int dsp_power_of_two(int x)
{
    for (int i = 0; i < 32; i++) {
        x = x >> 1;
        if (0 == x) {
            return i;
        }
    }
    return 0;
}
