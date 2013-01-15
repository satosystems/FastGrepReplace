#include "Clipboard.h"

/**
 * クリップボードのデータをすべて取得する。
 *
 * 取得したデータはすぐに複製を取っておく。
 * このデータは #Clipboard_SetClipboardAllData に渡すことで
 * クリップボードの内容を元に戻すことができる（はずである）。
 *
 * 戻り値を　#Clipboard_SetClipboardAllData に渡さない場合、
 * 呼び出し元は std::pair の second の HANDLE を適切に
 * GlobalFree で開放しなければならない。
 *
 * @return クリップボードの内容。クリップボードが空の場合はサイズが 0
 */
std::vector<std::pair<UINT, HANDLE> > Clipboard_GetClipboardAllData() {
	std::vector<std::pair<UINT, HANDLE> > data;
	UINT format = 0;
	OpenClipboard(NULL);
	while (format = EnumClipboardFormats(format)) {
		data.push_back(std::pair<UINT, HANDLE>(format, GetClipboardData(format)));
	}
	CloseClipboard();

	std::vector<std::pair<UINT, HANDLE> > copy; 
	std::vector<std::pair<UINT, HANDLE> >::iterator it;
	for (it = data.begin(); it != data.end(); it++) {
		SIZE_T size = GlobalSize(it->second);
		HANDLE h = GlobalAlloc(GHND, size);
		PVOID dst = GlobalLock(h);
		PVOID src = GlobalLock(it->second);
		memcpy(dst, src, size);
		GlobalUnlock(src);
		GlobalUnlock(dst);
		copy.push_back(std::pair<UINT, HANDLE>(it->first, dst));
	}
	return copy;
}

/**
 * クリップボードに内容を設定する。
 *
 * @param [in] data クリップボードに設定する内容。サイズが 0 の配列の場合、
 * 単にクリップボードが空になる
 */
void Clipboard_SetClipboardAllData(std::vector<std::pair<UINT, HANDLE> >& data) {
	OpenClipboard(NULL);
	EmptyClipboard();
	std::vector<std::pair<UINT, HANDLE> >::iterator it;
	for (it = data.begin(); it != data.end(); it++) {
		SetClipboardData(it->first, it->second);
	}
	CloseClipboard();
}
