/*****************************************************************************
 *
 * FILE:	conv.c
 * DESCRIPTION:	WKF: convert kanji code
 * DATE:	Sun, Jan 16 2000
 * UPDATE:	Tue, Jul 20 2004
 * AUTHOR:	Kouichi ABE (WALL) / à¢ïîçNàÍ
 * E-MAIL:	kouichi@MysticWALL.COM
 * URL:		http://www.MysticWALL.COM/
 * COPYRIGHT:	(c) 2000-2005 à¢ïîçNàÍÅ^Kouichi ABE (WALL), All rights reserved.
 * LICENSE:
 *
 *  Copyright (c) 2000-2005 Kouichi ABE (WALL) <kouichi@MysticWALL.COM>,
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   1. Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *
 *   2. Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 *   ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 *   THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 *   PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE
 *   LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *   INTERRUPTION)  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *   CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 *   THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $Id: conv.c,v 1.9 2005/04/07 20:09:36 kouichi Exp $
 *
 *****************************************************************************/

#if	HAVE_CONFIG_H
#include "config.h"
#endif	/* HAVE_CONFIG_H */
#include <stdio.h>
#include <string.h>
#include "kanji.h"
#include "functions.h"
#include "jiscodemap.h"
#include "euccodemap.h"
#include "sjiscodemap.h"
#include "kanacodemap.h"
#include "zenasciimap.h"
#ifdef _WIN32
#define snprintf _snprintf
#endif

/*****************************************************************************
 *
 *	Macros and structures definition
 *
 *****************************************************************************/
#define	LINE_END_LF	"\012"		/* LF in the end of line */
#define	LINE_END_CR	"\015"		/* CR in the end of line */
#define	LINE_END_CRLF	"\015\012"	/* CR+LF in the end of line */

typedef enum {
  ST_UNKNOWN,
  ST_ASCII,
  ST_JIS,
  ST_JIS_X0212,
  ST_KANA,
  ST_JIS_ESC,
  ST_JIS1,
  ST_JIS2,
  ST_JIS_ISO
} jstate_t;

typedef struct _Output {	/* output work structure */
  FILE *	fp;		/* output file pointer */
  String	buf;		/* output string buffer */
  String	end;		/* end of string buffer */
} Output;

/*****************************************************************************
 *
 *	Local functions declaration
 *
 *****************************************************************************/
static void		putChar(int);
static void		putStr(String);
static void		jis2euc(void);
static void		jis2sjis(void);
static void		euc2jis(void);
static void		euc2sjis(void);
static void		sjis2jis(void);
static void		sjis2euc(void);
static void		jis2jis(void);
static void		euc2euc(void);
static void		sjis2sjis(void);
static void		noConv(void);
static jis_state_t	getStateOfJIS(void);

/*****************************************************************************
 *
 *	Local variables declaration
 *
 *****************************************************************************/
static bool	zen2ascii = false;		/* 2byte -> 1byte flag	*/
static String	lineend	  = LINE_END_LF;	/* the end character of line */
static Output	output;				/* output structure */
static bool	wkf_error = false;	/* error flag to convert a string */

/*****************************************************************************
 *
 *	Functions definition
 *
 *****************************************************************************/

/*
 * FUNCTION NAME:	wkfSetLineEndCode
 * DESCRIPTION:		set the code of line end
 * ARGUMENTS:		the code of line end
 * RETURN VALUE:	none
 */
void
wkfSetLineEndCode(le_type)
	lineend_t	le_type;
{
  switch (le_type) {
    case LE_LF:
      lineend = LINE_END_LF;
      break;
    case LE_CR:
      lineend = LINE_END_CR;
      break;
    case LE_CRLF:
      lineend = LINE_END_CRLF;
      break;
    default:
      break;
  }
}

/*
 * FUNCTION NAME:	wkfConvertZenkaku2ASCII
 * DESCRIPTION:		convert 'zenkaku' alphabet and figures to ASCII ones
 * ARGUMENTS:		boolean type (enable: true, otherwise: false)
 * RETURN VALUE:	none
 */
void
wkfConvertZenkaku2ASCII(flag)
	bool	flag;
{
  zen2ascii = flag;
}

/*****************************************************************************/

/*
 * FUNCTION NAME:	setOutputFilePointer
 * DESCRIPTION:		set output file pointer
 * ARGUMENTS:		output file pointer
 * RETURN VALUE:	none
 */
void
setOutputFilePointer(fp)
	FILE *	fp;
{
  output.fp  = fp;
  output.buf = NULL;
  output.end = NULL;
  wkf_error  = false;
}

/*
 * FUNCTION NAME:	setOutputStringBuffer
 * DESCRIPTION:		set output string buffer and its buffer size
 * ARGUMENTS:		output string buffer, output buffer size
 * RETURN VALUE:	none
 */
void
setOutputStringBuffer(str, size)
	String		str;
	unsigned int	size;
{
  output.buf  = str;
  output.end  = str + size;
  output.fp   = NULL;
  memset(output.buf, 0, (size_t)size);
  wkf_error   = false;
}

/*
 * FUNCTION NAME:	putChar
 * DESCRIPTION:		put a character
 * ARGUMENTS:		a character
 * RETURN VALUE:	none
 */
static void
putChar(c)
	int	c;
{
  if (output.fp != NULL) {	/* output is file */
    fputc(c, output.fp);
  }
  /* output is buffer */
  else if (output.buf != NULL && output.end - output.buf > 0) {
    *output.buf++ = (char)c;
  } else {
    wkf_error = true;	/* fatal error: buffer overflow */
#if	0
    fputs("Fatal Error: buffer overflow\n", stderr);
#endif
  }
}

/*
 * FUNCTION NAME:	putStr
 * DESCRIPTION:		put a string
 * ARGUMENTS:		a string
 * RETURN VALUE:	none
 */
static void
putStr(s)
	String	s;
{
  if (output.fp != NULL) {
    fputs(s, output.fp);
  }
  else if (output.buf != NULL) {
    size_t	size = output.end - output.buf;
    if (size > strlen(s)) {
      output.buf += snprintf(output.buf, size, "%s", s);
    }
    else {
      wkf_error = true;	/* fatal error: buffer overflow */
    }
  }
  else {
    wkf_error = true;	/* fatal error: buffer overflow */
#if	0
    fputs("Fatal Error: buffer overflow\n", stderr);
#endif
  }
}

/*****************************************************************************/

/*
 * FUNCTION NAME:	convertKanjiCode
 * DESCRIPTION:		interface to convert kanji code of file/string
 * ARGUMENTS:		input kanji code, output kanji code
 * RETURN VALUE:	converted:    CONV_OK,
 *			no-converted: CONV_NO,
 *			otherwise:    CONV_ERR
 */
conv_t
convertKanjiCode(kin, kout)
	kcode_t	kin;
	kcode_t	kout;
{
  switch (kout) {	/* branch off according to kanji code */
    case KC_JIS:	/* output JIS code */
      switch (kin) {
	case KC_JIS:
	  jis2jis();
	  return wkf_error ? CONV_ERR : CONV_OK;
#if	CODE_EUC_JP
	case KC_EUCorSJIS:
#endif	/* CODE_EUC_JP */
	case KC_EUC:
	  euc2jis();
	  return wkf_error ? CONV_ERR : CONV_OK;
#if	CODE_SJIS
	case KC_EUCorSJIS:
#endif	/* CODE_SJIS */
	case KC_SJIS:
	  sjis2jis();
	  return wkf_error ? CONV_ERR : CONV_OK;
	case KC_ASCII:
	  noConv();
	  return CONV_NO;
	default:
	  return CONV_ERR;
      }
    case KC_EUC:	/* output Japanese EUC code */
      switch (kin) {
	case KC_JIS:
	  jis2euc();
	  return wkf_error ? CONV_ERR : CONV_OK;
#if	CODE_EUC_JP
	case KC_EUCorSJIS:
#endif	/* CODE_EUC_JP */
	case KC_EUC:
	  euc2euc();
	  return wkf_error ? CONV_ERR : CONV_OK;
#if	CODE_SJIS
	case KC_EUCorSJIS:
#endif	/* CODE_SJIS */
	case KC_SJIS:
	  sjis2euc();
	  return wkf_error ? CONV_ERR : CONV_OK;
	case KC_ASCII:
	  noConv();
	  return CONV_NO;
	default:
	  return CONV_ERR;
      }
    case KC_SJIS:	/* output Shift-JIS code */
      switch (kin) {
	case KC_JIS:
	  jis2sjis();
	  return wkf_error ? CONV_ERR : CONV_OK;
#if	CODE_EUC_JP
	case KC_EUCorSJIS:
#endif	/* CODE_EUC_JP */
	case KC_EUC:
	  euc2sjis();
	  return wkf_error ? CONV_ERR : CONV_OK;
#if	CODE_SJIS
	case KC_EUCorSJIS:
#endif	/* CODE_SJIS */
	case KC_SJIS:
	  sjis2sjis();
	  return wkf_error ? CONV_ERR : CONV_OK;
	case KC_ASCII:
	  noConv();
	  return CONV_NO;
	case KC_BROKEN:
	case KC_DATA:
	case KC_UNKNOWN:
	default:
	  return CONV_ERR;
      }
    default:
      break;
  }
  return CONV_ERR;
}

/*
 * FUNCTION NAME:	jis2euc
 * DESCRIPTION:		convert JIS to EUC-JP
 * ARGUMENTS:		none
 * RETURN VALUE:	none
 */
static void
jis2euc(void)
{
  int		c, c1, c2;
  jis_state_t	state = JS_ASCII;

  while ((c = getChar()) != EOF) {
    if (c == ESC) {	/* Escape sequence */
      state = getStateOfJIS();
      continue;
    }
    else if (c == SO) {	/* Shift Out */
      state = JS_JIS7;
      continue;
    }
    else if (c == SI) {	/* Shift In */
      state = JS_ASCII;
      continue;
    }
    switch (state) {
      case JS_ASCII:
	if (c == LF) {
	  putStr(lineend);
	}
	else {
	  putChar(c);
	}
	break;
      case JS_JIS_X0212_1990:	/* JIS X 0212-1990 */
	putChar(SS3);
      case JS_JIS_2BYTE:
	c1 = c;
	c2 = getChar();
	if (zen2ascii && (jis2code_map[c1] == 0x03)) {
	  putChar((int)zen2ascii_map[jis2code_map[c2]]);
	}
	else {
	  putChar((int)(c1 | 0x80));
	  putChar((int)(c2 | 0x80));
	}
	break;
      case JS_JIS7:	/* 1byte (kana) */
      case JS_JIS8:	/* 1byte (kana) */
	c1 = getChar();
	putChar((int)code2euc_map[kana2code_map[c].b1]);
	if (KANA_CHECK(c) &&
	   (c1 == 0xde || c1 == 0x5e)) {	/* sonant mark */
	  putChar((int)code2euc_map[kana2code_map[c].b2 + 1]);
	}
	else if (KANA_CHECK(c) &&
		(c1 == 0xdf || c1 == 0x5f)) { /* semi-sonant mark */
	  putChar((int)code2euc_map[kana2code_map[c].b2 + 2]);
	}
	else {
	  putChar((int)code2euc_map[kana2code_map[c].b2]);
	  ungetChar(c1);
	}
	break;
      default:
	break;
    }
  }
}

/*
 * FUNCTION NAME:	jis2sjis
 * DESCRIPTION:		convert JIS to Shift-JIS
 * ARGUMENTS:		none
 * RETURN VALUE:	none
 */
static void
jis2sjis(void)
{
  int		c, c1, c2;
  jis_state_t	state = JS_ASCII;

  while ((c = getChar()) != EOF) {
    if (c == ESC) {	/* Escape sequence */
      state = getStateOfJIS();
      continue;
    }
    else if (c == SO) {	/* Shift Out */
      state = JS_JIS7;
      continue;
    }
    else if (c == SI) {	/* Shift In */
      state = JS_ASCII;
      continue;
    }
    switch (state) {
      case JS_ASCII:
	if (c == LF) {
	  putStr(lineend);
	}
	else {
	  putChar(c);
	}
	break;
      case JS_JIS_X0212_1990:	/* unable to show in case of Shift-JIS.	*/
	getChar();		/* I discard the second byte character	*/
	putChar(0x81);		/* and show 'Å¨' instead.		*/
	putChar(0xac);
	break;
      case JS_JIS_2BYTE:
	c1 = c;		/* the  first byte of JIS */
	c2 = getChar();	/* the second byte of JIS */
	if (zen2ascii && (jis2code_map[c1] == 0x03)) {
	  putChar(c2);
	}
	else {
	  if (c1 % 2 == 0) {
	    putChar((int)code2sjis_map1[jis2code_map[c1]]);
	    putChar((int)code2sjis_map2_e[jis2code_map[c2]]);
	  }
	  else {
	    putChar((int)code2sjis_map1[jis2code_map[c1]]);
	    putChar((int)code2sjis_map2_o[jis2code_map[c2]]);
	  }
	}
	break;
      case JS_JIS7:	/* 1byte (kana) */
      case JS_JIS8:	/* 1byte (kana) */
	c1 = getChar();
	putChar((int)code2sjis_map1[kana2code_map[c].b1]);
	if (KANA_CHECK(c) &&
	   (c1 == 0xde || c1 == 0x5e)) {	/* sonant mark	*/
	  putChar((int)code2sjis_map2_o[kana2code_map[c].b2 + 1]);
	}
	else if (KANA_CHECK(c) &&
		(c1 == 0xdf || c1 == 0x5f)) {	/* semi-sonant mark */
	  putChar((int)code2sjis_map2_o[kana2code_map[c].b2 + 2]);
	}
	else {
	  putChar((int)code2sjis_map2_o[kana2code_map[c].b2]);
	  ungetChar(c1);
	}
	break;
      default:
	break;
    }
  }
}

/*
 * FUNCTION NAME:	euc2jis
 * DESCRIPTION:		convert EUC-JP to JIS
 * ARGUMENTS:		none
 * RETURN VALUE:	none
 */
static void
euc2jis(void)
{
  int		c, c1, c2;
  bool		zen2ascii_mode;
  jstate_t	state = ST_UNKNOWN;

  zen2ascii_mode = zen2ascii;
  while ((c = getChar()) != EOF) {
euc2jis_loop:
    if (IsCTRL(c) || IsASCII(c)) {
      if ((state == ST_JIS) || (state == ST_JIS_X0212)) {
	putStr(ESC_ISO_ASCII);
	state = ST_ASCII;
	zen2ascii_mode = zen2ascii;
      }
      if (c == LF) {
	putStr(lineend);
      }
      else {
	putChar(c);
      }
    }
    else if ((char)c == SS2) {
      if (state != ST_JIS) {
	putStr(ESC_JIS_NEW);
	state = ST_JIS;
      }
      c  = getChar();
      c1 = getChar();
      if ((char)c1 == SS2) {
	c2 = getChar();
	putChar((int)code2jis_map[kana2code_map[c].b1]);
	if (KANA_CHECK(c) && (c2 == 0xde)) {	/* sonant mark */
	  putChar((int)code2jis_map[kana2code_map[c].b2 + 1]);
	}
	else if (KANA_CHECK(c) && (c2 == 0xdf)) {	/* semi-sonant mark */
	  putChar((int)code2jis_map[kana2code_map[c].b2 + 2]);
	}
	else {
	  putChar((int)code2jis_map[kana2code_map[c].b2]);
	  ungetChar(c2);
	  c = c1;
	  goto euc2jis_loop;
	}
      }
      else {
	putChar((int)code2jis_map[kana2code_map[c].b1]);
	putChar((int)code2jis_map[kana2code_map[c].b2]);
	ungetChar(c1);
      }
    }
    else if ((char)c == SS3) {	/* JIS X 0212-1990 */
      if (state != ST_JIS_X0212) {
	putStr(ESC_JIS_X0212);
	state = ST_JIS_X0212;
      }
      c1 = getChar();	/* the  first byte of EUC-JP */
      c2 = getChar();	/* the second byte of EUC-JP */
      putChar(c1 & 0x7f);
      putChar(c2 & 0x7f);
    }
    else if (IsEUC_JP(c)) {
      c1 = c;		/* the  first byte of EUC-JP */
      c2 = getChar();	/* the second byte of EUC-JP */
      if (zen2ascii && (euc2code_map[c1] == 0x03)) {
	if (!zen2ascii_mode) {
	  putStr(ESC_ISO_ASCII);
	  state = ST_ASCII;
	  zen2ascii_mode = true;
	}
	putChar((int)zen2ascii_map[euc2code_map[c2]]);
      }
      else {
	if (state != ST_JIS) {
	  putStr(ESC_JIS_NEW);
	  state = ST_JIS;
	  zen2ascii_mode = false;
	}
	putChar(c1 & 0x7f);
	putChar(c2 & 0x7f);
      }
    }
    else {	/* output with no converting */
      putChar(c);
    }
  }
#if	1
  if (output.fp == NULL && ((state == ST_JIS) || (state == ST_JIS_X0212))) {
    putStr(ESC_ISO_ASCII);
  }
#endif
}

/*
 * FUNCTION NAME:	euc2sjis
 * DESCRIPTION:		convert EUC-JP to SJIS
 * ARGUMENTS:		none
 * RETURN VALUE:	none
 */
static void
euc2sjis(void)
{
  int	c, c1, c2;

  while ((c = getChar()) != EOF) {
euc2sjis_loop:
    if (IsCTRL(c) || IsASCII(c)) {
      if (c == LF) {
	putStr(lineend);
      }
      else {
	putChar(c);
      }
    }
    else if ((char)c == SS2) {
      c  = getChar();
      c1 = getChar();
      if ((char)c1 == SS2) {
	c2 = getChar();
	putChar((int)code2sjis_map1[kana2code_map[c].b1]);
	if (KANA_CHECK(c) && (c2 == 0xde)) {	/* sonant mark	*/
	  putChar((int)code2sjis_map2_o[kana2code_map[c].b2 + 1]);
	}
	else if (KANA_CHECK(c) && (c2 == 0xdf)) {	/* semi-sonant mark */
	  putChar((int)code2sjis_map2_o[kana2code_map[c].b2 + 2]);
	}
	else {
	  putChar((int)code2sjis_map2_o[kana2code_map[c].b2]);
	  ungetChar(c2);
	  c = c1;
	  goto euc2sjis_loop;
	}
      }
      else {
	putChar((int)code2sjis_map1[kana2code_map[c].b1]);
	putChar((int)code2sjis_map2_o[kana2code_map[c].b2]);
	ungetChar(c1);
      }
    }
    else if ((char)c == SS3) {	/* JIS X 0212-1990	*/
      getChar();		/* unable to show in case of Shift-JIS. */
      getChar();		/* I discard the next first byte and second */
      putChar(0x81);		/* byte character, and show 'Å¨' instead. */
      putChar(0xac);
    }
    else if (IsEUC_JP(c)) {
      c1 = c;		/* the  first byte of EUC-JP */
      c2 = getChar();	/* the second byte of EUC-JP */
      if (zen2ascii && (euc2code_map[c1] == 0x03)) {
	putChar((int)zen2ascii_map[euc2code_map[c2]]);
      }
      else {
	if (c1 % 2 == 0) {
	  putChar((int)code2sjis_map1[euc2code_map[c1]]);
	  putChar((int)code2sjis_map2_e[euc2code_map[c2]]);
	}
	else {
	  putChar((int)code2sjis_map1[euc2code_map[c1]]);
	  putChar((int)code2sjis_map2_o[euc2code_map[c2]]);
	}
      }
    }
    else {	/* output with no converting */
      putChar(c);
    }
  }
}

/*
 * FUNCTION NAME:	sjis2euc
 * DESCRIPTION:		convert SJIS to EUC-JP
 * ARGUMENTS:		none
 * RETURN VALUE:	none
 */
static void
sjis2euc(void)
{
  int	c, c1, c2;

  while ((c = getChar()) != EOF) {
    if (IsCTRL(c) || IsASCII(c)) {
      if (c == LF) {
	putStr(lineend);
      }
      else {
	putChar(c);
      }
    }
    else if (IsKANA8(c)) {
      c1 = getChar();
      putChar((int)code2euc_map[kana2code_map[c].b1]);
      if (KANA_CHECK(c) && (c1 == 0xde)) {	/* sonant mark	*/
	putChar((int)code2euc_map[kana2code_map[c].b2 + 1]);
      }
      else if (KANA_CHECK(c) && (c1 == 0xdf)) {	/* semi-sonant mark */
	putChar((int)code2euc_map[kana2code_map[c].b2 + 2]);
      }
      else {
	putChar((int)code2euc_map[kana2code_map[c].b2]);
	ungetChar(c1);
      }
    }
    else if (IsSJIS1(c)) {
      c1 = c;		/* the  first byte of SJIS */
      c2 = getChar();	/* the second byte of SJIS */
      if ((c2 >= 0x40 && c2 <= 0x7e) ||
	  (c2 >= 0x80 && c2 <= 0x9e)) {
	if (zen2ascii && (sjis2code_map1_l[c1] == 0x03)) {
	  putChar((int)zen2ascii_map[sjis2code_map2[c2]]);
	}
	else {
	  putChar((int)code2euc_map[sjis2code_map1_l[c1]]);
	  putChar((int)code2euc_map[sjis2code_map2[c2]]);
	}
      }
      else if (c2 >= 0x9f && c2 <= 0xfc) {
	if (zen2ascii && (sjis2code_map1_r[c1] == 0x03)) {
	  putChar((int)zen2ascii_map[sjis2code_map2[c2]]);
	}
	else {
	  putChar((int)code2euc_map[sjis2code_map1_r[c1]]);
	  putChar((int)code2euc_map[sjis2code_map2[c2]]);
	}
      }
    }
    else {	/* output with no converting */
      putChar(c);
    }
  }
}

/*
 * FUNCTION NAME:	sjis2jis
 * DESCRIPTION:		convert SJIS to JIS
 * ARGUMENTS:		none
 * RETURN VALUE:	none
 */
static void
sjis2jis(void)
{
  int		c, c1, c2;
  bool		zen2ascii_mode;
  jstate_t	state = ST_UNKNOWN;

  zen2ascii_mode = zen2ascii;
  while ((c = getChar()) != EOF) {
    if (IsCTRL(c) || IsASCII(c)) {
      if (state == ST_JIS) {
	putStr(ESC_ISO_ASCII);
	state = ST_ASCII;
	zen2ascii_mode = zen2ascii;
      }
      if (c == LF) {
	putStr(lineend);
      }
      else {
	putChar(c);
      }
    }
    else if (IsKANA8(c)) {
      if (state != ST_JIS) {
	putStr(ESC_JIS_NEW);
	state = ST_JIS;
      }
      c1 = getChar();
      putChar((int)code2jis_map[kana2code_map[c].b1]);
      if (KANA_CHECK(c) && (c1 == 0xde)) {	/* sonant mark	*/
	putChar((int)code2jis_map[kana2code_map[c].b2 + 1]);
      }
      else if (KANA_CHECK(c) && (c1 == 0xdf)) {	/* semi-sonant mark */
	putChar((int)code2jis_map[kana2code_map[c].b2 + 2]);
      }
      else {
	putChar((int)code2jis_map[kana2code_map[c].b2]);
	ungetChar(c1);
      }
    }
    else if (IsSJIS1(c)) {
      c1 = c;		/* the  first byte of SJIS */
      c2 = getChar();	/* the second byte of SJIS */
      if ((c2 >= 0x40 && c2 <= 0x7e) ||
	  (c2 >= 0x80 && c2 <= 0x9e)) {
	if (zen2ascii && (sjis2code_map1_l[c1] == 0x03)) {
	  if (!zen2ascii_mode) {
	    putStr(ESC_ISO_ASCII);
	    state = ST_ASCII;
	    zen2ascii_mode = true;
	  }
	  putChar((int)zen2ascii_map[sjis2code_map2[c2]]);
	}
	else {
	  if (state != ST_JIS) {
	    putStr(ESC_JIS_NEW);
	    state = ST_JIS;
	    zen2ascii_mode = false;
	  }
	  putChar((int)code2jis_map[sjis2code_map1_l[c1]]);
	  putChar((int)code2jis_map[sjis2code_map2[c2]]);
	}
      }
      else if (c2 >= 0x9f && c2 <= 0xfc) {
	if (zen2ascii && (sjis2code_map1_r[c1] == 0x03)) {
	  if (!zen2ascii_mode) {
	    putStr(ESC_ISO_ASCII);
	    state = ST_ASCII;
	    zen2ascii_mode = true;
	  }
	  putChar((int)zen2ascii_map[sjis2code_map2[c2]]);
	}
	else {
	  if (state != ST_JIS) {
	    putStr(ESC_JIS_NEW);
	    state = ST_JIS;
	    zen2ascii_mode = false;
	  }
	  putChar((int)code2jis_map[sjis2code_map1_r[c1]]);
	  putChar((int)code2jis_map[sjis2code_map2[c2]]);
	}
      }
    }
    else {	/* output with no converting */
      putChar(c);
    }
  }
#if	1
  if (output.fp == NULL && state == ST_JIS) {
    putStr(ESC_ISO_ASCII);
  }
#endif
}

/*
 * FUNCTION NAME:	jis2jis
 * DESCRIPTION:		convert JIS X 0201 (kana) to JIS X 0208
 * ARGUMENTS:		none
 * RETURN VALUE:	none
 */
static void
jis2jis(void)
{
  int		c, c1, c2;
  jis_state_t	state, pre_state;
  bool		zen2ascii_mode;

  zen2ascii_mode    = zen2ascii;
  state = pre_state = JS_ASCII;
  while ((c = getChar()) != EOF) {
    if (c == ESC) {	/* Escape sequence */
      state = getStateOfJIS();
      continue;
    }
    else if (c == SO) {	/* Shift Out */
      state = JS_JIS7;
      continue;
    }
    else if (c == SI) {	/* Shift In */
      state = JS_ASCII;
      continue;
    }
    if (state != pre_state) {	/* state was changed! */
      switch (state) {
	case JS_ASCII:
	  if (!zen2ascii_mode) {
	    putStr(ESC_ISO_ASCII);
	    zen2ascii_mode = zen2ascii;
	  }
	  break;
	case JS_JIS_X0212_1990:
	  putStr(ESC_JIS_X0212);
	  break;
	case JS_JIS_2BYTE:
	case JS_JIS7:
	case JS_JIS8:
	  if (!zen2ascii_mode) {
	    putStr(ESC_JIS_NEW);
	  }
	  break;
	default:
	  break;
      }
    }
    pre_state = state;

    switch (state) {
      case JS_ASCII:
	if (c == LF) {
	  putStr(lineend);
	}
	else {
	  putChar(c);
	}
	break;
      case JS_JIS_X0212_1990:
	c1 = c;
	c2 = getChar();
	putChar(c1);
	putChar(c2);
	break;
      case JS_JIS_2BYTE:
	c1 = c;
	c2 = getChar();
	if (zen2ascii && (jis2code_map[c1] == 0x03)) {
	  if (!zen2ascii_mode) {
	    putStr(ESC_ISO_ASCII);
	    zen2ascii_mode = true;
	  }
	  putChar((int)zen2ascii_map[jis2code_map[c2]]);
	}
	else {
	  if (zen2ascii_mode) {
	    putStr(ESC_JIS_NEW);
	    zen2ascii_mode = false;
	  }
	  putChar(c1);
	  putChar(c2);
	}
	break;
      case JS_JIS7:
      case JS_JIS8:
	c1 = getChar();
	putChar((int)code2jis_map[kana2code_map[c].b1]);
	if (KANA_CHECK(c) &&
	   (c1 == 0xde || c1 == 0x5e)) {	/* sonant mark	*/
	  putChar((int)code2jis_map[kana2code_map[c].b2 + 1]);
	}
	else if (KANA_CHECK(c) &&
		(c1 == 0xdf || c1 == 0x5f)) {	/* semi-sonant mark */
	  putChar((int)code2jis_map[kana2code_map[c].b2 + 2]);
	}
	else {
	  putChar((int)code2jis_map[kana2code_map[c].b2]);
	  ungetChar(c1);
	}
	break;
      default:
	break;
    }
  }
  if (state != pre_state || ( output.fp == NULL && state != JS_ASCII)) {
    putStr(ESC_ISO_ASCII);
  }
}

/*
 * FUNCTION NAME:	euc2euc
 * DESCRIPTION:		convert JIS X 0201 (kana) to JIS X 0208 in EUC-JP
 * ARGUMENTS:		none
 * RETURN VALUE:	none
 */
static void
euc2euc(void)
{
  int	c, c1, c2;

  while ((c = getChar()) != EOF) {
euc2euc_loop:
    if (IsCTRL(c) || IsASCII(c)) {
      if (c == LF) {
	putStr(lineend);
      }
      else {
	putChar(c);
      }
    }
    else if ((char)c == SS2) {
      c  = getChar();
      c1 = getChar();
      if ((char)c1 == SS2) {
	c2 = getChar();
	putChar((int)code2euc_map[kana2code_map[c].b1]);
	if (KANA_CHECK(c) && (c2 == 0xde)) {	/* sonant mark	*/
	  putChar((int)code2euc_map[kana2code_map[c].b2 + 1]);
	}
	else if (KANA_CHECK(c) && (c2 == 0xdf)) { /* semi-sonant mark */
	  putChar((int)code2euc_map[kana2code_map[c].b2 + 2]);
	}
	else {
	  putChar((int)code2euc_map[kana2code_map[c].b2]);
	  ungetChar(c2);
	  c = c1;
	  goto euc2euc_loop;
	}
      }
      else {
	putChar((int)code2euc_map[kana2code_map[c].b1]);
	putChar((int)code2euc_map[kana2code_map[c].b2]);
	ungetChar(c1);
      }
    }
    else if ((char)c == SS3) {	/* JIS X 0212-1990 */
      putChar(c);
    }
    else if (IsEUC_JP(c)) {
      c1 = c;
      c2 = getChar();
      if (zen2ascii && (euc2code_map[c1] == 0x03)) {
	putChar((int)zen2ascii_map[euc2code_map[c2]]);
      }
#if	0	/* ägí£ópãLçÜïœä∑ */
      else if (zen2ascii && (euc2code_map[c1] == 0x01)) {
        if (zen2ascii_map2[euc2code_map[c2]] != 0x00) {
	  putChar((int)zen2ascii_map2[euc2code_map[c2]]);
	}
	else {
	  putChar(c1);
          putChar(c2);
	}
      }
#endif
      else {
        putChar(c1);
        putChar(c2);
      }
    }
  }
}

/*
 * FUNCTION NAME:	sjis2sjis
 * DESCRIPTION:		convert JIS X 0201 (kana) to JIS X 0208 in SJIS
 * ARGUMENTS:		none
 * RETURN VALUE:	none
 */
static void
sjis2sjis(void)
{
  int	c, c1, c2;

  while ((c = getChar()) != EOF) {
    if (IsCTRL(c) || IsASCII(c)) {
      if (c == LF) {
	putStr(lineend);
      }
      else {
	putChar(c);
      }
    }
    else if (IsKANA8(c)) {
      c1 = getChar();
      putChar((int)code2sjis_map1[kana2code_map[c].b1]);
      if (KANA_CHECK(c) && (c1 == 0xde)) {	/* sonant mark	*/
	putChar((int)code2sjis_map2_o[kana2code_map[c].b2 + 1]);
      }
      else if (KANA_CHECK(c) && (c1 == 0xdf)) {	/* semi-sonant mark */
	putChar((int)code2sjis_map2_o[kana2code_map[c].b2 + 2]);
      }
      else {
	putChar((int)code2sjis_map2_o[kana2code_map[c].b2]);
	ungetChar(c1);
      }
    }
    else if (IsSJIS1(c)) {
      c1 = c;		/* the  first byte of SJIS */
      c2 = getChar();	/* the second byte of SJIS */
      if ((c2 >= 0x40 && c2 <= 0x7e) ||
	  (c2 >= 0x80 && c2 <= 0x9e)) {
	if (zen2ascii && (sjis2code_map1_l[c1] == 0x03)) {
	  putChar((int)zen2ascii_map[sjis2code_map2[c2]]);
	}
	else {
          putChar(c1);
          putChar(c2);
	}
      }
      else if (c2 >= 0x9f && c2 <= 0xfc) {
	if (zen2ascii && (sjis2code_map1_r[c1] == 0x03)) {
	  putChar((int)zen2ascii_map[sjis2code_map2[c2]]);
	}
	else {
          putChar(c1);
          putChar(c2);
	}
      }
    }
  }
}

/*
 * FUNCTION NAME:	noConv
 * DESCRIPTION:		output with no converting
 * ARGUMENTS:		none
 * RETURN VALUE:	none
 */
static void
noConv(void)
{
  int   c;

  while ((c = getChar()) != EOF) {
    putChar(c);
  }
}

/*****************************************************************************/

/*
 * FUNCTION NAME:	getStateOfJIS
 * DESCRIPTION:		return the current state of JIS code
 * ARGUMENTS:		none
 * RETURN VALUE:	the current state
 */
static jis_state_t
getStateOfJIS(void)
{
  int		c;
  jis_state_t	state = JS_ESC;

  while ((c = getChar()) != EOF) {
    switch (state) {
      case JS_ESC:
	if	(c == '$')	{ state = JS_JIS_2BYTE; }
	else if (c == '(')	{ state = JS_JIS_1BYTE; }
	else if (c == '&')	{ state = JS_JIS_X0208_1990_1; }
	break;
      case JS_JIS_1BYTE:
	if	((c == 'J') ||
		 (c == 'B'))	{ return JS_ASCII; }
	else if (c == 'I')	{ return JS_JIS8; }
	break;
      case JS_JIS_2BYTE:
	if	((c == '@') ||
		 (c == 'B'))	{ return JS_JIS_2BYTE; }
	else if (c == '(')	{ state = JS_JIS_X0212_1990; }
	break;
      case JS_JIS_X0208_1990_1:
	if	(c == '@')	{ state = JS_JIS_X0208_1990_2; }
	break;
      case JS_JIS_X0208_1990_2:
	if	(c == ESC)	{ state = JS_ESC; }
	break;
      case JS_JIS_X0212_1990:
	if	(c == 'D')	{ return JS_JIS_X0212_1990; }
	break;
      default:
	return JS_JIS_2BYTE;
    }
  }
  return JS_JIS_2BYTE;
}

