#include <windows.h>

#ifdef __cpluspluc
extern "C" {
#endif

/**
 * レジストリ値を取得する。
 *
 * 戻り値のポインタは呼び出し元が必ず free で開放しなければならない。
 *
 * 存在しないレジストリパスを指定した場合、NULL を返す。
 *
 * @param [in] path レジストリパス
 * @param [out] type レジストリ型
 * @return レジストリ値
 */
void *Registory_GetValue(const char *path, PDWORD type);

LONG Registory_SetValue(const char *path, DWORD type, CONST BYTE *data, DWORD size);

#ifdef __cpluscplus
}
#endif
