#pragma once

#include <windows.h>

#ifdef __cpluspluc
extern "C" {
#endif

/**
 * INI �t�@�C����ۑ�����f�B���N�g���i���̃��W���[�����z�u����Ă���f�B���N�g���j���擾����B
 *
 * ���̊֐��͐�������ƃf�B���N�g�����|�C���^�ŕԂ��B
 * �Ăяo�����͖߂�l�� NULL �ł͂Ȃ��Ȃ�A�|�C���^�� free ��p���ēK�؂ɊJ�����Ȃ���΂Ȃ�Ȃ��B
 *
 * @return �f�B���N�g���p�X�B���m�s�\�ȏꍇ�� NULL
 */
char *HideUtil_GetMacroDir();

/**
 * �G�ۂ̃C���X�g�[���f�B���N�g�����擾����B
 *
 * ���̊֐��͐�������ƃf�B���N�g�����|�C���^�ŕԂ��B
 * �Ăяo�����͖߂�l�� NULL �ł͂Ȃ��Ȃ�A�|�C���^�� free ��p���ēK�؂ɊJ�����Ȃ���΂Ȃ�Ȃ��B
 *
 * @return �f�B���N�g���p�X�B���m�s�\�ȏꍇ�� NULL
 */
char *HideUtil_GetHidemaruDir();

DWORD *HideUtil_GetClipHistFlag();

LONG HideUtil_SetClipHistFlag(DWORD flag);

#ifdef __cpluscplus
}
#endif
