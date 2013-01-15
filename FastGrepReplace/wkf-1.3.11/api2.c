/*****************************************************************************
 *
 * FILE:	api2.c
 * DESCRIPTION:	WKF: extended API
 * DATE:	Wed, Jun 13 2001
 * UPDATE:	Fri, Jul  6 2007
 * AUTHOR:	Kouichi ABE (WALL) / à¢ïîçNàÍ
 * E-MAIL:	kouichi@MysticWALL.COM
 * URL:		http://www.MysticWALL.COM/
 * COPYRIGHT:	(c) 2001-2007 à¢ïîçNàÍÅ^Kouichi ABE (WALL), All rights reserved.
 * LICENSE:
 *
 *  Copyright (c) 2001-2007 Kouichi ABE (WALL) <kouichi@MysticWALL.COM>,
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
 * $Id: api2.c,v 1.14 2007/07/05 16:32:14 kouichi Exp $
 *
 *****************************************************************************/

#if	HAVE_CONFIG_H
#include "config.h"
#endif	/* HAVE_CONFIG_H */

#include <stdio.h>
#if	HAVE_STDLIB_H
#include <stdlib.h>
#endif	/* HAVE_STDLIB_H */
#if	HAVE_STRING_H
#include <string.h>
#endif	/* HAVE_STRING_H */
#include <errno.h>
#include "kanji.h"
#include "functions.h"

/*****************************************************************************
 *
 *	Macros and structures definition
 *
 *****************************************************************************/
#define	ISO_8859_1_Q	"=?ISO-8859-1?Q?"
#define	ISO_2022_JP_Q	"=?ISO-2022-JP?Q?"
#define	ISO_2022_JP_B	"=?ISO-2022-JP?B?"
#define	JAPANESE_EUC_B	"=?JAPANESE_EUC?B?"
#define	SHIFT_JIS_B	"=?SHIFT_JIS?B?"

#define	length(x)	(sizeof(x) - 1)

#ifdef	_MSC_VER
#define	strncasecmp(x,y,z)	strnicmp((x), (y), (z))
#endif	/* _MSC_VER */

/*****************************************************************************
 *
 *	Local functions declaration
 *
 *****************************************************************************/
static bool	skipChar(int);

/*****************************************************************************
 *
 *	Local variables declaration
 *
 *****************************************************************************/
/*
 * bit sequence for mask
 *
 * 0xfc = 11111100
 * 0x03 = 00000011
 * 0xf0 = 11110000
 * 0x0f = 00001111
 * 0xc0 = 11000000
 * 0x3f = 00111111
 * 0x30 = 00110000
 * 0x3c = 00111100
 */

/*
 * Base64 alphabet
 */
const char	base64[] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";


/*****************************************************************************
 *
 *	Functions definition
 *
 *****************************************************************************/

/*
 * Function Name:	wkfEncodeBase64String
 * Description:		encode a string to a base64 string
 * Arguments:		string, received buffer, received buffer size
 * Return Value:	Successful: encoded string byte, otherwise: -1
 */
int
wkfEncodeBase64String(str, rbuf, rsize)
	const String	str;
	String		rbuf;	/* received base64 encoded string */
	size_t		rsize;	/* received buffer size */
{
  size_t	len;
  register size_t	i, j;

  len = strlen(str);
  if (rsize < (size_t)((double)len * 1.33) + 1) {
    return -1;
  }

  /*
   * encode 3-bytes (24-bits) at a time
   */
  memset(rbuf, 0, rsize);
  for (i = j = 0; i < len - (len % 3); i += 3, j += 4) {
    rbuf[j]   = base64[( str[i]	  & 0xfc) >> 2];
    rbuf[j+1] = base64[((str[i]   & 0x03) << 4) | ((str[i+1] & 0xf0) >> 4)];
    rbuf[j+2] = base64[((str[i+1] & 0x0f) << 2) | ((str[i+2] & 0xc0) >> 6)];
    rbuf[j+3] = base64[( str[i+2] & 0x3f)];
  }

  i = len - (len % 3);	/* rest size */
  switch (len % 3) {
    case 2:	/* one character padding */
      rbuf[j]	= base64[( str[i]   & 0xfc) >> 2];
      rbuf[j+1] = base64[((str[i]   & 0x03) << 4) | ((str[i+1] & 0xf0) >> 4)];
      rbuf[j+2] = base64[( str[i+1] & 0x0f) << 2];
      rbuf[j+3] = base64[64];	/* Pad	*/
      j += 4;
      break;
    case 1:	/* two character padding */
      rbuf[j]	= base64[(str[i] & 0xfc) >> 2];
      rbuf[j+1]	= base64[(str[i] & 0x03) << 4];
      rbuf[j+2] = base64[64];	/* Pad	*/
      rbuf[j+3] = base64[64];	/* Pad	*/
      j += 4;
      break;
  }
  rbuf[j] = EOL;

  return j;
}

/*
 * Function Name:	wkfDecodeBase64String
 * Description:		decode a base64 string to a normal string
 * Arguments:		string, received buffer, received buffer size
 * Return Value:	Successful: decoded string byte, otherwise: -1
 */
int
wkfDecodeBase64String(str, rbuf, rsize)
	const String	str;
	String		rbuf;	/* received base64 decoded string */
	size_t		rsize;	/* received buffer size */
{
#if	1
  return wkfEncodeBase64Bin(str, strlen(str), rbuf, rsize);
#else
#define	VAL(x)	(str[(x)] == '=' ? 0 : strchr(base64, str[(x)]) - base64)
  size_t	len;
  register int	i, j;

  len = strlen(str);
  if (rsize < (int)((double)len * 0.75) + 1) {
    return -1;
  }

  /*
   * work on 4-words (24-bits) at a time
   */
  memset(rbuf, 0, rsize);
  for (i = j = 0; i < len; i += 4, j += 3) {
    rbuf[j]   =  (VAL(i) << 2)		 | ((VAL(i+1) & 0x30) >> 4);
    rbuf[j+1] = ((VAL(i+1) & 0x0f) << 4) | ((VAL(i+2) & 0x3c) >> 2); 
    rbuf[j+2] = ((VAL(i+2) & 0x03) << 6) |  (VAL(i+3) & 0x3f);
  }
  /* remove padding data */
  if (str[len - 1] == '=') { j--; }
  if (str[len - 2] == '=') { j--; }

  return j;
#endif
}

/*
 * Function Name:	wkfEncodeBase64Bin
 * Description:		encode a string to a base64 string as binary
 * Arguments:		string, string size, received buffer, received buffer size
 * Return Value:	Successful: encoded string byte, otherwise: -1
 * Note:		written by Tamotsu Hasegawa on Fri, Jul 6 2007.
 */
int
wkfEncodeBase64Bin(str, len, rbuf, rsize)
	const String	str;
	size_t		len;
	String		rbuf;	/* received base64 encoded string */
	size_t		rsize;	/* received buffer size */
{
  register size_t	i, j;

  if (rsize < (size_t)((double)len * 1.33) + 1) {
    return -1;
  }

  /*
   * encode 3-bytes (24-bits) at a time
   */
  memset(rbuf, 0, rsize);
  for (i = j = 0; i < len - (len % 3); i += 3, j += 4) {
    rbuf[j]   = base64[( str[i]	  & 0xfc) >> 2];
    rbuf[j+1] = base64[((str[i]   & 0x03) << 4) | ((str[i+1] & 0xf0) >> 4)];
    rbuf[j+2] = base64[((str[i+1] & 0x0f) << 2) | ((str[i+2] & 0xc0) >> 6)];
    rbuf[j+3] = base64[( str[i+2] & 0x3f)];
  }

  i = len - (len % 3);	/* rest size */
  switch (len % 3) {
    case 2:	/* one character padding */
      rbuf[j]	= base64[( str[i]   & 0xfc) >> 2];
      rbuf[j+1] = base64[((str[i]   & 0x03) << 4) | ((str[i+1] & 0xf0) >> 4)];
      rbuf[j+2] = base64[( str[i+1] & 0x0f) << 2];
      rbuf[j+3] = base64[64];	/* Pad	*/
      j += 4;
      break;
    case 1:	/* two character padding */
      rbuf[j]	= base64[(str[i] & 0xfc) >> 2];
      rbuf[j+1]	= base64[(str[i] & 0x03) << 4];
      rbuf[j+2] = base64[64];	/* Pad	*/
      rbuf[j+3] = base64[64];	/* Pad	*/
      j += 4;
      break;
  }
  rbuf[j] = EOL;

  return j;
}

/*****************************************************************************/

/*
 * Function Name:	wkfDecodeBase64File
 * Description:		decode a base64 encoded file
 * Arguments:		input file pointer, output file pointer
 * Return Value:	none
 */
void
wkfDecodeBase64File(fin, fout)
	register FILE *	fin;
	register FILE *	fout;
{
#define	VAL2(x)	((x) == '=' ? 0 : strchr(base64, (x)) - base64)
  /*
   * work on 4-words (24-bits) at a time
   */
  while (!feof(fin)) {
    int	c1, c2, c3, c4;

    do { c1 = fgetc(fin); } while (skipChar(c1));
    do { c2 = fgetc(fin); } while (skipChar(c2));
    do { c3 = fgetc(fin); } while (skipChar(c3));
    do { c4 = fgetc(fin); } while (skipChar(c4));
    if (c1 == EOF || c2 == EOF || c3 == EOF || c4 == EOF) {
      break;
    }
    fputc( (VAL2(c1) << 2)	   | ((VAL2(c2) & 0x30) >> 4), fout);
    fputc(((VAL2(c2) & 0x0f) << 4) | ((VAL2(c3) & 0x3c) >> 2), fout); 
    fputc(((VAL2(c3) & 0x03) << 6) |  (VAL2(c4) & 0x3f), fout);
  }
}

static bool
skipChar(c)
	int	c;
{
  switch ((char)c) {
    case SPC:
    case TAB:
    case CR:
    case LF:
    case EOL:
      return true;
    default:
      return false;
  }
}

/*****************************************************************************/

/*
 * Function Name:	wkfEncodeQuotedPrintableString
 * Description:		encode a string to a quoted-printable string
 * Arguments:		string, received buffer, received buffer size
 * Return Value:	Successful: encoded string byte, otherwise: -1
 */
int
wkfEncodeQuotedPrintableString(str, rbuf, rsize)
	const String	str;
	String		rbuf;	/* received quoted-printable encoded string */
	size_t		rsize;	/* received buffer size */
{
#if	1
  return wkfEncodeQuotedPrintableBin(str, strlen(str), rbuf, rsize);
#else
  static char	HEX[]	= "0123456789ABCDEF";
  size_t	len	= 0;
  String	os	= str;
  String	s	= rbuf;
  char		c;
  int		i	= 0;

  len = strlen(str);
  if (rsize < (int)((double)len * 1.33) + 1) {
    return -1;
  }

  memset(rbuf, 0, rsize);
  while ((c = *os) != EOL) {
    /* White Space */
    if (c >= 9 && c <= 32) {
      *s++ = '='; i++;
      *s++ = '2'; i++;
      *s++ = '0'; i++;
      os++;
    }
    /* Literal representation */
    else if ((c >= 33 && c <= 60) || (c >= 62 && c <= 126)) {
      *s++ = c; i++;
      os++;
    }
    /* equal character */
    else if (c == 61) {
      *s++ = '='; i++;
      *s++ = '3'; i++;
      *s++ = 'D'; i++;
      os++;
    }
    /* General 8bit representation */
    else {
      *s++ = '='; i++;
      *s++ = HEX[(c >> 4) & 0x0f]; i++;
      *s++ = HEX[c & 0x0f]; i++;
      os++;
    }
  }

  return i;
#endif
}

/*
 * Function Name:	wkfEncodeQuotedPrintableBin
 * Description:		encode a string to a quoted-printable string as binary
 * Arguments:		string, string size, received buffer, received buffer size
 * Return Value:	Successful: encoded string byte, otherwise: -1
 * Note:		written by Tamotsu Hasegawa on Fri, Jul 6 2007.
 */
int
wkfEncodeQuotedPrintableBin(str, len, rbuf, rsize)
	const String	str;
	size_t		len;
	String		rbuf;	/* received quoted-printable encoded string */
	size_t		rsize;	/* received buffer size */
{
  static char	HEX[]	= "0123456789ABCDEF";
  String	os	= str;
  String	s	= rbuf;
  char		c;
  int		i	= 0;

  if (rsize < (size_t)((double)len * 1.33) + 1) {
    return -1;
  }

  memset(rbuf, 0, rsize);
  while ((c = *os) != EOL) {
    /* White Space */
    if (c >= 9 && c <= 32) {
      *s++ = '='; i++;
      *s++ = '2'; i++;
      *s++ = '0'; i++;
      os++;
    }
    /* Literal representation */
    else if ((c >= 33 && c <= 60) || (c >= 62 && c <= 126)) {
      *s++ = c; i++;
      os++;
    }
    /* equal character */
    else if (c == 61) {
      *s++ = '='; i++;
      *s++ = '3'; i++;
      *s++ = 'D'; i++;
      os++;
    }
    /* General 8bit representation */
    else {
      *s++ = '='; i++;
      *s++ = HEX[(c >> 4) & 0x0f]; i++;
      *s++ = HEX[c & 0x0f]; i++;
      os++;
    }
  }

  return i;
}

/*
 * Function Name:	wkfDecodeQuotedPrintableString
 * Description:		decode a quoted-printable string to a normal string
 * Arguments:		string, received buffer, received buffer size
 * Return Value:	Successful: decoded string byte, otherwise: -1
 */
int
wkfDecodeQuotedPrintableString(str, rbuf, rsize)
	const String	str;
	String		rbuf;	/* received quoted-printable decoded string */
	size_t		rsize;	/* received buffer size */
{
  size_t	len	= 0;
  String	os	= str;
  String	s	= rbuf;
  char		c, c1;
  int		i	= 0;

  len = strlen(str);
  if (rsize < (size_t)((double)len * 0.75) + 1) {
    return -1;
  }

  memset(rbuf, 0, rsize);
  while ((c = *os) != EOL) {
    if (c == '=') {
      os++;
      if ((c = *os++) && (c1 = *os++)) {
	if (c == CR || c == LF) {
	  continue;
	}
	*s++ =  ((c  >= 'A' ? ((c  & 0xdf) - 'A') + 10 : c  - '0') * 16) +
		((c1 >= 'A' ? ((c1 & 0xdf) - 'A') + 10 : c1 - '0'));
	i++;
	os--;	/* back one word */
      }
      else {
	return i;
      }
    }
    else {
      *s++ = c;
      i++;
      os++;
    }
  }

  return i;
}

/*
 * Function Name:	wkfQuotedPrintableBase64File
 * Description:		decode a quoted-printable encoded file
 * Arguments:		input file pointer, output file pointer
 * Return Value:	none
 */
void
wkfDecodeQuotedPrintableFile(fin, fout)
	register FILE *	fin;
	register FILE *	fout;
{
  while (!feof(fin)) {
    int	c, c1;

    c = fgetc(fin);
    if (c == '=') {
      c	 = fgetc(fin);
      c1 = fgetc(fin);
      if (c == EOF || c1 == EOF) {
	fflush(fout);
	break;
      }
      if (c == CR || c == LF || c == EOL) {
	ungetc(c1, fin);
	continue;
      }
      fputc(((c  >= 'A' ? ((c  & 0xdf) - 'A') + 10 : c  - '0') * 16) +
	    ((c1 >= 'A' ? ((c1 & 0xdf) - 'A') + 10 : c1 - '0')), fout);
    }
    else {
      fputc(c, fout);
    }
  }
  fflush(fout);
}

/*****************************************************************************/

/*
 * Function Name:	wkfEncodeMimeString
 * Description:		encode string with MIME
 * Arguments:		string, received buffer, received buffer size, kcode
 * Return Value:	Successful: CONV_OK, otherwise CONV_ERR
 */
conv_t
wkfEncodeMimeString(str, rbuf, rsize, kout)
	const String	str;
	String		rbuf;
	size_t		rsize;
	kcode_t		kout;
{
  conv_t	retval = CONV_ERR;
  size_t	klen;
  char *	kbuf;

  klen = rsize * 2;
  kbuf = (char *)calloc(klen, sizeof(char));
  if (kbuf != NULL) {
    retval = wkfConvertKanjiCodeOfString(KC_UNKNOWN, str, kout, kbuf, klen);
    if (retval != CONV_ERR) {
      size_t	b64len;
      char *	b64buf;

      b64len = strlen(kbuf) * 2;
      b64buf = (char *)calloc(b64len, sizeof(char));
      if (b64buf != NULL) {
	int	n;

	n = wkfEncodeBase64String(kbuf, b64buf, b64len);
	if (n > 0) {
  	  size_t	len = 0;
      
  	  memset(rbuf, 0, rsize);
	  switch (kout) {
	    case KC_JIS:
	      len = length(ISO_2022_JP_B);
	      memcpy(rbuf, ISO_2022_JP_B, len);
	      break;
	    case KC_EUC:
	      len = length(JAPANESE_EUC_B);
	      memcpy(rbuf, JAPANESE_EUC_B, len);
	      break;
	    case KC_SJIS:
	      len = length(SHIFT_JIS_B);
	      memcpy(rbuf, SHIFT_JIS_B, len);
	      break;
	    default:
	      retval = CONV_ERR;
	      break;
	  }
  	  memcpy(rbuf + len, b64buf, (size_t)n);
  	  len += n;
  	  memcpy(rbuf + len, "?=", 2);
   	}
	free((char *)b64buf);
      }
    }
    free((char *)kbuf);
  }

  return retval;
}

/*
 * Function Name:	wkfDecodeMimeString
 * Description:		decode a MIME encoded string
 * Arguments:		string, received buffer, received buffer size, kcode
 * Return Value:	Successful: CONV_OK, otherwise CONV_ERR
 */
conv_t
wkfDecodeMimeString(str, rbuf, rsize, kout)
	const String	str;
	String		rbuf;	/* received base64 decoded string */
	size_t		rsize;	/* received buffer size */
	kcode_t		kout;
{
  conv_t	retval	= CONV_ERR;
  String	mstr	= NULL;	/* mime encoded string (copy of str) */

  mstr = strdup(str);
  if (mstr != NULL) {
    size_t	len = 0;
    String	src = NULL;	/* pointer for str */
    String	dst = NULL;	/* pointer for recv */

    /* deocde mime string */
    memset(rbuf, 0, rsize);
    for (len = 0, src = mstr, dst = rbuf; *src != EOL && len < rsize; ) {
      if (*src == '=') {
#define	MIME_NONE	(0)
#define	MIME_BASE64	(1)
#define	MIME_QUOTED	(2)
	String	end  = NULL;
	int	mime = MIME_NONE;

	if (strncasecmp(src, ISO_2022_JP_B, length(ISO_2022_JP_B)) == 0) {
	  mime = MIME_BASE64;
	  src += length(ISO_2022_JP_B);
	}
	else if (strncasecmp(src, JAPANESE_EUC_B,
			     length(JAPANESE_EUC_B)) == 0) {
	  mime = MIME_BASE64;
	  src += length(JAPANESE_EUC_B);
	}
	else if (strncasecmp(src, SHIFT_JIS_B, length(SHIFT_JIS_B)) == 0) {
	  mime = MIME_BASE64;
	  src += length(SHIFT_JIS_B);
	}
	else if (strncasecmp(src, ISO_8859_1_Q, length(ISO_8859_1_Q)) == 0) {
	  mime = MIME_QUOTED;
	  src += length(ISO_8859_1_Q);
	}
	else if (strncasecmp(src, ISO_2022_JP_Q, length(ISO_2022_JP_Q)) == 0) {
	  mime = MIME_QUOTED;
	  src += length(ISO_2022_JP_Q);
	}

	if (mime != MIME_NONE) {
	  end = strstr(src, "?=");
	  if (end == NULL) {	/* invalid MIME encoding */
	    goto error;
	  }
	  *end = EOL;

	  switch (mime) {
	    case MIME_BASE64: {
		int	n;

		n = wkfDecodeBase64String(src, dst, rsize - len);
		if (n == -1) {
		  goto error;
		}
		src  = end + 2;
		dst += n;
		len += n;
	      }
	      break;
	    case MIME_QUOTED: {
		int	n;

		n = wkfDecodeQuotedPrintableString(src, dst, rsize - len);
		if (n == -1) {
		  goto error;
		}
		src  = end + 2;
		dst += n;
		len += n;
	      }
	    default:
	      break;
	  }
	  continue;
	}
      }
      *dst++ = *src++;
      len++;
    }
    retval = CONV_NO;
error:
    free((String)mstr);
  }

  /* convert kanji code */
  if ((retval != CONV_ERR) &&
      (kout == KC_JIS || kout == KC_EUC || kout == KC_SJIS)) {
    mstr = strdup(rbuf);
    if (mstr != NULL) {
      memset(rbuf, 0, rsize);
      retval = wkfConvertKanjiCodeOfString(KC_UNKNOWN, mstr, kout, rbuf, rsize);
      free((String)mstr);
    }
  }

  return retval;
}

/*
 * Function Name:	wkfStringToHexString
 * Description:		convert the string to the hex string
 * Arguments:		string, received buffer, received buffer size
 * Return Value:	CONV_OK, CONV_NO or CONV_ERR
 */
conv_t
wkfStringToHexString(str, rbuf, rsize)
	const String	str;
	String		rbuf;	/* received base64 decoded string */
	size_t		rsize;	/* received buffer size */
{
  static char	hex[]	= "0123456789abcdef";
  String	sjis	= NULL;	/* sjis string */
  size_t	len	= 0;
  String	p	= NULL;
  String	q	= NULL;

  /* convert str to SJIS kanji code */
  len	= strlen(str) + 1;
  sjis	= (String)calloc(len, sizeof(char));
  if (sjis == NULL) {
    return CONV_ERR;
  }
  switch (wkfConvertKanjiCodeOfString(KC_UNKNOWN, str, KC_SJIS, sjis, len)) {
    case CONV_ERR:
      free((String)sjis);
      return CONV_ERR;
    case CONV_NO:
      return CONV_NO;
    case CONV_OK:
    default:
      break;
  }

  /* convert sjis to hex string */
  for (p = sjis, q = rbuf; *p != EOL && (size_t)(q - rbuf) < rsize - 1; ) {
    if (IsASCII(*p)) {
      *q++ = *p++;
    }
    else if (IsSJIS1(*p) && IsSJIS2(*(p+1))) {
      *q++ = ':';
      *q++ = hex[(*p >> 4) & 0x0f];
      *q++ = hex[*p++ & 0x0f];
      *q++ = ':';
      *q++ = hex[(*p >> 4) & 0x0f];
      *q++ = hex[*p++ & 0x0f];
    }
    else {
      *q++ = *p++;
    }
  }
  *++q = EOL;

  free((String)sjis);

  return CONV_OK;
}

/*
 * Function Name:	wkfMimeStringToHexString
 * Description:		decode a MIME encoded string and convert hex string
 * Arguments:		string, received buffer, received buffer size
 * Return Value:	CONV_OK, CONV_NO or CONV_ERR
 */
conv_t
wkfMimeStringToHexString(str, rbuf, rsize)
	const String	str;
	String		rbuf;	/* received base64 decoded string */
	size_t		rsize;	/* received buffer size */
{
  String	mstr = NULL;
  conv_t	ret;

  memset(rbuf, 0, rsize);
  if (wkfDecodeMimeString(str, rbuf, rsize, KC_UNKNOWN) == CONV_ERR) {
    return CONV_ERR;
  }

  mstr = strdup(rbuf);
  if (mstr == NULL) {
    return CONV_ERR;
  }

  ret = wkfStringToHexString(mstr, rbuf, rsize);
  free((String)mstr);

  return ret;
}

/*****************************************************************************/

/*
 * Function Name:	wkfFoldString
 * Description:		fold string
 * Arguments:		string, fold length, received buffer,
 *			received buffer size, kcode
 * Return Value:	Successful: CONV_OK, otherwise CONV_ERR
 */
conv_t
wkfFoldString(str, flen, rbuf, rsize, kout)
	const String	str;
	size_t		flen;
	String		rbuf;
	size_t		rsize;
	kcode_t		kout;
{
  conv_t	retval = CONV_ERR;

  if (str != NULL && flen > 2) {
    String	text;
    size_t	tlen = strlen(str);

    if (tlen <= flen) {
      return CONV_NO;	/* no need fold */
    }

    text = (String)calloc(tlen + 1, sizeof(char));
    if (text != NULL) {
      switch ((retval = wkfConvertKanjiCodeOfString(KC_UNKNOWN, str,
						    KC_EUC, text, tlen))) {
   	case CONV_NO:
    	case CONV_OK: {
	    register size_t	i;

	    for (i = 0; i < flen; ) {
	      if (text[i] >= 0xa1 && text[i] <= 0xdf) {
		i += 2;
		if (i >= flen) {
		  i -= 2;
		  break;
		}
	      }
	      else {
		i++;
	      }
	    }
	    text[i] = EOL;

	    memset(rbuf, 0, rsize);
	    retval = wkfConvertKanjiCodeOfString(KC_EUC, text,
						 kout, rbuf, rsize);
	  }
	  /* through */
       	case CONV_ERR:
     	default:
  	  break;
      }
      free((String)text);
    }
  }

  return retval;
}

/*****************************************************************************/

/*
 * Function Name:	wkfDecodeEscapedURIString
 * Description:		decode escaped URI string
 * Arguments:		string, received buffer, received buffer size, kcode
 * Return Value:	Successful: CONV_OK, otherwise CONV_ERR
 * Reference:		RFC2396, Uniform Resource Identifiers (URI),
 *			Generic Syntax
 */
conv_t
wkfDecodeEscapedURIString(str, rbuf, rsize, kout)
	const String	str;
	String		rbuf;	/* received base64 decoded string */
	size_t		rsize;	/* received buffer size */
	kcode_t		kout;
{
  conv_t	retval	= CONV_ERR;
  String	tmp;	/* original string (to work) */
  String	s;

  tmp = s = strdup(str);
  if (s != NULL) {
    register char *	os  = tmp;
    register char *	ret = s;
    char		c, c1;

    while ((c = *os) != EOL) {
      if (c == '%') {
      	os++;
       	if ((c = *os++) && (c1 = *os++)) {
  	  *s++ = ((c  >= 'A' ? ((c  & 0xdf) - 'A') + 10 : c  - '0') * 16) +
  		 ((c1 >= 'A' ? ((c1 & 0xdf) - 'A') + 10 : c1 - '0'));
  	  os--;	/* back one word */
   	}
    	else {
  	  goto done;
    	}
      }
#if	0
      /* escaped white space */
      else if (c == '+') {
	*s++ = ' ';
      }
#endif
      /* normal characters */
      else {
	*s++ = c;
      }
      os++;
    }
    *s = EOL;
done:
    /* convert kanji code */
    if (kout == KC_JIS || kout == KC_EUC || kout == KC_SJIS) {
      memset(rbuf, 0, rsize);
      retval = wkfConvertKanjiCodeOfString(KC_UNKNOWN, ret, kout, rbuf, rsize);
    }
    free((char *)tmp);
  }

  return retval;
}

