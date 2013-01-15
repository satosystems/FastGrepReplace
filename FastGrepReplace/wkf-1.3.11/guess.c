/*****************************************************************************
 *
 * FILE:	guess.c
 * DESCRIPTION:	WKF: guess kanji code in Japanese text
 * DATE:	Sat, Jan 15 2000
 * UPDATE:	Tue, Dec  7 2004
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
 * $Id: guess.c,v 1.7 2005/04/05 08:26:56 kouichi Exp $
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
 *	Macros and structures definition
 *
 *****************************************************************************/
#define	START		(1)
#define	CHECK_BUFSIZE	(1024*2)

#define	PERCENT(x, y)	((x)*100/(y))
#define	BINARY_LEVEL	(12) /* I guess binary code (over broken/freq = 12%) */

typedef struct _Input {			/* pre-reading structure */
  String	buf;			/* pre-reading buffer */
  int		index;			/* index of buffer */
  int		size;			/* read size */
  FILE *	fp;			/* file pointer */
} Input;

typedef struct _Guess {	/* kanji code guess structure */
  ktype_t	guess;		/* guessed kanji code	*/
  int		freq;		/* frequency of code appearence */
  int		point;		/* evaluated point of code */
  int		ascii;		/* frequency of ASCII code */
  int		kana;		/* frequency of kana	*/
  int		ctrl;		/* frequency of control code */
  int		state;		/* transfer state of kanji code */
  bool		x0212;		/* include JIS X 0212?	*/
  bool		x0201;		/* include JIS X 0201 (kana) ?	*/
  int		broken;		/* include broken code	*/
} Guess;

/*****************************************************************************
 *
 *	Local variables declaration
 *
 *****************************************************************************/
static Input	input;	/* pre-reading buffer */

/*****************************************************************************
 *
 *	Functions definition
 *
 *****************************************************************************/

/*
 * FUNCTION NAME:	noPreRead
 * DESCRIPTION:		not pre-reading into buffer
 * ARGUMENTS:		file pointer
 * RETURN VALUE:	none
 */
void
noPreRead(fp)
	FILE *	fp;
{
  input.size = input.index = 0;
  input.fp   = fp;
}

/*
 * FUNCTION NAME:	getChar
 * DESCRIPTION:		get a character from pre-reading buffer
 * ARGUMENTS:		none
 * RETURN VALUE:	Success: a character, Otherwise: EOF
 */
int
getChar(void)
{
  static bool	crlf = false;
  int		c;

loop:
  if (input.index < input.size) {
    c = (uchar)input.buf[input.index++];
  }
  else {
    c = (input.fp == NULL ? EOF : fgetc(input.fp));
  }

  if (c == CR) {
    crlf = true;	/* set flag */
    return LF;
  }

  if (crlf) {
    crlf = false;	/* clear flag */
    if (c != LF) {
      return c;
    }
    goto loop;
  }

  return c;
}

/*
 * FUNCTION NAME:	ungetChar
 * DESCRIPTION:		put back a character into pre-reading buffer
 * ARGUMENTS:		a character (integer type)
 * RETURN VALUE:	none
 */
void
ungetChar(c)
	int	c;
{
  if (c != EOF) {
    if (input.index > 0 && input.index <= input.size) {
      input.index--;
      input.buf[input.index] = (char)c;
    }
    else {
      ungetc(c, input.fp);
    }
  }
}

/*****************************************************************************/

/*
 * FUNCTION NAME:	readFile
 * DESCRIPTION:		pre-reading text into buffer
 * ARGUMENTS:		file pointer
 * RETURN VALUE:	read size
 */
int
readFile(fp)
	register FILE *	fp;
{
  static char	buf[CHECK_BUFSIZE];	/* pre-reading buffer */

  memset(buf, 0, sizeof(buf));
  input.size  = fread(buf, 1, sizeof(buf), fp);
  input.buf   = buf;
  input.index = 0;
  input.fp    = fp;

  return input.size;
}

/*
 * FUNCTION NAME:	readString
 * DESCRIPTION:		put a string into pre-reading buffer
 * ARGUMENTS:		string
 * RETURN VALUE:	set size
 */
int
readString(str)
	register String	str;
{
  input.buf   = str;
  input.size  = strlen(str);
  input.index = 0;
  input.fp    = NULL;

  return input.size;
}

/*****************************************************************************/

/*
 * FUNCTION NAME:	guessKanjiCodeType
 * DESCRIPTION:		guess kanji code
 * ARGUMENTS:		file/string flag, output kanji code, output file pointer
 * RETURN VALUE:	guessed kanji code
 */
ktype_t
guessKanjiCodeType(file_f, kcode, out)
	bool		file_f;
	kcode_t		kcode;
	register FILE *	out;
{
  Guess	ascii, jis, sjis, euc;
  char	c;
  int	i;

retry:
  memset((String)&ascii, 0, sizeof(Guess));
  memset((String)&jis,   0, sizeof(Guess));
  memset((String)&sjis,  0, sizeof(Guess));
  memset((String)&euc,   0, sizeof(Guess));
  ascii.state  = jis.state  = sjis.state  = euc.state  = START;
  ascii.guess  = jis.guess  = sjis.guess  = euc.guess  = KT_UNKNOWN;
  ascii.x0212  = jis.x0212  = sjis.x0212  = euc.x0212  = false;
  ascii.x0201  = jis.x0201  = sjis.x0201  = euc.x0201  = false;

  /* guess */
  for (i = 0; i < input.size; i++) {
    c = input.buf[i];

    /* ASCII */
    switch (ascii.state) {
      case START:
	if	(IsASCII(c))	{ ascii.freq++;
				  ascii.point++;
				  ascii.ascii++;
				  ascii.guess = KT_ASCII; }
	else if (IsCTRL(c))	{ ascii.ctrl++; }
	else			{ ascii.state = false;	/* Not ASCII code! */
				  ascii.guess = KT_UNKNOWN; }
	break;
    }

    /* JIS */
    switch (jis.state) {
      case JS_START:		/* initial state */
	if	(c == ESC)	{ jis.state = JS_ESC; }
	else if (IsASCII(c))	{ jis.ascii++; }	/* ASCII code */
	else if (IsCTRL(c))	{ jis.ctrl++; }		/* Control code */
	else			{ jis.state = false;	/* Not JIS code! */
				  jis.guess = KT_UNKNOWN; }
	break;
      case JS_ESC:		/* ESC state */
	if	(c == '$')	{ jis.state = JS_DOLLAR;
				  jis.point++; }
	else if (c == '(')	{ jis.state = JS_BRACKET_L;
				  jis.point++; }
	else if (c == '&')	{ jis.state = JS_JIS_X0208_1990_1; }
	else			{ jis.state = false;	/* Not JIS code! */
				  jis.guess = KT_UNKNOWN; }
	break;
      case JS_DOLLAR:		/* '$' state */
	if	(c == '@')	{ jis.state = JS_JIS_1BYTE;
				  jis.point++;
				  jis.guess = jis.x0212 ? KT_JIS_OLD_X0212 :
							  KT_JIS_OLD;
				  jis.guess = jis.x0201 ? KT_JIS_OLD_X0201 :
							  jis.guess; }
	else if (c == 'B')	{ jis.state = JS_JIS_1BYTE;
				  jis.point++;
				  jis.guess = jis.x0212 ? KT_JIS_NEW_X0212 :
							  KT_JIS_NEW;
				  jis.guess = jis.x0201 ? KT_JIS_NEW_X0201 :
							  jis.guess; }
	else if (c == '(')	{ jis.state = JS_JIS_X0212_1990; }
	else			{ jis.state = false;	/* Not JIS code! */
				  jis.guess = KT_UNKNOWN; }
	break;
      case JS_BRACKET_L:	/* '(' state */
	if	(c == 'J')	{ jis.state = JS_ASCII;
					/* JIS X 0201-1976 (roma) */
				  jis.point++;
				  jis.guess = jis.guess == KT_UNKNOWN ?
							KT_JIS_X0201_ROMA :
							jis.guess; }
	else if	(c == 'B')	{ jis.state = JS_ASCII; } /* ISO 646-1991 */
	else if (c == 'I')	{ jis.state = JS_JIS7;
					/* JIS X 0201-1976 (kana) */
				  jis.point += 3;
				  jis.x0201 = true;
				  jis.guess = KT_JIS_X0201_KANA; }
	else			{ jis.state = false;	/* Not JIS code! */
				  jis.guess = KT_UNKNOWN; }
	break;
      case JS_JIS_1BYTE:
	if	(c == ESC)	{ jis.state = JS_ESC; }
	else if (IsJIS(c))	{ jis.state = JS_JIS_2BYTE; }
	else if (c == SO)	{ jis.state = JS_JIS7;	/* X 0201-1976 kana */
				  jis.point += 2; }
	else			{ jis.state = false;	/* Not JIS code! */
				  jis.guess = KT_UNKNOWN; }
	break;
      case JS_JIS_2BYTE:
	if	(IsJIS(c))	{ jis.state = JS_JIS_1BYTE;
				  jis.freq++;
				  jis.point += 2;}
	else			{ jis.state = false;	/* Not JIS code! */
				  jis.guess = KT_UNKNOWN; }
	break;
      case JS_ASCII:		/* ASCII state */
	if	(c == ESC)	{ jis.state = JS_ESC; }
	else if	(IsASCII(c))	{ jis.ascii++;
				  jis.point += 2; }
	else if (IsCTRL(c))	{ jis.ctrl++; }
	else			{ jis.state = false;	/* Not JIS code! */
				  jis.guess = KT_UNKNOWN; }
	break;
      case JS_JIS_X0212_1990:	/* JIS X 0212-1990 state */
	if	(c == 'D')	{ jis.state = JS_JIS_1BYTE;
				  jis.x0212 = true;
				  switch (jis.guess) {
				    case KT_JIS_OLD:
				      jis.guess = KT_JIS_OLD_X0212;
				      break;
				    case KT_JIS_NEW:
				      jis.guess = KT_JIS_NEW_X0212;
				      break;
				    case KT_JIS_1990:
				    default:
				      jis.guess = KT_JIS_X0212;
				      break;
				  }
				}
	else			{ jis.state = false;	/* Not JIS code! */
				  jis.guess = KT_UNKNOWN; }
	break;
      case JS_JIS7:		/* JIS7 state */
	if	(c == ESC)	{ jis.state = JS_ESC; }
	else if (c == SI)	{ jis.state = JS_JIS_1BYTE;
				  jis.point += 2; }
	else if (IsKANA7(c))	{ jis.kana++;
				  jis.point += 2; }
	else			{ jis.state = false;	/* Not JIS code! */
				  jis.guess = KT_UNKNOWN; }
	break;
      case JS_JIS_X0208_1990_1:	/* in JIS X 0208-1990? */
	if	(c == '@')	{ jis.state = JS_JIS_X0208_1990_2; }
	else			{ jis.state = false;	/* Not JIS code! */
				  jis.guess = KT_UNKNOWN; }
	break;
      case JS_JIS_X0208_1990_2:	/* JIS X 0208-1990 state */
	if	(c == ESC)	{ jis.state = JS_ESC;
				  jis.point += 2;
				  jis.guess = jis.x0212 ? KT_JIS_1990_X0212 :
							  KT_JIS_1990; }
	else			{ jis.state = false;	/* Not JIS code! */
				  jis.guess = KT_UNKNOWN; }
	break;
      default:
	jis.broken++;
	jis.state  = START;
	break;
    }

    /* EUC-JP */
    switch (euc.state) {
      case ES_CS0:		/* code set 0 */
	if	(IsASCII(c))	{ euc.ascii++; }
	else if (IsCTRL(c))	{ euc.ctrl++; }
	else if (IsEUC_JP(c))	{ euc.state = ES_CS1; }	/* EUC-JP code!? */
	else if (c == SS2)	{ euc.state = ES_CS2; }	/* code set 2 */
	else if (c == SS3)	{ euc.state = ES_CS3; }
	else			{ euc.state = false;	/* Not EUC-JP code! */
				  euc.guess = KT_UNKNOWN; }
	break;
      case ES_CS1:		/* code set 1 */
	if (IsEUC_JP(c))	{ euc.state = ES_CS0;
				  euc.freq++;
				  euc.point += 2;
				  euc.guess = KT_EUC_JP;
				  euc.guess = euc.x0212 ? KT_EUC_JP_X0212 :
							  KT_EUC_JP; }
	else			{ euc.state = false;	/* Not EUC-JP code! */
				  euc.guess = KT_UNKNOWN; }
	break;
      case ES_CS2:		/* code set 2 */
	if (IsKANA8(c))		{ euc.state = ES_CS0;
				  euc.kana++;
				  euc.freq++;
				  euc.point += 3;
				  euc.guess = KT_EUC_JP;
				}
	else			{ euc.state = false;	/* Not EUC-JP code! */
				  euc.guess = KT_UNKNOWN; }
	break;
      case ES_CS3:		/* code set 3!? */
	if (IsEUC_JP(c))	{ euc.state = ES_CS3_2; }
	else			{ euc.state = false;	/* Not EUC-JP code! */
				  euc.guess = KT_UNKNOWN; }
	break;
      case ES_CS3_2:		/* code set 3 (JIS X 0212-1990) */
	if (IsEUC_JP(c))	{ euc.state = ES_CS0;
				  euc.freq++;
				  euc.point += 3;
				  euc.x0212 = true;
				  euc.guess = KT_EUC_JP_X0212; }
	else			{ euc.state = false;	/* Not EUC-JP code! */
				  euc.guess = KT_UNKNOWN; }
	break;
      default:
	euc.broken++;
	euc.state  = START;
	break;
    }

    /* SJIS */
    switch (sjis.state) {
      case SS_1BYTE:		/* check first byte */
	if	(IsASCII(c))	{ sjis.ascii++; }
	else if (IsCTRL(c))	{ sjis.ctrl++; }
	else if (IsKANA8(c))	{ sjis.kana++;
				  sjis.point++;
				  sjis.guess = KT_SJIS;
				}
	else if (IsSJIS1(c))	{ sjis.state = SS_2BYTE; }
	else			{ sjis.state = false;	/* Not SJIS code! */
				  sjis.guess = KT_UNKNOWN; }
	break;
      case SS_2BYTE:		/* check second byte */
	if (IsSJIS2(c))		{ sjis.state = SS_1BYTE;
				  sjis.freq++;
				  sjis.point += 2;
				  sjis.guess = KT_SJIS;
				}
	else			{ sjis.state = false;	/* Not SJIS code! */
				  sjis.guess = KT_UNKNOWN; }
	break;
      default:
	sjis.broken++;
	sjis.state  = START;
	break;
    }
  }

  /* decision */
#if	DEBUG
  fprintf(stderr,
	"(%d)\tGUESS\tFREQ\tPOINT\tASCII\tKANA\tCTRL\tSTATE\tBROKEN(%%)\n",
	input.size);
  fprintf(stderr, "ASCII:\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n",
		ascii.guess,
		ascii.freq, ascii.point, ascii.ascii, ascii.kana, ascii.ctrl,
		ascii.state, ascii.broken,
		(int)(ascii.broken*100/(ascii.freq+1)));
  fprintf(stderr, "JIS:\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n",
		jis.guess,
		jis.freq, jis.point, jis.ascii, jis.kana, jis.ctrl, jis.state,
		jis.broken,
		(int)(jis.broken*100/(jis.freq+1)));
  fprintf(stderr, "SJIS:\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n",
		sjis.guess,
		sjis.freq, sjis.point, sjis.ascii, sjis.kana, sjis.ctrl,
		sjis.state, sjis.broken,
		(int)(sjis.broken*100/(sjis.freq+1)));
  fprintf(stderr, "EUC-JP:\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n",
		euc.guess,
		euc.freq, euc.point, euc.ascii, euc.kana, euc.ctrl, euc.state,
		euc.broken,
		(int)(euc.broken*100/(euc.freq+1)));
#endif	/* DEBUG */

  /* check broken code */
  if (jis.broken > 0 && jis.freq > 0) {
    switch (jis.guess) {
      case KT_JIS_OLD:
      case KT_JIS_OLD_X0201:
      case KT_JIS_OLD_X0212:
	jis.guess = PERCENT(jis.broken, jis.freq) < BINARY_LEVEL ?
		    KT_JIS_OLD_BROKEN :
		    KT_DATA;
	break;
      case KT_JIS_NEW:
      case KT_JIS_NEW_X0201:
      case KT_JIS_NEW_X0212:
	jis.guess = PERCENT(jis.broken, jis.freq) < BINARY_LEVEL ?
		    KT_JIS_NEW_BROKEN :
		    KT_DATA;
	break;
      case KT_JIS_1990:
      case KT_JIS_1990_X0212:
	jis.guess = PERCENT(jis.broken, jis.freq) < BINARY_LEVEL ?
		    KT_JIS_1990_BROKEN :
		    KT_DATA;
	break;
      default:
	break;
    }
  }
  else if (sjis.guess != KT_UNKNOWN && sjis.broken > 0 && sjis.freq > 0) {
    sjis.guess = PERCENT(sjis.broken, sjis.freq) < BINARY_LEVEL ?
		 KT_SJIS_BROKEN :
		 KT_DATA;
  }
  else if (euc.guess != KT_UNKNOWN && euc.broken > 0 && euc.freq > 0) {
    euc.guess = PERCENT(euc.broken, euc.freq) < BINARY_LEVEL ?
		KT_EUC_JP_BROKEN :
		KT_DATA;
  }

  /* I guess the kanji code as ASCII... */
  if (jis.guess   == KT_UNKNOWN &&
      euc.guess   == KT_UNKNOWN &&
      sjis.guess  == KT_UNKNOWN &&
      ascii.guess == KT_ASCII) {
    if (input.size == CHECK_BUFSIZE && file_f) {
      if (kcode != KC_UNKNOWN) {
	fwrite(input.buf, 1, (size_t)input.size, out);
      }
      readFile(input.fp);
      goto retry;	/* guess again */
    }
    else {
      return ascii.guess;
    }
  }
  /* I guess the kanji code as EUC or SJIS... */
  else if (euc.guess != KT_UNKNOWN && sjis.guess != KT_UNKNOWN) {
    if (euc.point > sjis.point && euc.freq >= sjis.freq) {
      return euc.guess;		/* I guess the kanji code as EUC-JP! */
    }
    else if (euc.point < sjis.point && euc.freq <= sjis.freq) {
      return sjis.guess;	/* I guess the kanji code as SJIS! */
    }
    else if (euc.freq > sjis.freq) {
      return euc.guess;		/* I guess the kanji code as EUC-JP! */
    }
    else if (euc.freq < sjis.freq) {
      return sjis.guess;	/* I guess the kanji code as SJIS! */
    }
    else {
      return KT_EUC_JP_or_SJIS;	/* I guess the kanji code as EUC-JP or SJIS. */
    }
  }
  else if (euc.guess != KT_UNKNOWN) {
    return euc.guess;		/* I guess the kanji code as EUC-JP! */
  }
  else if (sjis.guess != KT_UNKNOWN) {
    return sjis.guess;		/* I guess the kanji code as SJIS! */
  }
  else if (jis.guess != KT_UNKNOWN) {
    return jis.guess;		/* I guess the kanji code as JIS! */
  }
  else {
    return ascii.guess;		/* I guess the kanji code as ASCII! */
  }

  return KT_UNKNOWN;
}

#if	DEBUG_GUESS
/*
 *	MAIN FUNCTION
 */
int
main(argc, argv)
	int	argc;
	char	**argv;
{
  FILE *	fp;
  int		i;

  if (argc == 1) {
    fprintf(stderr, "usage: %s file ...\n", *argv);
    exit(1);
  }

  for (i = 1; i < argc; i++) {
    fp = fopen(argv[i], "r");
    if (fp == NULL) {
      fprintf(stderr, "can't open %s\n", argv[i]);
      continue;
    }
    fprintf(stderr, "\n[%s]\n", argv[i]);
    switch (wkfGuessKanjiCodeOfFile(fp)) {
      case KC_EUCorSJIS:
	fputs("EUC-JP or Shift-JIS\n", stderr);
	break;
      case KC_ASCII:
	fputs("ASCII\n", stderr);
	break;
      case KC_JIS:
	fputs("JIS\n", stderr);
	break;
      case KC_EUC:
	fputs("EUC-JP\n", stderr);
	break;
      case KC_SJIS:
	fputs("Shift-JIS\n", stderr);
	break;
      default:
	fputs("Unknown\n", stderr);
	break;
    }
    fclose(fp);
  }

  return 0;
}
#endif	/* DEBUG_GUESS */

