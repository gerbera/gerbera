#ifndef __DESTROYER_H__
#define __DESTROYER_H__

#include "common.h"

class Destroyer : public zmm::Object
{
protected:
    void *data;
    void (* destroy_func)(void *);
public:
    Destroyer(void (* destroy_func)(void *), void *data);
    virtual ~Destroyer();
    void destroy();
};


#endif // __DESTROYER_H__
