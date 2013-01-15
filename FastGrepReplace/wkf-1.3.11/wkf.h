/*****************************************************************************
 *
 * FILE:	wkf.h
 * DESCRIPTION:	WKF: header file
 * DATE:	Sat, Jan 15 2000
 * UPDATE:	Fri, Jul  6 2007
 * AUTHOR:	Kouichi ABE (WALL) / 阿部康一
 * E-MAIL:	kouichi@MysticWALL.COM
 * URL:		http://www.MysticWALL.COM/
 * COPYRIGHT:	(c) 2000-2007 阿部康一／Kouichi ABE (WALL), All rights reserved.
 * LICENSE:
 *
 *  Copyright (c) 2000-2007 Kouichi ABE (WALL) <kouichi@MysticWALL.COM>,
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
 * $Id: wkf.h,v 1.15 2007/07/05 16:32:14 kouichi Exp $
 *
 *****************************************************************************/
#ifndef	_WKF_H
#define	_WKF_H	1

/* Numeric release version identifier: MMNNFFRBB: major minor fix final beta */
#define	WKF_RELEASE	103110001

/*
 * Numeric release version identifier:
 * MNNFFPPS: major minor fix patch status
 * The status nibble has one of the values 0 for development,
 * 1 to e for betas 1 to 14, and f for release.
 * The patch level is exactly that.
 */
#define WKF_VERSION_NUMBER      0x10311010L
#define WKF_VERSION_TEXT        "WKF 1.3.11 (2007/07/06)"
#define WKF_VERSION_TEXT_SHORT  "WKF/1.3.11"
#define WKF_VERSION_TEXT_LONG   "WKF Library 1.3.11, Fri, Jul  6 2007"

typedef enum {		/* kanji code type for API */
  KC_UNKNOWN = -1,	/* Unknown */
  KC_EUCorSJIS,		/* EUC-JP or Shift-JIS */
  KC_ASCII,		/* ASCII */
  KC_JIS,		/* JIS */
  KC_EUC,		/* EUC-JP */
  KC_SJIS,		/* Shift-JIS */
  KC_BROKEN,		/* broken file */
  KC_DATA		/* binary data */
/* >>>> ogata: コードを追加（文字コード＋エラーコード） */
, KC_UTF16BE,				// BOM あり
  KC_UTF16LE,				// BOM あり
  KC_UTF16BE_NOBOM,			// BOM なし
  KC_UTF16LE_NOBOM,			// BOM なし
  KC_UTF8,					// BOM あり
  KC_UTF8N,					// BOM なし（このプログラム内部でのみ利用）
  KC_UTF7,					// UTF-7
  KC_ERROR = -100,			// エラー
/* <<<< */
} kcode_t;

typedef enum {		/* convert result type */
  CONV_ERR = -1,	/* any error was happened */
  CONV_NO,		/* no convert */
  CONV_OK		/* successful */
} conv_t;

typedef enum {	/* line end type */
  LE_LF,	/* usually used in UNIX */
  LE_CR,	/* usually used in Macintosh */
  LE_CRLF	/* usually used in MS-DOS/Windows */
} lineend_t;

#ifndef _STRING_T
#define _STRING_T	1
typedef unsigned char *	String;
#endif	/* _STRING_T */

#ifndef	_BOOL_T
#define	_BOOL_T	1
typedef enum {	/* boolean type */
  false = 0,
  true	= 1
} bool;
#endif	/* _BOOL_T */

/* >>>> ogata: C++ から呼び出すため */
#ifdef __cplusplus
extern "C" {
#endif
/* <<<< */

/* api.c */
extern void	wkfPrintGuessedKanjiCodeOfFile(FILE *, bool);

extern kcode_t	wkfGuessKanjiCodeOfFile(FILE *);
extern kcode_t	wkfGuessKanjiCodeOfString(String);

extern conv_t	wkfConvertKanjiCodeOfFile(kcode_t, FILE *, kcode_t, FILE *);
extern conv_t	wkfConvertKanjiCodeOfString(kcode_t, String, kcode_t, String, size_t);

extern FILE *	wkfFileOpen(String, kcode_t);
extern int	wkfFileClose(FILE *);

/* api2.c */
extern int	wkfEncodeBase64String(const String, String, size_t);
extern int	wkfDecodeBase64String(const String, String, size_t);
extern int	wkfEncodeBase64Bin(const String, size_t, String, size_t);
#define	wkfDecodeBase64Bin(a,b,c)	wkfDecodeBase64String((a),(b),(c))
extern void	wkfDecodeBase64File(FILE *, FILE *);

extern int	wkfEncodeQuotedPrintableString(const String, String, size_t);
extern int	wkfEncodeQuotedPrintableBin(const String, size_t, String, size_t);
extern int	wkfDecodeQuotedPrintableString(const String, String, size_t);
extern void	wkfDecodeQuotedPrintableFile(FILE *, FILE *);

extern conv_t	wkfEncodeMimeString(const String, String, size_t, kcode_t);
extern conv_t	wkfDecodeMimeString(const String, String, size_t, kcode_t);
extern conv_t	wkfStringToHexString(const String, String, size_t);
extern conv_t	wkfMimeStringToHexString(const String, String, size_t);
extern conv_t	wkfFoldString(const String, size_t, String, size_t, kcode_t);
extern conv_t	wkfDecodeEscapedURIString(const String, String, size_t, kcode_t);

/* conv.c */
extern void	wkfSetLineEndCode(lineend_t);
extern void	wkfConvertZenkaku2ASCII(bool);

/* >>>> ogata: C++ から呼び出すため */
#ifdef __cplusplus
}
#endif
/* <<<< */

/* >>>> ogata: 警告除去 */
size_t strlcpy (char *dst, const char *src, size_t size);
/* <<<< ogata: 警告除去 */



#endif	/* _WKF_H */
