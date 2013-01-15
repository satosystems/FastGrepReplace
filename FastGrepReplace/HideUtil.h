#pragma once

#include <windows.h>

#ifdef __cpluspluc
extern "C" {
#endif

/**
 * INI ファイルを保存するディレクトリ（このモジュールが配置されているディレクトリ）を取得する。
 *
 * この関数は成功するとディレクトリをポインタで返す。
 * 呼び出し元は戻り値が NULL ではないなら、ポインタを free を用いて適切に開放しなければならない。
 *
 * @return ディレクトリパス。検知不可能な場合は NULL
 */
char *HideUtil_GetMacroDir();

/**
 * 秀丸のインストールディレクトリを取得する。
 *
 * この関数は成功するとディレクトリをポインタで返す。
 * 呼び出し元は戻り値が NULL ではないなら、ポインタを free を用いて適切に開放しなければならない。
 *
 * @return ディレクトリパス。検知不可能な場合は NULL
 */
char *HideUtil_GetHidemaruDir();

DWORD *HideUtil_GetClipHistFlag();

LONG HideUtil_SetClipHistFlag(DWORD flag);

#ifdef __cpluscplus
}
#endif
