// 2004.12.15 ���n�߁B
// �u�ҏW�E�ϊ��v�z���̃R�}���h��DLL�ɒǂ��o���B���łɁA�����I�ɑ����̕ϊ��֐����v���O�C���I�ɃT�|�[�g����B

typedef HGLOBAL (_cdecl* HIDEMARUFILTERFUNCA)( HWND hwndHidemaru, char* pszIn, WORD wCodePage, char* pszParam, int cbParamBuffer );
typedef HGLOBAL (_cdecl* HIDEMARUFILTERFUNC)( HWND hwndHidemaru, WCHAR* pwszIn, char* pszParam, int cbParamBuffer );

struct HIDEMARUFILTERINFO {
    UINT                cbStructSize;
    const char*         pszExportName;
    const char*         pszNameJapan;
    const char*         pszNameUs;
    BYTE                chAccel;        // �A�N�Z�����[�^�L�[
    BYTE                fMustLineUnit;  // �s�P�ʂłȂ��Ǝ��s�ł��Ȃ��t���O
    BYTE                abReserve[2];
};


#define WM_HIDEMARUINFO (WM_USER + 181)

#define HIDEMARUINFO_GETTABWIDTH 0
#define HIDEMARUINFO_GETSTARTCOLUMN 1
#define HIDEMARUINFO_GETSPACETAB 2

// extern "C" HIDEMARUFILTER* _cdecl EnumHidemaruFilter();
