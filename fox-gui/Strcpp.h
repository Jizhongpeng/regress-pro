
#ifndef STRCPP_H
#define STRCPP_H

#include "str.h"

class Str {
    str_t _cstr;

    enum {
        DEFAULT_SIZE = 64
    };

public:
    Str() {
        str_init(_cstr, DEFAULT_SIZE);
    };
    Str(const char *s) {
        str_init_from_c(_cstr, s);
    };
    ~Str() {
        str_free(_cstr);
    };

    int length() {
        return STR_LENGTH(_cstr);
    };

    const char *cstr() {
        return CSTR(this->_cstr);
    };
    str_ptr str() {
        return this->_cstr;
    };
};

#endif
