#include <windows.h>
#include <vector>
#include <utility>

/**
 * �N���b�v�{�[�h�f�[�^��\���^�B
 * std::pair �� HANDLE �́A#Clipboard_SetClipboardAllData
 * ���g�p���Ă��̃f�[�^���N���b�v�{�[�h�Ɋi�[����ꍇ�͖��Ȃ���
 * �i�[���Ȃ��ꍇ�� GlobalFree ���g�p���ĊJ������K�v������B
 */
typedef std::vector<std::pair<UINT, HANDLE> > Clip;

Clip Clipboard_GetClipboardAllData();

void Clipboard_SetClipboardAllData(Clip& data);

