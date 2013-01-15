#define STRICT
#include "Registory.h"
#pragma intrinsic(memset, memcpy)

#define REG_ROOT_COUNT (9) ///< ルートレジストリの数。

/**
 * レジストリルートを表す構造体。
 */
struct {
	char *name;		///< ルート名称。
	char *alias;	///< エイリアス。
	HKEY key;		///< レジストリキー。
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
 * ルートレジストリ名からルートレジストリキーを取得する。
 *
 * 渡されたルートレジストリ名が不正な場合、この関数は NULL を返す。
 *
 * @param [in] root_name ルートレジストリ名、またはエイリアス名
 * @return キーのハンドル
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
 * レジストリをオープンする。
 *
 * @param [in] regpath オープンするレジストリのパス
 * @param [in] access セキュリティアクセスマスク
 * @param [out] regname レジストリキー名
 * @return キーのハンドル
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
	
	// パスからルートキーを取得する
	token = strtok(buf, "\\");
	strcpy(root_name, token);
	root_length = strlen(root_name);
	
	// サブキーおよびレジストリキーを取得する
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
	
	/* サブキーをオープンする */
	if ((err = RegOpenKeyEx(
				root,		// キーのハンドル
				subkeypath,	// オープンするサブキーの名前
				0,			// 予約（0を指定）
				access,		// セキュリティアクセスマスク
				&subkey		// ハンドルを格納する変数のアドレス
			)) != 0) {
		SetLastError(err);
		return NULL;
	}
	return subkey;
}


/**
 * レジストリをクローズする。
 *
 * クローズしようとしているレジストリが定義済みのレジストリの場合は
 * クローズせず、この関数は何も行なわない。
 *
 * @param [in] subkey キーのハンドル
 */
static void close_reg(HKEY subkey) {
	int i;
	for (i = 0; i < REG_ROOT_COUNT; i++) {
		if (subkey == reg_root[i].key) {
			// 定義済みのキーの場合クローズする必要はない
			return;
		}
	}
	// 定義済みのキーではないのでクローズする
	RegCloseKey(subkey);
}

/**
 * レジストリキーの値を返す。
 *
 * 存在しないレジストリキーを指定した場合は、この関数は NULL を返す。
 *
 * この関数は呼び出し元で必ず戻り値の領域を free で開放しなければならない。
 *
 * @param [in] subkey キーのハンドル
 * @param [in] key レジストリキー
 * @param [out] type レジストリ型
 * @li REG_NONE 0 定義されていない型。
 * @li REG_SZ 1 ヌル終端文字列。
 * ANSI 版の関数と Unicode 版の関数のどちらを使用しているかにより、
 * ANSI 文字列または Unicode 文字列になる
 * @li REG_EXPAND_SZ 2 展開前の環境変数への参照 (例えば“%PATH%”など) が入ったヌル終端文字列。
 * ANSI 版の関数と Unicode 版の関数のどちらを使用しているかにより、
 * ANSI 文字列または Unicode 文字列になる。
 * 環境変数への参照を展開するには ExpandEnvironmentStrings 関数を使う
 * @li REG_BINARY 3 バイナリデータ
 * @li REG_DWORD 4 32 ビットの数値
 * @li REG_DWORD_LITTLE_ENDIAN 4 リトルエンディアン形式の 32 ビット数値。
 * REG_DWORD と同等
 * @li REG_DWORD_BIG_ENDIAN 5 ビッグエンディアン形式の 32 ビット数値
 * @li REG_LINK 6 Unicode シンボリックリンク
 * @li REG_MULTI_SZ 7 ヌル終端文字列の配列。
 * それぞれの文字列がヌル文字で区切られ、連続する 2 つのヌル文字が配列の終端を表す
 * @li REG_RESOURCE_LIST 8 デバイスドライバのリソースリスト
 * @li REG_QWORD 11 64 ビット数値
 * @li REG_QWORD_LITTLE_ENDIAN 11 リトルエンディアン形式の 64 ビット数値。
 * REG_QWORD と同等
 * @retval NULL レジストリの取得に失敗した場合
 * @retval 上記以外 レジストリの値
 */
static void *get_val(HKEY subkey, char *key, PDWORD type) {
	void *data = NULL;
	DWORD sz = 0;
	LONG err;
	
	/* データサイズを求める */
	if ((err = RegQueryValueEx(
				subkey,	// 現在オープンしているキーのハンドル
				key,	// 取得する値の「名前」が入った文字列へのポインタ
				NULL,	// 予約パラメータ。NULLを指定する
				type,	// 値の「種類」を受け取る
				NULL,	// 値の「データ」を受け取る。NULLを指定することも可能だが、データは受け取れない
				&sz		// lpDataのサイズを指定する。関数実行後はlpDataにコピーされたデータのサイズになる
			)) != 0) {
		// 存在しないレジストリキーを指定した場合は NULL を返す。
		return NULL;
	}
	
	
	
	/* データ格納領域を確保する */
	if ((data = malloc((size_t)sz)) == NULL) {
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		return NULL;
	}
	
	/* データを取得する */
	if ((err = RegQueryValueEx(subkey, key, NULL, type, (LPBYTE)data, &sz)) != 0) {
		SetLastError(err);
		free(data);
		return NULL;
	}
	return data;
}

/**
 * レジストリ値を取得する。
 *
 * 戻り値のポインタは呼び出し元が必ず開放しなければならない。
 *
 * 存在しないレジストリパスを指定した場合、NULL を返す。
 *
 * @param [in] path レジストリパス
 * @param [out] type レジストリ型
 * @return レジストリ値
 */
void *Registory_GetValue(const char *path, PDWORD type) {
	HKEY subkey;
	char *regname;
	void *value = NULL;
	
	// サブキーをオープンする
	if ((subkey = open_reg(path, KEY_QUERY_VALUE, &regname)) == 0) {
		return NULL;
	}
	
	// 値を取得する
	if ((value = get_val(subkey, regname, type)) == NULL) {
		return NULL;
	}
	
	// サブキーをクローズする
	close_reg(subkey);
	
	return value;
}

/**
 * レジストリ値を設定する。
 *
 */
LONG Registory_SetValue(const char *path, DWORD type, CONST BYTE *data, DWORD size) {
	HKEY subkey;
	char *regname;

	// サブキーをオープンする
	if ((subkey = open_reg(path, KEY_SET_VALUE, &regname)) == 0) {
		return !ERROR_SUCCESS;
	}

	// 値を書き込む
	LONG rc = RegSetValueEx(
		subkey,			// HKEY hKey,           // 親キーのハンドル
		regname,		// LPCTSTR lpValueName, // レジストリエントリ名
		0,				// DWORD Reserved,      // 予約済み
		type,			// DWORD dwType,        // レジストリエントリのデータ型
		data,			// CONST BYTE *lpData,  // レジストリエントリのデータ
		size			// DWORD cbData         // レジストリエントリのデータのサイズ
	);

	// サブキーをクローズする
	close_reg(subkey);

	return rc;
}



