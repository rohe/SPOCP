/***************************************************************************
                          proto.h  -  description
                             -------------------
    begin                : Mon Jul 22 2002
    copyright            : (C) 2002 by SPOCP
    email                : roland@catalogix.se
 ***************************************************************************/

#ifndef __PROTO_H__
#define __PROTO_H__

#define ASCII_LOWER(c)   ( (c) >= 'a' && (c) <= 'z' )
#define ASCII_UPPER(c)   ( (c) >= 'A' && (c) <= 'Z' )
#define DIGIT(c)         ( (c) >= '0' && (c) <= '9' )
#define ALPHA(c)         ( ASCII_LOWER(c) || ASCII_UPPER(c) )
#define HEX_LOWER(c)     ( (c) >= 'a' && (c) <= 'f' )
#define HEX_UPPER(c)     ( (c) >= 'A' && (c) <= 'F' )
#define HEX(c)           ( DIGIT(c) || HEX_LOWER(c) || HEX_UPPER(c) )
#define BASE64CHAR(c)    ( ASCII_LOWER(c) || ASCII_UPPER(c) || DIGIT(c) || (c) == '+' || (c) == '/' )
#define WHITESPACE(c)    ( (c) == ' ' || (c) == '\n' || (c) == '\t' || (c) == '\v' || (c) == '\r' || (c) == '\f' )
#define TCHAR(c)         ( (c) == '-' || (c) == '.' || (c) ==  '/' || (c) ==  '_' || (c) ==  ':' || (c) ==  '*' || (c) ==  '+' || (c) ==  '=' )
/*
#define TOKENCHAR(c)     ( TCHAR(c) || ASCII_LOWER(c) || ASCII_UPPER(c) || DIGIT(c) ) 
*/
#define LISTDELIM(c)     ( (c) == '(' || (c) == ')' )
#define TOKENCHAR(c)     ( !WHITESPACE(c) && !LISTDELIM(c) )
#define CONTROLCHAR(c)   ( ((c) >= 0x00 && (c) <= 0x06) || ((c) >= 0x0e && (c) <= 0x1f ))

#define COMMENTSTART  0x23 /* # */
#define LF            0x0A /* linefeed */

#endif
