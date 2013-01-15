/*****************************************************************************
 *
 * FILE:	wkf.c
 * DESCRIPTION:	WKF: WALL's Kanji Filter
 * DATE:	Sat, Jan 15 2000
 * UPDATE:	Fri, Jul  6 2007
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
 * $Id: wkf.c,v 1.11 2007/07/05 16:35:21 kouichi Exp $
 *
 *****************************************************************************/

#if	HAVE_CONFIG_H
#include "config.h"
#endif	/* HAVE_CONFIG_H */

#include <stdio.h>
#include <stdlib.h>
#ifndef _WIN32
#include <unistd.h>
#endif	/* _WIN32 */
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#if defined(_WIN32) || defined(__EMX__)
#include <sys/utime.h>
#include <fcntl.h>
#else
#include <utime.h>
#endif	/* _WIN32 __EMX__ */
#include <signal.h>

#define	MAIN_MODULE
#include "wkf.h"
#undef	MAIN_MODULE

#ifdef _WIN32
#define snprintf _snprintf
extern int	optind;
#endif

/*****************************************************************************
 *
 *	Program and Copyright
 *
 *****************************************************************************/
const char	Program[] = "wkf";
const char	Copyright[] =
	"Copyright (c) 1997-2007 Kouichi ABE (WALL), All rights reserved.";

/*****************************************************************************
 *
 *	Macros and structures definition
 *
 *****************************************************************************/
#define	SOFTWARE_NAME		"WKF (WALL's Kanji Filter)"
#define	SOFTWARE_VERSION	WKF_VERSION_TEXT
#define	SOFTWARE_ADDRESS	"http://www.MysticWALL.COM/software/wkf/index.html"

#define	FILE_LEN	(256)	/* max length of filename	*/
#define	EOL		'\0'	/* End of Line */

typedef struct _option {	/* command option type */
  bool	checkcode;	/* flag to check kanji code */
  bool	showfname;	/* flag to show filename */
  bool	mimedecode;	/* flag to decode mime encoding string */
  bool	uridecode;	/* flag to decode escaped URI string */
  bool	backup;		/* flag to do backup */
  bool	savetstamp;	/* flag to save the time-stamp of file */
  bool	lineconv;	/* flag to convert string with each line */
  bool	lineend;	/* type of the code of line end	*/
} Option;

typedef enum {	/* error code type */
  ERR_NO_CONVERT_OPTION,	/* no option to convert kanji code */
  ERR_MULTIPLE_CONVERT_OPTION,	/* multiple option to convert */
  ERR_MULTIPLE_INPUT_CODE_OPTION, /* multiple option of input kanji code */
  ERR_MULTIPLE_EXTENDED_OPTION,	/* multiple extended option to convert */
  ERR_MULTIPLE_LINE_END_OPTION	/* multiple option of the code of line end */
} error_t;

/*****************************************************************************
 *
 *	Local functions declaration
 *
 *****************************************************************************/
static conv_t	convertKanjiCodeInFile(kcode_t, FILE *, kcode_t, FILE *,
				       Option *);
static conv_t	convertKanjiCodeOfEachLineInFile(kcode_t, FILE *,
						 kcode_t, FILE *, Option *);
static int	savePermission(String, String);
static int	saveTimeStamp(String, String);
static void	printErrorMessage(error_t);
static void	copyright(void);
static void	usage(void);

/*****************************************************************************
 *
 *	Functions definition
 *
 *****************************************************************************/

/*
 * FUNCTION NAME:	convertKanjiCodeInFile
 * DESCRIPTION:		convert kanji code in a file
 * ARGUMENTS:		input kanji code, input file pointer,
 *			output kanji code, output file pointer, command option
 * RETURN VALUE:	Successful: CONV_OK, otherwise: CONV_ERR
 */
static conv_t
convertKanjiCodeInFile(kin, fin, kout, fout, opt)
	kcode_t		kin;
	register FILE *	fin;
	kcode_t		kout;
	register FILE *	fout;
	Option *	opt;
{
  conv_t	ret;

#if defined(_WIN32) || defined(__EMX__)
  /* èoóÕêÊÇÕ D,M,U Çñæé¶ìIÇ…éwíËÇµÇ»ÇØÇÍÇŒ CRLF */
  _setmode(fileno(fin), O_BINARY);  /* ì¸óÕÇÕèÌÇ… binary open */
  if (opt->lineend == false) {
    wkfSetLineEndCode(LE_CRLF);
    _setmode(fileno(fout), O_BINARY);
  }
#endif	/* _WIN32 __EMX__ */
  setvbuf(fin,  (char *)NULL, _IOFBF, 0);
  setvbuf(fout, (char *)NULL, _IOFBF, 0);
  if (opt->lineconv) {
    ret = convertKanjiCodeOfEachLineInFile(kin, fin, kout, fout, opt);
  }
  else {
    ret = wkfConvertKanjiCodeOfFile(kin, fin, kout, fout);
  }

  if (fout == stdout) {
    fflush(fout);
  }
  else {
    fclose(fout);
  }
  fclose(fin);

  return ret;
}

/*
 * FUNCTION NAME:	convertKanjiCodeOfEachLineInFile
 * DESCRIPTION:		convert kanji code of each line in a file
 * ARGUMENTS:		input kanji code, input file pointer,
 *			output kanji code, output file pointer, command option
 * RETURN VALUE:	Successful: CONV_OK, otherwise: CONV_ERR
 */
static conv_t
convertKanjiCodeOfEachLineInFile(kin, fin, kout, fout, opt)
	kcode_t		kin;
	register FILE *	fin;
	kcode_t		kout;
	register FILE *	fout;
	Option *	opt;
{
  char	buf[BUFSIZ];
  int	err = 0;

  memset(buf, 0, sizeof(buf));
  while (fgets(buf, sizeof(buf), fin) != NULL) {
    char	rbuf[BUFSIZ*2];
    conv_t	ret;

    memset(rbuf, 0, sizeof(rbuf));
    ret = wkfConvertKanjiCodeOfString(kin, buf, kout, rbuf, sizeof(rbuf));
    switch (ret) {
      case CONV_ERR:
	err++;
      case CONV_NO:
	if (opt->mimedecode) {
	  ret = wkfDecodeMimeString(buf, rbuf, sizeof(rbuf), kout);
	}
	else if (opt->uridecode) {
	  ret = wkfDecodeEscapedURIString(buf, rbuf, sizeof(rbuf), kout);
	}
      case CONV_OK:
      default:
	if (ret != CONV_ERR) {
	  fputs(rbuf, fout);
	}
	else {
	  fputs(buf, fout);
	}
	break;
    }
    memset(buf, 0, sizeof(buf));
  }

  return (err > 0 ? CONV_ERR : CONV_OK);
}

/*****************************************************************************/

/*
 * FUNCTION NAME:	savePermission
 * DESCRIPTION:		save permission of file
 * ARGUMENTS:		input filename, output filename
 * RETURN VALUE:	Successful: 0, otherwise: -1
 */
static int
savePermission(org_name, new_name)
	String	org_name;
	String	new_name;
{
  struct stat		sbuf;

  if (stat(org_name, &sbuf) == 0 && chmod(new_name, sbuf.st_mode) == 0) {
    return 0;
  }
  return -1;
}

/*
 * FUNCTION NAME:	saveTimeStamp
 * DESCRIPTION:		save time-stamp of file
 * ARGUMENTS:		input filename, output filename
 * RETURN VALUE:	Successful: 0, otherwise: -1
 */
static int
saveTimeStamp(org_name, new_name)
	String	org_name;
	String	new_name;
{
  struct stat		sbuf;

  if (stat(org_name, &sbuf) == 0) {
    struct utimbuf	tbuf;

    tbuf.actime  = sbuf.st_atime;
    tbuf.modtime = sbuf.st_mtime;

    if (utime(new_name, &tbuf) == 0) {
      return 0;
    }
  }

  return -1;
}

/*****************************************************************************/

/*
 * FUNCTION NAME:	printErrorMessage
 * DESCRIPTION:		print error message
 * ARGUMENTS:		error type
 * RETURN VALUE:	none
 */
static void
printErrorMessage(err)
	error_t	err;
{
  fprintf(stderr, "%s: ", Program);
  switch (err) {
    case ERR_NO_CONVERT_OPTION:
      fprintf(stderr, "no convert option -- needs 'j' or 'e' or 's' option");
      break;
    case ERR_MULTIPLE_CONVERT_OPTION:
      fprintf(stderr, "bad option -- j, e, s, c, C selected only one");
      break;
    case ERR_MULTIPLE_INPUT_CODE_OPTION:
      fprintf(stderr, "bad option -- J, E, S selected only one");
      break;
    case ERR_MULTIPLE_EXTENDED_OPTION:
      fprintf(stderr, "bad option -- m, x selected only one");
      break;
    case ERR_MULTIPLE_LINE_END_OPTION:
      fprintf(stderr, "bad option -- M, D, U selected only one");
      break;
  }
  fprintf(stderr, ".\n");
  usage();
}

/*
 * FUNCTION NAME:	copyright
 * DESCRIPTION:		print copyright
 * ARGUMENTS:		none
 * RETURN VALUE:	none
 */
static void
copyright(void)
{
  fprintf(stderr, "%s\n%s\n%s\n%s\n",
	  SOFTWARE_NAME, SOFTWARE_VERSION, Copyright, SOFTWARE_ADDRESS);
}

/*
 * FUNCTION NAME:	usage
 * DESCRIPTION:		print usage
 * ARGUMENTS:		none
 * RETURN VALUE:	none
 */
static void
usage(void)
{
  fprintf(stderr,
	  "usage: "
	  "%s [-{j|e|s|c|C}] [-mxzALT] [-{J|E|S}] [-{M|D|U}] [<file>...]\n\n",
	  Program);
  exit(64);
}

/*****************************************************************************/

/*
 *	MAIN FUNCTION 
 */
int
main(argc, argv)
	int	argc;
	char *	argv[];
{
  register int	ch;
  kcode_t	kin  = KC_UNKNOWN;	/* input kanji code */
  kcode_t	kout = KC_UNKNOWN;	/* output kanji code */
  Option	opt  = {		/* command option */
    /* set default value */
    false,	/* checkcode:	no checking kanji code */
    false,	/* showfname:	no showing filename */
    false,	/* mimedecode:	no mime decode */
    false,	/* uridecode:	no escaped uri decode */
    false,	/* backup:	no backup */
    false,	/* savetstamp:	no saving time-stamp */
    false,	/* lineconv:	no each line converting */
    false	/* lineend:	default use (UNIX type) */
  };

  /* addressing arguments */
  while ((ch = getopt(argc, argv, "hv?jescCmxzALTJESMDU")) != -1) {
    switch (ch) {
      case 'C':	/* check kanji code and show filename */
	opt.showfname = true;
      case 'c':	/* check kanji code */
	opt.checkcode = true;
	if (kin != KC_UNKNOWN || kout != KC_UNKNOWN) {
	  printErrorMessage(ERR_MULTIPLE_CONVERT_OPTION);
	}
	break;
      case 'j':	/* convert to JIS */
	if (opt.checkcode) {
	  printErrorMessage(ERR_MULTIPLE_CONVERT_OPTION);
	}
	else if (kout != KC_UNKNOWN) {
	  printErrorMessage(ERR_MULTIPLE_CONVERT_OPTION);
	}
	kout = KC_JIS;
	break;
      case 'e':	/* convert to Japanese EUC */
	if (opt.checkcode) {
	  printErrorMessage(ERR_MULTIPLE_CONVERT_OPTION);
	}
	else if (kout != KC_UNKNOWN) {
	  printErrorMessage(ERR_MULTIPLE_CONVERT_OPTION);
	}
	kout = KC_EUC;
	break;
      case 's':	/* convert to Shift-JIS */
	if (opt.checkcode) {
	  printErrorMessage(ERR_MULTIPLE_CONVERT_OPTION);
	}
	else if (kout != KC_UNKNOWN) {
	  printErrorMessage(ERR_MULTIPLE_CONVERT_OPTION);
	}
	kout = KC_SJIS;
	break;
      case 'm':	/* decode mime string */
	if (opt.uridecode) {
	  printErrorMessage(ERR_MULTIPLE_EXTENDED_OPTION);
	}
	opt.mimedecode	= true;
	opt.lineconv	= true;	/* depended */
	break;
      case 'x':	/* decode escaped URI string */
	if (opt.mimedecode) {
	  printErrorMessage(ERR_MULTIPLE_EXTENDED_OPTION);
	}
	opt.uridecode	= true;
	opt.lineconv	= true;	/* depended */
	break;
      case 'z':	/* convert 2byte alphabet and figures to 1byte ones */
	wkfConvertZenkaku2ASCII(true);
	break;
      case 'A':	/* make a backup file */
	opt.backup = true;
	break;
      case 'L':	/* convert string with each line */
	opt.lineconv = true;
	break;
      case 'T':	/* save the time-stamp */
	opt.savetstamp = true;
	break;
      case 'M':	/* output the line end with Macintosh format */
	if (opt.lineend) {
	  printErrorMessage(ERR_MULTIPLE_LINE_END_OPTION);
	}
	opt.lineend = true;
	wkfSetLineEndCode(LE_CR);
	break;
      case 'D':	/* output the line end with DOS format */
	if (opt.lineend) {
	  printErrorMessage(ERR_MULTIPLE_LINE_END_OPTION);
	}
	opt.lineend = true;
	wkfSetLineEndCode(LE_CRLF);
	break;
      case 'U':	/* output the line end with UNIX format */
	if (opt.lineend) {
	  printErrorMessage(ERR_MULTIPLE_LINE_END_OPTION);
	}
	opt.lineend = true;
	wkfSetLineEndCode(LE_LF);
	break;
      case 'J':	/* input kanji code is JIS */
	if (kin != KC_UNKNOWN) {
	  printErrorMessage(ERR_MULTIPLE_INPUT_CODE_OPTION);
	}
	kin = KC_JIS;
	break;
      case 'E':	/* input kanji code is Japanese EUC */
	if (kin != KC_UNKNOWN) {
	  printErrorMessage(ERR_MULTIPLE_INPUT_CODE_OPTION);
	}
	kin = KC_EUC;
	break;
      case 'S':	/* input kanji code is Shift-JIS */
	if (kin != KC_UNKNOWN) {
	  printErrorMessage(ERR_MULTIPLE_INPUT_CODE_OPTION);
	}
	kin = KC_SJIS;
	break;
      case 'v':	/* show version */
	copyright();
	exit(0);
      case 'h':	/* show usage */
      default:
	copyright();
	usage();
    }
  }
  argc -= optind;
  argv += optind;

  /* check option to convert kanji code */
  if (kout == KC_UNKNOWN &&	/* no option to convert kanji code */
      opt.checkcode == false) {	/* no check kanji code */
    printErrorMessage(ERR_NO_CONVERT_OPTION);
  }

  /* processing every file */
  if (argc > 0) {
    register int	i;

    for (i = 0; i < argc; i++) {
      register FILE *	fin;

      signal(SIGINT,  SIG_IGN);
      fin = fopen(argv[i], "r");
      if (fin == NULL) {
	fprintf(stderr, "failed to open %s\n", argv[i]);
	continue;
      }
      /* only check kanji code */
      if (opt.checkcode) {
	if (opt.showfname) {
	  fprintf(stdout, "%-16s\t", argv[i]);
	  fflush(stdout);
	}
	setvbuf(fin, (char *)NULL, _IOFBF, 0);
	wkfPrintGuessedKanjiCodeOfFile(fin, false);
      }
      /* make backup file */
      else if (opt.backup) {
	char *	tname;
	
	asprintf(&tname, "%s.wkf~", argv[i]);
	if (tname != NULL) {
	  register FILE *	fout;
	  
	  fout = fopen(tname, "w");
	  if (fout != NULL) {
	    conv_t	ret;	/* return value of convert function */

	    ret = convertKanjiCodeInFile(kin, fin, kout, fout, &opt);
	    if (ret == CONV_OK) {
	      char *	bname;
	      
	      asprintf(&bname, "%s.bak", argv[i]);
	      if (bname != NULL) {
#ifdef	_MSC_VER
		unlink(bname);
#endif	/* _MSC_VER */
		if (rename(argv[i], bname) == 0 &&
		    rename(tname, argv[i]) == 0) {
		  /* save file time-stamp */
		  if (opt.savetstamp && saveTimeStamp(bname, argv[i]) == -1) {
		    fprintf(stderr,
			    "Warning: %s couldn't set time-stamp on file %s\n",
			    Program, argv[i]);
		  }
		  /* save file permission */
		  if (savePermission(bname, argv[i]) == -1) {
		    fprintf(stderr,
			    "Warning: %s couldn't set permission on file %s\n",
			    Program, argv[i]);
		  }
		}
		free(bname);
	      }
	    }
	    else {
	      unlink(tname);	/* remove the temporary file */
	    }
	  }
	  free(tname);
	}
	else {
	  fprintf(stderr, "failed to open backup file for %s\n", argv[i]);
	}
      }
      else {
	if (convertKanjiCodeInFile(kin, fin, kout, stdout, &opt) == CONV_ERR) {
	  fprintf(stderr, "%s: no converted ...\n", argv[i]);
	}
      }
      signal(SIGINT,  SIG_DFL);
    }
  }
  else if (argc == 0) {	/* for standard input */
    if (opt.checkcode) {	/* only check kanji code */
      if (opt.showfname) {
	fprintf(stdout, "stdin\t");
	fflush(stdout);
      }
      wkfPrintGuessedKanjiCodeOfFile(stdin, false);
      fflush(stdout);
    }
    else {
      convertKanjiCodeInFile(kin, stdin, kout, stdout, &opt);
    }
  }
  else {	/* show usage */
    usage();
  }

  return 0;
}
