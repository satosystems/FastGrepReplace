/*****************************************************************************
 *
 * FILE:	api.c
 * DESCRIPTION:	WKF: API to convert kanji code
 * DATE:	Sun, Jan 16 2000
 * UPDATE:	Sun, Apr 10 2005
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
 * $Id: api.c,v 1.8 2005/04/10 18:53:07 kouichi Exp $
 *
 *****************************************************************************/

#if	HAVE_CONFIG_H
#include "config.h"
#endif	/* HAVE_CONFIG_H */

#include <stdio.h>
#include <string.h>
#include "kanji.h"
#include "functions.h"

/*****************************************************************************
 *
 *	Local functions declaration
 *
 *****************************************************************************/
static conv_t	convertKanjiCodeOfFile(kcode_t, FILE *, kcode_t, FILE *);
static kcode_t	ktype2kcode(ktype_t);
static void	printGuessedKanjiCode(ktype_t, bool);

/*****************************************************************************
 *
 *	Functions definition
 *
 *****************************************************************************/

/*
 * FUNCTION NAME:	wkfPrintGuessKanjiCodeOfFile
 * DESCRIPTION:		print the guessed kanji code of file
 * ARGUMENTS:		input file pointer, show detail flag
 * RETURN VALUE:	none
 */
void
wkfPrintGuessedKanjiCodeOfFile(fin, detail)
	register FILE *	fin;
	bool		detail;
{
  readFile(fin);
  printGuessedKanjiCode(guessKanjiCodeType(true, KC_UNKNOWN, NULL), detail);
}

/*****************************************************************************/

/*
 * FUNCTION NAME:	wkfGuessKanjiCodeOfFile
 * DESCRIPTION:		guess kanji code of file
 * ARGUMENTS:		input file pointer
 * RETURN VALUE:	guessed kanji code
 */
kcode_t
wkfGuessKanjiCodeOfFile(fin)
	FILE *	fin;
{
  readFile(fin);
  return ktype2kcode(guessKanjiCodeType(true, KC_UNKNOWN, NULL));
}

/*
 * FUNCTION NAME:	wkfGuessKanjiCodeOfString
 * DESCRIPTION:		guess kanji code of string
 * ARGUMENTS:		input string pointer
 * RETURN VALUE:	guessed kanji code
 */
kcode_t
wkfGuessKanjiCodeOfString(str)
	String	str;
{
  readString(str);
  return ktype2kcode(guessKanjiCodeType(false, KC_UNKNOWN, NULL));
}

/*****************************************************************************/

/*
 * FUNCTION NAME:	wkfConvertKanjiCodeOfString
 * DESCRIPTION:		interface to convert kanji code of string
 * ARGUMENTS:		input kanji code, input string,
 *			output kanji code, output buffer,
 *			output buffer size
 * RETURN VALUE:	converted:    CONV_OK,
 *			no-converted: CONV_NO,
 *			otherwise:    CONV_ERR
 */
conv_t
wkfConvertKanjiCodeOfString(kin, sin, kout, sout, size)
	kcode_t	kin;
	String	sin;
	kcode_t	kout;
	String	sout;
	size_t	size;
{
  if ((strlen(sin) <= 2) && (sin[0] == CR || sin[0] == LF)) {
    strlcpy(sout, sin, size);
    return CONV_NO;
  }

  readString(sin);
  /* guess the input kanji code automatically */
  if (kin == KC_UNKNOWN) {
    kin = ktype2kcode(guessKanjiCodeType(false, kout, NULL));
  }

  setOutputStringBuffer(sout, size);

  return convertKanjiCode(kin, kout);
}

/*****************************************************************************/

/*
 * FUNCTION NAME:	wkfFileOpen
 * DESCRIPTION:		open a file while converting kanji code
 * ARGUMENTS:		filename, kanji code
 * RETURN VALUE:	Success: file pointer, otherwise: NULL
 */
FILE *
wkfFileOpen(fname, kcode)
	String	fname;
	kcode_t	kcode;
{
  register FILE *	fin  = NULL;
  register FILE *	fout = NULL;

  /* open file */
  fin = fopen(fname, "r");
  if (fin == NULL) {
    return NULL;
  }

  /* open temporary file */
  fout = tmpfile();
  if (fout == NULL) {
    fclose(fin);
    return NULL;
  }

  /* convert file */
  switch (convertKanjiCodeOfFile(KC_UNKNOWN, fin, kcode, fout)) {
    case CONV_OK:
      fclose(fin);
      if (fseek(fout, 0L, SEEK_SET) == -1) {
	fclose(fout);
	return NULL;
      }
      return fout;
    case CONV_NO:
      fclose(fout);
      return fin;
    case CONV_ERR:
    default:
      fclose(fin);
      fclose(fout);
      return NULL;
  }

  return NULL;
}

/*
 * FUNCTION NAME:	wkfFileClose
 * DESCRIPTION:		close a stream which is opened by wkfFileOpen
 * ARGUMENTS:		file pointer
 * RETURN VALUE:	Success: 0, otherwise: EOF
 */
int
wkfFileClose(fp)
	FILE *	fp;
{
  return fclose(fp);
}

/*****************************************************************************/

/*
 * FUNCTION NAME:	wkfConvertKanjiCodeOfFile
 * DESCRIPTION:		interface to convert kanji code
 * ARGUMENTS:		input kanji code, input file pointer,
 *			output kanji code, output file pointer
 * RETURN VALUE:	converted:    CONV_OK,
 *			no-converted: CONV_NO,
 *			otherwise:    CONV_ERR
 */
conv_t
wkfConvertKanjiCodeOfFile(kin, fin, kout, fout)
	kcode_t		kin;
	register FILE *	fin;
	kcode_t		kout;
	register FILE *	fout;
{
  return convertKanjiCodeOfFile(kin, fin, kout, fout);
}

/*****************************************************************************/

/*
 * FUNCTION NAME:	convertKanjiCodeOfFile
 * DESCRIPTION:		interface to convert kanji code
 * ARGUMENTS:		input kanji code, input file pointer,
 *			output kanji code, output file pointer
 * RETURN VALUE:	converted:    CONV_OK,
 *			no-converted: CONV_NO,
 *			otherwise:    CONV_ERR
 */
static conv_t
convertKanjiCodeOfFile(kin, fin, kout, fout)
	kcode_t		kin;
	register FILE *	fin;
	kcode_t		kout;
	register FILE *	fout;
{
  /* guess the input kanji code automatically */
  if (kin == KC_UNKNOWN) {
    readFile(fin);
    kin = ktype2kcode(guessKanjiCodeType(true, kout, fout));
  }
  else {
    noPreRead(fin);
  }

  setOutputFilePointer(fout);

  return convertKanjiCode(kin, kout);
}

/*****************************************************************************/

/*
 * FUNCTION NAME:	ktype2kcode
 * DESCRIPTION:		convert ktype_t to kcode_t
 * ARGUMENTS:		guessed kanji code (ktype_t)
 * RETURN VALUE:	guessed kanji code (kcode_t)
 */
static kcode_t
ktype2kcode(kcode)
	ktype_t	kcode;
{
  switch (kcode) {
    case KT_EUC_JP_or_SJIS:
      return KC_EUCorSJIS;
    case KT_ASCII:
      return KC_ASCII;
    case KT_JIS_X0201_ROMA:
    case KT_JIS_X0201_KANA:
    case KT_JIS_X0212:
    case KT_JIS_OLD:
    case KT_JIS_OLD_X0201:
    case KT_JIS_OLD_X0212:
    case KT_JIS_NEW:
    case KT_JIS_NEW_X0201:
    case KT_JIS_NEW_X0212:
    case KT_JIS_1990:
    case KT_JIS_1990_X0212:
      return KC_JIS;
    case KT_EUC_JP:
    case KT_EUC_JP_X0212:
      return KC_EUC;
    case KT_SJIS:
      return KC_SJIS;
    case KT_JIS_OLD_BROKEN:
    case KT_JIS_NEW_BROKEN:
    case KT_JIS_1990_BROKEN:
    case KT_EUC_JP_BROKEN:
    case KT_SJIS_BROKEN:
      return KC_BROKEN;
    case KT_DATA:
      return KC_DATA;
    case KT_UNKNOWN:
    default:
      return KC_UNKNOWN;
  }
}

/*
 * FUNCTION NAME:	printGuessedKanjiCode
 * DESCRIPTION:		print the guessed kanji code
 * ARGUMENTS:		guessed kanji code, show detail flag
 * RETURN VALUE:	none
 */
static void
printGuessedKanjiCode(kcode, detail)
	ktype_t	kcode;
	bool	detail;
{
  switch (kcode) {
    case KT_EUC_JP_or_SJIS:
      fputs(detail ? "Japanese EUC or Shift-JIS\n" : "euc-jp/sjis\n", stdout);
      break;
    case KT_ASCII:
      fputs(detail ? "ASCII\n" : "ascii\n", stdout);
      break;
    case KT_JIS_X0201_ROMA:
      fputs(detail ? "JIS X 0201-1976 (roma)\n" : "jisx0201(roma)\n", stdout);
      break;
    case KT_JIS_X0201_KANA:
      fputs(detail ? "JIS X 0201-1976 (kana)\n" : "jisx0201(kana)\n", stdout);
      break;
    case KT_JIS_X0212:
      fputs(detail ? "JIS X 0212-1990\n" : "jisx0212\n", stdout);
      break;
    case KT_JIS_OLD:
      fputs(detail ? "JIS C 6226-1978\n" : "oldjis\n", stdout);
      break;
    case KT_JIS_OLD_X0201:
      fputs(detail ? "JIS C 6226-1978 with JIS X 0201 (kana)\n" :
		     "oldjis+jisx0201\n", stdout);
      break;
    case KT_JIS_OLD_X0212:
      fputs(detail ? "JIS C 6226-1978 with JIS X 0212-1990\n" :
		     "oldjis+jisx0212\n", stdout);
      break;
    case KT_JIS_OLD_BROKEN:
      fputs(detail ? "JIS C 6226-1978 with some BROKEN codes\n" :
		     "oldjis(broken)\n", stdout);
      break;
    case KT_JIS_NEW:
      fputs(detail ? "JIS X 0208-1983\n": "newjis\n", stdout);
      break;
    case KT_JIS_NEW_X0201:
      fputs(detail ? "JIS X 0208-1983 with JIS X 0201 (kana)\n" :
		     "newjis+jisx0201\n", stdout);
      break;
    case KT_JIS_NEW_X0212:
      fputs(detail ? "JIS X 0208-1983 with JIS X 0212-1990\n" :
		     "newjis+jisx0212\n", stdout);
      break;
    case KT_JIS_NEW_BROKEN:
      fputs(detail ? "JIS X 0208-1983 with some BROKEN codes\n" :
		     "newjis(broken)\n", stdout);
      break;
    case KT_JIS_1990:
      fputs(detail ? "JIS X 0208-1990\n" : "jis1990\n", stdout);
      break;
    case KT_JIS_1990_X0212:
      fputs(detail ? "JIS X 0208-1990 with JIS X 0212-1990\n" :
		     "jis1990+jisx0212\n", stdout);
      break;
    case KT_JIS_1990_BROKEN:
      fputs(detail ? "JIS X 0208-1990 with some BROKEN codes\n" :
		     "jis1990(broken)\n", stdout);
      break;
    case KT_EUC_JP:
      fputs(detail ? "Japanese EUC\n" : "euc-jp\n", stdout);
      break;
    case KT_EUC_JP_X0212:
      fputs(detail ? "Japanese EUC with JIS X 0212-1990\n" :
		     "euc-jp+jisx0212\n", stdout);
      break;
    case KT_EUC_JP_BROKEN:
      fputs(detail ? "Japanese EUC with some BROKEN codes\n" :
		     "euc-jp(broken)\n", stdout);
      break;
    case KT_SJIS:
      fputs(detail ? "Shift-JIS\n" : "sjis\n", stdout);
      break;
    case KT_SJIS_BROKEN:
      fputs(detail ? "Shift-JIS with some BROKEN codes\n" :
		     "sjis(broken)\n", stdout);
      break;
    case KT_DATA:
      fputs(detail ? "Binary Data\n" : "data\n", stdout);
      break;
    default:
      fputs(detail ? "Unknown\n" : "unknown\n", stdout);
      break;
  }
}

