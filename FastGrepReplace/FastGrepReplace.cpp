#include "stdafx.h"
#pragma intrinsic(memset, memcpy)

#define VERSION		((1 << 17) + (0 << 8) + 50)		// 2.50  （バージョンアップ時に必ずここを修正する）

#define HIDEMARU_GREP_MAX_COLUMN 2048

extern "C" int nkf_kanji_code_detect(const wchar_t *filename, char **kanji_code);
extern "C" int nkf_main(int argc, char *argv[]);

typedef std::string bstring;

static void add_log(const wchar_t *log);

const unsigned char BOM_UTF8[] = {0xEF, 0xBB, 0xBF};	///< UTF-8 の BOM。
const unsigned char BOM_UTF16BE[] = {0xFE, 0xFF};		///< UTF-16BE の BOM。
const unsigned char BOM_UTF16LE[] = {0xFF, 0xFE};		///< UTF-16LE の BOM。
const wchar_t CRLF[] = {0x0D, 0x0A};					///< CRLF。

static HINSTANCE libiconv_dll;				///< libiconv の DLL のインスタンスハンドル。
static LIBICONV_OPEN libiconv_open;			///< iconv_open 関数。
static LIBICONV libiconv;					///< iconv 関数。
static LIBICONV_CLOSE libiconv_close;		///< iconv_close 関数。

static HMODULE hInstance;					///< プロセスのインスタンス。
static HWND hWndDlg;						///< ダイアログのウィンドウハンドル。
static HWND hWndHidemaru;					///< 秀丸のウィンドウハンドル。
static DWORD thread_id;						///< スレッド ID。

/**
 * このモジュールのあるフォルダパス。
 *
 * 最後にファイルセパレータ「\」が付いている。
 */
// TODO:
static std::wstring szModuleDirName;

static wchar_t editDlgMessage[256]; // 256 は必要十分な大きさ
static const wchar_t *editDlgFilePath;
static const wchar_t *editDlgLineBefore;
static const wchar_t *editDlgLineAfter;
static wchar_t *editDlgLineWork;

/**
 * キャンセルされたかどうかを判定するフラグ。
 * FALSE なら続行、TRUE ならキャンセル。
 */
static LONG cancel_flag;

static DWORD done_file_count;		///< 置換したファイル数。
static DWORD skip_file_count;		///< スキップしたファイル数。
static DWORD skip_line_count;		///< スキップした行数。
static DWORD count_change_line;		///< 変換した行数。

static int editDialogResult;
static LRESULT forceProcessLongLine;

typedef enum {
	R_MUSTNOT_SAVE = 1,			///< ファイルを保存する必要がない（変更されていない）。
	R_SUCCESS = 0,				///< 置換に成功した。
	R_INTERNAL_ERROR = -1,		///< 内部的なエラー。
	R_INVALID_FORMAT = -2,		///< 入力フォーマットが不正。
	R_FILE_SAVE_ERROR = -3,		///< ファイルの保存に失敗した。
	R_MAYBE_BINARY_FILE = -4,	///< ファイルがバイナリの可能性。
	R_LINE_NOT_EXISTS = -5,		///< 存在しない行なので置換しなかった。
	R_CONVERT_ERROR = -6,		///< 文字コードの変換に失敗した。
	R_LINE_IS_TOO_LONG = -7,	///< 行が HIDEMARU_GREP_MAX_COLUMN 以上。
} FGR_RC;

/**
 * 最後に発生したエラーメッセージを保持するバッファ。
 * サイズは適当で、メッセージが長い場合は切れてしまう。
 */
static wchar_t last_error_message[4096];

/**
 * マクロを動作させているファイルのベースディレクトリ。
 */
static wchar_t basedir[FILENAME_MAX + 1];

/**
 * 経過時間を記憶する変数。
 */
static DWORD tickcount;

/**
 * 最後に発生したエラーメッセージを保持する。
 *
 * GetLastError で取得したエラーメッセージを
 * last_error_message に保持する。
 *
 * last_error_message の内容は次回呼び出し時まで有効。
 *
 * @todo
 * 実はこの時点でログに吐いてもいいんじゃないか？
 */
static void StockLastError() {
	FormatMessage(
		FORMAT_MESSAGE_FROM_SYSTEM,		// 入力元と処理方法のオプション
		NULL,							// メッセージの入力元
		GetLastError(),					// メッセージ識別子
		LANG_USER_DEFAULT,				// 言語識別子
		last_error_message,				// メッセージバッファ
		sizeof(last_error_message),		// メッセージバッファの最大サイズ
		NULL							// 複数のメッセージ挿入シーケンスからなる配列
	);
}

/**
 * エラーメッセージを表示する。
 *
 * メッセージを表示すると、ストックされていたメッセージはクリアされる。
 */
static void msgbox() {
	MessageBox(NULL, last_error_message, L"FastGrepReplace", MB_ICONHAND | MB_OK);
	last_error_message[0] = '\0';
}

/**
 * エラーメッセージを表示する。
 *
 * システムのエラーメッセージとの間に自動的に改行が付与される。
 *
 * @param [in] append_msg GetLastError に追加するメッセージ
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
 * エラーメッセージを表示する。
 *
 * システムのエラーメッセージとの間に自動的に改行が付与される。
 *
 * @param [in] append_msg GetLastError に追加するメッセージ
 */
static void msgbox(const wchar_t *append_msg) {
	msgbox(std::wstring(append_msg));
}

/**
 * kcode_t の値を元に iconv で使用可能な名前を返す。
 *
 * @param [in] kanji_code 対象の kcode_t
 * @return エンコーディング名
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
 * kcode_t の名前を返す。
 *
 * @param [in] kanji_code 対象の kcode_t
 * @param [out] buf 名前を格納するバッファ
 * @param [in] size バッファのサイズ
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
 * とても長い行の編集ダイアログボックスのプロシージャ。
 *
 * @param [in] hDlg ダイアログのハンドル
 * @param [in] msg メッセージ
 * @param [in] wparam メッセージの追加情報
 * @param [in] lparam メッセージの追加情報
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
 * 行を管理するためのクラスです。
 *
 * このクラスは行本体を表す value と改行を表す lf をメンバに持ち、
 * データ構造としてはいずれも std::string となる。
 *
 * 行毎に改行コードを持つのは、改行コードが混在した
 * テキストファイルを許容させたいためである。
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
	 * コンストラクタ。
	 *
	 * @param [in] value 行本体。NULL ポインタならブランク行
	 * @param [in] size 行本体のバイト数
	 * @param [in] linefeed 改行コード。NULL なら改行なし
	 * @param [in] lflen 改行コードのバイト数
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
	 * デストラクタ。
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
	 * 文字数を返す。改行は含まない。
	 */
	size_t length() {
		return toWstr().size();
	}

	/**
	 * 改行を含むバイト数を返す。
	 */
	size_t size() {
		return sizeWithoutLf(false);
	}

	/**
	 * バイト数を返す。改行を含むかどうかは引数で指定する。
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
	 * 秀丸の grep 字のバイトサイズを返す。
	 *
	 * 秀丸の仕様として、grep 結果には 2047 バイトまでしか出力されず、
	 * その計算方法は「文字列を CP932 として扱い、CP932 に存在しない文字は
	 * 4 バイトとして計算する」というもの。
	 *
	 * このメソッドは上記計算式に基づきバイトサイズを返す。
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
	 * 行の本体。
	 *
	 * 空行の場合はサイズが 0になる。
	 */
	bstring value;

	/**
	 * この行の改行コード。
	 *
	 * 改行がない（ファイルの最後の行の）場合、サイズが 0。
	 */
	bstring lf;

	std::wstring *wcvalue;
	std::wstring *wclf;

	std::wstring wstr;
};

/**
 * ストリームへ出力する。
 *
 * 行本体と改行コードを続けて出力する。
 * 
 * @param [in] os 出力先
 * @param [in] line 出力するインスタンス
 * @return 出力した状態のストリーム
 */
//std::ostream& operator<<(std::ostream& os, Line& line) {
//	return os << line.value.c_str() << line.lf.c_str();
//}

/**
 * ファイルを表すクラス。
 */
class TextFile {
private:
	libiconv_t cd;	///< libiconv の文字コード変換ディスクリプタ。
	bool isModified; ///< 変更されたかどうか。
public:
	/**
	 * 唯一のコンストラクタ。
	 *
	 * @param [in] path ファイルのパス
	 * @param [in] text ファイルのテキスト。バイナリの場合もある
	 * @param [in] size ファイルのサイズ
	 */
	TextFile(const wchar_t *path, char *text, DWORD size) : size(size), kanji_code(KC_UNKNOWN), isModified(false) {
		cd = NULL;
		this->path = path;
		if (size) {
			this->text = (char *)malloc(size + 6); // + 6 は UTF-8 判定のために使用
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
	 * デストラクタ。
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
	 * このファイルの行を初期化する。
	 *
	 * このファイルを改行を区切りに行単位の構造に分割し、
	 * 行指定による書き換えを容易にする。
	 *
	 * 初期化にあたり文字コードを指定する必要がある。
	 * これは、文字コードによって改行コードを判別するためである。
	 *
	 * @param [in] kanji_code このファイルの文字コード
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
			// 失敗はありえない
			wchar_t encodingName[32];
			size_t len = strlen(encoding_name);
			for (size_t i = 0; i < len; i++) {
				encodingName[i] = encoding_name[i];
			}
			encodingName[len] = '\0';
			std::wstring buf1(L"◆変換ディスクリプタ生成エラーが発生しました。\n");
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

		// BOM を退避
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

		// 改行コードの判別
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
			// 改行がひとつも存在しないファイル
			lines.push_back(new Line(this, sol, sizeWithoutBom - (sol - sof)));
		} else if (!eol_is_eof) {
			// または最後の行が改行ではないファイル
			lines.push_back(new Line(this, sol, sizeWithoutBom - (sol - sof)));
		} else {
			// 最後の行が改行で終わっているファイルなら、ブランク行を追加する（一応行番号がついてるから）
			lines.push_back(new Line(this, (char *)NULL, 0));
		}
	}

	/**
	 * このファイルを元のファイルに上書きして保存する。
	 *
	 * ファイルが変更されていない場合は保存せず #R_MUSTNOT_SAVE を返す。
	 *
	 * この関数内で処理に失敗した場合は #StockLastError を呼び出し
	 * エラーメッセージを保存する。
	 *
	 * @param [in] force 強制的に保存するかどうか。
	 * @retval #R_SUCCESS ファイルの保存に成功した場合
	 * @retval #R_MUSTNOT_SAVE ファイルを保存する必要がない場合
	 * @retval #R_FILE_SAVE_ERROR ファイルの保存に失敗した場合
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
	 * このファイルを保存する。
	 *
	 * 指定されたパスにファイルを保存する。
	 * 指定パスにファイルが存在する場合、このファイルの内容で上書きする。
	 *
	 * この関数内で処理に失敗した場合は #StockLastError を呼び出し
	 * エラーメッセージを保存する。
	 *
	 * @param [in] save_path ファイルを保存するパス
	 * @retval #R_SUCCESS ファイルの保存に成功した場合
	 * @retval #R_FILE_SAVE_ERROR ファイルの保存に失敗した場合
	 */
	FGR_RC save(const wchar_t *save_path) {
		wchar_t num1[20];
		HANDLE file1;
		for (int i = 0; i < 3; i++) {
			file1 = CreateFile(save_path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
			if (file1 == INVALID_HANDLE_VALUE) {
				StockLastError();
				Sleep(20); // ちょっと待てば開けるようになる可能性が高い
			} else {
				break;
			}
		}
		if (file1 == INVALID_HANDLE_VALUE) {
			std::wstring buf1(L"◆保存先のファイルが開けませんでした。\n");
			buf1 += save_path;
			add_log(buf1.c_str());
			swprintf_s(num1, L"%d", ++skip_file_count);
			SetDlgItemText(hWndDlg, IDC_STATIC_SKIP_FILE_COUNT, num1);
			return R_FILE_SAVE_ERROR;
		}
		DWORD write_size;
		FGR_RC rc = R_SUCCESS;

		// 退避した BOM を書き込み
		if (kanji_code == KC_UTF8) {
			if (!WriteFile(file1, bom, 3, &write_size, NULL)) {
				StockLastError();
				std::wstring buf1(L"◆UTF-8のBOMの書き込みに失敗しました。\n");
				buf1 += save_path;
				add_log(buf1.c_str());
				swprintf_s(num1, L"%d", ++skip_file_count);
				SetDlgItemText(hWndDlg, IDC_STATIC_SKIP_FILE_COUNT, num1);
				rc = R_FILE_SAVE_ERROR;
			}
		} else if (kanji_code == KC_UTF16BE || kanji_code == KC_UTF16LE) {
			if (!WriteFile(file1, bom, 2, &write_size, NULL)) {
				StockLastError();
				std::wstring buf1(L"◆UTF-16のBOMの書き込みに失敗しました。\n");
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
					std::wstring buf1(L"◆行の書き込みに失敗しました。\n");
					buf1 += path;
					buf1 += L"(";
					swprintf_s(num1, L"%d", i + 1);
					buf1 += L"): ";

					libiconv_t cd = libiconv_open("UTF-16LE", get_charset_name(kanji_code));
					size_t in_length = out_length;
					char *c_str = buf;
					out_length = in_length * 3 + 1 + 1; // 3 倍してるのは UTF-8 のケースを想定、+1 は NULL Terminate、+1 は JIS の場合の改行
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
			std::wstring buf1(L"◆ファイルを閉じるのに失敗しました。\n");
			buf1 += save_path;
			add_log(buf1.c_str());
			rc = R_FILE_SAVE_ERROR;
			// ここはスキップファイルをカウントしない。
			//sprintf(num1, "%d", ++skip_file_count);
			//SetDlgItemText(hWndDlg, IDC_STATIC_SKIP_FILE_COUNT, num1);
		}
		return rc;
	}

	/**
	 * 2048 バイト以上の問い合わせを出すか出さないかを判断する。
	 *
	 * 秀丸の仕様で、grep 結果は Shift_JIS 換算したバイト数（ただし
	 * Shift_JIS に含まれない文字は 4 バイト扱い）で 2047 バイトまでを出力する。
	 *
	 * @param [in] line 対象行
	 * @return 2048 バイト以上なら true、そうでないなら false
	 */
	bool hasPrompt(Line *line) const {
		bool rc = false;
		switch (line->parent->kanji_code) {
			case KC_ASCII: // TODO: ASCII が UTF-8 として換算されてしまう
			case KC_EUCorSJIS:
			case KC_JIS:
			case KC_EUC:
			case KC_SJIS:
			case KC_UTF7: // UTF-7 は秀丸では grep できない
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
	 * 行を指定して置換を行う。
	 * 
	 * @param [in] line_no 行番号。最初の行なら 1。インデックスではないので注意
	 * @param [in] value 置換後の文字列。UTF-16LE で渡される
	 * @param [in] isReplaceLongLine 長い行を置換する場合は true、そうでない場合は false
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
				swprintf_s(editDlgMessage, L"置換対象行（%d）が規定（%dバイト）以上です。", line_no, HIDEMARU_GREP_MAX_COLUMN);
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
					// JIS の場合は改行コードがないとエスケープが閉じない
					if (kanji_code == KC_JIS) {
						value += L"\n";
					}
					const wchar_t *c_str = value.c_str();
					size_t in_length = value.size();
					size_t out_length = in_length * 3 + 1 + 1; // 3 倍してるのは UTF-8 のケースを想定、+1 は NULL Terminate、+1 は JIS の場合の改行
					in_length *= sizeof(wchar_t);
					char *buf = (char *)malloc(out_length);
					char *outptr = buf;
					memset(buf, '\0', out_length);
					size_t result = libiconv(cd, (const char **)&c_str, &in_length, &outptr, &out_length);
					if (result == (size_t)-1) {
						std::wstring buf1(L"◆文字コードの変換に失敗しました（UTF-16LE ⇒ ");
						buf1 += get_charset_nameW(kanji_code);
						buf1 += L"）。\n";
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
	 * このオブジェクトと渡されたオブジェクトを比較する。
	 *
	 * @param [in] right 比較対象のオブジェクト
	 * @return 内容が同一なら 0、異なるなら 0 以外
	 */
	int compare(TextFile *right) {
		int rc = 0;
		if (right == NULL) {
			MessageBox(NULL, L"rightがNULL", L"NULL", MB_OK);
			rc = -1;
		} else {
			size_t size = this->lines.size();
			size_t rsize = right->lines.size();
			if (this->lines.size() != right->lines.size()) {
				MessageBox(NULL, right->path.c_str(), L"行数が違う", MB_OK);
				rc = -1;
			} else if (this->kanji_code != right->kanji_code) {
				MessageBox(NULL, right->path.c_str(), L"文字コードが違う", MB_OK);
				rc = -1;
			} else {
				for (size_t j = 0; j < this->lines.size(); j++) {
					size_t aSize = this->lines[j]->size();
					size_t eSize = this->lines[j]->size();
					if (aSize != eSize) {
						MessageBox(NULL, this->path.c_str(), L"行のサイズが違う", MB_OK);
						rc = -1;
					} else {
						char *aData = this->lines[j]->data();
						char *eData = right->lines[j]->data();
						if (memcmp(aData, eData, aSize) != 0) {
							MessageBox(NULL, this->path.c_str(), L"行の内容が違う", MB_OK);
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

	std::wstring path;			///< このテキストファイルのパス。
	char *text;					///< このテキストファイルのテキスト。
	unsigned char bom[3];		///< このテキストファイルのバイトオーダーマーク。
	DWORD size;					///< このテキストファイルのサイズ。初期化中のみ有効。ファイルを書き換えた後は意味がない。
	kcode_t	kanji_code;			///< このテキストファイルの文字コード。
	std::vector<Line *> lines;	///< 行単位でこのファイルのテキストを保持する。
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
				size_t out_length = in_length * 3 + 1 + 1; // 3 倍してるのは UTF-8 のケースを想定、+1 は NULL Terminate、+1 は JIS の場合の改行
				wchar_t *buf = (wchar_t *)malloc(out_length);
				char *outptr = (char *)buf;
				memset(buf, '\0', out_length);
				size_t result = libiconv(cd, (const char **)&c_str, &in_length, &outptr, &out_length);
				if (result == (size_t)-1) {
					std::wstring buf1(L"◆文字コードの変換に失敗しました（UTF-16LE ⇒ ");
					buf1 += get_charset_nameW(kanji_code);
					buf1 += L"）。\n";
					buf1 += parent->path;
					buf1 += L"\n";
					MessageBox(NULL, L"変換失敗", buf1.c_str(), MB_OK);
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
 * 現在処理中のテキストファイルの構造を表す唯一のオブジェクト。
 *
 * 処理の前提として、同じファイルへの変更は連続して実行されるため、
 * 変更対象のファイルが切り替わるタイミングでこのオブジェクトは開放され
 * 次のテキストファイルを表すオブジェクトへと参照を切り替える。
 */
static TextFile *file;

/**
 * ログファイル。
 *
 * 処理中にエラーが発生した場合は、特定のフォーマットでここにエラー内容が吐き出される。
 */
static TextFile *logfile;

/**
 * ログを出力する。
 *
 * ログを出力する直前にエラーメッセージが保管されていたら
 * ログの後にエラーメッセージを出力する。
 *
 * @param [in] 出力するメッセージ
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
 * ファイルを読み込み構造化する。
 *
 * @param [in] filename 読み込むファイルのパス
 * @param [out] textfile 読み込んだファイルを構造化するクラスのインスタンスへのポインタ
 * @retval ERROR_FILE_NOT_FOUND ファイルが存在しない場合
 * @retval ERROR_INTERNAL_ERROR 内部処理でエラーが発生
 * @retval 上記以外 処理が成功した場合
 */
static int load_file(const wchar_t *filename, TextFile **textfile) {
    HANDLE file1;
    HANDLE map1;
    DWORD hsize1, lsize1; // lsize1 がファイルのサイズ
    char *view1;
    std::wstring mapstr(L"filemapping"); // ユニークな文字列を指定する
    wchar_t num[10] = {0}; // 一回につき 10 億ファイルまで OK
    static int count = 0;
	swprintf_s(num, L"%d", count++); // ユニークなマップファイル名を作るための処理
    mapstr += num;
	wchar_t num1[20];

    // ファイルの生成
    file1 = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    if (file1 == INVALID_HANDLE_VALUE) {
        // ファイルが存在しない
		StockLastError();
		std::wstring buf1(L"◆ファイルを開けませんでした。\n");
		buf1 += filename;
		add_log(buf1.c_str());
		swprintf_s(num1, L"%d", ++skip_file_count);
		SetDlgItemText(hWndDlg, IDC_STATIC_SKIP_FILE_COUNT, num1);
		return ERROR_FILE_NOT_FOUND;
    }

    // サイズの取得
    lsize1 = GetFileSize(file1, &hsize1);

	// サイズがゼロのテキストファイルも考慮に入れる
	if (lsize1 || hsize1) {
		// メモリマップドファイルの生成
		map1 = CreateFileMapping(file1, NULL, PAGE_READONLY, hsize1, lsize1, mapstr.c_str());
		if (map1 == NULL) {
			// エラー処理
			StockLastError();
			std::wstring buf1(L"◆メモリマップドファイルの作成に失敗しました。\n");
			buf1 += filename;
			add_log(buf1.c_str());
			if (!CloseHandle(file1)) {
				StockLastError();
				buf1 = L"◆ファイルのクローズに失敗しました。\n";
				buf1 += filename;
				add_log(buf1.c_str());
			}
			swprintf_s(num1, L"%d", ++skip_file_count);
			SetDlgItemText(hWndDlg, IDC_STATIC_SKIP_FILE_COUNT, num1);
			return ERROR_INTERNAL_ERROR;
		}
	    
		// マップビューの生成
		view1 = (char *)MapViewOfFile(map1, FILE_MAP_READ, 0, 0, 0);
		if (view1 == NULL) {
			// エラー処理
			StockLastError();
			std::wstring buf1(L"◆マップビューの作成に失敗しました。\n");
			buf1 += filename;
			add_log(buf1.c_str());
			if (!CloseHandle(map1)) {
				StockLastError();
				buf1 = L"◆メモリマップドファイルのクローズに失敗しました。\n";
				buf1 += filename;
				add_log(buf1.c_str());
			}
			if (!CloseHandle(file1)) {
				StockLastError();
				buf1 = L"◆ファイルのクローズに失敗しました。\n";
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
		// アンマップ
		if (!UnmapViewOfFile(view1)) {
			StockLastError();
			std::wstring buf1(L"◆ビューのクローズに失敗しました。\n");
			buf1 += filename;
			add_log(buf1.c_str());
		}
		if (!CloseHandle(map1)) {
			StockLastError();
			std::wstring buf1(L"◆メモリマップドファイルのクローズに失敗しました。\n");
			buf1 += filename;
			add_log(buf1.c_str());
		}
	}
	if (!CloseHandle(file1)) {
		StockLastError();
		std::wstring buf1(L"◆ファイルのクローズに失敗しました。\n");
		buf1 += filename;
		add_log(buf1.c_str());
	}

	return ERROR_SUCCESS;
}

/**
 * 文字コードを判別する。
 *
 * @param [in] text 対象のテキスト
 * @param [in] size テキストのサイズ（バイト数）
 * @retval KC_DATA バイナリデータ
#if 0
 * @retval KC_BROKEN おそらくバイナリ
#endif
 * @retval KC_UNKNOWN 不明（Latin-1 とか）
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
	
	// サイズが 1 で 0x00 なら、バイナリと判断する
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
				utf8nums = 3; // BOM の分だけスキップする
			}
		}
	}

	if (size % 2 == 1) {
		score_utf16_cp932 = -1;
		score_utf16_eucjp = -1;
		utf16bom = '\0';
	}

	buf = (unsigned char *)malloc(size + 6); // +6 は UTF-8 のためのバッファ
	memcpy(buf, text, size);
	buf[size] = buf[size + 1] = buf[size + 2] = buf[size + 3] = buf[size + 4] = buf[size + 5] = '\0';

	// 最初が 0x00, 0x00 ならバイナリと判断する
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
			if (!(i == 0 && utf16bom_cp932)) { // ファイルの先頭で BOM がある場合は BOM をスキップ
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
			if (!(i == 0 && utf16bom_eucjp)) { // ファイルの先頭で BOM がある場合は BOM をスキップ
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
 * ファイルの文字コードを判定する。
 *
 * この関数は、NKF と WKF と独自の文字コード判定処理を組み合わせて
 * 可能な限り正確にファイルの文字コードを判定する。
 *
 * - EUC-JP か Shift_JIS に迷った場合は、Shift_JIS と判定する
 * - UTF-7 は ASCII と判定される（が、そもそも秀丸の grep で UTF-7 は対象にならない）
 * - UTF-16 は CP932 の文字範囲を含むテキストであれば BOM がなくても判定可能
 *
 * @param [in] filename 対象のファイル
 * @param [out] textfile 読み込んだファイルを構造化して保持するクラスへのポインタ
 * @retval kcode_t で定義されている値のすべて
 */
static kcode_t detect_kanji_code(const wchar_t *filename, TextFile **textfile) {
	char *nkf_friendly_name = ""; // NKF が判定した可読可能な文字コード名
	kcode_t nkf_code; // NKF が判定した文字コード（nkf_friendly_name から導いている）
	kcode_t wkf_code; // WKF が判定した文字コード
	kcode_t fgr_code; // FGR が判定した文字コード
	kcode_t kanji_code = KC_UNKNOWN;  // 総合的に判断した文字コード

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
					nkf_code = KC_UNKNOWN; // おそらく NKF の誤判定
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
		kanji_code = KC_UTF8N; // サイズ 0 のテキストは UTF-8N にする
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
 * libiconv を初期化する。
 *
 * @retval true: 初期化に成功
 * @retval false: 初期化に失敗
 */
static bool init_libiconv() {
	if (libiconv_dll == NULL) {
		std::wstring libiconv_path(szModuleDirName);
		libiconv_path += L"FastGrepReplace_libiconv-2.dll";
		libiconv_dll = LoadLibrary(libiconv_path.c_str());

		if (libiconv_dll == NULL) {
			add_log(L"◆ライブラリの初期化に失敗しました。");
			return false;
		}

		// 関数へのポインタ値を取得する
		libiconv_open = (LIBICONV_OPEN)GetProcAddress(libiconv_dll, "libiconv_open");
		libiconv = (LIBICONV)GetProcAddress(libiconv_dll, "libiconv");
		libiconv_close = (LIBICONV_CLOSE)GetProcAddress(libiconv_dll, "libiconv_close");
	}
	return true;
}

/**
 * 指定されたリストに従ってファイルを置換する。
 *
 * 渡されるリストは「filename(line): value」という形式であり、
 * 例えば「aaa.txt(19): abcde」ならば
 * aaa.txt の 19 行目を abcde に置き換える。
 *
 * リストが想定した形式以外の場合は、置換をせずに無視する。
 *
 * @param [in] item 置換対象リストの一行
 * @retval R_SUCCESS: 置換に成功した
 * @retval R_INTERNAL_ERROR: 内部的なエラー
 * @retval R_INVALID_FORMAT: 入力フォーマットが不正
 * @retval R_FILE_SAVE_ERROR: ファイルの保存に失敗した
 * @retval R_MAYBE_BINARY_FILE: ファイルがバイナリの可能性
 * @retval R_LINE_NOT_EXISTS: 存在しない行なので置換しなかった
 * @retval R_CONVERT_ERROR: 文字コードの変換に失敗した
 */
static FGR_RC replace_line(std::wstring& item) {
	size_t pos, off;
	wchar_t num1[20];

	if (item.size() == 0) {
		// 空行は無視する
		return R_INVALID_FORMAT;
	}

	if ((off = item.find(L"(")) == std::string::npos) {
		std::wstring buf1(L"◆フォーマットが不正です。\n");
		buf1 += item;
		add_log(buf1.c_str());
		return R_INVALID_FORMAT;
	}

	if ((pos = item.find(L"): ", off)) == std::string::npos) {
		std::wstring buf1(L"◆フォーマットが不正です。\n");
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
			std::wstring buf1(L"◆フォーマットが不正です。\n");
			buf1 += item;
			add_log(buf1.c_str());
			return R_INVALID_FORMAT;
		}
	}
	if (line_no <= 0) {
		std::wstring buf1(L"◆フォーマットが不正です。\n");
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

	// 前のファイルと同じではない場合は保存する
	if (file != NULL && filepath.compare(file->path) != 0) {
		FGR_RC rc;
		if ((rc = file->save()) == R_FILE_SAVE_ERROR) {
			std::wstring buf1(L"◆ファイルの保存に失敗しました。\n");
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
			std::wstring buf1(L"◆バイナリファイルっぽいので置換しませんでした。\n");
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
			std::wstring buf1(L"◆指定された行は存在しません。\n");
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
			std::wstring buf1(L"◆置換に失敗しました（このファイルの内容を見直してください）。\n");
			buf1 += filepath.c_str();
			buf1 += L"(";
			swprintf_s(num1, L"%d", line_no);
			buf1 += num1;
			buf1 += L"): ";
			buf1 += value;
			add_log(buf1.c_str());
// 以下のコメントアウトはおそらくバグなので、確認後削除する
//			swprintf_s(num1, L"%d", ++skip_file_count);
//			SetDlgItemText(hWndDlg, IDC_STATIC_SKIP_FILE_COUNT, num1);
			return R_CONVERT_ERROR;
		}

		case R_LINE_IS_TOO_LONG: {
			std::wstring buf1(L"◆対象行が長いので置換しませんでした。\n");
			buf1 += filepath.c_str();
			buf1 += L"(";
			swprintf_s(num1, L"%d", line_no);
			buf1 += num1;
			buf1 += L"): ";
			buf1 += value;
			add_log(buf1.c_str());
// 以下のコメントアウトはおそらくバグなので、確認後削除する
//			swprintf_s(num1, L"%d", ++skip_file_count);
//			SetDlgItemText(hWndDlg, IDC_STATIC_SKIP_FILE_COUNT, num1);
			return R_LINE_IS_TOO_LONG;
		}
	}
	return R_SUCCESS;
}

/**
 * まとめて置換する。
 *
 * この関数が呼び出される前に、呼び出し側の秀丸で全選択＋コピーが
 * 実行されていなければならない。
 *
 * 処理途中で何らかのエラーが発生しても、致命的なエラーでない場合は
 * そのエラーはログに吐かれ、処理は継続される。
 *
 * ログはこのモジュールと同じフォルダにある
 * FastGrepReplace.log という拡張子のファイルになる。
 *
 * @param [in] pszIn 置換リスト
 * @param [in] size 置換リストのサイズ
 * @return 処理に成功した場合は 0、そうでない場合は 0 以外
 */
int replace_all(WCHAR *pszIn, size_t size) {
	wchar_t num1[10];
	done_file_count = 0;
	skip_file_count = 0;
	skip_line_count = 0;
	wchar_t *context;
	wchar_t *token = wcstok_s(pszIn, L"\r\n", &context);
	long progress = -1; // 置換行が 65535 超過だった場合に使用
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
 * 終了処理を行う。
 *
 * 未保存のファイルがあれば、保存する。
 * 無ければ何もしない。
 *
 * @retval R_SUCCESS: 処理に成功した、または何もしなかった
 * @retval R_FILE_SAVE_ERROR: ファイルの保存に失敗した
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
				std::wstring buf1(L"◆ファイルの保存に失敗しました。");
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
		cancel_msg = L"キャンセルしましたが、";
	} else {
		cancel_msg = L"";
	}

	if (logfile->lines.size()) {
		swprintf_s(result, L"%s%d行置換しました（%d分 %d秒 %d）。\n置換しなかった行を表示しますか？", cancel_msg, count_change_line, min, sec, msec);
		type = MB_ICONQUESTION | MB_YESNO;
	} else {
		swprintf_s(result, L"%s%d行置換しました（%d分 %d秒 %d）。", cancel_msg, count_change_line, min, sec, msec);
		type = MB_OK;
	}

	if (!isCancel) {
		SetDlgItemText(hWndDlg, IDC_STATIC_CURRENT_FILE_NAME, L"");
	}
	if (MessageBox(hWndDlg, result, L"FastGrepReplace", type) == IDYES) {
		const wchar_t *msg = L"---- FastGrepReplaceで置換しなかったファイルと行番号など ----";
		logfile->lines.insert(logfile->lines.begin(), new Line(logfile, msg, wcslen(msg), CRLF, sizeof(CRLF)));
		if (logfile->save(true) == R_FILE_SAVE_ERROR) {
			msgbox(L"ログファイルの保存に失敗しました。");
		}
	}
	delete logfile;
	logfile = NULL;
	return rc;
}

/**
 * 開始準備を行う。
 *
 * - libiconv を初期化する
 * - 古いログファイルを削除する
 *
 * @param [in] 秀丸のウィンドウハンドル
 * @param [in] ベースディレクトリ
 * @retval ERROR_SUCCESS: 処理に成功した場合
 * @retval ERROR_INTERNAL_ERROR: 処理に失敗した場合
 */
int setup(HWND hWnd, const wchar_t *base) {
	hWndHidemaru = hWnd;
	last_error_message[0] = '\0';
	count_change_line = 0;
	file = NULL;
	tickcount = GetTickCount();

	// ログファイルのオープン
	std::wstring logpath(szModuleDirName);
	logpath += L"FastGrepReplace.log";
	logfile = new TextFile(logpath.c_str(), NULL, 0);
	logfile->bom[0] = BOM_UTF16LE[0];
	logfile->bom[1] = BOM_UTF16LE[1];
	logfile->bom[2] = '\0';
	logfile->kanji_code = KC_UTF16LE;

	// ログファイルは消しておく
	if (PathFileExists(logpath.c_str()) && !DeleteFile(logpath.c_str())) {
		StockLastError();
		std::wstring buf1(L"ログファイルが削除できませんでした\n手動で削除を試みてみてください。\n\n");
		buf1 += logpath.c_str();
		buf1 += L"\n";
		buf1 += last_error_message;
		last_error_message[0] = '\0';
		MessageBox(hWndHidemaru, buf1.c_str(), L"FastGrepReplace", MB_OK | MB_ICONWARNING);
		return ERROR_INTERNAL_ERROR;
	}

	if (!base) {
		add_log(L"◆ベースディレクトリの指定が不正（NULL）です。"); // ありえない
		return ERROR_INTERNAL_ERROR;
	} else if (!wcslen(base)) {
		// メニューから選択した場合（ありえない）
		MessageBox(hWndHidemaru, L"マクロから起動してください。\nメニューから起動しても何も行いません。", L"FastGrepReplace", MB_OK | MB_ICONINFORMATION);
		return ERROR_INTERNAL_ERROR;
	}
	if (!PathIsDirectory(base)) {
		std::wstring buf1(L"◆指定されたベースディレクトリはディレクトリではありません。\n"); // ありえない
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
 * この DLL のメインエントリポイント。
 *
 * プロセスにアタッチされたタイミングでモジュールハンドルを保持する。
 *
 * @param [in] hModule モジュールハンドル
 * @param [in] ul_reason_for_call 呼び出された理由
 * @return 常に TRUE
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
						// このエラーはあり得ない（はず）
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
					MessageBox(NULL, L"プログレスダイアログが初期化できませんでした。", L"FastGrepReplace", MB_ICONHAND | MB_OK);
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
 * 置換処理をキャンセルする。
 */
static void replace_cancel() {
	if (hWndDlg) {
		ShowWindow(hWndDlg, SW_HIDE);
		DestroyWindow(hWndDlg);
		hWndDlg = NULL;
	}
}

/**
 * プログレスダイアログボックスのプロシージャ。
 *
 * キャンセルボタンが押されたら、置換処理をその場でキャンセルする。
 *
 * @param [in] hDlg ダイアログのハンドル
 * @param [in] msg メッセージ
 * @param [in] wparam メッセージの追加情報
 * @param [in] lparam メッセージの追加情報
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
 * 置換用のスレッド。
 *
 * @param [in] vdParam 置換リスト
 * @return 常に 0
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
			wcscpy_s(errmsg, L"置換用のスレッドが生成できませんでした。");
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
