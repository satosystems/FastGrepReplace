/***********************************************************************
** Copyright (C) 1996,1998
** Copyright (C) 2002
** $BO"Mm@h!'(B $BN05eBg3X>pJs9)3X2J(B $B2OLn(B $B??<#(B  mime/X0208 support
** $B!J(BE-Mail Address: kono@ie.u-ryukyu.ac.jp$B!K(B
** $BO"Mm@h!'(B COW for DOS & Win16 & Win32 & OS/2
** $B!J(BE-Mail Address: GHG00637@niftyserve.or.p$B!K(B
**    
**    $B$3$N%=!<%9$N$$$+$J$kJ#<L!$2~JQ!$=$@5$b5vBz$7$^$9!#$?$@$7!"(B
**    $B$=$N:]$K$O!"C/$,9W8%$7$?$r<($9$3$NItJ,$r;D$9$3$H!#(B
**    $B:FG[I[$d;(;o$NIUO?$J$I$NLd$$9g$o$;$bI,MW$"$j$^$;$s!#(B
**    $B1DMxMxMQ$b>e5-$KH?$7$J$$HO0O$G5v2D$7$^$9!#(B
**    $B%P%$%J%j$NG[I[$N:]$K$O(Bversion message$B$rJ]B8$9$k$3$H$r>r7o$H$7$^$9!#(B
**    $B$3$N%W%m%0%i%`$K$D$$$F$OFC$K2?$NJ]>Z$b$7$J$$!"0-$7$+$i$:!#(B
**    
**    Everyone is permitted to do anything on this program
**    including copying, modifying, improving, 
**    as long as you don't try to pretend that you wrote it.
**    i.e., the above copyright notice has to appear in all copies.  
**    Binar y distribution requires original version messages.
**    You don't have to ask before copying, redistribution or publishing.
**    THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE.
***********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif
#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"
#ifdef __cplusplus
}
#endif

/* Replace nkf's getchar/putchar for variable modification */
/* we never use getc, ungetc */

#undef getc
#undef ungetc
#define getc(f)   	(input_ctr>=i_len?-1:input[input_ctr++])
#define ungetc(c,f)	input_ctr--

#define INCSIZE		32
#undef putchar
#undef TRUE
#undef FALSE
#define putchar(c)	nkf_putchar(c)

/* Input/Output pointers */

static unsigned char *output;
static unsigned char *input;
static STRLEN input_ctr;
static STRLEN i_len;
static STRLEN output_ctr;
static STRLEN o_len;
static STRLEN incsize;

static
SV *result;

/* put one char in the result string variable */

static int nkf_putchar_grow(unsigned int c) ;

/* inline ... no use */
static
int
nkf_putchar(unsigned int c) 
{
    /* string length is enough? */
    if(output_ctr<o_len) {
	return output[output_ctr++] = c;
    } else {
	return nkf_putchar_grow(c) ;
    }
}

static
int
nkf_putchar_grow(unsigned int c) 
{
    /* extends string length */
    o_len += incsize;
    SvGROW(result, o_len);
    /* to avoid linear growing, increase extension size */
    incsize *= 2;
    output = SvPVX(result);
    /* SvPV(result,o_len) breaks o_len */
    return output[output_ctr++] = c;
}

/* Include kanji filter main part */
/* getchar and putchar will be replaced during inclusion */

#define PERL_XS 1
#include "../utf8tbl.c"
#include "../nkf.c"

/* package defenition  */

/* nkf accepts variable length arugments. The last argument is */
/* the input data. Other strings are flags for nkf translation.    */

MODULE = NKF		PACKAGE = NKF		

SV *
nkf(...)
    PROTOTYPE: @
    PREINIT:
    SV* sv;
    SV* last;
    char **argv;
    char *cp;
    char *data;
    STRLEN cplen,rlen;
    int i,argc;
    CODE:

    /* Flags are reset at each call. */
    reinit();

    argc = items - 1;

    /* Process flags except the last once */
    for (i=0;i<argc;i++) {
        sv = ST(i);
        cp = SvPV(sv,cplen);
        if(*cp != '-') continue;
	options(cp);
    }

    /* Get input data pointer from the last variable. */
    data = SvPV(ST(argc),i_len);
    input_ctr = 0;

    if(x0201_f == WISH_TRUE)
         x0201_f = ((!iso2022jp_f)? TRUE : NO_X0201);

    /* allocate the result buffer */

    /* During conversion, stirngs length may grow. This is the unit */
    /* of growth */
    incsize = INCSIZE; 
    rlen = i_len+INCSIZE;
    result = newSV(rlen);
    input  = data;

    /* SvPV(result,o_len) does not work here. */
    output = SvPVX(result);
    o_len = rlen;
    output_ctr = 0;

    /* Convestion */
    kanji_convert(NULL);
    nkf_putchar(0);     /* Null terminator */

    RETVAL = result;
    SvPOK_on(RETVAL);       
    /* We cannot use 
	   SvCUR_set(RETVAL, strlen(output)); 
       because output can contain \0. 
     */
    SvCUR_set(RETVAL, output_ctr - 1);

    OUTPUT:
    RETVAL

SV *
nkf_continue(...)
    PROTOTYPE: @
    PREINIT:
    char *data;
    STRLEN rlen;
    CODE:

    /* Get input data pointer from the last variable. */
    data = SvPV(ST(0),i_len);
    input_ctr = 0;

    if(x0201_f == WISH_TRUE)
         x0201_f = ((!iso2022jp_f)? TRUE : NO_X0201);

    /* allocate the result buffer */

    /* During conversion, stirngs length may grow. This is the unit */
    /* of growth */
    incsize = INCSIZE; 
    rlen = i_len+INCSIZE;
    result = newSV(rlen);
    input  = data;

    /* SvPV(result,o_len) does not work here. */
    output = SvPVX(result);
    o_len = rlen;
    output_ctr = 0;

    /* Convestion */
    kanji_convert(NULL);
    nkf_putchar(0);     /* Null terminator */

    RETVAL = result;
    SvPOK_on(RETVAL);       
    /* We cannot use 
	   SvCUR_set(RETVAL, strlen(output)); 
       because output can contain \0. 
     */
    SvCUR_set(RETVAL, output_ctr - 1);

    OUTPUT:
    RETVAL

SV*
inputcode(...)
    CODE:
    RETVAL = newSV(strlen(input_codename) + 1);
    sv_setpv(RETVAL, input_codename);
    OUTPUT:
    RETVAL
