
#include "rexp.h"

using namespace zmm;

static String error_string(int code, regex_t *regex)
{
    int size = regerror(code, regex, NULL, 0);
    String buf = String::allocate(size);
    regerror(code, regex, buf.c_str(), size);
    buf.setLength(size - 1);
    return buf;
}

RExp::RExp() : Object()
{
    isCompiled = false;
}
RExp::~RExp()
{
    if (isCompiled)
    {
        regfree(&regex);
    }
}
String RExp::getPattern()
{
    return pattern;
}
void RExp::compile(String pattern, int flags)
{
    int ret;
    flags |= REG_EXTENDED; // alsays use extended regexps
    this->pattern = pattern;
    ret = regcomp(&regex, pattern.c_str(), flags);
    if (ret != 0)
        throw Exception(error_string(ret, &regex));
    isCompiled = true;
}

void RExp::compile(zmm::String pattern, const char *sflags)
{
    int flags = 0;
    char *p = (char *)sflags;
    char c;
    while ((c = *p) != 0)
    {
        switch (c)
        {
            case 'i': flags |= REG_ICASE; break;
            case 's': flags |= REG_NEWLINE; break;
            default: throw Exception(String("RExp: unknown flag: ")+ c);
        }
        p++;
    }
    compile(pattern, flags);
}

Ref<Matcher> RExp::matcher(String text, int nmatch)
{
    return Ref<Matcher>(new Matcher(Ref<RExp>(this), text, nmatch));
}

bool RExp::matches(String text)
{
    Ref<Matcher> matcher(new Matcher(Ref<RExp>(this), text, 0));
    return matcher->next();
}

Matcher::Matcher(zmm::Ref<RExp> rexp, String text, int nmatch)
{
    this->rexp = rexp;
    this->text = text;
    this->ptr = NULL;
    this->nmatch = nmatch;
    if (nmatch)
        this->pmatch = (regmatch_t *)malloc(nmatch * sizeof(regmatch_t));
    else
        this->pmatch = NULL;
}
Matcher::~Matcher()
{
    if (pmatch)
        free(pmatch);
}
String Matcher::group(int i)
{
    regmatch_t *m = pmatch + i;
    if (m->rm_so >= 0)
        return String(ptr + m->rm_so, m->rm_eo - m->rm_so);
    else
        return nil;
}
bool Matcher::next()
{
    int ret;

    if (ptr == NULL) // first match
    {
        ptr = text.c_str();
    }
    else
    {
        if (! *ptr)
            return false;
        ptr += pmatch->rm_eo;
        if (! *ptr)
            return false;
    }
    
    int flags = ((nmatch == 0) ? REG_NOSUB : 0);
    ret = regexec(&rexp->regex, ptr, nmatch, pmatch, flags);
    switch (ret)
    {
        case REG_NOMATCH:
            return false;
        case 0:
            return true;
        default:
            throw Exception(error_string(ret, &rexp->regex));
    }
}

