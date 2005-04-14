#include "destroyer.h"

using namespace zmm;

Destroyer::Destroyer(void (* destroy_func)(void *), void *data) : Object()
{
    this->destroy_func = destroy_func;
    this->data = data;
}
Destroyer::~Destroyer()
{
    destroy();
}
void Destroyer::destroy()
{
    if (destroy_func)
    {
        destroy_func(data);
        destroy_func = NULL;
    }
}
