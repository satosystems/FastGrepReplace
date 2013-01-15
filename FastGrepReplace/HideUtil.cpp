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
 * INI �t�@�C����ۑ�����f�B���N�g���i���̃��W���[�����z�u����Ă���f�B���N�g���j���擾����B
 *
 * ���̊֐��͐�������ƃf�B���N�g�����|�C���^�ŕԂ��B
 * �Ăяo�����͖߂�l�� NULL �ł͂Ȃ��Ȃ�A�|�C���^�� free ��p���ēK�؂ɊJ�����Ȃ���΂Ȃ�Ȃ��B
 *
 * @return �f�B���N�g���p�X�B���m�s�\�ȏꍇ�� NULL
 */
char *HideUtil_GetMacroDir() {
	char *work;
	char *temp;
	DWORD type;
	
	if ((temp = (char *)Registory_GetValue(hideinstloc, &type)) == NULL) {
		// �G�ۂ��C���X�g�[������Ă��Ȃ��Ȃ�A�G�ۃ��[���f�B���N�g�����z�[��
		work = (char *)Registory_GetValue(turukamedir, &type);
	} else {
		// �G�ۂ��C���X�g�[������Ă���Ȃ�}�N���p�X�𒲂ׂ�
		if ((work = (char *)Registory_GetValue(macropath, &type)) == NULL) {
			// �}�N���p�X�����݂��Ȃ���ΏG�ۂ� InstallLocation ���z�[��
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
 * �G�ۂ̃C���X�g�[���f�B���N�g�����擾����B
 *
 * ���̊֐��͐�������ƃf�B���N�g�����|�C���^�ŕԂ��B
 * �Ăяo�����͖߂�l�� NULL �ł͂Ȃ��Ȃ�A�|�C���^�� free ��p���ēK�؂ɊJ�����Ȃ���΂Ȃ�Ȃ��B
 *
 * @return �f�B���N�g���p�X�B���m�s�\�ȏꍇ�� NULL
 */
char *HideUtil_GetHidemaruDir() {
	DWORD type;
	
	return (char *)Registory_GetValue(hideinstloc, &type);
}


/**
 * �G�ۂ̏풓�ݒ���擾����B
 *
 * �߂�l�̃|�C���^�͌Ăяo������ free ���g���ĊJ������K�v������B
 *
 * @retval NULL: ���W�X�g���ւ̃A�N�Z�X�����s
 * @retval �|�C���^�̎w���l�� 0: �풓�Ȃ�
 * @retval �|�C���^�̎w���l�� 1: �풓����
 */
DWORD *HideUtil_GetResident() {
	DWORD type;
	return (DWORD *)Registory_GetValue(resident, &type);
}

/**
 * �G�ۂ̃N���b�v�{�[�h�����̐ݒ���擾����B
 *
 * @retval NULL: ���W�X�g���ւ̃A�N�Z�X�����s
 * @retval �|�C���^�̎w���l�̍ŉ��ʃr�b�g�� 0: �N���b�v�{�[�h���������Ȃ�
 * @retval �|�C���^�̎w���l�̍ŉ��ʃr�b�g�� 1: �N���b�v�{�[�h���������
 */
DWORD *HideUtil_GetClipHistFlag() {
	DWORD type;
	return (DWORD *)Registory_GetValue(cliphistflag, &type);
}

/**
 * �G�ۂ̃N���b�v�{�[�h�����̐ݒ��ݒ肷��B
 *
 * @param [in] �N���b�v�{�[�h�����̐ݒ�
 * @retval ERROR_SUCCESS: �ݒ�ɐ���
 * @retval ��L�ȊO: �ݒ�Ɏ��s
 */
LONG HideUtil_SetClipHistFlag(DWORD flag) {
	CONST BYTE *data = (CONST BYTE *)&flag;
	return Registory_SetValue(cliphistflag, REG_DWORD, data, sizeof(DWORD));
}
