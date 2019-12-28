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

#include "parser.h"

using namespace zmm;
using namespace mxml;

#include "mxml.h"
#include "util/tools.h"

void XMLCALL Parser::element_start(void *userdata, const char *name, const char **attrs)
{
    auto *parser = (Parser *)userdata;
    Ref<Element> el(new Element(name));
    for (int i = 0; attrs[i]; i += 2) 
    {
        el->addAttribute(attrs[i], attrs[i + 1]);
    }

    if (parser->document->getRoot() == nullptr)
    {
        parser->document->setRoot(el);
        parser->curEl = el;
    }
    else if (parser->curEl != nullptr)
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
    auto *parser = (Parser *)userdata;
    parser->curEl = parser->elements->pop();
}

void XMLCALL Parser::character_data(void *userdata, const XML_Char *s, int len)
{
    auto *parser = (Parser *)userdata;
    std::string text = std::string(s, len);
    
    if (!text.empty())
    {
        Ref<Text> textEl(new Text(text));
        if (parser->curEl == nullptr)
            parser->document->appendChild(RefCast(textEl, Node));
        else
            parser->curEl->appendChild(RefCast(textEl, Node));
    }
}

void XMLCALL Parser::comment_callback(void *userdata, const XML_Char *s)
{
    auto *parser = (Parser *)userdata;
    std::string text = s;
    if (!text.empty())
    {
        Ref<Comment> cm(new Comment(text));
        if (parser->curEl == nullptr)
            parser->document->appendChild(RefCast(cm, Node));
        else
            parser->curEl->appendChild(RefCast(cm, Node));
    }
}

void XMLCALL Parser::default_callback(void *userdata, const XML_Char *s, int len)
{
    auto *parser = (Parser *)userdata;
    std::string text = std::string(s, len);
    
    if (text.at(0) == '<')
        parser->ignoreNextDefaultNewline = true;
    else
    {
        bool add_as_character_data = true;
        if (parser->ignoreNextDefaultNewline)
        {
            parser->ignoreNextDefaultNewline = false;
            if (text == "\n")
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

Ref<Document> Parser::parseFile(std::string filename)
{
    Ref<Context> ctx(new Context(filename));
    return parse(ctx, read_text_file(filename));
}

Ref<Document> Parser::parseString(std::string str)
{
    Ref<Context> ctx(new Context(""));
    return parse(ctx, str);
}

Ref<Document> Parser::parse(Ref<Context> ctx, std::string input)
{
    XML_Parser parser = XML_ParserCreate(nullptr);
    if (!parser)
        throw Exception("Unable to allocate XML parser");

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
        std::string message = XML_ErrorString(XML_GetErrorCode(parser));
        XML_ParserFree(parser);
        throw ParseException(message, ctx);
    }

    XML_ParserFree(parser);
    return document;
}
