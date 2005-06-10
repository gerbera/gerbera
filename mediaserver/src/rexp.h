#ifndef __REXP_H__
#define __REXP_H__

#include "common.h"
#include <sys/types.h>
#include <regex.h>

#define DEFAULT_NMATCH 5

class Matcher;

class RExp : public zmm::Object
{
public:
    RExp();
    virtual ~RExp();
    void compile(zmm::String pattern, int flags = 0);
    void compile(zmm::String pattern, const char *flags);
    zmm::Ref<Matcher> matcher(zmm::String text, int nmatch = DEFAULT_NMATCH);
    bool matches(zmm::String text);
    zmm::String getPattern();
protected:
    bool isCompiled;
    zmm::String pattern;
    regex_t regex;

    friend class Matcher;
};


class Matcher : public zmm::Object
{
public:
    virtual ~Matcher();
    zmm::String group(int i);
    bool next();
protected:
    Matcher(zmm::Ref<RExp> rexp, zmm::String text, int nmatch);
protected:    
    zmm::Ref<RExp> rexp;
    zmm::String text;
    char *ptr;
    int nmatch;
    regmatch_t *pmatch;
    
    friend class RExp;
};


#endif // __REXP_H__

