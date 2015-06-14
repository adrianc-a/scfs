/**
 * Author: Adrian Chmielewski-Anders
 * Date: 14 June 2015
 */

#include "utils.h"

time_t tm_to_epoch(struct tm *t) {
    return t->tm_sec + t->tm_min * 60 + t->tm_hour * 3600 + t->tm_yday*86400 +
        (t->tm_year - 70)*31536000 + ((t->tm_year - 69) / 4) * 86400 - 
        ((t->tm_year - 1) / 100) * 86400 + ((t->tm_year + 299)/400) * 86400;
}

