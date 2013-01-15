/* nkf32.dll nfk32dll.c */
/* e-mail:tkaneto@nifty.com */
/* URL: http://www1.ttcn.ne.jp/~kaneto */

/*WIN32DLL*/
/* $B$3$A$i$N%P!<%8%g%s$b99?7$7$F$/$@$5$$!#(B */
#define NKF_VERSIONW L"2.0.8"
/* NKF_VERSION $B$N%o%$%IJ8;z(B */
#define DLL_VERSION   "2.0.8.0 1"
/* DLL$B$,JV$9(B */
#define DLL_VERSIONW L"2.0.8.0 1"
/* DLL$B$,JV$9(B DLL_VERSION $B$N%o%$%IJ8;z(B */

/* nkf32.dll main */
#include <windows.h>
#include <tchar.h>
#include <stdarg.h>

#ifdef DLLDBG /* DLLDBG @@*/
#include "nkf.h"

void dumpn(unsigned char *buff,unsigned n)
{
    int i;

    i = 0;
    while ( n ) {
        if ( i == 0 ) {
            printf(":%x  ",buff);
        }
        printf("%02x ",*buff++);
        i++;
        if ( i == 16 ) {
            printf("\n");
            i = 0;
        }
        n--;
    }
    printf("\n");
}

void dumpf(char *f);
void mkfile(char *f,char *p);
#endif /* DLLDBG @@*/

#ifndef GUESS
#define GUESS 64
#endif /*GUESS*/

char *guessbuffA = NULL;
#ifdef UNICODESUPPORT
wchar_t *guessbuffW = NULL;
UINT guessCodePage = CP_OEMCP;
DWORD guessdwFlags = MB_PRECOMPOSED;

wchar_t *tounicode(char *p)
{
static wchar_t buff[GUESS];
    int sts;

    sts = MultiByteToWideChar(guessCodePage,guessdwFlags,p,-1,buff,sizeof(buff) / sizeof(wchar_t));
    if ( sts ) {
        return buff;
    } else {
        return L"(NULL)";
    }
}
#endif /*UNICODESUPPORT*/

char *ubuff;
int ulen;
int uret;

int dllprintf(FILE *fp,char *fmt,...)
{
    va_list argp;
    int sts;

    if ( uret != FALSE && ulen >= 1 && fmt != NULL && *fmt != 0 ) {
        va_start(argp, fmt);
        sts = _vsnprintf(ubuff,ulen - 1,fmt,argp);
        va_end(argp);
        if ( sts >= 0 ) {
            ubuff += sts;
            ulen -= sts;
        } else {
            uret = FALSE;
        }
        return sts;
    } else return 0;
}

/** Network Kanji Filter. (PDS Version)
************************************************************************
** Copyright (C) 1987, Fujitsu LTD. (Itaru ICHIKAWA)
** $BO"Mm@h!'(B $B!J3t!KIY;NDL8&5f=j!!%=%U%H#38&!!;T@n!!;j(B 
** $B!J(BE-Mail Address: ichikawa@flab.fujitsu.co.jp$B!K(B
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
**    Binary distribution requires original version messages.
**    You don't have to ask before copying, redistribution or publishing.
**    THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE.
***********************************************************************/

static const unsigned char *cin = NULL;
static int nin = -1;
static int ninmax = -1;
static int std_getc_mode = 1;

int 
std_getc(f)
FILE *f;
{
    if (std_gc_ndx){
        return std_gc_buf[--std_gc_ndx];
    } else {
        if ( std_getc_mode == 1 ) {
            return getc(f);
        }
        if ( std_getc_mode == 2 && cin != NULL ) {
            if ( ninmax >= 0 ) {
                if ( nin >= ninmax ) {
                    return EOF;
                } else {
                    nin++;
                    return *cin++;
                }
            } else {
                if ( *cin ) {
                    return *cin++;
                } else {
                    return EOF;
                }
            }
        }
    }
    return EOF;
}

static FILE *fout = NULL;
static unsigned char *cout = NULL;
static int nout = -1;
static int noutmax = -1;
static int std_putc_mode = 1;

void 
std_putc(c)
int c;
{
    if(c!=EOF)
    {
        if ( (std_putc_mode & 1) && fout != NULL ) {
            putc(c,fout);
        }
        if ( (std_putc_mode & 4) && nout != -1 ) {
            if ( noutmax >= 0 && nout >= noutmax ) std_putc_mode &= ~2;
            nout++;
        }
        if ( (std_putc_mode & 2) && cout != NULL ) {
            *cout++ = c;
        }
    }
}

void
print_guessed_code (filename)
    char *filename;
{
    char *codename = "BINARY";
    if (!is_inputcode_mixed) {
        if (strcmp(input_codename, "") == 0) {
            codename = "ASCII";
        } else {
            codename = input_codename;
        }
    }
    if (filename != NULL) {
        guessbuffA = realloc(guessbuffA,(strlen(filename) + GUESS + 1) * sizeof (char) );
        sprintf(guessbuffA,"%s:%s", filename,codename);
    } else {
        guessbuffA = realloc(guessbuffA,(GUESS + 1) * sizeof (char) );
        sprintf(guessbuffA,"%s", codename);
    }
}

#ifdef UNICODESUPPORT
void
print_guessed_codeW (filename)
    wchar_t *filename;
{
    char *codename = "BINARY";
    if (!is_inputcode_mixed) {
        if (strcmp(input_codename, "") == 0) {
            codename = "ASCII";
        } else {
            codename = input_codename;
        }
    }
    if (filename != NULL) {
        guessbuffW = realloc(guessbuffW,(wcslen(filename) + GUESS + 1) * sizeof (wchar_t) );
        swprintf(guessbuffW,L"%s:%s",filename,tounicode(codename));
    } else {
        guessbuffW = realloc(guessbuffW,(GUESS + 1) * sizeof (wchar_t));
        swprintf(guessbuffW,L"%s",tounicode(codename));
    }
}
#endif /*UNICODESUPPORT*/

/**
 ** $B%Q%C%A@):n<T(B
 **  void@merope.pleiades.or.jp (Kusakabe Youichi)
 **  NIDE Naoyuki <nide@ics.nara-wu.ac.jp>
 **  ohta@src.ricoh.co.jp (Junn Ohta)
 **  inouet@strl.nhk.or.jp (Tomoyuki Inoue)
 **  kiri@pulser.win.or.jp (Tetsuaki Kiriyama)
 **  Kimihiko Sato <sato@sail.t.u-tokyo.ac.jp>
 **  a_kuroe@kuroe.aoba.yokohama.jp (Akihiko Kuroe)
 **  kono@ie.u-ryukyu.ac.jp (Shinji Kono)
 **  GHG00637@nifty-serve.or.jp (COW)
 **
 **/

void 
reinitdll()
{
    cin = NULL;
    nin = -1;
    ninmax = -1;
    std_getc_mode = 1;
    fout = stdout;
    cout = NULL;
    nout = -1;
    noutmax = -1;
    std_putc_mode = 1;
    if ( guessbuffA ) {
        free(guessbuffA);
        guessbuffA = NULL;
    }
#ifdef UNICODESUPPORT
    if ( guessbuffW ) {
        free(guessbuffW);
        guessbuffW = NULL;
    }
#endif /*UNICODESUPPORT*/
}

#ifndef DLLDBG /* DLLDBG @@*/
int WINAPI DllEntryPoint(HINSTANCE hinst,unsigned long reason,void* lpReserved)
{
        return 1;
}
#endif /* DLLDBG @@*/

static LPSTR nkfverA = NKF_VERSION;
static LPSTR dllverA = DLL_VERSION;
#ifdef UNICODESUPPORT
static LPWSTR nkfverW = NKF_VERSIONW;
static LPWSTR dllverW = DLL_VERSIONW;
#endif /*UNICODESUPPORT*/

BOOL scp(LPSTR s,LPSTR t,DWORD n)
{
    while ( n ) {
        if ( (*s = *t) == 0 ) return TRUE;
        if ( --n == 0 ) {
            *s = 0;
            break;
        }
        s++;
        t++;
    }
    return FALSE;
}

#ifdef UNICODESUPPORT
BOOL wscp(LPWSTR s,LPWSTR t,DWORD n)
{
    while ( n ) {
        if ( (*s = *t) == 0 ) return TRUE;
        if ( --n == 0 ) {
            *s = 0;
            break;
        }
        s++;
        t++;
    }
    return FALSE;
}
#endif /*UNICODESUPPORT*/

void CALLBACK GetNkfVersion(LPSTR verStr){
    strcpy(verStr,dllverA);
}

BOOL WINAPI GetNkfVersionSafeA(LPSTR verStr,DWORD nBufferLength /*in TCHARs*/,LPDWORD lpTCHARsReturned /*in TCHARs*/)
{
    *lpTCHARsReturned = strlen(dllverA) + 1;
    if ( verStr == NULL || nBufferLength == 0 ) return FALSE;
    return scp(verStr,dllverA,nBufferLength);
}

BOOL WINAPI GetNkfVersionSafeW(LPWSTR verStr,DWORD nBufferLength /*in TCHARs*/,LPDWORD lpTCHARsReturned /*in TCHARs*/)
{
#ifdef UNICODESUPPORT
    *lpTCHARsReturned = wcslen(dllverW) + 1;
    if ( verStr == NULL || nBufferLength == 0 ) return FALSE;
    wcsncpy(verStr,dllverW,nBufferLength);
    if ( wcslen(dllverW) >= nBufferLength )  {
        *(verStr + nBufferLength - 1) = 0;
        return FALSE;
    }
    return TRUE;
#else /*UNICODESUPPORT*/
    return FALSE;
#endif /*UNICODESUPPORT*/
}

int CALLBACK SetNkfOption(LPCSTR optStr)
{
    LPSTR p;

    if ( *optStr == '-' ) {
        reinit();
        options(optStr);
    } else {
        p = malloc(strlen(optStr) + 2);
        if ( p == NULL ) return -1;
        *p = '-';
        strcpy(p + 1,optStr);
        reinit();
        options(p);
        free(p);
    }
    return 0;
}

void CALLBACK NkfConvert(LPSTR outStr, LPCSTR inStr)
{
    std_putc_mode = 2;
    cout = outStr;
    noutmax = -1;
    nout = -1;
    std_getc_mode = 2;
    cin = inStr;
    ninmax = -1;
    nin = -1;
    kanji_convert(NULL);
    *cout = 0;
}

BOOL WINAPI NkfConvertSafe(LPSTR outStr,DWORD nOutBufferLength /*in Bytes*/,LPDWORD lpBytesReturned /*in Bytes*/, LPCSTR inStr,DWORD nInBufferLength /*in Bytes*/){
    if ( inStr == NULL ) return FALSE;
    std_putc_mode = 6;
    cout = outStr;
    noutmax = nOutBufferLength;
    nout = 0;
    std_getc_mode = 2;
    cin = inStr;
    ninmax = nInBufferLength;
    nin = 0;
    kanji_convert(NULL);
    *lpBytesReturned = nout;
    if ( nout < noutmax ) *cout = 0;
    return TRUE;
}

void CALLBACK ToHankaku(LPSTR inStr)
{
    unsigned char *p;
    int len;

    len = strlen(inStr) + 1;
    p = malloc(len);
    if ( p == NULL ) return;
    memcpy(p,inStr,len);
    reinit();
    options("-ZSs");
    NkfConvert(inStr,p);
    free(p);
}

BOOL WINAPI ToHankakuSafe(LPSTR outStr,DWORD nOutBufferLength /*in Bytes*/,LPDWORD lpBytesReturned /*in Bytes*/,LPCSTR inStr,DWORD nInBufferLength /*in Bytes*/)
{
    reinit();
    options("-ZSs");
    return NkfConvertSafe(outStr,nOutBufferLength,lpBytesReturned,inStr,nInBufferLength);
}

void CALLBACK ToZenkakuKana(LPSTR outStr, LPCSTR inStr)
{
    reinit();
    options("-Ss");
    NkfConvert(outStr, inStr);
}

BOOL WINAPI ToZenkakuKanaSafe(LPSTR outStr,DWORD nOutBufferLength /*in Bytes*/,LPDWORD lpBytesReturned /*in Bytes*/,LPCSTR inStr,DWORD nInBufferLength /*in Bytes*/)
{
    reinit();
    options("-Ss");
    return NkfConvertSafe(outStr,nOutBufferLength,lpBytesReturned,inStr,nInBufferLength);
}

void CALLBACK EncodeSubject(LPSTR outStr ,LPCSTR inStr){
    reinit();
    options("-jM");
    NkfConvert(outStr, inStr);
}

BOOL WINAPI EncodeSubjectSafe(LPSTR outStr,DWORD nOutBufferLength /*in Bytes*/,LPDWORD lpBytesReturned /*in Bytes*/,LPCSTR inStr,DWORD nInBufferLength /*in Bytes*/)
{
    reinit();
    options("-jM");
    return NkfConvertSafe(outStr,nOutBufferLength,lpBytesReturned,inStr,nInBufferLength);
}

#ifdef TOMIME
void CALLBACK ToMime(LPSTR outStr ,LPCSTR inStr)
{
    EncodeSubject(outStr,inStr);
}
#endif /*TOMIME*/

#ifdef GETKANJICODE
int CALLBACK NkfGetKanjiCode(VOID)
{
    int iCode=0;
    /* if(iconv == s_iconv)iCode=0; */ /* 0:$B%7%U%H(BJIS */
    if(iconv == w_iconv)iCode=3; /* UTF-8 */
    else if(iconv == w_iconv16){
        if(input_endian == ENDIAN_BIG)iCode=5; /* 5:UTF-16BE */
        else iCode=4; /* 4:UTF-16LE */
    }else if(iconv == e_iconv){
        if(estab_f == FALSE)iCode=2; /* 2:ISO-2022-JP */
        else iCode=1; /* 1:EUC */
    }
    return iCode;
}
#endif /*GETKANJICODE*/

#ifdef FILECONVERT1
void CALLBACK NkfFileConvert1(LPCSTR fName)
{
    FILE *fin;
    char *tempdname;
    char tempfname[MAX_PATH];
    char d[4];
    DWORD len;
    BOOL sts;

    len = GetTempPath(sizeof d,d);
    tempdname = malloc(len + 1);
    if ( tempdname == NULL ) return;
    len = GetTempPath(len + 1,tempdname);
    sts = GetTempFileName(tempdname,"NKF",0,tempfname);
    if ( sts != 0 )  {
        sts = CopyFileA(fName,tempfname,FALSE);
         if ( sts ) {
             if ((fin = fopen(tempfname, "rb")) != NULL) {
                 if ((fout = fopen(fName, "wb")) != NULL) {
                     cin = NULL;
                     nin = -1;
                     ninmax = -1;
                     std_getc_mode = 1;
                     cout = NULL;
                     nout = -1;
                     noutmax = -1;
                     std_putc_mode = 1;
                     kanji_convert(fin);
                     fclose(fin);
                 }
                 fclose(fout);
             }
        DeleteFile(tempfname);
        }
    }
    free(tempdname);
}
#endif /*FILECONVERT1*/

BOOL WINAPI NkfFileConvert1SafeA(LPCSTR fName,DWORD nBufferLength /*in TCHARs*/)
{
    FILE *fin;
    char *tempdname;
    char tempfname[MAX_PATH];
    char d[4];
    DWORD len;
    BOOL sts;
    BOOL ret;
    LPCSTR p;

    ret = FALSE;
    p = fName;
    for ( ;; ) {
        if ( nBufferLength == 0 ) return ret;
        if ( *p == 0 ) break;
        p++;
        --nBufferLength;
    }
    if ( chmod(fName,_S_IREAD | _S_IWRITE) == -1 ) return ret;
    len = GetTempPath(sizeof d,d);
    tempdname = malloc(len + 1);
    if ( tempdname == NULL ) return FALSE;
    len = GetTempPath(len + 1,tempdname);
    sts = GetTempFileName(tempdname,"NKF",0,tempfname);
    if ( sts != 0 )  {
        sts = CopyFileA(fName,tempfname,FALSE);
        if ( sts ) {
            if ((fin = fopen(tempfname, "rb")) != NULL) {
                if ((fout = fopen(fName, "wb")) != NULL) {
                    cin = NULL;
                    nin = -1;
                    ninmax = -1;
                    std_getc_mode = 1;
                    cout = NULL;
                    nout = -1;
                    noutmax = -1;
                    std_putc_mode = 1;
                    kanji_convert(fin);
                    fclose(fin);
                    ret = TRUE;
                }
                fclose(fout);
            }
            DeleteFileA(tempfname);
        }
    }
    free(tempdname);
    return ret;
}

BOOL WINAPI NkfFileConvert1SafeW(LPCWSTR fName,DWORD nBufferLength /*in TCHARs*/)
{
#ifdef UNICODESUPPORT
    FILE *fin;
    wchar_t *tempdname;
    wchar_t tempfname[MAX_PATH];
    wchar_t d[2];
    DWORD len;
    BOOL sts;
    BOOL ret;
    LPCWSTR p;

    ret = FALSE;
    p = fName;
    for ( ;; ) {
        if ( nBufferLength == 0 ) return ret;
        if ( *p == 0 ) break;
        p++;
        --nBufferLength;
    }
    if ( _wchmod(fName,_S_IREAD | _S_IWRITE) == -1 ) return ret;
    len = GetTempPathW(sizeof d / sizeof(WCHAR),d);
    tempdname = malloc((len + 1) * sizeof(WCHAR));
    if ( tempdname == NULL ) return FALSE;
    len = GetTempPathW(len + 1,tempdname);
    sts = GetTempFileNameW(tempdname,L"NKF",0,tempfname);
    if ( sts != 0 )  {
        sts = CopyFileW(fName,tempfname,FALSE);
        if ( sts ) {
            if ((fin = _wfopen(tempfname,L"rb")) != NULL) {
                if ((fout = _wfopen(fName,L"wb")) != NULL) {
                    cin = NULL;
                    nin = -1;
                    ninmax = -1;
                    std_getc_mode = 1;
                    cout = NULL;
                    nout = -1;
                    noutmax = -1;
                    std_putc_mode = 1;
                    kanji_convert(fin);
                    fclose(fin);
                    ret = TRUE;
                }
                fclose(fout);
            }
            DeleteFileW(tempfname);
        }
    }
    free(tempdname);
    return ret;
#else /*UNICODESUPPORT*/
    return FALSE;
#endif /*UNICODESUPPORT*/
}

#ifdef FILECONVERT2
void CALLBACK NkfFileConvert2(LPCSTR fInName,LPCSTR fOutName)
{
    FILE *fin;

    if ((fin = fopen(fInName, "rb")) == NULL) return;
    if((fout=fopen(fOutName, "wb")) == NULL) {
        fclose(fin);
        return;
    }
    cin = NULL;
    nin = -1;
    ninmax = -1;
    std_getc_mode = 1;
    cout = NULL;
    nout = -1;
    noutmax = -1;
    std_putc_mode = 1;
    kanji_convert(fin);
    fclose(fin);
    fclose(fout);
}
#endif /*FILECONVERT2*/

BOOL WINAPI NkfFileConvert2SafeA(LPCSTR fInName,DWORD fInBufferLength /*in TCHARs*/,LPCSTR fOutName,DWORD fOutBufferLength /*in TCHARs*/)
{
    FILE *fin;
    BOOL sts;
    BOOL ret;
    LPCSTR p;

    ret = FALSE;
    p = fInName;
    for ( ;; ) {
        if ( fInBufferLength == 0 ) return ret;
        if ( *p == 0 ) break;
        p++;
        --fInBufferLength;
    }
    p = fOutName;
    for ( ;; ) {
        if ( fOutBufferLength == 0 ) return ret;
        if ( *p == 0 ) break;
        p++;
        --fOutBufferLength;
    }
    if ((fin = fopen(fInName, "rb")) != NULL) {
        if((fout=fopen(fOutName, "wb")) != NULL) {
            cin = NULL;
            nin = -1;
            ninmax = -1;
            std_getc_mode = 1;
            cout = NULL;
            nout = -1;
            noutmax = -1;
            std_putc_mode = 1;
            kanji_convert(fin);
            fclose(fin);
            ret = TRUE;
        }
        fclose(fout);
    }
    return ret;
}

BOOL WINAPI NkfFileConvert2SafeW(LPCWSTR fInName,DWORD fInBufferLength /*in TCHARs*/,LPCWSTR fOutName,DWORD fOutBufferLength /*in TCHARs*/)
{
#ifdef UNICODESUPPORT
    FILE *fin;
    BOOL sts;
    BOOL ret;
    LPCWSTR p;

    ret = FALSE;
    p = fInName;
    for ( ;; ) {
        if ( fInBufferLength == 0 ) return ret;
        if ( *p == 0 ) break;
        p++;
        --fInBufferLength;
    }
    p = fOutName;
    for ( ;; ) {
        if ( fOutBufferLength == 0 ) return ret;
        if ( *p == 0 ) break;
        p++;
        --fOutBufferLength;
    }
    if ( (fin = _wfopen(fInName,L"rb")) != NULL) {
        if( (fout = _wfopen(fOutName,L"wb")) != NULL) {
            cin = NULL;
            nin = -1;
            ninmax = -1;
            std_getc_mode = 1;
            cout = NULL;
            nout = -1;
            noutmax = -1;
            std_putc_mode = 1;
            kanji_convert(fin);
            fclose(fin);
            ret = TRUE;
        }
        fclose(fout);
    }
    return ret;
#else /*UNICODESUPPORT*/
    return FALSE;
#endif /*UNICODESUPPORT*/
}

BOOL WINAPI GetNkfGuessA(LPSTR outStr,DWORD nBufferLength /*in TCHARs*/,LPDWORD lpTCHARsReturned /*in TCHARs*/)
{
    if ( outStr == NULL || nBufferLength == 0 ) return FALSE;
    print_guessed_code(NULL);
    *lpTCHARsReturned = strlen(guessbuffA) + 1;
    return scp(outStr,guessbuffA,nBufferLength);
}

BOOL WINAPI GetNkfGuessW(LPWSTR outStr,DWORD nBufferLength /*in TCHARs*/,LPDWORD lpTCHARsReturned /*in TCHARs*/)
{
#ifdef UNICODESUPPORT
    if ( outStr == NULL || nBufferLength == 0 ) return FALSE;
    print_guessed_codeW(NULL);
    *lpTCHARsReturned = wcslen(guessbuffW) + 1;
    return wscp(outStr,guessbuffW,nBufferLength);
#else /*UNICODESUPPORT*/
    return FALSE;
#endif /*UNICODESUPPORT*/
}

static struct {
DWORD size;
LPCSTR copyrightA;
LPCSTR versionA;
LPCSTR dateA;
DWORD functions;
} NkfSupportFunctions = {
sizeof(NkfSupportFunctions),
NULL,
NKF_VERSION,
NKF_RELEASE_DATE,
1 /* nkf32103a.lzh uminchu 1.03 */
/* | 2 */ /* nkf32dll.zip 0.91 */
#if defined(TOMIME) && defined(GETKANJICODE) && defined(FILECONVERT1) && defined(FILECONVERT2) 
| 4 /* nkf32204.zip Kaneto 2.0.4.0 */
#endif
| 8 /* this */
#ifdef UNICODESUPPORT
| 0x80000000
#endif /*UNICODESUPPORT*/
,
};

BOOL WINAPI GetNkfSupportFunctions(void *outStr,DWORD nBufferLength /*in Bytes*/,LPDWORD lpBytesReturned /*in Bytes*/)
{
    *lpBytesReturned = sizeof NkfSupportFunctions;
    if ( outStr == NULL || nBufferLength == 0 ) return FALSE;
    NkfSupportFunctions.copyrightA = CopyRight;
    memcpy(outStr,&NkfSupportFunctions,sizeof NkfSupportFunctions > nBufferLength ? nBufferLength : sizeof NkfSupportFunctions);
    return TRUE;
}

BOOL WINAPI NkfUsage(LPSTR outStr,DWORD nBufferLength /*in Bytes*/,LPDWORD lpBytesReturned /*in Bytes*/)
{
    ubuff = outStr;
    ulen = nBufferLength;
    uret = TRUE;
    usage();
    if ( uret == TRUE ) {
        *lpBytesReturned = nBufferLength - ulen;
    }
    return uret;
}

/* nkf32.dll main end */

#ifdef DLLDBG /* DLLDBG @@*/
/* dbg.exe */
unsigned char buff[65536];
unsigned char buff2[65536];
unsigned char buff3[65536];
unsigned char buff4[65536];
char *code[] = {"$B%7%U%H(BJIS","EUC","ISO-2022-JP","UTF-8","UTF-16LE","UTF-16BE"};

    int n;
    BOOL sts;
    DWORD len;

void mimeencode(unsigned char *buff2)
{
    memset(buff,0,sizeof buff);
    EncodeSubject(buff,buff2);
    printf("EncodeSubject(%s)=%s\n",buff2,buff);
        memset(buff,0,sizeof buff);
        ToMime(buff,buff2);
        printf("ToMime(%s)=%s\n",buff2,buff);
        memset(buff,0,sizeof buff);
        sts = EncodeSubjectSafe(buff,sizeof buff,&len,buff2,strlen(buff2));
        printf("EncodeSubjectSafe(%s)=%d len=%d '%s'\n",buff,sts,len,buff);
        dumpn(buff2,strlen(buff2));
        dumpn(buff,len);
}

void convert(char *arg,unsigned char *buff2)
{
    sts = SetNkfOption(arg);
    printf("SetNkfOption(%s)=%d\n",arg,sts);
    memset(buff,0,sizeof buff);
    NkfConvert(buff,buff2);
    printf("NkfConvert(%s)=%s\n",buff2,buff);
    n = NkfGetKanjiCode();
    printf("NkfGetKanjiCode()=%d\n",n);
        sts = SetNkfOption(arg);
        printf("SetNkfOption(%s)=%d\n",arg,sts);
        memset(buff,0,sizeof buff);
        sts = NkfConvertSafe(buff,sizeof buff,&len,buff2,strlen(buff2));
        printf("NkfConvertSafe(%s)=%d len=%d '%s'\n",buff2,sts,len,buff);
        dumpn(buff2,strlen(buff2));
        dumpn(buff,len);
        n = NkfGetKanjiCode();
        printf("NkfGetKanjiCode()=%d\n",n);
}

void guess(unsigned char *buff2)
{
    char *g = "--guess";

        sts = SetNkfOption(g);
        printf("SetNkfOption(%s)=%d\n",g,sts);
        memset(buff,0,sizeof buff);
        NkfConvert(buff,buff2);
        printf("NkfConvert(%s)=%s\n",buff2,buff);
        dumpn(buff2,strlen(buff2));
        n = NkfGetKanjiCode();
        printf("NkfGetKanjiCode()=%d %s\n",n,code[n]);
        memset(buff,0,sizeof buff);
        sts = GetNkfGuessA(buff,sizeof buff,&len);
        printf("GetNkfGuessA()=%d len=%d '%s'\n",sts,len,buff);
        dumpn(buff,len);
        memset(buff,0,sizeof buff);
        sts = GetNkfGuessW((LPWSTR)buff,sizeof buff / sizeof(WCHAR),&len);
        printf("GetNkfGuessW()=%d len=%d\n",sts,len);
        dumpn(buff,len * sizeof(WCHAR));
}

void dumpf(char *f)
{
    FILE *fp;
    unsigned int n;

    fp = fopen(f,"rb");
    if ( fp == NULL ) return;
    n = fread(buff,1,sizeof buff,fp);
    fclose(fp);
    printf("dumpf(%s,%d)\n",f,n);
    dumpn(buff,n);
}

void mkfile(char *f,char *p)
{
    FILE *fp;

    fp = fopen(f,"w");
    if ( fp == NULL ) return;
    fputs(p,fp);
    fclose(fp);
    dumpf(f);
}

void file(char *arg2,char *arg3,unsigned char *buf)
{
            sts = SetNkfOption(arg2);
            printf("SetNkfOption(%s)=%d\n",arg2,sts);
            mkfile(arg3,buf);
            NkfFileConvert1(arg3);
            printf("NkfFileConvert1(%s)\n",arg3);
            dumpf(arg3);
            sts = SetNkfOption(arg2);
            printf("SetNkfOption(%s)=%d\n",arg2,sts);
            mkfile(arg3,buf);
            sts = NkfFileConvert1SafeA(arg3,strlen(arg3) + 1);
            printf("NkfFileConvert1SafeA(%s)=%d\n",arg3,sts);
            dumpf(arg3);
            sts = SetNkfOption(arg2);
            printf("SetNkfOption(%s)=%d\n",arg2,sts);
            mkfile(arg3,buf);
            sts = MultiByteToWideChar(CP_OEMCP,MB_PRECOMPOSED,arg3,-1,(wchar_t *)buff,sizeof(buff) / sizeof(wchar_t));
            printf("MultiByteToWideChar(%s)=%d\n",arg3,sts);
            dumpn(buff,(wcslen((wchar_t *)buff) + 1) * sizeof(wchar_t));
            sts = NkfFileConvert1SafeW((wchar_t *)buff,sizeof buff / sizeof(wchar_t) /*wcslen((wchar_t *)buff) + 1*/);
            printf("NkfFileConvert1SafeW()=%d\n",sts);
            dumpf(arg3);
}

void file2(char *arg2,char *arg3,char *arg4,unsigned char *buf)
{
            sts = SetNkfOption(arg2);
            printf("SetNkfOption(%s)=%d\n",arg2,sts);
            mkfile(arg3,buf);
            NkfFileConvert2(arg3,arg4);
            printf("NkfFileConvert1(%s,%s)\n",arg3,arg4);
            dumpf(arg3);
            dumpf(arg4);
            sts = SetNkfOption(arg2);
            printf("SetNkfOption(%s)=%d\n",arg2,sts);
            mkfile(arg3,buf);
            sts = NkfFileConvert2SafeA(arg3,strlen(arg3) + 1,arg4,strlen(arg4) + 1);
            printf("NkfFileConvert2SafeA(%s,%s)=%d\n",arg3,arg4,sts);
            dumpf(arg3);
            dumpf(arg4);
            sts = SetNkfOption(arg2);
            printf("SetNkfOption(%s)=%d\n",arg2,sts);
            mkfile(arg3,buf);
            sts = MultiByteToWideChar(CP_OEMCP,MB_PRECOMPOSED,arg3,-1,(wchar_t *)buff,sizeof(buff) / sizeof(wchar_t));
            printf("MultiByteToWideChar(%s)=%d\n",arg3,sts);
            dumpn(buff,(wcslen((wchar_t *)buff) + 1) * sizeof(wchar_t));
            sts = MultiByteToWideChar(CP_OEMCP,MB_PRECOMPOSED,arg4,-1,(wchar_t *)buff4,sizeof(buff4) / sizeof(wchar_t));
            printf("MultiByteToWideChar(%s)=%d\n",arg4,sts);
            dumpn(buff4,(wcslen((wchar_t *)buff4) + 1) * sizeof(wchar_t));
            sts = NkfFileConvert2SafeW((wchar_t *)buff,sizeof buff / sizeof(wchar_t) ,(wchar_t *)buff4,sizeof buff4 / sizeof(wchar_t));
            printf("NkfFileConvert2SafeW()=%d\n",sts);
            dumpf(arg3);
            dumpf(arg4);
}

int main(int argc,char **argv)
{
    struct NKFSUPPORTFUNCTIONS fnc;

    if ( argc < 2 ) return 0;
    switch ( *argv[1] ) {
      case 'v':
        memset(buff,0,sizeof buff);
        GetNkfVersion(buff);
        printf("GetNkfVersion() '%s'\n",buff);
            sts = GetNkfVersionSafeA(buff,sizeof buff,&len);
            printf("GetNkfVersionSafeA()=%d len=%d '%s'\n",sts,len,buff);
            sts = GetNkfVersionSafeW((LPWSTR)buff,sizeof buff / sizeof(WCHAR),&len);
            printf("GetNkfVersionSafeW()=%d len=%d\n",sts,len);
            dumpn(buff,len * sizeof(WCHAR));
            sts = GetNkfSupportFunctions(&fnc,sizeof fnc,&len);
            printf("GetNkfSupportFunctions()=%d len=%d\n",sts,len);
            printf("size=%d\n",fnc.size);
            printf("copyrightA='%s'\n",fnc.copyrightA);
            printf("versionA='%s'\n",fnc.versionA);
            printf("dateA='%s'\n",fnc.dateA);
            printf("functions=%d %x\n",fnc.functions,fnc.functions);
        break;
      case 'm':
        if ( argc < 3 ) return 0;
        mimeencode(argv[2]);
        break;
      case 'M':
        if ( argc < 2 ) return 0;
        gets(buff2);
        mimeencode(buff2);
        break;
      case 'c':
        if ( argc < 4 ) return 0;
        convert(argv[2],argv[3]);
        break;
      case 'C':
        if ( argc < 3 ) return 0;
        gets(buff2);
        convert(argv[2],buff2);
        break;
      case 'g':
        if ( argc < 3 ) return 0;
        guess(argv[2]);
        break;
      case 'G':
        if ( argc < 2 ) return 0;
        gets(buff2);
        guess(buff2);
        break;
      case 'f':
        if ( argc < 5 ) return 0;
        file(argv[2],argv[3],argv[4]);
        break;
      case 'F':
        if ( argc < 4 ) return 0;
        gets(buff3);
        file(argv[2],argv[3],buff3);
        break;
      case '2':
        if ( argc < 6 ) return 0;
        file2(argv[2],argv[3],argv[4],argv[5]);
        break;
      case '#':
        if ( argc < 5 ) return 0;
        gets(buff3);
        file2(argv[2],argv[3],argv[4],buff3);
        break;
      case 'u':
        sts = NkfUsage(buff,sizeof buff,&len);
        printf("strlen(buff)=%d\n",strlen(buff));
        printf("NkfUsage()=%d len=%d \n%s",sts,len,buff);
        break;
    }
    return 0;
}
/* dbg.exe end */
#endif /* DLLDBG @@*/
/*WIN32DLL*/
