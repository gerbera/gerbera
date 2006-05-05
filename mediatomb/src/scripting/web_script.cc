
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

#include "autoconfig.h"

#ifdef HAVE_JS

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#include "web_script.h"
#include "web_request_handler.h"

#define JSSP_TAG_OPEN "<%"
#define JSSP_TAG_CLOSE "%>"

using namespace zmm;
using namespace mxml;

#include "tools.h"

void WebScript::xml2js(Ref<Element> el, JSObject *js)
{
    int i, count;

    setProperty(js, _("name"), el->getName());
    
    // setting element's attributes
    count = el->attributeCount();
    if (count)
    {
        JSObject *attr_js = JS_NewObject(cx, NULL, NULL, js);
        for (i = 0; i < count; i++)
        {
            Ref<Attribute> attr = el->getAttribute(i);
            setProperty(attr_js, attr->name, attr->value);
        }
        setObjectProperty(js, _("attributes"), attr_js);
    }
    
    String text = el->getText();
    if (text != nil)
    {
        setProperty(js, _("text"), text);
    }
    else
    {
        // setting element's children
        JSObject *children_js = JS_NewArrayObject(cx, 0, NULL);
        count = el->childCount();
        for (i = 0; i < count; i++)
        {
            Ref<Element> child = el->getChild(i);
            JSObject *child_js = JS_NewObject(cx, NULL, NULL, js);
            xml2js(child, child_js);
            jsval child_val = OBJECT_TO_JSVAL(child_js);
            JS_SetElement(cx, children_js, i, &child_val);
        }
        setObjectProperty(js, _("children"), children_js);
    }
}    


/* **************** */
static JSBool
js_echo(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    WebScript *self = (WebScript *)JS_GetContextPrivate(cx);
    JSString *str;

    str = JS_ValueToString(self->cx, argv[0]);
    if (!str)
        return JS_FALSE;
    *self->output << JS_GetStringBytes(str);
    return JS_TRUE;
}
static JSBool
js_frag(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    WebScript *self = (WebScript *)JS_GetContextPrivate(cx);

    int index;
    JSBool ret = JS_ValueToInt32(self->cx, argv[0], &index);
    if (!ret)
        return JS_FALSE;
    if (index < 0 || index >= self->fragments->size())
        return JS_FALSE;
    *self->output << self->fragments->get(index);
    return JS_TRUE;
}
static JSBool
js_include(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    WebScript *self = (WebScript *)JS_GetContextPrivate(cx);
    JSString *str;

    str = JS_ValueToString(self->cx, argv[0]);
    if (!str)
        return JS_FALSE;
    String subscriptFilename(JS_GetStringBytes(str));
   
    try
    {
        if (self->includeIndex < self->includedScripts->size())
        {
            Ref<WebScript> subscript = self->includedScripts->get(self->includeIndex);
            *self->output << subscript->process();
        }
        else
        {
            String subscriptPath = WebRequestHandler::buildScriptPath(subscriptFilename);
            Ref<WebScript> subscript(new WebScript(Runtime::getInstance(),
                                     subscriptPath, self->getGlobalObject()));
            *self->output << subscript->processUnlocked();
            self->includedScripts->append(subscript);
        }
    }
    catch (Exception e)
    {
        log_warning(("WebScript: failed to include script %s : %s\n",
                    subscriptFilename.c_str(), e.getMessage().c_str()));
        return JS_FALSE;
    }
    self->includeIndex++;
    
    return JS_TRUE;
}

static JSBool
js_log(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    uintN i;
    JSString *str;

    for (i = 0; i < argc; i++) {
        str = JS_ValueToString(cx, argv[i]);
        if (!str)
            return JS_FALSE;
        log_info(("%s%s\n", i ? " " : "", JS_GetStringBytes(str)));
    }
    return JS_TRUE;
}

static JSFunctionSpec global_functions[] = {
    {"echo",    js_echo,    1},
    {"frag",    js_frag,    1},
    {"include", js_include, 1},
    {"log",     js_log,     0},
    {0}
};

WebScript::WebScript(Ref<Runtime> runtime, String srcPath, JSObject *glob) : Script(runtime)
{
    this->srcPath = srcPath;
    srcCompileTime = 0L;
    lock();
    try
    {
        JS_SetContextPrivate(cx, this);
        if (glob)
            setGlobalObject(glob);
        defineFunctions(global_functions);
    }
    catch (Exception e)
    {
        unlock();
        throw e;
    }
    unlock();
}

String WebScript::process(Ref<Element> root)
{
    lock();
    try
    {
        String ret = processUnlocked(root);
        unlock();
        return ret;
    }
    catch (Exception e)
    {
        unlock();
        throw e;
    }
}

String WebScript::processUnlocked(Ref<Element> root)
{
    output = Ref<StringBuffer>(new StringBuffer());
    checkCompile();
    if (root != nil)
    {
        JSObject *root_js = JS_NewObject(cx, NULL, NULL, glob);
        xml2js(root, root_js);
        setObjectProperty(glob, _("root"), root_js);
        if (sessionID == nil)
            deleteProperty(glob, _("SID"));
        else
            setProperty(glob, _("SID"), sessionID);
    }
    includeIndex = 0;

    execute();
    String ret = output->toString();
    output = nil;
    
    if (root != nil)
    {
        deleteProperty(glob, _("root"));
        deleteProperty(glob, _("SID"));
    }
    return ret;
}

void WebScript::checkCompile()
{
    long time = getMtime();
    if (time > srcCompileTime)
    {
        String content = read_text_file(srcPath);
        compileTemplate(content);
        srcCompileTime = time;
    }
}

long WebScript::getMtime()
{
    struct stat st;

    if (stat(srcPath.c_str(), &st) < 0)
    {
        throw Exception(_("jssp: template not found: ") + srcPath +
                        " : " + strerror(errno));
    }
    return st.st_mtime;
}

/*
static String js_escape(String str)
{
    int len = str.length();
    StringBase *ret_base = new StringBase(len * 2);
    String ret(ret_base);

    char *src = str.c_str();
    char *dest = ret.c_str();

    char c;
    for (int i = 0; i < len; i++)
    {
        c = *(src++);
        switch (c)
        {
            case '\'' :
                *dest++ = '\\';
                break;
            case '\\' :
                *dest++ = '\\';
                break;
        }
        *dest++ = c;
    }
    ret_base->len = dest - ret.c_str();
    *dest = 0;
    return ret;
}
*/

void WebScript::printFrag(String str)
{
    int index = fragments->size();
    fragments->append(str);
    *output << "frag(" << index << ");";
}
void WebScript::compileTemplate(String contents)
{
    fragments = Ref<Array<StringBase> >(new Array<StringBase>());
    includedScripts = Ref<Array<WebScript> >(new Array<WebScript>());

    char *pos = contents.c_str();

    char *start, *end;
    int len;

    while (*pos)
    {
        start = strstr(pos, JSSP_TAG_OPEN);
        if (start == NULL) // append printing of the rest of the file
        {
            printFrag(String(pos));
            break;
        }
        len = start - pos;
        if (len > 0)
            printFrag(String(pos, len));
        start += 2; // skip open tag
        
        if (! *start)
            throw Exception(_("jssp: template error: unclosed tag"));

        bool print_tag = false;
        if (*start == '=')
        {
            print_tag = true; // check for special print tag
            start++;
        }

        end = strstr(start, JSSP_TAG_CLOSE);
        if (end == NULL) // unclosed tag
            throw Exception(_("jssp: template error: unclosed tag"));
        len = end - start;
        
        String code(start, len);
        if (print_tag)
            *output << "echo(" << code << ");";
        else
            *output << code;

        pos = end + 2;
    }

    String ret = output->toString();
    load(ret, _("jssp_compiled ") + srcPath);
    output->clear();

//    log_debug(("TPL:--------------\n%s\n--------------\n", ret.c_str()));
}

#endif // ifdef HAVE_JS

