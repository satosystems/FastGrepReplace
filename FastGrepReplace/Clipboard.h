#include <windows.h>
#include <vector>
#include <utility>

/**
 * クリップボードデータを表す型。
 * std::pair の HANDLE は、#Clipboard_SetClipboardAllData
 * を使用してこのデータをクリップボードに格納する場合は問題ないが
 * 格納しない場合は GlobalFree を使用して開放する必要がある。
 */
typedef std::vector<std::pair<UINT, HANDLE> > Clip;

Clip Clipboard_GetClipboardAllData();

void Clipboard_SetClipboardAllData(Clip& data);

