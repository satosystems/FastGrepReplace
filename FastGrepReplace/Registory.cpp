#define STRICT
#include "Registory.h"
#pragma intrinsic(memset, memcpy)

#define REG_ROOT_COUNT (9) ///< ���[�g���W�X�g���̐��B

/**
 * ���W�X�g�����[�g��\���\���́B
 */
struct {
	char *name;		///< ���[�g���́B
	char *alias;	///< �G�C���A�X�B
	HKEY key;		///< ���W�X�g���L�[�B
} reg_root[REG_ROOT_COUNT] = {
	{ "HKEY_CLASSES_ROOT\\", NULL, HKEY_CLASSES_ROOT },
	{ "HKEY_CURRENT_USER\\", "USR\\", HKEY_CURRENT_USER },
	{ "HKEY_LOCAL_MACHINE\\", "SYS\\", HKEY_LOCAL_MACHINE },
	{ "HKEY_USERS\\", NULL, HKEY_USERS },
	{ "HKEY_PERFORMANCE_DATA\\", NULL, HKEY_PERFORMANCE_DATA },			// Windows NT/2000/XP
	{ "HKEY_CURRENT_CONFIG\\", NULL, HKEY_CURRENT_CONFIG },
	{ "HKEY_DYN_DATA\\", NULL, HKEY_DYN_DATA },							// Windows 95/98/Me
	{ "HKEY_PERFORMANCE_TEXT\\", NULL, HKEY_PERFORMANCE_TEXT },			// Windows XP
	{ "HKEY_PERFORMANCE_NLSTEXT\\", NULL, HKEY_PERFORMANCE_NLSTEXT },	// Windows XP
};

/**
 * ���[�g���W�X�g�������烋�[�g���W�X�g���L�[���擾����B
 *
 * �n���ꂽ���[�g���W�X�g�������s���ȏꍇ�A���̊֐��� NULL ��Ԃ��B
 *
 * @param [in] root_name ���[�g���W�X�g�����A�܂��̓G�C���A�X��
 * @return �L�[�̃n���h��
 */
static HKEY open_reg_root(char *root_name) {
	char root_up[MAX_PATH + 1];
	HKEY root = NULL;
	int i, length;

	strcpy(root_up, root_name);
	length = (int)strlen(root_up);
	for (i = 0; i < length; i++) {
		root_up[i] = (char)toupper(root_up[i]);
	}
	if (root_up[length - 1] != '\\') {
		strcat(root_up, "\\");
		length++;
	}
	for (i = 0; i < REG_ROOT_COUNT; i++) {
		if (!strcmp(root_up, reg_root[i].name) ||
				(reg_root[i].alias && !strcmp(root_up, reg_root[i].alias))) {
			root = reg_root[i].key;
			break;
		}
	}
	if (!root) {
		SetLastError(ERROR_PATH_NOT_FOUND);
		return NULL;
	}
	return root;
}

/**
 * ���W�X�g�����I�[�v������B
 *
 * @param [in] regpath �I�[�v�����郌�W�X�g���̃p�X
 * @param [in] access �Z�L�����e�B�A�N�Z�X�}�X�N
 * @param [out] regname ���W�X�g���L�[��
 * @return �L�[�̃n���h��
 */
static HKEY open_reg(const char *regpath, unsigned long access, char **regname) {
	char buf[MAX_PATH + 1];
	char subkeypath[MAX_PATH + 1];
	char root_name[MAX_PATH + 1];
	size_t length = 0, root_length;
	// *regname // MacroPath
	int err;
	char *token;
	HKEY root = NULL;
	HKEY subkey;
	
	strcpy(buf, regpath);
	
	// �p�X���烋�[�g�L�[���擾����
	token = strtok(buf, "\\");
	strcpy(root_name, token);
	root_length = strlen(root_name);
	
	// �T�u�L�[����у��W�X�g���L�[���擾����
	subkeypath[0] = '\0';
	while ((token = strtok(NULL, "\\")) != NULL) {
		length = strlen(subkeypath);
		strcat(subkeypath, token);
		strcat(subkeypath, "\\");
		*regname = (char *)(regpath + length + root_length + 1);
	}
	subkeypath[length - 1] = '\0';

	if ((root = open_reg_root(root_name)) == NULL) {
		return NULL;
	}
	
	/* �T�u�L�[���I�[�v������ */
	if ((err = RegOpenKeyEx(
				root,		// �L�[�̃n���h��
				subkeypath,	// �I�[�v������T�u�L�[�̖��O
				0,			// �\��i0���w��j
				access,		// �Z�L�����e�B�A�N�Z�X�}�X�N
				&subkey		// �n���h�����i�[����ϐ��̃A�h���X
			)) != 0) {
		SetLastError(err);
		return NULL;
	}
	return subkey;
}


/**
 * ���W�X�g�����N���[�Y����B
 *
 * �N���[�Y���悤�Ƃ��Ă��郌�W�X�g������`�ς݂̃��W�X�g���̏ꍇ��
 * �N���[�Y�����A���̊֐��͉����s�Ȃ�Ȃ��B
 *
 * @param [in] subkey �L�[�̃n���h��
 */
static void close_reg(HKEY subkey) {
	int i;
	for (i = 0; i < REG_ROOT_COUNT; i++) {
		if (subkey == reg_root[i].key) {
			// ��`�ς݂̃L�[�̏ꍇ�N���[�Y����K�v�͂Ȃ�
			return;
		}
	}
	// ��`�ς݂̃L�[�ł͂Ȃ��̂ŃN���[�Y����
	RegCloseKey(subkey);
}

/**
 * ���W�X�g���L�[�̒l��Ԃ��B
 *
 * ���݂��Ȃ����W�X�g���L�[���w�肵���ꍇ�́A���̊֐��� NULL ��Ԃ��B
 *
 * ���̊֐��͌Ăяo�����ŕK���߂�l�̗̈�� free �ŊJ�����Ȃ���΂Ȃ�Ȃ��B
 *
 * @param [in] subkey �L�[�̃n���h��
 * @param [in] key ���W�X�g���L�[
 * @param [out] type ���W�X�g���^
 * @li REG_NONE 0 ��`����Ă��Ȃ��^�B
 * @li REG_SZ 1 �k���I�[������B
 * ANSI �ł̊֐��� Unicode �ł̊֐��̂ǂ�����g�p���Ă��邩�ɂ��A
 * ANSI ������܂��� Unicode ������ɂȂ�
 * @li REG_EXPAND_SZ 2 �W�J�O�̊��ϐ��ւ̎Q�� (�Ⴆ�΁g%PATH%�h�Ȃ�) ���������k���I�[������B
 * ANSI �ł̊֐��� Unicode �ł̊֐��̂ǂ�����g�p���Ă��邩�ɂ��A
 * ANSI ������܂��� Unicode ������ɂȂ�B
 * ���ϐ��ւ̎Q�Ƃ�W�J����ɂ� ExpandEnvironmentStrings �֐����g��
 * @li REG_BINARY 3 �o�C�i���f�[�^
 * @li REG_DWORD 4 32 �r�b�g�̐��l
 * @li REG_DWORD_LITTLE_ENDIAN 4 ���g���G���f�B�A���`���� 32 �r�b�g���l�B
 * REG_DWORD �Ɠ���
 * @li REG_DWORD_BIG_ENDIAN 5 �r�b�O�G���f�B�A���`���� 32 �r�b�g���l
 * @li REG_LINK 6 Unicode �V���{���b�N�����N
 * @li REG_MULTI_SZ 7 �k���I�[������̔z��B
 * ���ꂼ��̕����񂪃k�������ŋ�؂��A�A������ 2 �̃k���������z��̏I�[��\��
 * @li REG_RESOURCE_LIST 8 �f�o�C�X�h���C�o�̃��\�[�X���X�g
 * @li REG_QWORD 11 64 �r�b�g���l
 * @li REG_QWORD_LITTLE_ENDIAN 11 ���g���G���f�B�A���`���� 64 �r�b�g���l�B
 * REG_QWORD �Ɠ���
 * @retval NULL ���W�X�g���̎擾�Ɏ��s�����ꍇ
 * @retval ��L�ȊO ���W�X�g���̒l
 */
static void *get_val(HKEY subkey, char *key, PDWORD type) {
	void *data = NULL;
	DWORD sz = 0;
	LONG err;
	
	/* �f�[�^�T�C�Y�����߂� */
	if ((err = RegQueryValueEx(
				subkey,	// ���݃I�[�v�����Ă���L�[�̃n���h��
				key,	// �擾����l�́u���O�v��������������ւ̃|�C���^
				NULL,	// �\��p�����[�^�BNULL���w�肷��
				type,	// �l�́u��ށv���󂯎��
				NULL,	// �l�́u�f�[�^�v���󂯎��BNULL���w�肷�邱�Ƃ��\�����A�f�[�^�͎󂯎��Ȃ�
				&sz		// lpData�̃T�C�Y���w�肷��B�֐����s���lpData�ɃR�s�[���ꂽ�f�[�^�̃T�C�Y�ɂȂ�
			)) != 0) {
		// ���݂��Ȃ����W�X�g���L�[���w�肵���ꍇ�� NULL ��Ԃ��B
		return NULL;
	}
	
	
	
	/* �f�[�^�i�[�̈���m�ۂ��� */
	if ((data = malloc((size_t)sz)) == NULL) {
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		return NULL;
	}
	
	/* �f�[�^���擾���� */
	if ((err = RegQueryValueEx(subkey, key, NULL, type, (LPBYTE)data, &sz)) != 0) {
		SetLastError(err);
		free(data);
		return NULL;
	}
	return data;
}

/**
 * ���W�X�g���l���擾����B
 *
 * �߂�l�̃|�C���^�͌Ăяo�������K���J�����Ȃ���΂Ȃ�Ȃ��B
 *
 * ���݂��Ȃ����W�X�g���p�X���w�肵���ꍇ�ANULL ��Ԃ��B
 *
 * @param [in] path ���W�X�g���p�X
 * @param [out] type ���W�X�g���^
 * @return ���W�X�g���l
 */
void *Registory_GetValue(const char *path, PDWORD type) {
	HKEY subkey;
	char *regname;
	void *value = NULL;
	
	// �T�u�L�[���I�[�v������
	if ((subkey = open_reg(path, KEY_QUERY_VALUE, &regname)) == 0) {
		return NULL;
	}
	
	// �l���擾����
	if ((value = get_val(subkey, regname, type)) == NULL) {
		return NULL;
	}
	
	// �T�u�L�[���N���[�Y����
	close_reg(subkey);
	
	return value;
}

/**
 * ���W�X�g���l��ݒ肷��B
 *
 */
LONG Registory_SetValue(const char *path, DWORD type, CONST BYTE *data, DWORD size) {
	HKEY subkey;
	char *regname;

	// �T�u�L�[���I�[�v������
	if ((subkey = open_reg(path, KEY_SET_VALUE, &regname)) == 0) {
		return !ERROR_SUCCESS;
	}

	// �l����������
	LONG rc = RegSetValueEx(
		subkey,			// HKEY hKey,           // �e�L�[�̃n���h��
		regname,		// LPCTSTR lpValueName, // ���W�X�g���G���g����
		0,				// DWORD Reserved,      // �\��ς�
		type,			// DWORD dwType,        // ���W�X�g���G���g���̃f�[�^�^
		data,			// CONST BYTE *lpData,  // ���W�X�g���G���g���̃f�[�^
		size			// DWORD cbData         // ���W�X�g���G���g���̃f�[�^�̃T�C�Y
	);

	// �T�u�L�[���N���[�Y����
	close_reg(subkey);

	return rc;
}



