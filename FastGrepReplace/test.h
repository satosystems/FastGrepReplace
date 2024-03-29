wchar_t *tempFolder;

std::wstring copyToTempFolder(const wchar_t *path) {
	std::wstring filename = path;
	filename = filename.substr(filename.find_last_of(L"\\"));
	std::wstring save_path = tempFolder;
	save_path += filename;
	CopyFile(path, save_path.c_str(), FALSE);
	return save_path;
}

void test01() {
#define BASE L"X:\\works\\FastGrepReplace\\test\\"
#define SUB L"testdata\\"
	const wchar_t *list[] = {
		BASE SUB L"EUC-JP.CR.actual.txt",
		BASE SUB L"EUC-JP.CR.expect.txt",
		BASE SUB L"EUC-JP.CRLF.actual.txt",
		BASE SUB L"EUC-JP.CRLF.expect.txt",
		BASE SUB L"EUC-JP.LF.actual.txt",
		BASE SUB L"EUC-JP.LF.expect.txt",
		BASE SUB L"JIS.CR.actual.txt",
		BASE SUB L"JIS.CR.expect.txt",
		BASE SUB L"JIS.CRLF.actual.txt",
		BASE SUB L"JIS.CRLF.expect.txt",
		BASE SUB L"JIS.LF.actual.txt",
		BASE SUB L"JIS.LF.expect.txt",
		BASE SUB L"Shift_JIS.CR.actual.txt",
		BASE SUB L"Shift_JIS.CR.expect.txt",
		BASE SUB L"Shift_JIS.CRLF.actual.txt",
		BASE SUB L"Shift_JIS.CRLF.expect.txt",
		BASE SUB L"Shift_JIS.LF.actual.txt",
		BASE SUB L"Shift_JIS.LF.expect.txt",
		BASE SUB L"UTF-16BE_NOBOM.CR.actual.txt",
		BASE SUB L"UTF-16BE_NOBOM.CR.expect.txt",
		BASE SUB L"UTF-16BE_NOBOM.CRLF.actual.txt",
		BASE SUB L"UTF-16BE_NOBOM.CRLF.expect.txt",
		BASE SUB L"UTF-16BE_NOBOM.LF.actual.txt",
		BASE SUB L"UTF-16BE_NOBOM.LF.expect.txt",
		BASE SUB L"UTF-16BE_WITHBOM.CR.actual.txt",
		BASE SUB L"UTF-16BE_WITHBOM.CR.expect.txt",
		BASE SUB L"UTF-16BE_WITHBOM.CRLF.actual.txt",
		BASE SUB L"UTF-16BE_WITHBOM.CRLF.expect.txt",
		BASE SUB L"UTF-16BE_WITHBOM.LF.actual.txt",
		BASE SUB L"UTF-16BE_WITHBOM.LF.expect.txt",
		BASE SUB L"UTF-16LE_NOBOM.CR.actual.txt",
		BASE SUB L"UTF-16LE_NOBOM.CR.expect.txt",
		BASE SUB L"UTF-16LE_NOBOM.CRLF.actual.txt",
		BASE SUB L"UTF-16LE_NOBOM.CRLF.expect.txt",
		BASE SUB L"UTF-16LE_NOBOM.LF.actual.txt",
		BASE SUB L"UTF-16LE_NOBOM.LF.expect.txt",
		BASE SUB L"UTF-16LE_WITHBOM.CR.actual.txt",
		BASE SUB L"UTF-16LE_WITHBOM.CR.expect.txt",
		BASE SUB L"UTF-16LE_WITHBOM.CRLF.actual.txt",
		BASE SUB L"UTF-16LE_WITHBOM.CRLF.expect.txt",
		BASE SUB L"UTF-16LE_WITHBOM.LF.actual.txt",
		BASE SUB L"UTF-16LE_WITHBOM.LF.expect.txt",
		BASE SUB L"UTF-7.CR.actual.txt",
		BASE SUB L"UTF-7.CR.expect.txt",
		BASE SUB L"UTF-7.CRLF.actual.txt",
		BASE SUB L"UTF-7.CRLF.expect.txt",
		BASE SUB L"UTF-7.LF.actual.txt",
		BASE SUB L"UTF-7.LF.expect.txt",
		BASE SUB L"UTF-8_NOBOM.CR.actual.txt",
		BASE SUB L"UTF-8_NOBOM.CR.expect.txt",
		BASE SUB L"UTF-8_NOBOM.CRLF.actual.txt",
		BASE SUB L"UTF-8_NOBOM.CRLF.expect.txt",
		BASE SUB L"UTF-8_NOBOM.LF.actual.txt",
		BASE SUB L"UTF-8_NOBOM.LF.expect.txt",
		BASE SUB L"UTF-8_WITHBOM.CR.actual.txt",
		BASE SUB L"UTF-8_WITHBOM.CR.expect.txt",
		BASE SUB L"UTF-8_WITHBOM.CRLF.actual.txt",
		BASE SUB L"UTF-8_WITHBOM.CRLF.expect.txt",
		BASE SUB L"UTF-8_WITHBOM.LF.actual.txt",
		BASE SUB L"UTF-8_WITHBOM.LF.expect.txt",
	};

	for (int i = 0; i < sizeof(list) / sizeof(list[0]); i++) {
		std::wstring workFile = copyToTempFolder(list[i]);
		detect_kanji_code(list[i], &file);
		if (file->save() == R_FILE_SAVE_ERROR) {
			std::wstring s(L"ファイル: ");
			s += workFile;
			msgbox(s);
		}
		TextFile *actual;
		detect_kanji_code(workFile.c_str(), &actual);
		file->compare(actual);
		delete actual;
		DeleteFile(workFile.c_str());
	}
}

void test02(const wchar_t *sub, bool multiLineTest = true) {
	TextFile *list;
	wchar_t path[MAX_PATH + 1];
	swprintf_s(path, L"%s%s%s", BASE, sub, L"_list.txt");
	detect_kanji_code(path, &list);
	done_file_count = 0;
	skip_file_count = 0;

	std::wstring before;
	std::wstring workFile;
	for (size_t i = 0; i < list->lines.size(); i++) {
		std::wstring target = list->lines[i]->toWstr();
		std::wstring basename = target.substr(0, target.find(L"("));
		
		if (!multiLineTest || multiLineTest && before != basename) {
			before = basename;
			swprintf_s(path, L"%s%s%s", BASE, sub, basename.c_str());
			workFile = copyToTempFolder(path);
		}
		bool hasTest = false;
		if (multiLineTest) {
			if (i == list->lines.size() - 1) {
				hasTest = true;
			} else {
				std::wstring next = list->lines[i + 1]->toWstr();
				next= next.substr(0, next.find(L"("));
				if (basename != next) {
					hasTest = true;
				}
			}
		}
		FGR_RC rc = replace_line(target);
		if (rc != R_SUCCESS) {
			wchar_t message[4096];
			swprintf_s(message, L"path:%s\nrc:%d\n%s", file->path, rc, target.c_str());
			MessageBox(NULL, message, L"replace_lineで失敗", MB_OK);
		} else {
			if (!multiLineTest || hasTest) {
				file->save();
				if (i == list->lines.size() - 1 || !multiLineTest || before != L"" && before != basename) {
					TextFile *actual, *expect;
					basename = basename.substr(0, basename.find(L".actual"));
					detect_kanji_code(workFile.c_str(), &actual);
					std::wstring s = BASE;
					s += sub;
					s = s.append(basename).append(L".expect.txt");
					detect_kanji_code(s.c_str(), &expect);

					if (actual->lines.size() != expect->lines.size()) {
						MessageBox(NULL, actual->path.c_str(), L"行数が違う", MB_OK);
					} else if (actual->kanji_code != expect->kanji_code) {
						MessageBox(NULL, actual->path.c_str(), L"文字コードが違う", MB_OK);
					} else {
						for (size_t j = 0; j < actual->lines.size(); j++) {
							std::wstring aWstr = actual->lines[j]->toWstr();
							std::wstring eWstr = expect->lines[j]->toWstr();
							char *aData = actual->lines[j]->data();
							char *eData = expect->lines[j]->data();
							size_t aSize = actual->lines[j]->size();
							size_t eSize = expect->lines[j]->size();
							if (aWstr != eWstr || memcmp(aData, eData, max(aSize, eSize)) != 0) {
								MessageBox(NULL, actual->path.c_str(), L"行の内容が違う", MB_OK);
							}
							free(aData);
							free(eData);
						}
					}
					delete actual;
					delete expect;
				}
				DeleteFile(workFile.c_str());
			}
		}
	}

	delete list;
}

void test99() {
#undef SUB
#define SUB L"binarydata\\"
	wchar_t *list[] = {
		BASE SUB L"_bitmap.bmp",
		BASE SUB L"_excel.xls",
		BASE SUB L"_gif.gif",
		BASE SUB L"_jpeg.jpg",
		BASE SUB L"_pdf.pdf",
		BASE SUB L"_png.png",
		BASE SUB L"_powerpoint.ppt",
		BASE SUB L"_project.mpp",
		BASE SUB L"_tiff.tif",
		BASE SUB L"_wave.wav",
		BASE SUB L"_word.doc",
	};

	for (int i = 0; i < sizeof(list) / sizeof(list[0]); i++) {
		std::wstring filename = wcsstr(list[i], SUB) + wcslen(SUB);
		detect_kanji_code(list[i], &file);
		if (file->kanji_code != KC_DATA) {
			if (file->size == 0 && file->kanji_code == KC_SJIS) {
			} else {
				wchar_t msg[1000];
				wcscpy_s(msg, get_charset_nameW(file->kanji_code));
				wcscat(msg, L"\n");
				wcscat(msg, file->path.c_str());
				MessageBox(hWndHidemaru, msg, L"FastGrepReplace", MB_OK);
			}
		}
	}
}


void test06() {
	last_error_message[0] = '\0';
	setup(NULL, L"C:\\Users\\ogata\\Desktop\\src\\");
	wchar_t *list[] = {
		L"com\\sun\\java\\swing\\plaf\\nimbus\\ComboBoxComboBoxArrowButtonEditableState.java(25): ",
	};

	for (int i = 0; i < sizeof(list) / sizeof(list[0]); i++) {
		FGR_RC rc = replace_line(std::wstring(list[i]));
		file->save();

		detect_kanji_code(list[i], &file);
		if (file->kanji_code != KC_DATA) {
			if (file->size == 0 && file->kanji_code == KC_SJIS) {
			} else {
				wchar_t msg [1000];
				wcscpy_s(msg, get_charset_nameW(file->kanji_code));
				wcscat(msg, L"\n");
				wcscat(msg, file->path.c_str());
				MessageBox(hWndHidemaru, msg, L"FastGrepReplace", MB_OK);
			}
		}
	}
}



int _tmain(int argc, _TCHAR* argv[]) {
//	nkf_main(argc, argv);
	DllMain(GetModuleHandle(NULL), DLL_PROCESS_ATTACH, NULL);
#pragma warning(disable:4996)
	tempFolder = _wgetenv(L"TEMP");
#pragma warning(default:4996)
	setup(NULL, tempFolder);

//	test01();							// すべてパスしなければならない
//	test02(L"testdata\\");				// すべてパスしなければならない
//	test02(L"linefeed_test\\", true);	// すべてパスしなければならない
	test02(L"longline\\");				// すべてパスしなければならない（ダイアログがでるが、「以降問い合わせない」にチェックしてすべて置換
//	test05();
//	test99(); // バイナリの文字コードをチェックするため、いくつかダイアログが出る（ファイルは書き換えない）
//	test06();
	
	teardown();
	return 0;
}

#undef BASE
