#ifndef __SCRIPTING_H__
#define __SCRIPTING_H__

#define XP_UNIX 1
#include <jsapi.h>
#include "common.h"
#include "cds_objects.h"

class Scripting : public zmm::Object
{
protected:
    JSVersion version;
    JSRuntime *rt;
    JSContext *cx;
    JSObject  *glob;
	JSScript *script;
public:
	Scripting();
	virtual ~Scripting();
	void init();
	void processCdsObject(zmm::Ref<CdsObject> obj);	
};

#endif // __SCRIPTING_H__

