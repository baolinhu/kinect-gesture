
/*
文件：SelectFolderDlg.h
说明：提供一个选择文件夹的对话框
*/

#ifndef SELECT_FOLDER_DLG_H
#define SELECT_FOLDER_DLG_H


#ifndef BIF_NEWDIALOGSTYLE
#define  BIF_NEWDIALOGSTYLE  0x0040
#endif

class CSelectFolderDlg
{
public:
	CSelectFolderDlg(){}
	~CSelectFolderDlg(){}
	//创建一个选择文件夹的对话框，返回所选路径
	static CString Show()
	{
		TCHAR			lpszPath[MAX_PATH];
		CString			strFolder = TEXT("");
		LPMALLOC		lpMalloc;
		BROWSEINFO		sInfo;
		LPITEMIDLIST	lpidlBrowse;
		//CString			strInitFolder;

		if (::SHGetMalloc(&lpMalloc) != NOERROR)
			return FALSE;

		::ZeroMemory(&sInfo, sizeof(BROWSEINFO));
		sInfo.pidlRoot = 0;
		sInfo.pszDisplayName = lpszPath;
		sInfo.lpszTitle = _T("请选择工作路径：");
		sInfo.ulFlags = BIF_DONTGOBELOWDOMAIN | BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE | BIF_EDITBOX;
		sInfo.lpfn = NULL;
		//sInfo.lParam   = (LPARAM)strInitFolder.GetBuffer(0);
		// 显示文件夹选择对话框
		lpidlBrowse = ::SHBrowseForFolder(&sInfo);
		if (lpidlBrowse != NULL)
		{
			// 取得文件夹名
			if (::SHGetPathFromIDList(lpidlBrowse, lpszPath))
			{
				strFolder = lpszPath;
			}
		}
		if (lpidlBrowse != NULL)
		{
			::CoTaskMemFree(lpidlBrowse);
		}

		lpMalloc->Release();

		return strFolder;

	}

};

#endif