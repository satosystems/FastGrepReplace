#include <windows.h>

#ifdef __cpluspluc
extern "C" {
#endif

/**
 * ���W�X�g���l���擾����B
 *
 * �߂�l�̃|�C���^�͌Ăяo�������K�� free �ŊJ�����Ȃ���΂Ȃ�Ȃ��B
 *
 * ���݂��Ȃ����W�X�g���p�X���w�肵���ꍇ�ANULL ��Ԃ��B
 *
 * @param [in] path ���W�X�g���p�X
 * @param [out] type ���W�X�g���^
 * @return ���W�X�g���l
 */
void *Registory_GetValue(const char *path, PDWORD type);

LONG Registory_SetValue(const char *path, DWORD type, CONST BYTE *data, DWORD size);

#ifdef __cpluscplus
}
#endif
