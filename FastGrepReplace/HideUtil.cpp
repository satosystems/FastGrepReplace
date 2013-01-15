#define STRICT
#include "HideUtil.h"
#include "Registory.h"
#pragma intrinsic(memset, memcpy)

#define turukamedir "HKEY_CURRENT_USER\\Software\\Hidemaruo\\TuruKame\\Config\\TuruKameDir"
#define hideinstloc "HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Hidemaru\\InstallLocation"
#define macropath "HKEY_CURRENT_USER\\Software\\Hidemaruo\\Hidemaru\\Env\\MacroPath"
#define cliphistflag "HKEY_CURRENT_USER\\Software\\Hidemaruo\\Hidemaru\\Env\\ClipHistFlag"
#define resident "HKEY_CURRENT_USER\\Software\\Hidemaruo\\Hidemaru\\Env\\Resident"

/**
 * INI ファイルを保存するディレクトリ（このモジュールが配置されているディレクトリ）を取得する。
 *
 * この関数は成功するとディレクトリをポインタで返す。
 * 呼び出し元は戻り値が NULL ではないなら、ポインタを free を用いて適切に開放しなければならない。
 *
 * @return ディレクトリパス。検知不可能な場合は NULL
 */
char *HideUtil_GetMacroDir() {
	char *work;
	char *temp;
	DWORD type;
	
	if ((temp = (char *)Registory_GetValue(hideinstloc, &type)) == NULL) {
		// 秀丸がインストールされていないなら、秀丸メールディレクトリがホーム
		work = (char *)Registory_GetValue(turukamedir, &type);
	} else {
		// 秀丸がインストールされているならマクロパスを調べる
		if ((work = (char *)Registory_GetValue(macropath, &type)) == NULL) {
			// マクロパスが存在しなければ秀丸の InstallLocation がホーム
			work = (char *)malloc(strlen(temp) + 1);
			strcpy(work, temp);
		}
	}
	
	if (temp != NULL) {
		free(temp);
	}
	return work;
}

/**
 * 秀丸のインストールディレクトリを取得する。
 *
 * この関数は成功するとディレクトリをポインタで返す。
 * 呼び出し元は戻り値が NULL ではないなら、ポインタを free を用いて適切に開放しなければならない。
 *
 * @return ディレクトリパス。検知不可能な場合は NULL
 */
char *HideUtil_GetHidemaruDir() {
	DWORD type;
	
	return (char *)Registory_GetValue(hideinstloc, &type);
}


/**
 * 秀丸の常駐設定を取得する。
 *
 * 戻り値のポインタは呼び出し元が free を使って開放する必要がある。
 *
 * @retval NULL: レジストリへのアクセスが失敗
 * @retval ポインタの指す値が 0: 常駐なし
 * @retval ポインタの指す値が 1: 常駐あり
 */
DWORD *HideUtil_GetResident() {
	DWORD type;
	return (DWORD *)Registory_GetValue(resident, &type);
}

/**
 * 秀丸のクリップボード履歴の設定を取得する。
 *
 * @retval NULL: レジストリへのアクセスが失敗
 * @retval ポインタの指す値の最下位ビットが 0: クリップボード履歴を取らない
 * @retval ポインタの指す値の最下位ビットが 1: クリップボード履歴を取る
 */
DWORD *HideUtil_GetClipHistFlag() {
	DWORD type;
	return (DWORD *)Registory_GetValue(cliphistflag, &type);
}

/**
 * 秀丸のクリップボード履歴の設定を設定する。
 *
 * @param [in] クリップボード履歴の設定
 * @retval ERROR_SUCCESS: 設定に成功
 * @retval 上記以外: 設定に失敗
 */
LONG HideUtil_SetClipHistFlag(DWORD flag) {
	CONST BYTE *data = (CONST BYTE *)&flag;
	return Registory_SetValue(cliphistflag, REG_DWORD, data, sizeof(DWORD));
}
