#ifndef _CONFIG_H_
#define _CONFIG_H_

/* UTF8 ���o�� */
#define UTF8_INPUT_ENABLE
#define UTF8_OUTPUT_ENABLE

/* Shift_JIS �͈͊O�̕������ACP932 �œ��l�ȕ����ɓǂ݊����� */
#define SHIFTJIS_CP932

/* �I�v�V�����œ��͂��w�肵�����ɁA�����R�[�h���Œ肷�� */
#define INPUT_CODE_FIX

/* --overwrite �I�v�V���� */
/* by Satoru Takabayashi <ccsatoru@vega.aichi-u.ac.jp> */
#define OVERWRITE

/* --cap-input, --url-input �I�v�V���� */
#define INPUT_OPTION

/* --numchar-input �I�v�V���� */
#define NUMCHAR_OPTION

/* --debug, --no-output �I�v�V���� */
#define CHECK_OPTION

/* JIS X0212 */
#define X0212_ENABLE

/* --exec-in, --exec-out �I�v�V����
 * pipe, fork, execvp �����肪�����Ɠ����܂���B
 * MS-DOS, MinGW �Ȃǂł� undef �ɂ��Ă�������
 * child process �I�����̏���������������Ȃ̂ŁA
 * �f�t�H���g�Ŗ����ɂ��Ă��܂��B
 */
/* #define EXEC_IO */

/* SunOS �� cc ���g���Ƃ��� undef �ɂ��Ă������� */
#define ANSI_C_PROTOTYPE

/* int �� 32bit �����̊��� NUMCHAR_OPTION ���g���ɂ́A
 * �R�����g���O���Ă��������B
 */
/* #define INT_IS_SHORT */


#if defined(INT_IS_SHORT)
typedef long nkf_char;
typedef unsigned char nkf_nfchar;
#else
typedef int nkf_char;
typedef int nkf_nfchar;
#endif

/* Unicode Normalization */
#define UNICODE_NORMALIZATION

#ifndef WIN32DLL
/******************************/
/* �f�t�H���g�̏o�̓R�[�h�I�� */
/* Select DEFAULT_CODE */
#define DEFAULT_CODE_JIS
/* #define DEFAULT_CODE_SJIS */
/* #define DEFAULT_CODE_EUC */
/* #define DEFAULT_CODE_UTF8 */
/******************************/
#else
#define DEFAULT_CODE_SJIS
#endif

#if defined(NUMCHAR_OPTION) && !defined(UTF8_INPUT_ENABLE)
#define UTF8_INPUT_ENABLE
#endif

#ifdef UNICODE_NORMALIZATION
#ifndef UTF8_INPUT_ENABLE
#define UTF8_INPUT_ENABLE
#endif
#define NORMALIZATION_TABLE_LENGTH 942
#define NORMALIZATION_TABLE_NFC_LENGTH 3
#define NORMALIZATION_TABLE_NFD_LENGTH 9
struct normalization_pair{
    const nkf_nfchar nfc[NORMALIZATION_TABLE_NFC_LENGTH];
    const nkf_nfchar nfd[NORMALIZATION_TABLE_NFD_LENGTH];
};
#endif

#endif /* _CONFIG_H_ */
