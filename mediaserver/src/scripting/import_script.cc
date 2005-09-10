
/*  scripting.cc - this file is part of MediaTomb.
                                                                                
    Copyright (C) 2005 Gena Batyan <bgeradz@deadlock.dhs.org>,
                       Sergey Bostandzhyan <jin@deadlock.dhs.org>
                                                                                
    MediaTomb is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
                                                                                
    MediaTomb is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
                                                                                
    You should have received a copy of the GNU General Public License
    along with MediaTomb; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifdef HAVE_JS

#include "import_script.h"
#include "storage.h"
#include "content_manager.h"
#include "metadata_handler.h"
#include "config_manager.h"
#include "tools.h"

using namespace zmm;

Ref<CdsObject> ImportScript::jsObject2cdsObject(JSObject *js)
{
    String val;
    int objectType;
    int b;
    int i;

    objectType = getIntProperty(js, "objectType", -1);
    if (objectType == -1)
    {
        log_info(("scripting: addObject: missing objectType property\n"));
        return nil;
    }

    Ref<CdsObject> obj = CdsObject::createObject(objectType);

    // CdsObject
    obj->setVirtual(1); // JS creates only virtual objects

    i = getIntProperty(js, "id", -333);
    if (i != -333)
        obj->setID(i);
    i = getIntProperty(js, "refID", -333);
    if (i != -333)
        obj->setRefID(i);
    i = getIntProperty(js, "parentID", -333);
    if (i != -333)
        obj->setParentID(i);
    val = getProperty(js, "title");
    if (val != nil)
        obj->setTitle(val);
    val = getProperty(js, "class");
    if (val != nil)
        obj->setClass(val);
    val = getProperty(js, "location");
    if (val != nil)
        obj->setLocation(val);
    b = getBoolProperty(js, "restricted");
    if (b >= 0)
        obj->setRestricted(b);

    JSObject *js_meta = getObjectProperty(js, "meta");
    if (js_meta)
    {
        JSIdArray *js_meta_ids = JS_Enumerate(cx, js_meta);
        if (js_meta_ids)
        {
            for (int i = 0; i < js_meta_ids->length; i++)
            {
                jsid id = js_meta_ids->vector[i];
                log_debug(("META ID: %d\n", id));
            }
        }
        JS_DestroyIdArray(cx, js_meta_ids);

        /// \TODO: only metadata enumerated in MT_KEYS is taken
        for (int i = 0; i < M_MAX; i++)
        {
            val = getProperty(js_meta, MT_KEYS[i].upnp);
            if (val != nil)
                obj->setMetadata(MT_KEYS[i].upnp, val);
        }
    }

    // CdsItem
    if (IS_CDS_ITEM(objectType))
    {
        Ref<CdsItem> item = RefCast(obj, CdsItem);

        val = getProperty(js, "mimetype");
        if (val != nil)
            item->setMimeType(val);

        if (IS_CDS_ACTIVE_ITEM(objectType))
        {
            Ref<CdsActiveItem> aitem = RefCast(obj, CdsActiveItem);
            val = getProperty(js, "action");
            if (val != nil)
                aitem->setAction(val);
            val = getProperty(js, "state");
            if (val != nil)
                aitem->setState(val);
        }
    }

    // CdsDirectory
    if (IS_CDS_CONTAINER(objectType))
    {
        Ref<CdsContainer> cont = RefCast(obj, CdsContainer);
        i = getIntProperty(js, "updateID", -1);
        if (i >= 0)
            cont->setUpdateID(i);
        b = getBoolProperty(js, "searchable");
        if (b >= 0)
            cont->setSearchable(b);
    }

    return obj;
}

void ImportScript::cdsObject2jsObject(Ref<CdsObject> obj, JSObject *js)
{
	String val;
	int i;

	int objectType = obj->getObjectType();

    // CdsObject
	setIntProperty(js, "objectType", objectType);

    i = obj->getID();
    if (i != INVALID_OBJECT_ID)
        setIntProperty(js, "id", i);
    i = obj->getParentID();
    if (i != INVALID_OBJECT_ID)
        setIntProperty(js, "parentID", i);
    val = obj->getTitle();
    if (val != nil)
        setProperty(js, "title", val);
    val = obj->getClass();
    if (val != nil)
        setProperty(js, "class", val);
    val = obj->getLocation();
    if (val != nil)
        setProperty(js, "location", val);
	// TODO: boolean type
	i = obj->isRestricted();
    setIntProperty(js, "restricted", i);
   
    // setting metadata
    {
    	JSObject *meta_js = JS_NewObject(cx, NULL, NULL, js);
        Ref<Dictionary> meta = obj->getMetadata();
        Ref<Array<DictionaryElement> > elements = meta->getElements();
        int len = elements->size();
        for (int i = 0; i < len; i++)
        {
            Ref<DictionaryElement> el = elements->get(i);
            setProperty(meta_js, el->getKey().c_str(), el->getValue().c_str());
        }
    	setObjectProperty(js, "meta", meta_js);
    }

    // setting auxdata
    {
    	JSObject *aux_js = JS_NewObject(cx, NULL, NULL, js);
        Ref<Dictionary> aux = obj->getAuxData();
        Ref<Array<DictionaryElement> > elements = aux->getElements();
        int len = elements->size();
        for (int i = 0; i < len; i++)
        {
            Ref<DictionaryElement> el = elements->get(i);
            setProperty(aux_js, el->getKey().c_str(), el->getValue().c_str());
        }
    	setObjectProperty(js, "aux", aux_js);
    }

    /// \todo add resources
    
    // CdsItem
    if (IS_CDS_ITEM(objectType))
    {
        Ref<CdsItem> item = RefCast(obj, CdsItem);
		val = item->getMimeType();
		if (val != nil)
			setProperty(js, "mimetype", val);

        if (IS_CDS_ACTIVE_ITEM(objectType))
        {
            Ref<CdsActiveItem> aitem = RefCast(obj, CdsActiveItem);
			val = aitem->getAction();
			if (val != nil)
				setProperty(js, "action", val);
			val = aitem->getState();
			if (val != nil)
				setProperty(js, "state", val);
        }
    }

    // CdsDirectory
    if (IS_CDS_CONTAINER(objectType))
    {
        Ref<CdsContainer> cont = RefCast(obj, CdsContainer);
        // TODO: boolean type, hide updateID
        i = cont->getUpdateID();
        setIntProperty(js, "updateID", i);
        
        i = cont->isSearchable();
        setIntProperty(js, "searchable", i);
    }
}

/* native js functions */
extern "C" {

static JSBool
js_print(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    uintN i;
    JSString *str;

    for (i = 0; i < argc; i++) {
        str = JS_ValueToString(cx, argv[i]);
        if (!str)
            return JS_FALSE;
        log_info(("%s%s", i ? " " : "", JS_GetStringBytes(str)));
    }
    log_info(("\n"));
    return JS_TRUE;
}

static JSBool
js_addCdsObject(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{ 
    ImportScript *self = (ImportScript *)JS_GetContextPrivate(cx);

    try
    {
        jsval arg;

        JSObject *js_cds_obj;

        arg = argv[0];
        if (!JSVAL_IS_OBJECT(arg))
            return JS_FALSE;
        if (!JS_ValueToObject(cx, arg, &js_cds_obj))
            return JS_FALSE;

        int id;

        Ref<CdsObject> cds_obj = self->jsObject2cdsObject(js_cds_obj);

        Ref<Storage> storage = Storage::getInstance();
        Ref<CdsObject> db_obj = storage->findObjectByTitle(cds_obj->getTitle(),
                                                           cds_obj->getParentID());
        if (db_obj == nil)
        {
            ContentManager::getInstance()->addObject(cds_obj);
            id = cds_obj->getID();
        }
        else
        {
            id = db_obj->getID();
        }

        /* setting object ID as return value */
    	*rval = INT_TO_JSVAL(id);

        return JS_TRUE;
    }
    catch (Exception e)
    {
        e.printStackTrace();
    }
    return JS_FALSE;
}

} // extern "C"

static JSFunctionSpec global_functions[] = {
    {"print",           js_print,          0},
    {"addCdsObject",    js_addCdsObject,   1},
    {0}
};


/* **************** */

ImportScript::ImportScript(Ref<Runtime> runtime) : Script(runtime)
{
    JS_SetContextPrivate(cx, this);
    defineFunctions(global_functions);

    /* initialize contstants */
    setIntProperty(glob, "OBJECT_TYPE_CONTAINER",
                          OBJECT_TYPE_CONTAINER);
    setIntProperty(glob, "OBJECT_TYPE_ITEM",
                          OBJECT_TYPE_ITEM);
    setIntProperty(glob, "OBJECT_TYPE_ACTIVE_ITEM",
                          OBJECT_TYPE_ACTIVE_ITEM);
    setIntProperty(glob, "OBJECT_TYPE_ITEM_EXTERNAL_URL",
                          OBJECT_TYPE_ITEM_EXTERNAL_URL);

    for (int i = 0; i < M_MAX; i++)
    {
        setProperty(glob, MT_KEYS[i].sym, MT_KEYS[i].upnp);
    }
    
    String scriptPath = ConfigManager::getInstance()->getOption("/import/script");
    load(scriptPath);
}

void ImportScript::processCdsObject(Ref<CdsObject> obj)
{
    JSObject *orig = JS_NewObject(cx, NULL, NULL, glob);
    cdsObject2jsObject(obj, orig);
    setObjectProperty(glob, "orig", orig);
    execute();
}


#endif // HAVE_JS

