
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

#import "autoconfig.h"
#ifdef HAVE_JS

#include "script.h"
#include "tools.h"

using namespace zmm;

String Script::getProperty(JSObject *obj, String name)
{
    jsval val;
    JSString *str;
    if (!JS_GetProperty(cx, obj, name.c_str(), &val))
        return nil;
    if (val == JSVAL_VOID)
        return nil;
    str = JS_ValueToString(cx, val);
    if (! str)
        return nil;
    return String(JS_GetStringBytes(str));
}
bool Script::getBoolProperty(JSObject *obj, String name)
{
    jsval val;
    JSBool boolVal;

    if (!JS_GetProperty(cx, obj, name.c_str(), &val))
        return -1;
    if (val == JSVAL_VOID)
        return -1;
    if (!JS_ValueToBoolean(cx, val, &boolVal))
        return -1;
    return (boolVal ? 1 : 0);
}
int Script::getIntProperty(JSObject *obj, String name, int def)
{
    jsval val;
    int intVal;

    if (!JS_GetProperty(cx, obj, name.c_str(), &val))
        return def;
    if (val == JSVAL_VOID)
        return def;
    if (!JS_ValueToInt32(cx, val, &intVal))
        return def;
    return intVal;
}

JSObject *Script::getObjectProperty(JSObject *obj, String name)
{
    jsval val;
    JSObject *js_obj;

    if (!JS_GetProperty(cx, obj, name.c_str(), &val))
        return NULL;
    if (val == JSVAL_VOID)
        return NULL;
    if (!JS_ValueToObject(cx, val, &js_obj))
        return NULL;
    return js_obj;
}

void Script::setProperty(JSObject *obj, String name, String value)
{
    jsval val;
    JSString *str = JS_NewStringCopyN(cx, value.c_str(), value.length());
	if (!str)
		return;
	val = STRING_TO_JSVAL(str);
    if (!JS_SetProperty(cx, obj, name.c_str(), &val))
        return;
}

void Script::setIntProperty(JSObject *obj, String name, int value)
{
    jsval val;
    if (!JS_NewNumberValue(cx, (jsdouble)value, &val))
        return;
    if (!JS_SetProperty(cx, obj, name.c_str(), &val))
        return;
}

void Script::setObjectProperty(JSObject *parent, String name, JSObject *obj)
{
    jsval val;
	val = OBJECT_TO_JSVAL(obj);
    if (!JS_SetProperty(cx, parent, name.c_str(), &val))
        return;
}

void Script::deleteProperty(JSObject *obj, String name)
{
    JS_DeleteProperty(cx, obj, name.c_str());
}

static void
js_error_reporter(JSContext *cx, const char *message, JSErrorReport *report)
{
    int n;
    const char *ctmp;

    int reportWarnings = 1; // TODO move to object field

    Ref<StringBuffer> buf(new StringBuffer());

    do
    {
        if (!report)
        {
            *buf << (char *)message;
            break;
        }

        // Conditionally ignore reported warnings.
        if (JSREPORT_IS_WARNING(report->flags) && !reportWarnings)
            return;

        String prefix;
        Ref<StringBuffer> prefix_buf(new StringBuffer());

        if (report->filename)
            *prefix_buf << (char *)report->filename << ":";

        if (report->lineno)
        {
            *prefix_buf << (int)report->lineno << ": ";
        }
        if (JSREPORT_IS_WARNING(report->flags))
        {
            if (JSREPORT_IS_STRICT(report->flags))
                *prefix_buf << "(STRICT WARN)";
            else
                *prefix_buf << "(WARN)";
        }

        prefix = prefix_buf->toString();

        // embedded newlines
        while ((ctmp = strchr(message, '\n')) != 0)
        {
            ctmp++;
            if (prefix.length())
                *buf << prefix;
            *buf << String((char *)message, ctmp - message);
            message = ctmp;
        }

        // If there were no filename or lineno, the prefix might be empty
        if (prefix.length())
            *buf << prefix;
        *buf << (char *)message << "\n";

        if (report->linebuf)
        {
            // report->linebuf usually ends with a newline.
            n = strlen(report->linebuf);
            *buf << prefix << (char *)report->linebuf;
            *buf << (char *)((n > 0 && report->linebuf[n-1] == '\n') ? "" : "\n");
            *buf << prefix;
            /*
            n = PTRDIFF(report->tokenptr, report->linebuf, char);
            for (i = j = 0; i < n; i++)
            {
                if (report->linebuf[i] == '\t')
                {
                    for (k = (j + 8) & ~7; j < k; j++)
                    {
                        fputc('.', gErrFile);
                    }
                    continue;
                }
                fputc('.', stdout);
                j++;
            }
            fputs("^\n", stdout);
            */
        }
    }
    while (0);

    String err = buf->toString();
    log_info(("%s\n", err.c_str()));
}

/* **************** */

Script::Script(Ref<Runtime> runtime, JSContext *cx) : Object()
{
    this->runtime = runtime;
    rt = runtime->getRT();
    glob = NULL;
	script = NULL;

    /* create a context and associate it with the JS run time */
    lock();
    if (! cx)
        cx = JS_NewContext(rt, 8192);
    unlock();
    if (! cx)
        throw Exception(_("Scripting: could not initialize js context"));
    this->cx = cx;
    JS_SetErrorReporter(cx, js_error_reporter);
}
Script::~Script()
{
    lock();
    if (script)
        JS_DestroyScript(cx, script);
    if (cx)
		JS_DestroyContext(cx);
    unlock();
}

void Script::setGlobalObject(JSObject *glob)
{
    this->glob = glob;
    JS_SetGlobalObject(cx, glob);
}
JSObject *Script::getGlobalObject()
{
    return glob;
}
JSContext *Script::getContext()
{
    return cx;
}

void Script::initGlobalObject()
{
    /* create the global object here */
    glob = JS_NewObject(cx, /* global_class */ NULL, NULL, NULL);
    if (! glob)
        throw Exception(_("Scripting: could not initialize glboal class"));

    /* initialize the built-in JS objects and the global object */
    if (! JS_InitStandardClasses(cx, glob))
        throw Exception(_("Scripting: JS_InitStandardClasses failed"));
}

void Script::defineFunctions(JSFunctionSpec *functions)
{
    if (glob == NULL)
        initGlobalObject();
    if (!JS_DefineFunctions(cx, glob, functions))
        throw Exception(_("Scripting: JS_DefineFunctions failed"));
}

void Script::load(zmm::String scriptPath)
{
    String scriptText = read_text_file(scriptPath);
    load(scriptText, scriptPath);
}
void Script::load(String scriptText, String scriptPath)
{
    if (glob == NULL)
        initGlobalObject();
/*
	if (!JS_EvaluateScript(cx, glob, script.c_str(), script.length(),
		scriptPath.c_str(), 0, &ret_val))
	{
             throw Exception(_("Scripting: failed to evaluate script");
	}
*/

    script = JS_CompileScript(cx, glob, scriptText.c_str(), scriptText.length(),
                              scriptPath.c_str(), 1);
    if (! script)
        throw Exception(_("Scripting: failed to compile ") + scriptPath);
}
void Script::execute()
{
    jsval ret_val;

    if (!JS_ExecuteScript(cx, glob, script, &ret_val))
        throw Exception(_("Script: failed to execute script"));
}

#endif // HAVE_JS

