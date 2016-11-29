/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    parser_expat.cc - this file is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>
    
    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>
    
    MediaTomb is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.
    
    MediaTomb is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    version 2 along with MediaTomb; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
    
    $Id$
*/

/// \file parser_expat.cc

#ifdef HAVE_EXPAT

#include "parser.h"

using namespace zmm;
using namespace mxml;

#include "mxml.h"
#include "tools.h"

void XMLCALL Parser::element_start(void *userdata, const char *name, const char **attrs)
{
    Parser *parser = (Parser *)userdata;
    Ref<Element> el(new Element(name));
    for (int i = 0; attrs[i]; i += 2) 
    {
        el->addAttribute(attrs[i], attrs[i + 1]);
    }

    if (parser->document->getRoot() == nil)
    {
        parser->document->setRoot(el);
        parser->curEl = el;
    }
    else if (parser->curEl != nil)
    {
        parser->curEl->appendElementChild(el);
        parser->elements->push(parser->curEl);
        parser->curEl = el;
    }
    else
    {
        // second root? - should not happen...
        print_backtrace();
    }
}

void XMLCALL Parser::element_end(void *userdata, const char *name)
{
    Parser *parser = (Parser *)userdata;
    parser->curEl = parser->elements->pop();
}

void XMLCALL Parser::character_data(void *userdata, const XML_Char *s, int len)
{
    Parser *parser = (Parser *)userdata;
    String text = String(s, len);
    
    if (text != nil)
    {
        Ref<Text> textEl(new Text(text));
        if (parser->curEl == nil)
            parser->document->appendChild(RefCast(textEl, Node));
        else
            parser->curEl->appendChild(RefCast(textEl, Node));
    }
}

void XMLCALL Parser::comment_callback(void *userdata, const XML_Char *s)
{
    Parser *parser = (Parser *)userdata;
    String text = s;
    if (text != nil)
    {
        Ref<Comment> cm(new Comment(text));
        if (parser->curEl == nil)
            parser->document->appendChild(RefCast(cm, Node));
        else
            parser->curEl->appendChild(RefCast(cm, Node));
    }
}

void XMLCALL Parser::default_callback(void *userdata, const XML_Char *s, int len)
{
    Parser *parser = (Parser *)userdata;
    String text = String(s, len);
    
    if (text.charAt(0) == '<')
        parser->ignoreNextDefaultNewline = true;
    else
    {
        bool add_as_character_data = true;
        if (parser->ignoreNextDefaultNewline)
        {
            parser->ignoreNextDefaultNewline = false;
            if (text == _("\n"))
                add_as_character_data = false;
        }
        if (add_as_character_data)
            character_data(userdata, s, len);
    }
    
}

Parser::Parser()
{
    ignoreNextDefaultNewline = false;
}

Ref<Document> Parser::parseFile(String filename)
{
    Ref<Context> ctx(new Context(filename));
    return parse(ctx, read_text_file(filename));
}

Ref<Document> Parser::parseString(String str)
{
    Ref<Context> ctx(new Context(_("")));
    return parse(ctx, str);
}

Ref<Document> Parser::parse(Ref<Context> ctx, String input)
{
    XML_Parser parser = XML_ParserCreate(NULL);
    if (!parser)
        throw Exception(_("Unable to allocate XML parser"));

    XML_SetUserData(parser, this);
    XML_SetElementHandler(parser, Parser::element_start, Parser::element_end);
    XML_SetCharacterDataHandler(parser, Parser::character_data);
    XML_SetCommentHandler(parser, Parser::comment_callback);
    XML_SetDefaultHandler(parser, Parser::default_callback);

    document = Ref<Document>(new Document());
    elements = Ref<ObjectStack<Element> >(new ObjectStack<Element>(8));

    if (XML_Parse(parser, input.c_str(), input.length(), 1) != XML_STATUS_OK)
    {
        ctx->line = XML_GetCurrentLineNumber(parser);
        ctx->col = XML_GetCurrentColumnNumber(parser);
        String message = XML_ErrorString(XML_GetErrorCode(parser));
        XML_ParserFree(parser);
        throw ParseException(message, ctx);
    }

    XML_ParserFree(parser);
    return document;
}

#endif
