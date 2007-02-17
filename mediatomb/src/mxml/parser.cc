/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    parser.cc - this file is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>
    
    Copyright (C) 2006-2007 Gena Batyan <bgeradz@mediatomb.cc>,
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

/// \file parser.cc

#ifdef HAVE_CONFIG_H
    #include "autoconfig.h"
#endif

#include "parser.h"

using namespace zmm;
using namespace mxml;

#include "mxml.h"

enum xml_parse_state
{
	XML_SKIP,
	XML_TAG_NAME,
	XML_TAG_SKIP,
    XML_TAG_OPEN,
    XML_CLOSE_TAG_NAME,
	XML_END_SLASH,
	XML_ATTR_NAME,
	XML_ATTR_QUOTE,
	XML_ATTR_VALUE,
	XML_TAG_CLOSE,
    XML_DECL_TEXT,
    XML_DECL_CLOSE,
    XML_PARSE_TAG,
    XML_PARSE_TAG_TEXT,
    XML_COMMENT_START,
    XML_COMMENT_START_NEXT,
    XML_COMMENT_SKIP,
    XML_COMMENT_END_SKIP,
    XML_COMMENT_END
};

int readChar(Ref<Context> ctx, Ref<Input> input)
{
	int res = input->readChar();
	if(res == '\r')
		res = input->readChar();
	if(res < 0)
		return res;
	if(res == '\n')
	{
		ctx->line ++;
		ctx->col = 1;
	}
	else
		ctx->col ++;
	return res;
}

int isSkip(char c)
{
	return (c == ' ' || c == '\t' || c == '\r' || c == '\n');
}

int isID(char c)
{
	return
	(
		(c >= 'A' && c <= 'Z') ||
		(c >= 'a' && c <= 'z') ||
		(c >= '0' && c <= '9') ||
		c == '-' || c == '_' || c == ':'
	);
}


Parser::Parser()
{

}
Ref<Element> Parser::parseFile(String filename)
{
    Ref<Input> input(new FileInput(filename));
	Ref<Context> ctx(new Context(filename));
	return parse(ctx, input, nil, XML_SKIP);
}

Ref<Element> Parser::parseString(String str)
{
    Ref<Input> input(new StringInput(str));
	Ref<Context> ctx(new Context(_("")));
	return parse(ctx, input, nil, XML_SKIP);
}

#define THROW_ERROR(msg) throw ParseException(msg, ctx)

                
Ref<Element> Parser::parse(Ref<Context> ctx, Ref<Input> input, String parentTag, int state)
{
//    log_debug("parse: %s\n", parentTag.c_str());
	Ref<StringBuffer> buf = Ref<StringBuffer>(new StringBuffer());

	Ref<Element> element;
	Ref<Attribute> attr;

	int ic;
	char c;

	while((ic = readChar(ctx, input)) > 0)
	{
		c = (char)ic;
        
//      log_info("state: %s, buf: %s  char:%c\n", getStateName(state), buf->toString().c_str(), c);

		switch(state)
		{
            case XML_SKIP:
            {
                if(isSkip(c))
                    break;
                if(c == '<')
                {
                    state = XML_TAG_OPEN;
                    break;
                }
                THROW_ERROR(_("unexpected symbol:") + c);
            }
            case XML_TAG_OPEN:
            {
                if(c == '/')
                {
                    state = XML_CLOSE_TAG_NAME;
                    break;
                }
                else if(c == '?')
                {
                    state = XML_DECL_TEXT;
                    break;
                }
                else if (c == '!')
                {
                    state = XML_COMMENT_START;
                    break;
                }
                else
                {
                    *buf << c;
                    state = XML_TAG_NAME;
                    break;
                }
            }
            case XML_COMMENT_START:
            {
                if (c == '-')
                {
                    state = XML_COMMENT_START_NEXT;
                    break;
                }
                
                THROW_ERROR(_("unexpected symbol:") + c);
            }
            case XML_COMMENT_START_NEXT:
            {
                if (c == '-')
                {
                    state = XML_COMMENT_SKIP;
                    break;
                }

                THROW_ERROR(_("unexpected symbol:") + c);
            }
            case XML_COMMENT_SKIP:
            {
                if (c == '-')
                {
                    state = XML_COMMENT_END_SKIP;
                    break;
                }

                state = XML_COMMENT_SKIP;
                break;
            }
            case XML_COMMENT_END_SKIP:
            {
                if (c == '-')
                {
                    state = XML_COMMENT_END;
                }
                else
                {
                    state = XML_COMMENT_SKIP;
                }
                break;
            }
            case XML_COMMENT_END:
            {
                if (c == '>')
                {
                    state = XML_SKIP;
                }
                else
                {
                    state = XML_COMMENT_SKIP;
                }
                break;
            }
            case XML_DECL_TEXT:
            {
                if(c == '?')
                {
                    state = XML_DECL_CLOSE;
                }
                break;
            }
            case XML_DECL_CLOSE:
            {
                if(c == '>')
                {
                    state = XML_SKIP;
                    break;
                }
                else
                {
                    THROW_ERROR(_("unexpected symbol: ") + c);
                }
            }
            case XML_CLOSE_TAG_NAME:
            {
                if(isID(c))
                {
                    *buf << c;
                    break;
                }
                else if(c == '>')
                {
                    String cTag = buf->toString();
                    if(cTag != parentTag)
                    {
                        THROW_ERROR(_("unexpected closing tag: </") + cTag +"> expecting </"+ parentTag +">");
                    }
                    return nil;
                }
                else
                {
                    THROW_ERROR(_("unexpected symbol:") + c);
                }
            }
            case XML_TAG_NAME:
            {
                if(isID(c))
                {
                    *buf << c;
                    break;
                }
                else if(isSkip(c))
                {
                    element = Ref<Element>(new Element(buf->toString(), ctx->clone()));
                    buf->clear();
                    state = XML_TAG_SKIP;
                    break;
                }
                else if(c == '>')
                {
                    element = Ref<Element>(new Element(buf->toString(), ctx->clone()));
                    buf->clear();
                    state = XML_PARSE_TAG;
                    break;
                }
                else if(c == '/')
                {
                    element = Ref<Element>(new Element(buf->toString(), ctx->clone()));
                    buf->clear();
                    state = XML_END_SLASH;
                    break;
                }
                THROW_ERROR(_("unexpected symbol:") + c);
            }
            case XML_PARSE_TAG:
            {
                if(c == '<')
                {
                    Ref<Element> child;
                    int startState = XML_TAG_OPEN;
                    while((child = parse(ctx, input, element->name, startState)) != nil)
                    {
                        startState = XML_SKIP;
                        element->appendChild(child);
                    }
                    return element;
                }
                else if(isSkip(c))
                {
                    *buf << c;
                    break;
                }
                else
                {
                    *buf << c;
                    state = XML_PARSE_TAG_TEXT;
                    break;
                }
            }
            case XML_PARSE_TAG_TEXT:
            {
                if(c == '<')
                {
                    element->setText(buf->toString());
                    buf->clear();
                    // trick to parse he rest of the tag
                    parse(ctx, input, element->name, XML_TAG_OPEN);
                    return element;
                }
                else
                {
                    *buf << c;
                    break;
                }
            }
            case XML_TAG_SKIP:
            {
                if(isSkip(c))
                {
                    break;
                }
                else if(isID(c))
                {
                    *buf << c;
                    state = XML_ATTR_NAME;
                    break;
                }
                else if(c == '>')
                {
                    state = XML_PARSE_TAG;
                    break;
                }
                else if(c == '/')
                {
                    state = XML_END_SLASH;
                    break;
                }
                THROW_ERROR(_("unexpected symbol:") + c);
            }
            case XML_END_SLASH:
            {
                if(c == '>')
                {
                    return element;
                }
                THROW_ERROR(_("unexpected symbol:") + c);
            }
            case XML_ATTR_NAME:
            {
                if(isID(c))
                {
                    *buf << c;
                    break;
                }
                else if(c == '=')
                {
                    attr = Ref<Attribute>(new Attribute(buf->toString()));
                    buf->clear();
                    state = XML_ATTR_QUOTE;
                    break;
                }
                THROW_ERROR(_("unexpected symbol:") + c);
            }
            case XML_ATTR_QUOTE:
            {
                if(c == '"')
                {
                    state = XML_ATTR_VALUE;
                    break;
                }
                THROW_ERROR(_("unexpected symbol:") + c);
            }
            case XML_ATTR_VALUE:
            {
                if(c == '"')
                {
                    attr->setValue(buf->toString());
                    buf->clear();
                    element->addAttribute(attr);
                    state = XML_TAG_SKIP;
                }
                else
                    *buf << c;
                break;
            }
        }
    }
    
    THROW_ERROR(_("unexpected end of input"));
}

