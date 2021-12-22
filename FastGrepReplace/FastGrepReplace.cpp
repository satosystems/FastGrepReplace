#include "stdafx.h"
#pragma intrinsic(memset, memcpy)

#define VERSION		((1 << 17) + (0 << 8) + 50)		// 2.50  �i�o�[�W�����A�b�v���ɕK���������C������j

#define HIDEMARU_GREP_MAX_COLUMN 2048

extern "C" int nkf_kanji_code_detect(const wchar_t *filename, char **kanji_code);
extern "C" int nkf_main(int argc, char *argv[]);

typedef std::string bstring;

static void add_log(const wchar_t *log);

const unsigned char BOM_UTF8[] = {0xEF, 0xBB, 0xBF};	///< UTF-8 �� BOM�B
const unsigned char BOM_UTF16BE[] = {0xFE, 0xFF};		///< UTF-16BE �� BOM�B
const unsigned char BOM_UTF16LE[] = {0xFF, 0xFE};		///< UTF-16LE �� BOM�B
const wchar_t CRLF[] = {0x0D, 0x0A};					///< CRLF�B

static HINSTANCE libiconv_dll;				///< libiconv �� DLL �̃C���X�^���X�n���h���B
static LIBICONV_OPEN libiconv_open;			///< iconv_open �֐��B
static LIBICONV libiconv;					///< iconv �֐��B
static LIBICONV_CLOSE libiconv_close;		///< iconv_close �֐��B

static HMODULE hInstance;					///< �v���Z�X�̃C���X�^���X�B
static HWND hWndDlg;						///< �_�C�A���O�̃E�B���h�E�n���h���B
static HWND hWndHidemaru;					///< �G�ۂ̃E�B���h�E�n���h���B
static DWORD thread_id;						///< �X���b�h ID�B

/**
 * ���̃��W���[���̂���t�H���_�p�X�B
 *
 * �Ō�Ƀt�@�C���Z�p���[�^�u\�v���t���Ă���B
 */
// TODO:
static std::wstring szModuleDirName;

static wchar_t editDlgMessage[256]; // 256 �͕K�v�\���ȑ傫��
static const wchar_t *editDlgFilePath;
static const wchar_t *editDlgLineBefore;
static const wchar_t *editDlgLineAfter;
static wchar_t *editDlgLineWork;

/**
 * �L�����Z�����ꂽ���ǂ����𔻒肷��t���O�B
 * FALSE �Ȃ瑱�s�ATRUE �Ȃ�L�����Z���B
 */
static LONG cancel_flag;

static DWORD done_file_count;		///< �u�������t�@�C�����B
static DWORD skip_file_count;		///< �X�L�b�v�����t�@�C�����B
static DWORD skip_line_count;		///< �X�L�b�v�����s���B
static DWORD count_change_line;		///< �ϊ������s���B

static int editDialogResult;
static LRESULT forceProcessLongLine;

typedef enum {
	R_MUSTNOT_SAVE = 1,			///< �t�@�C����ۑ�����K�v���Ȃ��i�ύX����Ă��Ȃ��j�B
	R_SUCCESS = 0,				///< �u���ɐ��������B
	R_INTERNAL_ERROR = -1,		///< �����I�ȃG���[�B
	R_INVALID_FORMAT = -2,		///< ���̓t�H�[�}�b�g���s���B
	R_FILE_SAVE_ERROR = -3,		///< �t�@�C���̕ۑ��Ɏ��s�����B
	R_MAYBE_BINARY_FILE = -4,	///< �t�@�C�����o�C�i���̉\���B
	R_LINE_NOT_EXISTS = -5,		///< ���݂��Ȃ��s�Ȃ̂Œu�����Ȃ������B
	R_CONVERT_ERROR = -6,		///< �����R�[�h�̕ϊ��Ɏ��s�����B
	R_LINE_IS_TOO_LONG = -7,	///< �s�� HIDEMARU_GREP_MAX_COLUMN �ȏ�B
} FGR_RC;

/**
 * �Ō�ɔ��������G���[���b�Z�[�W��ێ�����o�b�t�@�B
 * �T�C�Y�͓K���ŁA���b�Z�[�W�������ꍇ�͐؂�Ă��܂��B
 */
static wchar_t last_error_message[4096];

/**
 * �}�N���𓮍삳���Ă���t�@�C���̃x�[�X�f�B���N�g���B
 */
static wchar_t basedir[FILENAME_MAX + 1];

/**
 * �o�ߎ��Ԃ��L������ϐ��B
 */
static DWORD tickcount;

/**
 * �Ō�ɔ��������G���[���b�Z�[�W��ێ�����B
 *
 * GetLastError �Ŏ擾�����G���[���b�Z�[�W��
 * last_error_message �ɕێ�����B
 *
 * last_error_message �̓��e�͎���Ăяo�����܂ŗL���B
 *
 * @todo
 * ���͂��̎��_�Ń��O�ɓf���Ă������񂶂�Ȃ����H
 */
static void StockLastError() {
	FormatMessage(
		FORMAT_MESSAGE_FROM_SYSTEM,		// ���͌��Ə������@�̃I�v�V����
		NULL,							// ���b�Z�[�W�̓��͌�
		GetLastError(),					// ���b�Z�[�W���ʎq
		LANG_USER_DEFAULT,				// ���ꎯ�ʎq
		last_error_message,				// ���b�Z�[�W�o�b�t�@
		sizeof(last_error_message),		// ���b�Z�[�W�o�b�t�@�̍ő�T�C�Y
		NULL							// �����̃��b�Z�[�W�}���V�[�P���X����Ȃ�z��
	);
}

/**
 * �G���[���b�Z�[�W��\������B
 *
 * ���b�Z�[�W��\������ƁA�X�g�b�N����Ă������b�Z�[�W�̓N���A�����B
 */
static void msgbox() {
	MessageBox(NULL, last_error_message, L"FastGrepReplace", MB_ICONHAND | MB_OK);
	last_error_message[0] = '\0';
}

/**
 * �G���[���b�Z�[�W��\������B
 *
 * �V�X�e���̃G���[���b�Z�[�W�Ƃ̊ԂɎ����I�ɉ��s���t�^�����B
 *
 * @param [in] append_msg GetLastError �ɒǉ����郁�b�Z�[�W
 */
static void msgbox(std::wstring append_msg) {
	std::wstring s;
	if (last_error_message[0]) {
		s += last_error_message;
	}
	s += append_msg;
	wcscpy_s(last_error_message, s.c_str());
	msgbox();
}

/**
 * �G���[���b�Z�[�W��\������B
 *
 * �V�X�e���̃G���[���b�Z�[�W�Ƃ̊ԂɎ����I�ɉ��s���t�^�����B
 *
 * @param [in] append_msg GetLastError �ɒǉ����郁�b�Z�[�W
 */
static void msgbox(const wchar_t *append_msg) {
	msgbox(std::wstring(append_msg));
}

/**
 * kcode_t �̒l������ iconv �Ŏg�p�\�Ȗ��O��Ԃ��B
 *
 * @param [in] kanji_code �Ώۂ� kcode_t
 * @return �G���R�[�f�B���O��
 */
static const char *get_charset_name(kcode_t kanji_code) {
	switch (kanji_code) {
		case KC_UNKNOWN:        return "CP932";
		case KC_EUCorSJIS:      return "CP932";
		case KC_ASCII:          return "UTF-8";
		case KC_JIS:            return "ISO-2022-JP-2";
		case KC_EUC:            return "EUC-JP";
		case KC_SJIS:           return "CP932";
		case KC_BROKEN:         return "";
		case KC_DATA:           return "";
		case KC_UTF16BE:        return "UTF-16BE";
		case KC_UTF16LE:        return "UTF-16LE";
		case KC_UTF16BE_NOBOM:  return "UTF-16BE";
		case KC_UTF16LE_NOBOM:  return "UTF-16LE";
		case KC_UTF8:           return "UTF-8";
		case KC_UTF8N:          return "UTF-8";
		case KC_UTF7:			return "UTF-7";
		default:				return "";
	}
}

/**
 * kcode_t �̖��O��Ԃ��B
 *
 * @param [in] kanji_code �Ώۂ� kcode_t
 * @param [out] buf ���O���i�[����o�b�t�@
 * @param [in] size �o�b�t�@�̃T�C�Y
 */
static const wchar_t *get_charset_nameW(kcode_t kanji_code) {
	switch (kanji_code) {
		case KC_UNKNOWN:        return L"KC_UNKNOWN";
		case KC_EUCorSJIS:      return L"KC_EUCorSJIS";
		case KC_ASCII:          return L"KC_ASCII";
		case KC_JIS:            return L"KC_JIS";
		case KC_EUC:            return L"KC_EUC";
		case KC_SJIS:           return L"KC_SJIS";
		case KC_BROKEN:         return L"KC_BROKEN";
		case KC_DATA:           return L"KC_DATA";
		case KC_UTF16BE:        return L"KC_UTF16BE";
		case KC_UTF16LE:        return L"KC_UTF16LE";
		case KC_UTF16BE_NOBOM:  return L"KC_UTF16BE_NOBOM";
		case KC_UTF16LE_NOBOM:  return L"KC_UTF16LE_NOBOM";
		case KC_UTF8:           return L"KC_UTF8";
		case KC_UTF8N:          return L"KC_UTF8N";
		case KC_UTF7:			return L"KC_UTF7";
		default:				return L"KC_UNKNOWN";
	}
}

/**
 * �ƂĂ������s�̕ҏW�_�C�A���O�{�b�N�X�̃v���V�[�W���B
 *
 * @param [in] hDlg �_�C�A���O�̃n���h��
 * @param [in] msg ���b�Z�[�W
 * @param [in] wparam ���b�Z�[�W�̒ǉ����
 * @param [in] lparam ���b�Z�[�W�̒ǉ����
 */
BOOL CALLBACK DlgProcEdit(HWND hDlg, UINT msg, WPARAM wparam, LPARAM lparam) {
	switch (msg) {
		case WM_INITDIALOG:
			SetDlgItemText(hDlg, IDC_STATIC1, editDlgMessage);
			SetDlgItemText(hDlg, IDC_EDIT1, editDlgFilePath);
			SetDlgItemText(hDlg, IDC_EDIT2, editDlgLineBefore);
			SetDlgItemText(hDlg, IDC_EDIT3, editDlgLineAfter);
			SendMessage(GetDlgItem(hDlg, IDC_CHECK1), BM_SETCHECK, 0, 0);
			return TRUE;

		case WM_COMMAND:
			switch (LOWORD(wparam)) {
				case IDOK: {
					size_t size = HIDEMARU_GREP_MAX_COLUMN;
					while (true) {
						if (editDlgLineWork) {
							free(editDlgLineWork);
							editDlgLineAfter = NULL;
						}
						size *= 2;
						editDlgLineWork = (wchar_t *)malloc(size * sizeof(wchar_t));
						UINT rc = GetDlgItemTextW(hDlg, IDC_EDIT3, editDlgLineWork, size);
						if (rc == 0 || rc >= size - 1) {
							StockLastError();
							continue;
						}
						break;
					}
				}
				case IDC_NO_REPLACE:
				case IDC_ABORT:
					forceProcessLongLine = SendMessage(GetDlgItem(hDlg, IDC_CHECK1), BM_GETCHECK, 0, 0);
				case IDCANCEL:
					editDialogResult = LOWORD(wparam);
					ShowWindow(hDlg, SW_HIDE);
					DestroyWindow(hDlg);
					break;
			}

			return TRUE;
	}
	return FALSE;
}

class TextFile;

/**
 * �s���Ǘ����邽�߂̃N���X�ł��B
 *
 * ���̃N���X�͍s�{�̂�\�� value �Ɖ��s��\�� lf �������o�Ɏ����A
 * �f�[�^�\���Ƃ��Ă͂������ std::string �ƂȂ�B
 *
 * �s���ɉ��s�R�[�h�����̂́A���s�R�[�h�����݂���
 * �e�L�X�g�t�@�C�������e�����������߂ł���B
 */
class Line {
public:
	Line(TextFile *parent, const wchar_t *value, size_t length, const wchar_t *linefeed = NULL, DWORD lflen = 0) : parent(parent) {
		if (length) {
			wcvalue = new std::wstring(value, length);
		} else {
			wcvalue = new std::wstring(L"");
		}
		if (lflen) {
			wclf = new std::wstring(linefeed, lflen / sizeof(wchar_t));
		} else {
			wclf = NULL;
		}
	}

	/**
	 * �R���X�g���N�^�B
	 *
	 * @param [in] value �s�{�́BNULL �|�C���^�Ȃ�u�����N�s
	 * @param [in] size �s�{�̂̃o�C�g��
	 * @param [in] linefeed ���s�R�[�h�BNULL �Ȃ���s�Ȃ�
	 * @param [in] lflen ���s�R�[�h�̃o�C�g��
	 */
	Line(TextFile *parent, char *value, size_t length, char *linefeed = NULL, DWORD lflen = 0) : parent(parent) {
		if (value != NULL) {
			setValue(value, length);
			if (linefeed) {
				for (DWORD i = 0; i < lflen; i++) {
					lf.push_back(linefeed[i]);
				}
			}
		}
		wcvalue = wclf = NULL;
	}

	/**
	 * �f�X�g���N�^�B
	 */
	~Line() {
		if (wcvalue) {
			delete wcvalue;
		}
		if (wclf) {
			delete wclf;
		}
	}

	std::wstring &toWstr();

	char *data() {
		size_t len = size();
		char *bytes = (char *)malloc(len);
		if (wcvalue) {
			memcpy(bytes, (const char *)wcvalue->c_str(), wcvalue->size() * sizeof(wchar_t));
			size_t lflen;
			if (wclf && (lflen = wclf->length())) {
				memcpy(bytes + len - lflen * sizeof(wchar_t), (const char *)wclf->c_str(), lflen * sizeof(wchar_t));
			}
		} else {
			memcpy(bytes, value.c_str(), value.size());
			size_t lflen;
			if ((lflen = lf.size())) {
				memcpy(bytes + len - lflen, lf.c_str(), lflen);
			}
		}
		return bytes;
	}

	/**
	 * ��������Ԃ��B���s�͊܂܂Ȃ��B
	 */
	size_t length() {
		return toWstr().size();
	}

	/**
	 * ���s���܂ރo�C�g����Ԃ��B
	 */
	size_t size() {
		return sizeWithoutLf(false);
	}

	/**
	 * �o�C�g����Ԃ��B���s���܂ނ��ǂ����͈����Ŏw�肷��B
	 */
	size_t sizeWithoutLf(bool withoutLf = true) {
		size_t size;
		if (wcvalue) {
			size = wcvalue->size();
			if (!withoutLf && wclf) {
				size += wclf->size();
			}
			size *= sizeof(wchar_t);
		} else {
			size = value.size();
			if (!withoutLf) {
				size += lf.size();
			}
		}
		return size;
	}

	void setValue(char *newValue, size_t newLength) {
		value.clear();
		if (newValue) {
			for (size_t i = 0; i < newLength; i++) {
				value.push_back(newValue[i]);
			}
		}
	}

	void setValue(wchar_t *newValue) {
		wcvalue->clear();
		wcvalue->append(newValue);
	}

	/**
	 * �G�ۂ� grep ���̃o�C�g�T�C�Y��Ԃ��B
	 *
	 * �G�ۂ̎d�l�Ƃ��āAgrep ���ʂɂ� 2047 �o�C�g�܂ł����o�͂��ꂸ�A
	 * ���̌v�Z���@�́u������� CP932 �Ƃ��Ĉ����ACP932 �ɑ��݂��Ȃ�������
	 * 4 �o�C�g�Ƃ��Čv�Z����v�Ƃ������́B
	 *
	 * ���̃��\�b�h�͏�L�v�Z���Ɋ�Â��o�C�g�T�C�Y��Ԃ��B
	 */
	size_t grepSize() {
		size_t len = length();
		size_t gSize = 0;
		const wchar_t *wcs = wstr.c_str();
		for (size_t i = 0; i < len; i++) {
			wchar_t c = wcs[i];
			if (0x0001 <= c && c <= 0x007F) {
				gSize += 1;
			} else if (is_cp932unicode(c)) {
				gSize += 2;
			} else {
				gSize += 4;
			}
		}
		return gSize;
	}

	TextFile *parent;
private:
	/**
	 * �s�̖{�́B
	 *
	 * ��s�̏ꍇ�̓T�C�Y�� 0�ɂȂ�B
	 */
	bstring value;

	/**
	 * ���̍s�̉��s�R�[�h�B
	 *
	 * ���s���Ȃ��i�t�@�C���̍Ō�̍s�́j�ꍇ�A�T�C�Y�� 0�B
	 */
	bstring lf;

	std::wstring *wcvalue;
	std::wstring *wclf;

	std::wstring wstr;
};

/**
 * �X�g���[���֏o�͂���B
 *
 * �s�{�̂Ɖ��s�R�[�h�𑱂��ďo�͂���B
 * 
 * @param [in] os �o�͐�
 * @param [in] line �o�͂���C���X�^���X
 * @return �o�͂�����Ԃ̃X�g���[��
 */
//std::ostream& operator<<(std::ostream& os, Line& line) {
//	return os << line.value.c_str() << line.lf.c_str();
//}

/**
 * �t�@�C����\���N���X�B
 */
class TextFile {
private:
	libiconv_t cd;	///< libiconv �̕����R�[�h�ϊ��f�B�X�N���v�^�B
	bool isModified; ///< �ύX���ꂽ���ǂ����B
public:
	/**
	 * �B��̃R���X�g���N�^�B
	 *
	 * @param [in] path �t�@�C���̃p�X
	 * @param [in] text �t�@�C���̃e�L�X�g�B�o�C�i���̏ꍇ������
	 * @param [in] size �t�@�C���̃T�C�Y
	 */
	TextFile(const wchar_t *path, char *text, DWORD size) : size(size), kanji_code(KC_UNKNOWN), isModified(false) {
		cd = NULL;
		this->path = path;
		if (size) {
			this->text = (char *)malloc(size + 6); // + 6 �� UTF-8 ����̂��߂Ɏg�p
			memcpy(this->text, text, size);
			this->text[size] = 
				this->text[size + 1] = 
				this->text[size + 2] = 
				this->text[size + 3] = 
				this->text[size + 4] = 
				this->text[size + 5] = '\0';
		} else {
			this->text = NULL;
		}
	}

	/**
	 * �f�X�g���N�^�B
	 */
	~TextFile() {
		if (text) {
			free(text);
		}
		if (cd && cd != (libiconv_t)-1) {
			libiconv_close(cd);
		}
		std::vector<Line *>::iterator it = lines.begin();
		while (it != lines.end()) {
			delete (*it);
			it++;
		}
	}

	/**
	 * ���̃t�@�C���̍s������������B
	 *
	 * ���̃t�@�C�������s����؂�ɍs�P�ʂ̍\���ɕ������A
	 * �s�w��ɂ�鏑��������e�Ղɂ���B
	 *
	 * �������ɂ����蕶���R�[�h���w�肷��K�v������B
	 * ����́A�����R�[�h�ɂ���ĉ��s�R�[�h�𔻕ʂ��邽�߂ł���B
	 *
	 * @param [in] kanji_code ���̃t�@�C���̕����R�[�h
	 */
	void init_lines(const kcode_t kanji_code) {
		this->kanji_code = kanji_code;
		const char *encoding_name = get_charset_name(kanji_code);
		if (encoding_name != "") {
			if (encoding_name != "UTF-16LE") {
				cd = libiconv_open(encoding_name, "UTF-16LE");
			}
		}
		if (cd == (libiconv_t)-1) {
			// ���s�͂��肦�Ȃ�
			wchar_t encodingName[32];
			size_t len = strlen(encoding_name);
			for (size_t i = 0; i < len; i++) {
				encodingName[i] = encoding_name[i];
			}
			encodingName[len] = '\0';
			std::wstring buf1(L"���ϊ��f�B�X�N���v�^�����G���[���������܂����B\n");
			buf1 += path;
			buf1 += L"\nlibiconv_open(\"";
			buf1 += encodingName;
			buf1 += L"\", \"UTF-16LE\")";
			add_log(buf1.c_str());
			return;
		}

		char *sof = text;	// start of file
		char *sol = sof;	// start of line
		char *lf = NULL;
		DWORD lflen = 0;
		size_t sizeWithoutBom = size;

		// BOM ��ޔ�
		if (kanji_code == KC_UTF8) {
			memcpy(bom, sol, 3);
			sol += 3;
			sof += 3;
			sizeWithoutBom -= 3;
		} else if (kanji_code == KC_UTF16BE || kanji_code == KC_UTF16LE) {
			memcpy(bom, sol, 2);
			sol += 2;
			sof += 2;
			sizeWithoutBom -= 2;
		}
		bool eol_is_eof = false;

		// ���s�R�[�h�̔���
		for (DWORD i = 0, p = 0; i < size; i++) {
			eol_is_eof = false;
			if (kanji_code == KC_UTF16BE || kanji_code == KC_UTF16BE_NOBOM || kanji_code == KC_UTF16LE || kanji_code == KC_UTF16LE_NOBOM) {
				if (i % 2 == 0) {
					if (i + 3 < size && text[i] == 0x0D && text[i + 1] == 0x00 && text[i + 2] == 0x0A && text[i + 3] == 0x00 && (kanji_code == KC_UTF16LE || kanji_code == KC_UTF16LE_NOBOM)) {
						lines.push_back(new Line(this, sol, text + i - sol, text + i, 4)); // LineFeed_CRLF_UTF16LE
						sol = text + i + 4;
						i += 4;
						eol_is_eof = true;
					} else if (i + 1 < size && text[i] == 0x0D && text[i + 1] == 0x00 && (kanji_code == KC_UTF16LE || kanji_code == KC_UTF16LE_NOBOM)) {
						lines.push_back(new Line(this, sol, text + i - sol, text + i, 2)); // LineFeed_CR_UTF16LE
						sol = text + i + 2;
						i += 2;
						eol_is_eof = true;
					} else if (i + 3 < size && text[i + 1] == 0x0D && text[i] == 0x00 && text[i + 2] == 0x00 && text[i + 3] == 0x0A && (kanji_code == KC_UTF16BE || kanji_code == KC_UTF16BE_NOBOM)) {
						lines.push_back(new Line(this, sol, text + i - sol, text + i, 4)); // LineFeed_CRLF_UTF16BE
						sol = text + i + 4;
						i += 4;
						eol_is_eof = true;
					} else if (i + 1 < size && text[i] == 0x00 && text[i + 1] == 0x0D && (kanji_code == KC_UTF16BE || kanji_code == KC_UTF16BE_NOBOM)) {
						lines.push_back(new Line(this, sol, text + i - sol, text + i, 2)); // LineFeed_CR_UTF16BE
						sol = text + i + 2;
						i += 2;
						eol_is_eof = true;
					} else if (i + 1 < size && text[i] == 0x0A && text[i + 1] == 0x00 && (kanji_code == KC_UTF16LE || kanji_code == KC_UTF16LE_NOBOM)) {
						lines.push_back(new Line(this, sol, text + i - sol, text + i, 2)); // LineFeed_LF_UTF16LE
						sol = text + i + 2;
						i += 2;
						eol_is_eof = true;
					} else if (i + 1 < size && text[i] == 0x00 && text[i + 1] == 0x0A && (kanji_code == KC_UTF16BE || kanji_code == KC_UTF16BE_NOBOM)) {
						lines.push_back(new Line(this, sol, text + i - sol, text + i, 2)); // LineFeed_LF_UTF16BE
						sol = text + i + 2;
						i += 2;
						eol_is_eof = true;
					}
				}
			} else {
				if (text[i] == 0x0D) {
					if (i + 1 < size && text[i + 1] == 0x0A) {
						lines.push_back(new Line(this, sol, text + i - sol, text + i, 2)); // LineFeed_CRLF
						sol = text + i + 2;
						i++;
						eol_is_eof = true;
					} else {
						lines.push_back(new Line(this, sol, text + i - sol, text + i, 1)); // LineFeed_CR
						sol = text + i + 1;
						eol_is_eof = true;
					}
				} else if (text[i] == 0x0A) {
					lines.push_back(new Line(this, sol, text + i - sol, text + i, 1)); // LineFeed_LF
					sol = text + i + 1;
					eol_is_eof = true;
				}
			}
		}

		if (sol == sof) {
			// ���s���ЂƂ����݂��Ȃ��t�@�C��
			lines.push_back(new Line(this, sol, sizeWithoutBom - (sol - sof)));
		} else if (!eol_is_eof) {
			// �܂��͍Ō�̍s�����s�ł͂Ȃ��t�@�C��
			lines.push_back(new Line(this, sol, sizeWithoutBom - (sol - sof)));
		} else {
			// �Ō�̍s�����s�ŏI����Ă���t�@�C���Ȃ�A�u�����N�s��ǉ�����i�ꉞ�s�ԍ������Ă邩��j
			lines.push_back(new Line(this, (char *)NULL, 0));
		}
	}

	/**
	 * ���̃t�@�C�������̃t�@�C���ɏ㏑�����ĕۑ�����B
	 *
	 * �t�@�C�����ύX����Ă��Ȃ��ꍇ�͕ۑ����� #R_MUSTNOT_SAVE ��Ԃ��B
	 *
	 * ���̊֐����ŏ����Ɏ��s�����ꍇ�� #StockLastError ���Ăяo��
	 * �G���[���b�Z�[�W��ۑ�����B
	 *
	 * @param [in] force �����I�ɕۑ����邩�ǂ����B
	 * @retval #R_SUCCESS �t�@�C���̕ۑ��ɐ��������ꍇ
	 * @retval #R_MUSTNOT_SAVE �t�@�C����ۑ�����K�v���Ȃ��ꍇ
	 * @retval #R_FILE_SAVE_ERROR �t�@�C���̕ۑ��Ɏ��s�����ꍇ
	 */
	FGR_RC save(bool force = false) {
		FGR_RC rc = R_SUCCESS;
		if (isModified || force) {
			rc = save(path.c_str());
		} else {
			rc = R_MUSTNOT_SAVE;
		}
		return rc;
	}

	/**
	 * ���̃t�@�C����ۑ�����B
	 *
	 * �w�肳�ꂽ�p�X�Ƀt�@�C����ۑ�����B
	 * �w��p�X�Ƀt�@�C�������݂���ꍇ�A���̃t�@�C���̓��e�ŏ㏑������B
	 *
	 * ���̊֐����ŏ����Ɏ��s�����ꍇ�� #StockLastError ���Ăяo��
	 * �G���[���b�Z�[�W��ۑ�����B
	 *
	 * @param [in] save_path �t�@�C����ۑ�����p�X
	 * @retval #R_SUCCESS �t�@�C���̕ۑ��ɐ��������ꍇ
	 * @retval #R_FILE_SAVE_ERROR �t�@�C���̕ۑ��Ɏ��s�����ꍇ
	 */
	FGR_RC save(const wchar_t *save_path) {
		wchar_t num1[20];
		HANDLE file1;
		for (int i = 0; i < 3; i++) {
			file1 = CreateFile(save_path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
			if (file1 == INVALID_HANDLE_VALUE) {
				StockLastError();
				Sleep(20); // ������Ƒ҂ĂΊJ����悤�ɂȂ�\��������
			} else {
				break;
			}
		}
		if (file1 == INVALID_HANDLE_VALUE) {
			std::wstring buf1(L"���ۑ���̃t�@�C�����J���܂���ł����B\n");
			buf1 += save_path;
			add_log(buf1.c_str());
			swprintf_s(num1, L"%d", ++skip_file_count);
			SetDlgItemText(hWndDlg, IDC_STATIC_SKIP_FILE_COUNT, num1);
			return R_FILE_SAVE_ERROR;
		}
		DWORD write_size;
		FGR_RC rc = R_SUCCESS;

		// �ޔ����� BOM ����������
		if (kanji_code == KC_UTF8) {
			if (!WriteFile(file1, bom, 3, &write_size, NULL)) {
				StockLastError();
				std::wstring buf1(L"��UTF-8��BOM�̏������݂Ɏ��s���܂����B\n");
				buf1 += save_path;
				add_log(buf1.c_str());
				swprintf_s(num1, L"%d", ++skip_file_count);
				SetDlgItemText(hWndDlg, IDC_STATIC_SKIP_FILE_COUNT, num1);
				rc = R_FILE_SAVE_ERROR;
			}
		} else if (kanji_code == KC_UTF16BE || kanji_code == KC_UTF16LE) {
			if (!WriteFile(file1, bom, 2, &write_size, NULL)) {
				StockLastError();
				std::wstring buf1(L"��UTF-16��BOM�̏������݂Ɏ��s���܂����B\n");
				buf1 += save_path;
				add_log(buf1.c_str());
				swprintf_s(num1, L"%d", ++skip_file_count);
				SetDlgItemText(hWndDlg, IDC_STATIC_SKIP_FILE_COUNT, num1);
				rc = R_FILE_SAVE_ERROR;
			}
		}

		if (rc == R_SUCCESS) {
			std::vector<Line *>::iterator it;
			size_t i;
			char *buf = NULL;
			for (it = lines.begin(), i = 1; it != lines.end(); it++, i++) {
				size_t out_length = (*it)->size();
				buf = (*it)->data();
				if (out_length && !WriteFile(file1, buf, (DWORD)out_length, &write_size, NULL)) {
					StockLastError();
					std::wstring buf1(L"���s�̏������݂Ɏ��s���܂����B\n");
					buf1 += path;
					buf1 += L"(";
					swprintf_s(num1, L"%d", i + 1);
					buf1 += L"): ";

					libiconv_t cd = libiconv_open("UTF-16LE", get_charset_name(kanji_code));
					size_t in_length = out_length;
					char *c_str = buf;
					out_length = in_length * 3 + 1 + 1; // 3 �{���Ă�̂� UTF-8 �̃P�[�X��z��A+1 �� NULL Terminate�A+1 �� JIS �̏ꍇ�̉��s
					wchar_t *buf2 = (wchar_t *)malloc(out_length);
					char *outptr = (char *)buf2;
					memset(buf2, '\0', out_length);
					size_t result = libiconv(cd, (const char **)&c_str, &in_length, &outptr, &out_length);
					if (result != (size_t)-1) {
						buf1 += buf2;
					}
					free(buf2);
					libiconv_close(cd);
					add_log(buf1.c_str());
					rc = R_FILE_SAVE_ERROR;

					swprintf_s(num1, L"%d", ++skip_file_count);
					SetDlgItemText(hWndDlg, IDC_STATIC_SKIP_FILE_COUNT, num1);
					break;
				}
				if (buf) {
					free(buf);
					buf = NULL;
				}
			}
			if (buf) {
				free(buf);
			}
		}
		if (!CloseHandle(file1)) {
			StockLastError();
			std::wstring buf1(L"���t�@�C�������̂Ɏ��s���܂����B\n");
			buf1 += save_path;
			add_log(buf1.c_str());
			rc = R_FILE_SAVE_ERROR;
			// �����̓X�L�b�v�t�@�C�����J�E���g���Ȃ��B
			//sprintf(num1, "%d", ++skip_file_count);
			//SetDlgItemText(hWndDlg, IDC_STATIC_SKIP_FILE_COUNT, num1);
		}
		return rc;
	}

	/**
	 * 2048 �o�C�g�ȏ�̖₢���킹���o�����o���Ȃ����𔻒f����B
	 *
	 * �G�ۂ̎d�l�ŁAgrep ���ʂ� Shift_JIS ���Z�����o�C�g���i������
	 * Shift_JIS �Ɋ܂܂�Ȃ������� 4 �o�C�g�����j�� 2047 �o�C�g�܂ł��o�͂���B
	 *
	 * @param [in] line �Ώۍs
	 * @return 2048 �o�C�g�ȏ�Ȃ� true�A�����łȂ��Ȃ� false
	 */
	bool hasPrompt(Line *line) const {
		bool rc = false;
		switch (line->parent->kanji_code) {
			case KC_ASCII: // TODO: ASCII �� UTF-8 �Ƃ��Ċ��Z����Ă��܂�
			case KC_EUCorSJIS:
			case KC_JIS:
			case KC_EUC:
			case KC_SJIS:
			case KC_UTF7: // UTF-7 �͏G�ۂł� grep �ł��Ȃ�
				if (line->sizeWithoutLf() >= HIDEMARU_GREP_MAX_COLUMN) {
					rc = true;
				}
				break;

			case KC_UTF8:
			case KC_UTF8N:
			case KC_UTF16BE:
			case KC_UTF16LE:
			case KC_UTF16BE_NOBOM:
			case KC_UTF16LE_NOBOM:
				if (line->sizeWithoutLf() >= HIDEMARU_GREP_MAX_COLUMN / 4) {
					size_t gSize = line->grepSize();
					if (gSize >= HIDEMARU_GREP_MAX_COLUMN) {
						rc = true;
					}
				}
				break;
		}
		return rc;
	}

	/**
	 * �s���w�肵�Ēu�����s���B
	 * 
	 * @param [in] line_no �s�ԍ��B�ŏ��̍s�Ȃ� 1�B�C���f�b�N�X�ł͂Ȃ��̂Œ���
	 * @param [in] value �u����̕�����BUTF-16LE �œn�����
	 * @param [in] isReplaceLongLine �����s��u������ꍇ�� true�A�����łȂ��ꍇ�� false
	 * @retval #R_SUCCESS
	 * @retval #R_LINE_NOT_EXISTS
	 * @retval #R_CONVERT_ERROR
	 */
	FGR_RC change_line(unsigned int line_no, std::wstring value, LRESULT isReplaceLongLine = FALSE) {
		FGR_RC rc = R_SUCCESS;
		isReplaceLongLine = isReplaceLongLine | forceProcessLongLine;
		if (lines.size() < line_no) {
			rc = R_LINE_NOT_EXISTS;
		} else if (hasPrompt(lines[line_no - 1])) {
			isReplaceLongLine = isReplaceLongLine | forceProcessLongLine;
			editDlgLineWork = NULL;
			if (!isReplaceLongLine) {
				swprintf_s(editDlgMessage, L"�u���Ώۍs�i%d�j���K��i%d�o�C�g�j�ȏ�ł��B", line_no, HIDEMARU_GREP_MAX_COLUMN);
				editDlgFilePath = path.c_str();
				editDlgLineBefore = lines[line_no - 1]->toWstr().c_str();
				editDlgLineAfter = value.c_str();

				DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG2), hWndDlg, DlgProcEdit);
			}

			switch (editDialogResult) {
				case IDOK:
					if (editDlgLineWork) {
						value = editDlgLineWork;
						free(editDlgLineWork);
						editDlgLineWork = NULL;
					}
					break;

				case IDC_ABORT:
					InterlockedCompareExchange(&cancel_flag, TRUE, FALSE);

				default:
					rc = R_LINE_IS_TOO_LONG;
			}
		}
		if (rc == R_SUCCESS) {
			if (cd) {
				if (kanji_code == KC_UTF7) {
					std::string UTF7_encode(std::wstring &src);
					std::string utf7 = UTF7_encode(value);
					lines[line_no - 1]->setValue((char *)utf7.c_str(), utf7.size());
				} else {
					// JIS �̏ꍇ�͉��s�R�[�h���Ȃ��ƃG�X�P�[�v�����Ȃ�
					if (kanji_code == KC_JIS) {
						value += L"\n";
					}
					const wchar_t *c_str = value.c_str();
					size_t in_length = value.size();
					size_t out_length = in_length * 3 + 1 + 1; // 3 �{���Ă�̂� UTF-8 �̃P�[�X��z��A+1 �� NULL Terminate�A+1 �� JIS �̏ꍇ�̉��s
					in_length *= sizeof(wchar_t);
					char *buf = (char *)malloc(out_length);
					char *outptr = buf;
					memset(buf, '\0', out_length);
					size_t result = libiconv(cd, (const char **)&c_str, &in_length, &outptr, &out_length);
					if (result == (size_t)-1) {
						std::wstring buf1(L"�������R�[�h�̕ϊ��Ɏ��s���܂����iUTF-16LE �� ");
						buf1 += get_charset_nameW(kanji_code);
						buf1 += L"�j�B\n";
						buf1 += path;
						buf1 += L"(";
						wchar_t num1[20];
						swprintf_s(num1, L"%d", line_no);
						buf1 += num1;
						buf1 += L"): ";
						buf1 += value;
						buf1 += L"\n";
						add_log(buf1.c_str());
						rc = R_CONVERT_ERROR;
					} else {
						if (kanji_code == KC_JIS) {
							--outptr;
							*outptr = '\0';
						}
						lines[line_no - 1]->setValue(buf, outptr - buf);
					}
					free(buf);
				}
			} else {
				lines[line_no - 1]->setValue((char *)value.c_str(), value.length() * sizeof(wchar_t));
			}
		}
		if (rc == R_SUCCESS) {
			isModified = true;
		}
		return rc;
	}

#ifdef _DEBUG
//	bool operator==(TextFile &te

	/**
	 * ���̃I�u�W�F�N�g�Ɠn���ꂽ�I�u�W�F�N�g���r����B
	 *
	 * @param [in] right ��r�Ώۂ̃I�u�W�F�N�g
	 * @return ���e������Ȃ� 0�A�قȂ�Ȃ� 0 �ȊO
	 */
	int compare(TextFile *right) {
		int rc = 0;
		if (right == NULL) {
			MessageBox(NULL, L"right��NULL", L"NULL", MB_OK);
			rc = -1;
		} else {
			size_t size = this->lines.size();
			size_t rsize = right->lines.size();
			if (this->lines.size() != right->lines.size()) {
				MessageBox(NULL, right->path.c_str(), L"�s�����Ⴄ", MB_OK);
				rc = -1;
			} else if (this->kanji_code != right->kanji_code) {
				MessageBox(NULL, right->path.c_str(), L"�����R�[�h���Ⴄ", MB_OK);
				rc = -1;
			} else {
				for (size_t j = 0; j < this->lines.size(); j++) {
					size_t aSize = this->lines[j]->size();
					size_t eSize = this->lines[j]->size();
					if (aSize != eSize) {
						MessageBox(NULL, this->path.c_str(), L"�s�̃T�C�Y���Ⴄ", MB_OK);
						rc = -1;
					} else {
						char *aData = this->lines[j]->data();
						char *eData = right->lines[j]->data();
						if (memcmp(aData, eData, aSize) != 0) {
							MessageBox(NULL, this->path.c_str(), L"�s�̓��e���Ⴄ", MB_OK);
							rc = -1;
						}
						free(aData);
						free(eData);
					}
					if (rc != 0) {
						break;
					}
				}
			}
		}
		return rc;
	}
#endif

	std::wstring path;			///< ���̃e�L�X�g�t�@�C���̃p�X�B
	char *text;					///< ���̃e�L�X�g�t�@�C���̃e�L�X�g�B
	unsigned char bom[3];		///< ���̃e�L�X�g�t�@�C���̃o�C�g�I�[�_�[�}�[�N�B
	DWORD size;					///< ���̃e�L�X�g�t�@�C���̃T�C�Y�B���������̂ݗL���B�t�@�C����������������͈Ӗ����Ȃ��B
	kcode_t	kanji_code;			///< ���̃e�L�X�g�t�@�C���̕����R�[�h�B
	std::vector<Line *> lines;	///< �s�P�ʂł��̃t�@�C���̃e�L�X�g��ێ�����B
};


std::wstring &Line::toWstr() {
	if (wcvalue) {
		return *wcvalue;
	} else {
		if (wstr.size() == 0 && value.size() != 0) {
			kcode_t kanji_code = parent->kanji_code;
			if (kanji_code == KC_UTF16LE || kanji_code == KC_UTF16LE_NOBOM) {
				wstr = std::wstring((wchar_t *)value.c_str(), value.size() / sizeof(wchar_t));
			} else {
				libiconv_t cd = libiconv_open("UTF-16LE", get_charset_name(kanji_code));
				size_t in_length = value.size();
				const char *c_str = value.c_str();
				size_t out_length = in_length * 3 + 1 + 1; // 3 �{���Ă�̂� UTF-8 �̃P�[�X��z��A+1 �� NULL Terminate�A+1 �� JIS �̏ꍇ�̉��s
				wchar_t *buf = (wchar_t *)malloc(out_length);
				char *outptr = (char *)buf;
				memset(buf, '\0', out_length);
				size_t result = libiconv(cd, (const char **)&c_str, &in_length, &outptr, &out_length);
				if (result == (size_t)-1) {
					std::wstring buf1(L"�������R�[�h�̕ϊ��Ɏ��s���܂����iUTF-16LE �� ");
					buf1 += get_charset_nameW(kanji_code);
					buf1 += L"�j�B\n";
					buf1 += parent->path;
					buf1 += L"\n";
					MessageBox(NULL, L"�ϊ����s", buf1.c_str(), MB_OK);
				} else {
					wstr = buf;
				}
				libiconv_close(cd);
				if (buf) {
					free(buf);
				}
			}
		}
		return wstr;
	}
}

/**
 * ���ݏ������̃e�L�X�g�t�@�C���̍\����\���B��̃I�u�W�F�N�g�B
 *
 * �����̑O��Ƃ��āA�����t�@�C���ւ̕ύX�͘A�����Ď��s����邽�߁A
 * �ύX�Ώۂ̃t�@�C�����؂�ւ��^�C�~���O�ł��̃I�u�W�F�N�g�͊J������
 * ���̃e�L�X�g�t�@�C����\���I�u�W�F�N�g�ւƎQ�Ƃ�؂�ւ���B
 */
static TextFile *file;

/**
 * ���O�t�@�C���B
 *
 * �������ɃG���[�����������ꍇ�́A����̃t�H�[�}�b�g�ł����ɃG���[���e���f���o�����B
 */
static TextFile *logfile;

/**
 * ���O���o�͂���B
 *
 * ���O���o�͂��钼�O�ɃG���[���b�Z�[�W���ۊǂ���Ă�����
 * ���O�̌�ɃG���[���b�Z�[�W���o�͂���B
 *
 * @param [in] �o�͂��郁�b�Z�[�W
 */
static void add_log(const wchar_t *log) {
	logfile->lines.push_back(new Line(logfile, log, wcslen(log), CRLF, sizeof(CRLF)));
	if (last_error_message[0]) {
		logfile->lines.push_back(new Line(logfile, last_error_message, wcslen(last_error_message), CRLF, sizeof(CRLF)));
		last_error_message[0] = '\0';
	} else {
		logfile->lines.push_back(new Line(logfile, NULL, 0, CRLF, sizeof(CRLF)));
	}
}

/**
 * �t�@�C����ǂݍ��ݍ\��������B
 *
 * @param [in] filename �ǂݍ��ރt�@�C���̃p�X
 * @param [out] textfile �ǂݍ��񂾃t�@�C�����\��������N���X�̃C���X�^���X�ւ̃|�C���^
 * @retval ERROR_FILE_NOT_FOUND �t�@�C�������݂��Ȃ��ꍇ
 * @retval ERROR_INTERNAL_ERROR ���������ŃG���[������
 * @retval ��L�ȊO ���������������ꍇ
 */
static int load_file(const wchar_t *filename, TextFile **textfile) {
    HANDLE file1;
    HANDLE map1;
    DWORD hsize1, lsize1; // lsize1 ���t�@�C���̃T�C�Y
    char *view1;
    std::wstring mapstr(L"filemapping"); // ���j�[�N�ȕ�������w�肷��
    wchar_t num[10] = {0}; // ���ɂ� 10 ���t�@�C���܂� OK
    static int count = 0;
	swprintf_s(num, L"%d", count++); // ���j�[�N�ȃ}�b�v�t�@�C��������邽�߂̏���
    mapstr += num;
	wchar_t num1[20];

    // �t�@�C���̐���
    file1 = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    if (file1 == INVALID_HANDLE_VALUE) {
        // �t�@�C�������݂��Ȃ�
		StockLastError();
		std::wstring buf1(L"���t�@�C�����J���܂���ł����B\n");
		buf1 += filename;
		add_log(buf1.c_str());
		swprintf_s(num1, L"%d", ++skip_file_count);
		SetDlgItemText(hWndDlg, IDC_STATIC_SKIP_FILE_COUNT, num1);
		return ERROR_FILE_NOT_FOUND;
    }

    // �T�C�Y�̎擾
    lsize1 = GetFileSize(file1, &hsize1);

	// �T�C�Y���[���̃e�L�X�g�t�@�C�����l���ɓ����
	if (lsize1 || hsize1) {
		// �������}�b�v�h�t�@�C���̐���
		map1 = CreateFileMapping(file1, NULL, PAGE_READONLY, hsize1, lsize1, mapstr.c_str());
		if (map1 == NULL) {
			// �G���[����
			StockLastError();
			std::wstring buf1(L"���������}�b�v�h�t�@�C���̍쐬�Ɏ��s���܂����B\n");
			buf1 += filename;
			add_log(buf1.c_str());
			if (!CloseHandle(file1)) {
				StockLastError();
				buf1 = L"���t�@�C���̃N���[�Y�Ɏ��s���܂����B\n";
				buf1 += filename;
				add_log(buf1.c_str());
			}
			swprintf_s(num1, L"%d", ++skip_file_count);
			SetDlgItemText(hWndDlg, IDC_STATIC_SKIP_FILE_COUNT, num1);
			return ERROR_INTERNAL_ERROR;
		}
	    
		// �}�b�v�r���[�̐���
		view1 = (char *)MapViewOfFile(map1, FILE_MAP_READ, 0, 0, 0);
		if (view1 == NULL) {
			// �G���[����
			StockLastError();
			std::wstring buf1(L"���}�b�v�r���[�̍쐬�Ɏ��s���܂����B\n");
			buf1 += filename;
			add_log(buf1.c_str());
			if (!CloseHandle(map1)) {
				StockLastError();
				buf1 = L"���������}�b�v�h�t�@�C���̃N���[�Y�Ɏ��s���܂����B\n";
				buf1 += filename;
				add_log(buf1.c_str());
			}
			if (!CloseHandle(file1)) {
				StockLastError();
				buf1 = L"���t�@�C���̃N���[�Y�Ɏ��s���܂����B\n";
				buf1 += filename;
				add_log(buf1.c_str());
			}
			swprintf_s(num1, L"%d", ++skip_file_count);
			SetDlgItemText(hWndDlg, IDC_STATIC_SKIP_FILE_COUNT, num1);
			return ERROR_INTERNAL_ERROR;
		}
	} else {
		view1 = NULL;
	}

	*textfile = new TextFile(filename, view1, lsize1);

	if (lsize1) {
		// �A���}�b�v
		if (!UnmapViewOfFile(view1)) {
			StockLastError();
			std::wstring buf1(L"���r���[�̃N���[�Y�Ɏ��s���܂����B\n");
			buf1 += filename;
			add_log(buf1.c_str());
		}
		if (!CloseHandle(map1)) {
			StockLastError();
			std::wstring buf1(L"���������}�b�v�h�t�@�C���̃N���[�Y�Ɏ��s���܂����B\n");
			buf1 += filename;
			add_log(buf1.c_str());
		}
	}
	if (!CloseHandle(file1)) {
		StockLastError();
		std::wstring buf1(L"���t�@�C���̃N���[�Y�Ɏ��s���܂����B\n");
		buf1 += filename;
		add_log(buf1.c_str());
	}

	return ERROR_SUCCESS;
}

/**
 * �����R�[�h�𔻕ʂ���B
 *
 * @param [in] text �Ώۂ̃e�L�X�g
 * @param [in] size �e�L�X�g�̃T�C�Y�i�o�C�g���j
 * @retval KC_DATA �o�C�i���f�[�^
#if 0
 * @retval KC_BROKEN �����炭�o�C�i��
#endif
 * @retval KC_UNKNOWN �s���iLatin-1 �Ƃ��j
 * @retval KC_UTF8
 * @retval KC_UTF8N
 * @retval KC_UTF16BE
 * @retval KC_UTF16LE
 * @retval KC_UTF16BE_NOBOM
 * @retval KC_UTF16LE_NOBOM
 */
static kcode_t detect_utf(String text, DWORD size) {
	unsigned long long score_utf16_cp932 = 0;
	unsigned long long score_utf16_eucjp = 0;
	unsigned long long score_utf8 = 0;
	char utf16bom_cp932 = '\0'; // 'b': be  'l': le  'B': no bom be  'L': no bom le  'n': not utf16  '\0': still unknown
	char utf16bom_eucjp = '\0'; // 'b': be  'l': le  'B': no bom be  'L': no bom le  'n': not utf16  '\0': still unknown
	char utf16bom = '\0'; // 'b': be  'l':le  '\0': no bom
	bool utf8bom = false;
	unsigned long long score_utf7 = 0;
	bool utf7base64 = false;
	char *base64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

	kcode_t code = KC_UNKNOWN;
	int utf8nums = 0;
	bool binary = false;
	unsigned char *buf = NULL;
#if 0
	int linefeed[3] = { 0 }; // [0]=CR, [1]=LF, [2]=CRLF
#endif
	
	// �T�C�Y�� 1 �� 0x00 �Ȃ�A�o�C�i���Ɣ��f����
	if (size == 1 && text[0] == '\0') {
		binary = true;
		goto DONE;
	}

	if (size >= 2) {
		if (memcmp(text, BOM_UTF16BE, sizeof(BOM_UTF16BE)) == 0) {
			utf16bom_cp932 = utf16bom_eucjp = utf16bom = 'b';
		} else if (memcmp(text, BOM_UTF16LE, sizeof(BOM_UTF16LE)) == 0) {
			utf16bom_cp932 = utf16bom_eucjp = utf16bom = 'l';
		} else if (size >= 3) {
			if (memcmp(text, BOM_UTF8, sizeof(BOM_UTF8)) == 0) {
				utf8bom = true;
				utf8nums = 3; // BOM �̕������X�L�b�v����
			}
		}
	}

	if (size % 2 == 1) {
		score_utf16_cp932 = -1;
		score_utf16_eucjp = -1;
		utf16bom = '\0';
	}

	buf = (unsigned char *)malloc(size + 6); // +6 �� UTF-8 �̂��߂̃o�b�t�@
	memcpy(buf, text, size);
	buf[size] = buf[size + 1] = buf[size + 2] = buf[size + 3] = buf[size + 4] = buf[size + 5] = '\0';

	// �ŏ��� 0x00, 0x00 �Ȃ�o�C�i���Ɣ��f����
	if (size > 1 && buf[0] == '\0' && buf[1] == '\0') {
		binary = true;
		goto DONE;
	}


	for (DWORD i = 0; i < size; i++) {
#if 0
		if (buf[i] == 0x0d) { // CR
			linefeed[0]++;
		} else if (buf[i] == 0x0a) { // LF
			if (i > 0 && buf[i - 1] == 0x0d) { // CRLF
				linefeed[2]++;
				linefeed[0]--;
			} else {
				linefeed[1]++;
			}
		}
#endif
		unsigned char c0 = buf[i];
		unsigned char c1 = buf[i+1];
		unsigned char c2 = buf[i+2];
		unsigned char c3 = buf[i+3];
		unsigned char c4 = buf[i+4];
		unsigned char c5 = buf[i+5];

		// UTF-8
		if (score_utf8 != -1) {
			if (utf8nums == 0) {
				if (0x01 <= c0 && c0 <= 0x7f) {
					utf8nums = 1;
				} else if ((0xc0 <= c0 && c0 <= 0xdf) && (0x80 <= c1 && c1 <= 0xbf)) {
					utf8nums = 2;
				} else if ((0xe0 <= c0 && c0 <= 0xef) && (0x80 <= c1 && c1 <= 0xbf) && (0x80 <= c2 && c2 <= 0xbf)) {
					utf8nums = 3;
				} else if ((0xf0 <= c0 && c0 <= 0xf7) && (0x80 <= c1 && c1 <= 0xbf) && (0x80 <= c2 && c2 <= 0xbf) && (0x80 <= c3 && c3 <= 0xbf)) {
					utf8nums = 4;
				} else if ((0xf8 <= c0 && c0 <= 0xfb) && (0x80 <= c1 && c1 <= 0xbf) && (0x80 <= c2 && c2 <= 0xbf) && (0x80 <= c3 && c3 <= 0xbf) && (0x80 <= c4 && c4 <= 0xbf)) {
					utf8nums = 5;
				} else if ((0xfc <= c0 && c0 <= 0xfd) && (0x80 <= c1 && c1 <= 0xbf) && (0x80 <= c2 && c2 <= 0xbf) && (0x80 <= c3 && c3 <= 0xbf) && (0x80 <= c4 && c4 <= 0xbf) &&(0x80 <= c5 && c5 <= 0xbf)) {
					utf8nums = 6;
				}
				if (utf8nums != 0) {
					score_utf8 += utf8nums;
				} else {
					score_utf8 = -1;
				}
			}
			utf8nums--;
		}

		// UTF-16 CP932
		if (score_utf16_cp932 != -1 && i % 2 == 0) {
			if (!(i == 0 && utf16bom_cp932)) { // �t�@�C���̐擪�� BOM ������ꍇ�� BOM ���X�L�b�v
				unsigned short ucs2 = (buf[i] << 8) + buf[i + 1];
				utf16bom_cp932 = is_cp932ucs2(ucs2, utf16bom_cp932);
				if (utf16bom_cp932 != 'n') {
					score_utf16_cp932 += 2;
				} else {
					score_utf16_cp932 = -1;
				}
			}
		}

		// UTF-16 EUC-JP
		if (score_utf16_eucjp != -1 && i % 2 == 0) {
			if (!(i == 0 && utf16bom_eucjp)) { // �t�@�C���̐擪�� BOM ������ꍇ�� BOM ���X�L�b�v
				unsigned short ucs2 = (buf[i] << 8) + buf[i + 1];
				utf16bom_eucjp = is_eucjp_ucs2(ucs2, utf16bom_eucjp);
				if (utf16bom_eucjp != 'n') {
					score_utf16_eucjp += 2;
				} else {
					score_utf16_eucjp = -1;
				}
			}
		}

		// UTF-7
		if (score_utf7 != -1) {
			if (buf[i] > 0x7f) {
				score_utf7 = -1;
			} else if (buf[i] == '+') {
				utf7base64 = true;
				score_utf7++;
			} else if (buf[i] == '-') {
				utf7base64 = false;
				score_utf7++;
			} else if (utf7base64) {
				char q[] = { (char) buf[i], '\0' };
				if (strstr(base64, q) == NULL) {
					score_utf7 = -1;
				} else {
					score_utf7 += 2;
				}
			} else {
				score_utf7++;
			}
		}
	}

DONE:
	if (buf) {
		free(buf);
	}

	if (binary) {
		code = KC_DATA;
	} else if (score_utf8 != -1) {
		if (utf8bom) {
			code = KC_UTF8;
		} else {
			if (score_utf7 != -1) {
				double utf7ratio = (double)score_utf7 / (size * 2);
				if (utf7ratio >= 0.75) {
					code = KC_UTF7;
				}
			}
			if (code != KC_UTF7) {
				code = KC_UTF8N;
			}
		}
	} else if (score_utf16_cp932 != -1 || score_utf16_eucjp != -1) {
		if (utf16bom_cp932 == 'b' || utf16bom_eucjp == 'b') {
			code = KC_UTF16BE;
		} else if (utf16bom_cp932 == 'l' || utf16bom_eucjp == 'l') {
			code = KC_UTF16LE;
		} else if (utf16bom_cp932 == 'B' || utf16bom_eucjp == 'B') {
			code = KC_UTF16BE_NOBOM;
		} else if (utf16bom_cp932 == 'L' || utf16bom_eucjp == 'L') {
			code = KC_UTF16LE_NOBOM;
		}
	} else if (utf16bom == 'b') {
		code = KC_UTF16BE;
	} else if (utf16bom == 'l') {
		code = KC_UTF16LE;
	}
#if 0
	if (code == KC_UNKNOWN) {
		if (linefeed[0] != 0 && linefeed[1] != 0 && linefeed[1] != 0) {
			code = KC_BROKEN;
		}
	}
#endif
	return code;
}

/**
 * �t�@�C���̕����R�[�h�𔻒肷��B
 *
 * ���̊֐��́ANKF �� WKF �ƓƎ��̕����R�[�h���菈����g�ݍ��킹��
 * �\�Ȍ��萳�m�Ƀt�@�C���̕����R�[�h�𔻒肷��B
 *
 * - EUC-JP �� Shift_JIS �ɖ������ꍇ�́AShift_JIS �Ɣ��肷��
 * - UTF-7 �� ASCII �Ɣ��肳���i���A���������G�ۂ� grep �� UTF-7 �͑ΏۂɂȂ�Ȃ��j
 * - UTF-16 �� CP932 �̕����͈͂��܂ރe�L�X�g�ł���� BOM ���Ȃ��Ă�����\
 *
 * @param [in] filename �Ώۂ̃t�@�C��
 * @param [out] textfile �ǂݍ��񂾃t�@�C�����\�������ĕێ�����N���X�ւ̃|�C���^
 * @retval kcode_t �Œ�`����Ă���l�̂��ׂ�
 */
static kcode_t detect_kanji_code(const wchar_t *filename, TextFile **textfile) {
	char *nkf_friendly_name = ""; // NKF �����肵���ǉ\�ȕ����R�[�h��
	kcode_t nkf_code; // NKF �����肵�������R�[�h�inkf_friendly_name ���瓱���Ă���j
	kcode_t wkf_code; // WKF �����肵�������R�[�h
	kcode_t fgr_code; // FGR �����肵�������R�[�h
	kcode_t kanji_code = KC_UNKNOWN;  // �����I�ɔ��f���������R�[�h

	switch (load_file(filename, textfile)) {
		case ERROR_FILE_NOT_FOUND:
		case ERROR_INTERNAL_ERROR:
			return KC_ERROR;
	}

	if ((*textfile)->size) {
		fgr_code = detect_utf((String)(*textfile)->text, (*textfile)->size);
		if (fgr_code == KC_DATA) {
			kanji_code = KC_DATA;
		} else {
			wkf_code = wkfGuessKanjiCodeOfString((String)(*textfile)->text);
			nkf_kanji_code_detect(filename, &nkf_friendly_name);
			if (strcmp(nkf_friendly_name, "UTF-16") == 0) {
				if (memcmp((*textfile)->text, BOM_UTF16BE, sizeof(BOM_UTF16BE)) == 0) {
					nkf_code = KC_UTF16BE;
				} else if (memcmp((*textfile)->text, BOM_UTF16LE, sizeof(BOM_UTF16BE)) == 0) {
					nkf_code = KC_UTF16LE;
				} else {
					nkf_code = KC_UNKNOWN; // �����炭 NKF �̌딻��
				}
			} else if (strcmp(nkf_friendly_name, "UTF-8") == 0) {
				if (memcmp((*textfile)->text, BOM_UTF8, sizeof(BOM_UTF8)) == 0) {
					nkf_code = KC_UTF8;
				} else {
					nkf_code = KC_UTF8N;
				}
			} else if (strcmp(nkf_friendly_name, "EUC-JP") == 0) {
				nkf_code = KC_EUC;
			} else if (strcmp(nkf_friendly_name, "ISO-2022-JP") == 0) {
				nkf_code = KC_JIS;
			} else if (strcmp(nkf_friendly_name, "Shift_JIS") == 0) {
				nkf_code = KC_SJIS;
			} else {
				nkf_code = KC_UNKNOWN;
			}

			if (fgr_code != KC_UNKNOWN) {
				if (fgr_code == KC_UTF8N) {
					if (wkf_code == KC_JIS && nkf_code == KC_JIS) {
						kanji_code = KC_JIS;
					} else if (nkf_code == KC_UTF8N) {
						kanji_code = KC_UTF8N;
					} else if (wkf_code == KC_ASCII) {
						kanji_code = KC_ASCII;
					} else {
						kanji_code = KC_UTF8N;
					}
				} else {
					kanji_code = fgr_code;
				}
			} else {
				if (wkf_code == KC_UNKNOWN ||
						wkf_code == KC_BROKEN ||
						wkf_code == KC_DATA) {
					kanji_code = nkf_code;
				} else if (wkf_code == KC_EUCorSJIS) {
					kanji_code = KC_SJIS;
				} else {
					kanji_code = wkf_code;
				}
			}
		}
	} else {
		kanji_code = KC_UTF8N; // �T�C�Y 0 �̃e�L�X�g�� UTF-8N �ɂ���
	}

	if (kanji_code == KC_UNKNOWN ||
			kanji_code == KC_DATA ||
			kanji_code == KC_BROKEN) {
	} else {
		(*textfile)->init_lines(kanji_code);
	}
	return kanji_code;
}

/**
 * libiconv ������������B
 *
 * @retval true: �������ɐ���
 * @retval false: �������Ɏ��s
 */
static bool init_libiconv() {
	if (libiconv_dll == NULL) {
		std::wstring libiconv_path(szModuleDirName);
		libiconv_path += L"FastGrepReplace_libiconv-2.dll";
		libiconv_dll = LoadLibrary(libiconv_path.c_str());

		if (libiconv_dll == NULL) {
			add_log(L"�����C�u�����̏������Ɏ��s���܂����B");
			return false;
		}

		// �֐��ւ̃|�C���^�l���擾����
		libiconv_open = (LIBICONV_OPEN)GetProcAddress(libiconv_dll, "libiconv_open");
		libiconv = (LIBICONV)GetProcAddress(libiconv_dll, "libiconv");
		libiconv_close = (LIBICONV_CLOSE)GetProcAddress(libiconv_dll, "libiconv_close");
	}
	return true;
}

/**
 * �w�肳�ꂽ���X�g�ɏ]���ăt�@�C����u������B
 *
 * �n����郊�X�g�́ufilename(line): value�v�Ƃ����`���ł���A
 * �Ⴆ�΁uaaa.txt(19): abcde�v�Ȃ��
 * aaa.txt �� 19 �s�ڂ� abcde �ɒu��������B
 *
 * ���X�g���z�肵���`���ȊO�̏ꍇ�́A�u���������ɖ�������B
 *
 * @param [in] item �u���Ώۃ��X�g�̈�s
 * @retval R_SUCCESS: �u���ɐ�������
 * @retval R_INTERNAL_ERROR: �����I�ȃG���[
 * @retval R_INVALID_FORMAT: ���̓t�H�[�}�b�g���s��
 * @retval R_FILE_SAVE_ERROR: �t�@�C���̕ۑ��Ɏ��s����
 * @retval R_MAYBE_BINARY_FILE: �t�@�C�����o�C�i���̉\��
 * @retval R_LINE_NOT_EXISTS: ���݂��Ȃ��s�Ȃ̂Œu�����Ȃ�����
 * @retval R_CONVERT_ERROR: �����R�[�h�̕ϊ��Ɏ��s����
 */
static FGR_RC replace_line(std::wstring& item) {
	size_t pos, off;
	wchar_t num1[20];

	if (item.size() == 0) {
		// ��s�͖�������
		return R_INVALID_FORMAT;
	}

	if ((off = item.find(L"(")) == std::string::npos) {
		std::wstring buf1(L"���t�H�[�}�b�g���s���ł��B\n");
		buf1 += item;
		add_log(buf1.c_str());
		return R_INVALID_FORMAT;
	}

	if ((pos = item.find(L"): ", off)) == std::string::npos) {
		std::wstring buf1(L"���t�H�[�}�b�g���s���ł��B\n");
		buf1 += item;
		add_log(buf1.c_str());
		return R_INVALID_FORMAT;
	}

	std::wstring line_num = item.substr(off + 1, pos - off - 1);
	unsigned int line_no = 0;
	for (size_t i = 0; i < line_num.size(); i++) {
		if (isdigit(line_num[i])) {
			line_no *= 10;
			line_no += line_num[i] - '0';
		} else {
			std::wstring buf1(L"���t�H�[�}�b�g���s���ł��B\n");
			buf1 += item;
			add_log(buf1.c_str());
			return R_INVALID_FORMAT;
		}
	}
	if (line_no <= 0) {
		std::wstring buf1(L"���t�H�[�}�b�g���s���ł��B\n");
		buf1 += item;
		add_log(buf1.c_str());
		return R_INVALID_FORMAT;
	}

	std::wstring value = item.substr(pos + 3);
	std::wstring filename = item.substr(0, off);
	std::wstring filepath;
	if (filename.find(L":\\") == std::string::npos && (filename.size() < 2 || filename[0] != '\\' || filename[1] != '\\')) {
		std::wstring temp = basedir;
		if (temp[temp.size() - 1] != '\\') {
			temp += L"\\";
		}
		filepath = temp + filename;
	} else {
		filepath = filename;
	}
	while ((pos = filename.find(L"\\")) != std::string::npos) {
		filename = filename.substr(pos + 1);
	}

	if (file == NULL) {
		SetDlgItemText(hWndDlg, IDC_STATIC_CURRENT_FILE_NAME, filename.c_str());
	}		

	// �O�̃t�@�C���Ɠ����ł͂Ȃ��ꍇ�͕ۑ�����
	if (file != NULL && filepath.compare(file->path) != 0) {
		FGR_RC rc;
		if ((rc = file->save()) == R_FILE_SAVE_ERROR) {
			std::wstring buf1(L"���t�@�C���̕ۑ��Ɏ��s���܂����B\n");
			buf1 += file->path;
			add_log(buf1.c_str());
		}
		delete file;
		file = NULL;
		if (rc == R_FILE_SAVE_ERROR) {
			swprintf_s(num1, L"%d", ++skip_file_count);
			SetDlgItemText(hWndDlg, IDC_STATIC_SKIP_FILE_COUNT, num1);
			return R_FILE_SAVE_ERROR;
		} else if (rc == R_MUSTNOT_SAVE) {
			swprintf_s(num1, L"%d", ++skip_file_count);
			SetDlgItemText(hWndDlg, IDC_STATIC_SKIP_FILE_COUNT, num1);
		} else {
			swprintf_s(num1, L"%d", ++done_file_count);
			SetDlgItemText(hWndDlg, IDC_STATIC_DONE_FILE_COUNT, num1);
			SetDlgItemText(hWndDlg, IDC_STATIC_CURRENT_FILE_NAME, filename.c_str());
		}
	}

	if (file == NULL) {
		switch (detect_kanji_code(filepath.c_str(), &file)) {
			case KC_ERROR:
				return R_INTERNAL_ERROR;
		}
	}

	switch (file->kanji_code) {
		case KC_BROKEN:
		case KC_DATA:
		case KC_UNKNOWN: {
			std::wstring buf1(L"���o�C�i���t�@�C�����ۂ��̂Œu�����܂���ł����B\n");
			buf1 += file->path;
			add_log(buf1.c_str());

			delete file;
			file = NULL;
			swprintf_s(num1, L"%d", ++skip_file_count);
			SetDlgItemText(hWndDlg, IDC_STATIC_SKIP_FILE_COUNT, num1);
			return R_MAYBE_BINARY_FILE;
		}
	}

	switch (file->change_line(line_no, value)) {
		case R_LINE_NOT_EXISTS: {
			std::wstring buf1(L"���w�肳�ꂽ�s�͑��݂��܂���B\n");
			buf1 += file->path;
			buf1 += L"(";
			swprintf_s(num1, L"%d", line_no);
			buf1 += num1;
			buf1 += L"): ";
			buf1 += value;
			add_log(buf1.c_str());
			return R_LINE_NOT_EXISTS;
		}

		case R_CONVERT_ERROR: {
			std::wstring buf1(L"���u���Ɏ��s���܂����i���̃t�@�C���̓��e���������Ă��������j�B\n");
			buf1 += filepath.c_str();
			buf1 += L"(";
			swprintf_s(num1, L"%d", line_no);
			buf1 += num1;
			buf1 += L"): ";
			buf1 += value;
			add_log(buf1.c_str());
// �ȉ��̃R�����g�A�E�g�͂����炭�o�O�Ȃ̂ŁA�m�F��폜����
//			swprintf_s(num1, L"%d", ++skip_file_count);
//			SetDlgItemText(hWndDlg, IDC_STATIC_SKIP_FILE_COUNT, num1);
			return R_CONVERT_ERROR;
		}

		case R_LINE_IS_TOO_LONG: {
			std::wstring buf1(L"���Ώۍs�������̂Œu�����܂���ł����B\n");
			buf1 += filepath.c_str();
			buf1 += L"(";
			swprintf_s(num1, L"%d", line_no);
			buf1 += num1;
			buf1 += L"): ";
			buf1 += value;
			add_log(buf1.c_str());
// �ȉ��̃R�����g�A�E�g�͂����炭�o�O�Ȃ̂ŁA�m�F��폜����
//			swprintf_s(num1, L"%d", ++skip_file_count);
//			SetDlgItemText(hWndDlg, IDC_STATIC_SKIP_FILE_COUNT, num1);
			return R_LINE_IS_TOO_LONG;
		}
	}
	return R_SUCCESS;
}

/**
 * �܂Ƃ߂Ēu������B
 *
 * ���̊֐����Ăяo�����O�ɁA�Ăяo�����̏G�ۂőS�I���{�R�s�[��
 * ���s����Ă��Ȃ���΂Ȃ�Ȃ��B
 *
 * �����r���ŉ��炩�̃G���[���������Ă��A�v���I�ȃG���[�łȂ��ꍇ��
 * ���̃G���[�̓��O�ɓf����A�����͌p�������B
 *
 * ���O�͂��̃��W���[���Ɠ����t�H���_�ɂ���
 * FastGrepReplace.log �Ƃ����g���q�̃t�@�C���ɂȂ�B
 *
 * @param [in] pszIn �u�����X�g
 * @param [in] size �u�����X�g�̃T�C�Y
 * @return �����ɐ��������ꍇ�� 0�A�����łȂ��ꍇ�� 0 �ȊO
 */
int replace_all(WCHAR *pszIn, size_t size) {
	wchar_t num1[10];
	done_file_count = 0;
	skip_file_count = 0;
	skip_line_count = 0;
	wchar_t *context;
	wchar_t *token = wcstok_s(pszIn, L"\r\n", &context);
	long progress = -1; // �u���s�� 65535 ���߂������ꍇ�Ɏg�p
	std::vector<std::wstring> list;
	while (token != NULL) {
		list.push_back(std::wstring(token));
		token = wcstok_s(NULL, L"\r\n", &context);
	}

	HWND hWndProgress = GetDlgItem(hWndDlg, IDC_PROGRESS);
	size_t listSize = list.size();
	size_t progressMax = listSize;
	if (progressMax > 65535) {
		progressMax = 65535;
		progress = 0;
	}
	SendMessage(hWndProgress, PBM_SETRANGE, 0, MAKELPARAM(0, progressMax));
	SendMessage(hWndProgress, PBM_SETSTEP, 1, 0);

	for (size_t i = 0; i < list.size(); i++) {
		LONG isCancel = InterlockedCompareExchange(&cancel_flag, TRUE, TRUE);
		if (isCancel) {
			break;
		}
		if (list[i].length()) {
			if (replace_line(list[i]) == R_SUCCESS) {
				swprintf_s(num1, L"%d", ++count_change_line);
				SetDlgItemText(hWndDlg, IDC_STATIC_DONE_LINE_COUNT, num1);
			} else {
				swprintf_s(num1, L"%d", ++skip_line_count);
				SetDlgItemText(hWndDlg, IDC_STATIC_SKIP_LINE_COUNT, num1);
			}
		}
		if (progress == -1) {
			SendMessage(hWndProgress, PBM_STEPIT, 0, 0);
		} else {
			++progress;
			int pos = (int)(((double)progress / listSize) * 65535 + 0.5);
			SendMessage(hWndProgress, PBM_SETPOS, (WPARAM)pos, 0);
		}
	}
	return count_change_line;
}

/**
 * �I���������s���B
 *
 * ���ۑ��̃t�@�C��������΁A�ۑ�����B
 * ������Ή������Ȃ��B
 *
 * @retval R_SUCCESS: �����ɐ��������A�܂��͉������Ȃ�����
 * @retval R_FILE_SAVE_ERROR: �t�@�C���̕ۑ��Ɏ��s����
 */
FGR_RC teardown() {
	FGR_RC rc = R_SUCCESS;
	LONG isCancel = InterlockedCompareExchange(&cancel_flag, TRUE, TRUE);
	if (file != NULL) {
		if (!isCancel) {
			wchar_t num1[10];
			rc = file->save();
			if (rc == R_SUCCESS) {
				swprintf_s(num1, L"%d", ++done_file_count);
				SetDlgItemText(hWndDlg, IDC_STATIC_DONE_FILE_COUNT, num1);
			} else if (rc == R_MUSTNOT_SAVE) {
				swprintf_s(num1, L"%d", ++skip_file_count);
				SetDlgItemText(hWndDlg, IDC_STATIC_SKIP_FILE_COUNT, num1);
			} else {
				std::wstring buf1(L"���t�@�C���̕ۑ��Ɏ��s���܂����B");
				buf1 += file->path.c_str();
				add_log(buf1.c_str());
			}
		}
		delete file;
		file = NULL;
	}
	if (logfile->lines.size()) {
		rc = R_INTERNAL_ERROR;
	}

	DWORD now = GetTickCount();
	DWORD time = (now - tickcount) / 1000;
	int min = time / 60;
	int sec = time % 60;
	int msec = (now - tickcount) % 1000;
	UINT type;
	wchar_t result[1024];
	wchar_t *cancel_msg;
	if (isCancel) {
		cancel_msg = L"�L�����Z�����܂������A";
	} else {
		cancel_msg = L"";
	}

	if (logfile->lines.size()) {
		swprintf_s(result, L"%s%d�s�u�����܂����i%d�� %d�b %d�j�B\n�u�����Ȃ������s��\�����܂����H", cancel_msg, count_change_line, min, sec, msec);
		type = MB_ICONQUESTION | MB_YESNO;
	} else {
		swprintf_s(result, L"%s%d�s�u�����܂����i%d�� %d�b %d�j�B", cancel_msg, count_change_line, min, sec, msec);
		type = MB_OK;
	}

	if (!isCancel) {
		SetDlgItemText(hWndDlg, IDC_STATIC_CURRENT_FILE_NAME, L"");
	}
	if (MessageBox(hWndDlg, result, L"FastGrepReplace", type) == IDYES) {
		const wchar_t *msg = L"---- FastGrepReplace�Œu�����Ȃ������t�@�C���ƍs�ԍ��Ȃ� ----";
		logfile->lines.insert(logfile->lines.begin(), new Line(logfile, msg, wcslen(msg), CRLF, sizeof(CRLF)));
		if (logfile->save(true) == R_FILE_SAVE_ERROR) {
			msgbox(L"���O�t�@�C���̕ۑ��Ɏ��s���܂����B");
		}
	}
	delete logfile;
	logfile = NULL;
	return rc;
}

/**
 * �J�n�������s���B
 *
 * - libiconv ������������
 * - �Â����O�t�@�C�����폜����
 *
 * @param [in] �G�ۂ̃E�B���h�E�n���h��
 * @param [in] �x�[�X�f�B���N�g��
 * @retval ERROR_SUCCESS: �����ɐ��������ꍇ
 * @retval ERROR_INTERNAL_ERROR: �����Ɏ��s�����ꍇ
 */
int setup(HWND hWnd, const wchar_t *base) {
	hWndHidemaru = hWnd;
	last_error_message[0] = '\0';
	count_change_line = 0;
	file = NULL;
	tickcount = GetTickCount();

	// ���O�t�@�C���̃I�[�v��
	std::wstring logpath(szModuleDirName);
	logpath += L"FastGrepReplace.log";
	logfile = new TextFile(logpath.c_str(), NULL, 0);
	logfile->bom[0] = BOM_UTF16LE[0];
	logfile->bom[1] = BOM_UTF16LE[1];
	logfile->bom[2] = '\0';
	logfile->kanji_code = KC_UTF16LE;

	// ���O�t�@�C���͏����Ă���
	if (PathFileExists(logpath.c_str()) && !DeleteFile(logpath.c_str())) {
		StockLastError();
		std::wstring buf1(L"���O�t�@�C�����폜�ł��܂���ł���\n�蓮�ō폜�����݂Ă݂Ă��������B\n\n");
		buf1 += logpath.c_str();
		buf1 += L"\n";
		buf1 += last_error_message;
		last_error_message[0] = '\0';
		MessageBox(hWndHidemaru, buf1.c_str(), L"FastGrepReplace", MB_OK | MB_ICONWARNING);
		return ERROR_INTERNAL_ERROR;
	}

	if (!base) {
		add_log(L"���x�[�X�f�B���N�g���̎w�肪�s���iNULL�j�ł��B"); // ���肦�Ȃ�
		return ERROR_INTERNAL_ERROR;
	} else if (!wcslen(base)) {
		// ���j���[����I�������ꍇ�i���肦�Ȃ��j
		MessageBox(hWndHidemaru, L"�}�N������N�����Ă��������B\n���j���[����N�����Ă������s���܂���B", L"FastGrepReplace", MB_OK | MB_ICONINFORMATION);
		return ERROR_INTERNAL_ERROR;
	}
	if (!PathIsDirectory(base)) {
		std::wstring buf1(L"���w�肳�ꂽ�x�[�X�f�B���N�g���̓f�B���N�g���ł͂���܂���B\n"); // ���肦�Ȃ�
		buf1 += base;
		add_log(buf1.c_str());
		return ERROR_INTERNAL_ERROR;
	}
	wcscpy_s(basedir, base);
	if (init_libiconv()) {
		return ERROR_SUCCESS;
	} else {
		return ERROR_INTERNAL_ERROR;
	}
}

/**
 * ���� DLL �̃��C���G���g���|�C���g�B
 *
 * �v���Z�X�ɃA�^�b�`���ꂽ�^�C�~���O�Ń��W���[���n���h����ێ�����B
 *
 * @param [in] hModule ���W���[���n���h��
 * @param [in] ul_reason_for_call �Ăяo���ꂽ���R
 * @return ��� TRUE
 */
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID) {
	switch (ul_reason_for_call) {
		case DLL_PROCESS_ATTACH:
			{
				size_t len = FILENAME_MAX;
				wchar_t *szPath = NULL;
				while (szPath == NULL) {
					szPath = (wchar_t *)malloc(len * sizeof(wchar_t));
					DWORD rc = GetModuleFileName(hModule, szPath, len);
					if (rc == 0) {
						// ���̃G���[�͂��蓾�Ȃ��i�͂��j
					} else if (rc >= len - 1) {
						free(szPath);
						szPath = NULL;
						len *= 2;
					}
				}
				wchar_t *c = szPath;
				wchar_t *pos;
				while ((pos = wcsstr(c, L"\\")) != NULL) {
					c = pos + 1;
				}
				*c = '\0';
				szModuleDirName = szPath;
				free(szPath);

				libiconv_dll = NULL;
				hInstance = hModule;

				INITCOMMONCONTROLSEX iccex;
				iccex.dwSize = sizeof(INITCOMMONCONTROLSEX);
				iccex.dwICC = ICC_PROGRESS_CLASS;
				if (!InitCommonControlsEx(&iccex)) {
					MessageBox(NULL, L"�v���O���X�_�C�A���O���������ł��܂���ł����B", L"FastGrepReplace", MB_ICONHAND | MB_OK);
				}
			}
			break;
		case DLL_THREAD_ATTACH:
			break;
		case DLL_THREAD_DETACH:
			break;
		case DLL_PROCESS_DETACH:
			if (libiconv_dll) {
				FreeLibrary(libiconv_dll);
				libiconv_dll = NULL;
			}
			break;
	}
    return TRUE;
}

/**
 * �u���������L�����Z������B
 */
static void replace_cancel() {
	if (hWndDlg) {
		ShowWindow(hWndDlg, SW_HIDE);
		DestroyWindow(hWndDlg);
		hWndDlg = NULL;
	}
}

/**
 * �v���O���X�_�C�A���O�{�b�N�X�̃v���V�[�W���B
 *
 * �L�����Z���{�^���������ꂽ��A�u�����������̏�ŃL�����Z������B
 *
 * @param [in] hDlg �_�C�A���O�̃n���h��
 * @param [in] msg ���b�Z�[�W
 * @param [in] wparam ���b�Z�[�W�̒ǉ����
 * @param [in] lparam ���b�Z�[�W�̒ǉ����
 */
BOOL CALLBACK DlgProc(HWND hDlg, UINT msg, WPARAM wparam, LPARAM lparam) {
	switch (msg) {
		case WM_COMMAND:
			if (LOWORD(wparam) == IDCANCEL) {
				InterlockedCompareExchange(&cancel_flag, TRUE, FALSE);
			}
			return TRUE;
	}
	return FALSE;
}

/**
 * �u���p�̃X���b�h�B
 *
 * @param [in] vdParam �u�����X�g
 * @return ��� 0
 */
static void _cdecl replace_thread(LPVOID vdParam) {
	int rc;
	WCHAR *pszIn = (WCHAR *)vdParam;
	rc = replace_all(pszIn, wcslen(pszIn));
	rc = teardown();
	replace_cancel();
}

extern "C" DWORD _cdecl HidemaruFilterGetVersion() {
	return VERSION;
}

extern "C" HIDEMARUFILTERINFO* _cdecl EnumHidemaruFilter() {
	static struct HIDEMARUFILTERINFO aFilterInfo[] = {
		{ sizeof(HIDEMARUFILTERINFO), "Replace", "FastGrepReplace", "FastGrepReplace", '\t', 0, 0, 0 },
		{ sizeof(HIDEMARUFILTERINFO), "Setup", "FastGrepReplace", "FastGrepReplace", '\t', 0, 0, 0 },
		{ 0, NULL, NULL, NULL, NULL, 0, 0, 0 }
	};
	return aFilterInfo;
}

extern "C" HGLOBAL _cdecl Setup( HWND hwndHidemaru, WCHAR* pwszIn, char* pszParam, int cbParamBuffer ) {
	const size_t mem_size = 128;
	static bool isInitProgress = false;
	HGLOBAL hGlobal = GlobalAlloc(LMEM_MOVEABLE, mem_size);
	if (hGlobal == NULL) {
		wchar_t sz[256];
		swprintf_s(sz, L"FastGrepReplace.hmf: Not enough memory, size=%u error-code=%u", mem_size, GetLastError());
		MessageBox(hwndHidemaru, sz, L"FastGrepReplace", MB_ICONHAND | MB_OK);
		return NULL;
	}
	wchar_t *result = (wchar_t *)GlobalLock(hGlobal);
	wcscpy_s(result, mem_size / sizeof(wchar_t), L"ERROR");
	int rc = setup(hwndHidemaru, (const wchar_t *)pwszIn);
	if (rc == ERROR_SUCCESS) {
		wcscpy_s(result, mem_size / sizeof(wchar_t), L"SUCCESS");
	}
	GlobalUnlock(hGlobal);
	return hGlobal;
}

extern "C" HGLOBAL _cdecl Replace( HWND hwndHidemaru, WCHAR* pwszIn, char* pszParam, int cbParamBuffer ) {
	wchar_t errmsg[1024] = { '\0' };
	cancel_flag = FALSE;
	forceProcessLongLine = FALSE;
	editDialogResult = -1;
	hWndDlg = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), hwndHidemaru, DlgProc);
	if (hWndDlg) {
		ShowWindow(hWndDlg, SW_SHOW);
		if (_beginthread(replace_thread, sizeof(void *), pwszIn) != NULL) {
			MSG msg;
			BOOL b;
			while (hWndDlg && (b = GetMessage(&msg, NULL, 0, 0))) {
				if (b == -1) {
					break;
				}
				if (!IsDialogMessage(hWndDlg, &msg)) {
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
			}
		} else {
			wcscpy_s(errmsg, L"�u���p�̃X���b�h�������ł��܂���ł����B");
		}
	}

	if (errmsg[0]) {
		MessageBox(hwndHidemaru, errmsg, L"FastGrepReplace", MB_OK | MB_ICONHAND);
	}
	return NULL;
}

#ifdef _DEBUG

#include "test.h"

#endif
