// 2004.12.15 作り始め。
// 「編集・変換」配下のコマンドをDLLに追い出す。ついでに、将来的に多くの変換関数をプラグイン的にサポートする。

typedef HGLOBAL (_cdecl* HIDEMARUFILTERFUNCA)( HWND hwndHidemaru, char* pszIn, WORD wCodePage, char* pszParam, int cbParamBuffer );
typedef HGLOBAL (_cdecl* HIDEMARUFILTERFUNC)( HWND hwndHidemaru, WCHAR* pwszIn, char* pszParam, int cbParamBuffer );

struct HIDEMARUFILTERINFO {
    UINT                cbStructSize;
    const char*         pszExportName;
    const char*         pszNameJapan;
    const char*         pszNameUs;
    BYTE                chAccel;        // アクセラレータキー
    BYTE                fMustLineUnit;  // 行単位でないと実行できないフラグ
    BYTE                abReserve[2];
};


#define WM_HIDEMARUINFO (WM_USER + 181)

#define HIDEMARUINFO_GETTABWIDTH 0
#define HIDEMARUINFO_GETSTARTCOLUMN 1
#define HIDEMARUINFO_GETSPACETAB 2

// extern "C" HIDEMARUFILTER* _cdecl EnumHidemaruFilter();
