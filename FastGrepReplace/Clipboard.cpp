#include "Clipboard.h"

/**
 * �N���b�v�{�[�h�̃f�[�^�����ׂĎ擾����B
 *
 * �擾�����f�[�^�͂����ɕ���������Ă����B
 * ���̃f�[�^�� #Clipboard_SetClipboardAllData �ɓn�����Ƃ�
 * �N���b�v�{�[�h�̓��e�����ɖ߂����Ƃ��ł���i�͂��ł���j�B
 *
 * �߂�l���@#Clipboard_SetClipboardAllData �ɓn���Ȃ��ꍇ�A
 * �Ăяo������ std::pair �� second �� HANDLE ��K�؂�
 * GlobalFree �ŊJ�����Ȃ���΂Ȃ�Ȃ��B
 *
 * @return �N���b�v�{�[�h�̓��e�B�N���b�v�{�[�h����̏ꍇ�̓T�C�Y�� 0
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
 * �N���b�v�{�[�h�ɓ��e��ݒ肷��B
 *
 * @param [in] data �N���b�v�{�[�h�ɐݒ肷����e�B�T�C�Y�� 0 �̔z��̏ꍇ�A
 * �P�ɃN���b�v�{�[�h����ɂȂ�
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
