/*****************************************************************************
 *
 * FILE:	functions.h
 * DESCRIPTION:	WKF: functions declaration file
 * DATE:	Sat, Jan 15 2000
 * UPDATE:	Tue, Apr  5 2005
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
 * $Id: functions.h,v 1.6 2005/04/05 08:26:56 kouichi Exp $
 *
 *****************************************************************************/

#ifndef	_FUNCTIONS_H
#define	_FUNCTIONS_H	1

/* guess.c */
extern int	readFile(FILE *);
extern int	readString(String);
extern void	noPreRead(FILE *);
extern int	getChar(void);
extern void	ungetChar();
extern ktype_t	guessKanjiCodeType(bool, kcode_t, FILE *);

/* conv.c */
extern void	setOutputFilePointer(FILE *);
extern void	setOutputStringBuffer(String, unsigned int);
extern conv_t	convertKanjiCode(kcode_t, kcode_t);

#endif	/* _FUNCTIONS_H */
