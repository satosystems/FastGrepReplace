#pragma once

typedef void *libiconv_t;

/**
 * �ϊ��f�B�X�N���v�^�����蓖�Ă�B
 *
 * @param [in] tocode �ϊ���̕����R�[�h
 * @param [in] fromcode �ϊ��O�̕����R�[�h
 * @return �ϊ��f�B�X�N���v�^�B�G���[�̏ꍇ�� errno ��ݒ肵 (libiconv_t)-1 ��Ԃ�
 */
typedef libiconv_t (* LIBICONV_OPEN)(const char *tocode, const char *fromcode);

/**
 * LIBICONV_OPEN �Ő��������ϊ��f�B�X�N���v�^��p���ĕ����R�[�h�̕ϊ����s���B
 *
 * @param [in] cd �ϊ��f�B�X�N���v�^
 * @param [in,out] inbuf ���̓o�b�t�@
 * @param [in,out] inbytesleft ���̓o�b�t�@�̃T�C�Y
 * @param [in,out] outbuf �o�̓o�b�t�@
 * @param [in,out] outbytesleft �o�̓o�b�t�@�̃T�C�Y
 * @return ��t�ɕϊ������������B�G���[�����������ꍇ�� errno ��ݒ肵 (size_t)-1
 */
typedef size_t (* LIBICONV)(libiconv_t cd, const char **inbuf, size_t *inbytesleft, char **outbuf, size_t *outbytesleft);

/**
 * �ϊ��f�B�X�N���v�^���J������B
 *
 * @param [in] cd �J������ϊ��f�B�X�N���v�^
 * @return ���������ꍇ�� 0�A���s�����ꍇ�� errno ��ݒ肵 -1
 */
typedef int (* LIBICONV_CLOSE)(libiconv_t cd);

extern HINSTANCE libiconv_dll;
extern LIBICONV_OPEN libiconv_open;
extern LIBICONV libiconv;
extern LIBICONV_CLOSE libiconv_close;

