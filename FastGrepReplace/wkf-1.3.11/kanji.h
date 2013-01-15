/*****************************************************************************
 *
 * FILE:	kanji.h
 * DESCRIPTION:	WKF: kanji code header
 * DATE:	Sat, Jan 15 2000
 * UPDATE:	Fri, Mar 14 2003
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
 * $Id: kanji.h,v 1.4 2005/04/05 08:26:56 kouichi Exp $
 *
 *****************************************************************************/

#ifndef	_kanji_h
#define	_kanji_h	1

#include "wkf.h"

/*****************************************************************************
 *
 *	definition of macros and structures
 *
 *****************************************************************************/
typedef unsigned int	uint;
typedef unsigned char	uchar;

typedef enum {		/* kanji code type	*/
  KT_UNKNOWN = -1,	/* unknown code ...	*/
  KT_EUC_JP_or_SJIS,	/* EUC-JP or SJIS code	*/
  KT_ASCII,		/* ASCII code		*/
  KT_JIS_X0201_ROMA,	/* JIS code with only JIS X 0201-1976 (roma)	*/
  KT_JIS_X0201_KANA,	/* JIS code with only JIS X 0201-1976 (kana)	*/
  KT_JIS_X0212,		/* JIS code with only JIS X 0212-1990		*/
  KT_JIS_OLD,		/* JIS code (old version)			*/
  KT_JIS_OLD_X0201,	/* JIS code (old version) with JIS X 0201 (kana)*/
  KT_JIS_OLD_X0212,	/* JIS code (old version) with JIS X 0212-1990	*/
  KT_JIS_OLD_BROKEN,	/* JIS code (old version) with broken codes	*/
  KT_JIS_NEW,		/* JIS code (new version)			*/
  KT_JIS_NEW_X0201,	/* JIS code (new version) with JIS X 0201 (kana)*/
  KT_JIS_NEW_X0212,	/* JIS code (new version) with JIS X 0212-1990	*/
  KT_JIS_NEW_BROKEN,	/* JIS code (new version) with broken codes	*/
  KT_JIS_1990,		/* JIS code (1990 version)			*/
  KT_JIS_1990_X0212,	/* JIS code (1990 version) with JIS X 0212-1990	*/
  KT_JIS_1990_BROKEN,	/* JIS code (1990 version) with broken codes	*/
  KT_EUC_JP,		/* Japanese EUC code				*/
  KT_EUC_JP_X0212,	/* Japanese EUC code with JIS X 0212-1990	*/
  KT_EUC_JP_BROKEN,	/* Japanese EUC code with broken codes		*/
  KT_SJIS,		/* Shift-JIS code				*/
  KT_SJIS_BROKEN,	/* Shift-JIS code with broken codes		*/
  KT_DATA		/* input file will be binary code or data	*/
} ktype_t;

typedef enum {
  JS_START = 1,
  JS_ESC,
  JS_DOLLAR,
  JS_BRACKET_L,
  JS_JIS_1BYTE,
  JS_JIS_2BYTE,
  JS_ASCII,
  JS_JIS7,
  JS_JIS8,
  JS_JIS_X0212_1990,
  JS_JIS_X0208_1990_1,
  JS_JIS_X0208_1990_2
} jis_state_t;

typedef enum {
  ES_CS0 = 1,
  ES_CS1,
  ES_CS2,
  ES_CS3,
  ES_CS3_2
} euc_state_t;

typedef enum {
  SS_1BYTE = 1,
  SS_2BYTE
} sjis_state_t;

#define	EOL	'\0'	/* End of Line */

#define	CR	'\015'	/* Carriage Return (0D) */
#define	LF	'\012'	/* Line Feed (0A)	*/

#define	SPC	'\040'	/* white space (1F)	*/
#define	TAB	'\011'	/* white space (09)	*/
#define	BS	'\010'	/* back space (08)	*/
#define	DEL	'\177'	/* delete (7F)		*/

#define	ESC	'\033'	/* JIS: Escape Sequence (1B)	*/
#define	SO	'\016'	/* JIS: Shift-Out (0E)		*/
#define	SI	'\017'	/* JIS: Shift-In  (0F)		*/

#define	SS2	'\216'	/* EUC: Single Shift 2 (8E)	*/
#define	SS3	'\217'	/* EUC: Single Shift 3 (8F)	*/

#define	ESC_JIS_OLD	"\033$@"	/* JIS C 6226-1978 (old ver.)	*/
#define	ESC_JIS_NEW	"\033$B"	/* JIS X 0208-1983 (new ver.)	*/
#define	ESC_JIS_1990	"\033&@\033$B"	/* JIS X 0208-1990		*/
#define	ESC_JIS_X0212	"\033$(D"	/* JIS X 0212-1990 (support)	*/
#define	ESC_JIS_ROMA	"\033(J"	/* JIS X 0201-1976 (roma)	*/
#define	ESC_ISO_ASCII	"\033(B"	/* ISO 646-1991 ASCII		*/
#define	ESC_JIS_KANA	"\033(I"	/* JIS X 0201-1976 (kana)	*/

/* useful macros */
#define	IsCTRL(c)	((((c) >= 0x00 && (c) <= 0x1f) || (c) == 0x7f) ? 1 : 0)
#define	IsASCII(c)	(((c) >= 0x20 && (c) <= 0x7e) ? 1 : 0)
#define	IsJIS(c)	(((c) >= 0x21 && (c) <= 0x7e) ? 1 : 0)
#define	IsSJIS1(c)	(((((uchar)c) >= 0x81 && ((uchar)c) <= 0x9f) || \
			  (((uchar)c) >= 0xe0 && ((uchar)c) <= 0xef)) ? 1 : 0)
#define	IsSJIS2(c)	(((((uchar)c) >= 0x40 && ((uchar)c) <= 0x7e) || \
			  (((uchar)c) >= 0x80 && ((uchar)c) <= 0xfc)) ? 1 : 0)
#define	IsEUC_JP(c)	((((uchar)c) >= 0xa1 && ((uchar)c) <= 0xfe) ? 1 : 0)
#define	IsKANA7(c)	(((c) >= 0x21 && (c) <= 0x5f) ? 1 : 0)
#define	IsKANA8(c)	((((uchar)c) >= 0xa1 && ((uchar)c) <= 0xdf) ? 1 : 0)

#endif	/* _kanji_h */
