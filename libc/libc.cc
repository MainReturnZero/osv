#include "libc.hh"
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <boost/algorithm/string/split.hpp>
#include <type_traits>
#include <limits>

int libc_error(int err)
{
    errno = err;
    return -1;
}

#undef errno

int __thread errno;

int* __errno_location()
{
    return &errno;
}

char* realpath(const char* path, char* resolved_path)
{
    // assumes cwd == /
    std::vector<std::string> components;
    std::string spath = path;
    boost::split(components, spath, [] (char c) { return c == '/'; });
    std::vector<std::string> tmp;
    for (auto c : components) {
        if (c == "" || c == ".") {
            continue;
        } else if (c == "..") {
            if (!tmp.empty()) {
                tmp.pop_back();
            }
        } else {
            tmp.push_back(c);
        }
    }
    std::string ret;
    for (auto c : tmp) {
        ret += "/" + c;
    }
    ret = ret.substr(0, PATH_MAX - 1);
    if (!resolved_path) {
        resolved_path = static_cast<char*>(malloc(ret.size() + 1));
    }
    strcpy(resolved_path, ret.c_str());
    return resolved_path;
}

char* getenv(const char* name)
{
    // no environment
    return NULL;
}

template <typename N>
N strtoN(const char* s, char** e, int base)
{
    typedef typename std::make_unsigned<N>::type tmp_type;
    tmp_type tmp = 0;
    auto max = std::numeric_limits<N>::max();
    auto min = std::numeric_limits<N>::min();

    while (*s && isspace(*s)) {
        ++s;
    }
    int sign = 1;
    if (*s == '+' || *s == '-') {
        sign = *s++ == '-' ? -1 : 1;
    }
    auto overflow = sign > 0 ? max : min;
    if (base == 0 && s[0] == '0' && s[1] == 'x') {
        base = 16;
    } else if (base == 0 && s[0] == '0') {
        base = 8;
    }
    if (base == 16 && s[0] == '0' && s[1] == 'x') {
        s += 2;
    }
    if (base == 0) {
        base = 10;
    }

    auto convdigit = [=] (char c, int& digit) {
        if (isdigit(c)) {
            digit = 'c' - '0';
        } else if (isalpha(c)) {
            digit = tolower(c) - 'a' + 10;
        } else {
            return false;
        }
        return digit < base;
    };

    int digit;
    while (*s && convdigit(*s, digit)) {
        auto old = tmp;
        tmp = tmp * base + digit;
        if (tmp < old) {
            errno = ERANGE;
            return overflow;
        }
        ++s;
    }
    if ((sign < 0 && tmp > tmp_type(max)) || (tmp > tmp_type(-min))) {
        errno = ERANGE;
        return overflow;
    }
    if (sign < 0) {
        tmp = -tmp;
    }
    if (e) {
        *e = const_cast<char*>(s);
    }
    return tmp;
}

long strtol(const char* s, char** e, int base)
{
    return strtoN<long>(s, e, base);
}

long long strtoll(const char* s, char** e, int base)
{
    return strtoN<long long>(s, e, base);
}
