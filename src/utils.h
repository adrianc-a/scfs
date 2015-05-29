/**
 * Author: Adrian Chmielewski-Anders
 * Date: 6 May 2015
 */

#ifndef UTILS_H
#define UTILS_H

#include <math.h>

#define NUM_LEN(x) (floor(log10(abs(x))) + 1)

#define I_TO_A(_i, _buf) int _len = NUM_LEN(_i);                               \
                         _buf = malloc(_len + 1);                              \
                         sprintf(_buf, "%d", _i);                              \
                         _buf[_len] = '\0'


#endif
