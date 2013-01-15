#pragma once

typedef void *libiconv_t;

/**
 * 変換ディスクリプタを割り当てる。
 *
 * @param [in] tocode 変換後の文字コード
 * @param [in] fromcode 変換前の文字コード
 * @return 変換ディスクリプタ。エラーの場合は errno を設定し (libiconv_t)-1 を返す
 */
typedef libiconv_t (* LIBICONV_OPEN)(const char *tocode, const char *fromcode);

/**
 * LIBICONV_OPEN で生成した変換ディスクリプタを用いて文字コードの変換を行う。
 *
 * @param [in] cd 変換ディスクリプタ
 * @param [in,out] inbuf 入力バッファ
 * @param [in,out] inbytesleft 入力バッファのサイズ
 * @param [in,out] outbuf 出力バッファ
 * @param [in,out] outbytesleft 出力バッファのサイズ
 * @return 非可逆に変換した文字数。エラーが発生した場合は errno を設定し (size_t)-1
 */
typedef size_t (* LIBICONV)(libiconv_t cd, const char **inbuf, size_t *inbytesleft, char **outbuf, size_t *outbytesleft);

/**
 * 変換ディスクリプタを開放する。
 *
 * @param [in] cd 開放する変換ディスクリプタ
 * @return 成功した場合は 0、失敗した場合は errno を設定し -1
 */
typedef int (* LIBICONV_CLOSE)(libiconv_t cd);

extern HINSTANCE libiconv_dll;
extern LIBICONV_OPEN libiconv_open;
extern LIBICONV libiconv;
extern LIBICONV_CLOSE libiconv_close;

