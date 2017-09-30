//--------------------------------------【程序说明】-------------------------------------------
//		程序说明：本科毕业设计--基于Kinect的人体动作识别系统
//		程序描述：基于Kinect的人体动作识别系统
//		开发测试所用IDE版本：Visual Studio 2013
//		开发测试所用OpenCV版本：	3.0 beta
//		开发测试所使用硬件：	KinectV2 Xbox
//		操作系统：Windows 10
//		Kinect SDK版本：KinectSDK-v2.0-PublicPreview1409-Setup 
//		2017年4月 Created by @胡保林 hu_nobuone@163.com
//------------------------------------------------------------------------------------------------
// MFC_DEMO01Dlg.cpp : 实现文件
//

#include "stdafx.h"
#include "MFC_DEMO01.h"
#include "MFC_DEMO01Dlg.h"
#include "afxdialogex.h"
#include"Mykinect.h"
#include"SelectFolderDlg.h"
#include <io.h>  
#include <fcntl.h> 

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//全局变量
CBodyBasics   myKinect;		//实例化一个CBodyBasics对象myKinect


CMFC_DEMO01Dlg *CMFC_DEMO01Dlg::s_pDlg = nullptr;

void InitConsole()
{
	int nRet = 0;
	FILE* fp;
	AllocConsole();
	nRet = _open_osfhandle((long)GetStdHandle(STD_OUTPUT_HANDLE), _O_TEXT);
	fp = _fdopen(nRet, "w");
	*stdout = *fp;
	setvbuf(stdout, NULL, _IONBF, 0);
}
// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CMFC_DEMO01Dlg 对话框


//构造函数
CMFC_DEMO01Dlg::CMFC_DEMO01Dlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CMFC_DEMO01Dlg::IDD, pParent)
	, m_edit(_T(""))
	, m_numedit(0)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}
//CMFC_DEMO01Dlg::CMFC_DEMO01Dlg(CBodyBasics* ckinect)
//{
//	mykinect = ckinect;
//}

void CMFC_DEMO01Dlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PIC_COMBO1, m_comboPic);
	DDX_Control(pDX, IDC_OUT_EDIT1, m_outedit);
	DDX_Text(pDX, IDC_PATH_EDIT1, m_edit);
	DDX_Text(pDX, IDC_NUM_EDIT1, m_numedit);
}

BEGIN_MESSAGE_MAP(CMFC_DEMO01Dlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
//	ON_BN_CLICKED(IDC_BUTTON1, &CMFC_DEMO01Dlg::OnBnClickedButton1)
	ON_BN_CLICKED(IDYES, &CMFC_DEMO01Dlg::OnBnClickedYes)
	ON_BN_CLICKED(IDOK, &CMFC_DEMO01Dlg::OnBnClickedOk)
	ON_BN_CLICKED(IDC_START_BUTTON1, &CMFC_DEMO01Dlg::OnBnClickedStartButton1)
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_CHOOSE_BUTTON2, &CMFC_DEMO01Dlg::OnBnClickedChooseButton2)
	ON_BN_CLICKED(IDC_SAVE_BUTTON1, &CMFC_DEMO01Dlg::OnBnClickedSaveButton1)
	ON_CBN_SELCHANGE(IDC_PIC_COMBO1, &CMFC_DEMO01Dlg::OnCbnSelchangePicCombo1)
	ON_BN_CLICKED(IDC_SAVE2_BUTTON1, &CMFC_DEMO01Dlg::OnBnClickedSave2Button1)
	ON_BN_CLICKED(IDC_SAVE3_BUTTON2, &CMFC_DEMO01Dlg::OnBnClickedSave3Button2)
	ON_BN_CLICKED(IDC_STOP_BUTTON3, &CMFC_DEMO01Dlg::OnBnClickedStopButton3)
	ON_BN_CLICKED(IDC_GET_BUTTON3, &CMFC_DEMO01Dlg::OnBnClickedGetButton3)
	ON_EN_CHANGE(IDC_PATH_EDIT1, &CMFC_DEMO01Dlg::OnEnChangePathEdit1)
END_MESSAGE_MAP()


// CMFC_DEMO01Dlg 消息处理程序

BOOL CMFC_DEMO01Dlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO:  在此添加额外的初始化代码
	s_pDlg = this;
	InitConsole();
	SetWindowPos(&wndTopMost,0,0,500,500, SWP_NOSIZE);
	// 为组合框控件的列表框添加列表项:  
	m_comboPic.AddString(_T("骨骼图"));  
	m_comboPic.AddString(_T("深度图"));
	m_comboPic.AddString(_T("RGB图"));
	m_comboPic.AddString(_T("IR图"));
	//也可以使用下面这种,可以调节序号
	//m_comboPic.InsertString(1,_T("IR图"));
	// 默认选择第一项   
	m_comboPic.SetCurSel(1);	

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CMFC_DEMO01Dlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CMFC_DEMO01Dlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//画图到picture控件的函数
 void CMFC_DEMO01Dlg::DrawPicToHDC(IplImage *img, UINT ID)
{
	CDC *pDC = GetDlgItem(ID)->GetDC();
	HDC hDC = pDC->GetSafeHdc();
	CRect rect;
	GetDlgItem(ID)->GetClientRect(&rect);
	CvvImage cimg;
	cimg.CopyOf(img); // 复制图片  
	cimg.DrawToHDC(hDC, &rect); // 将图片绘制到显示控件的指定区域内  
	ReleaseDC(pDC);
}


//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CMFC_DEMO01Dlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CMFC_DEMO01Dlg::OnBnClickedYes()
{
	// TODO:  在此添加控件通知处理程序代码
	//IplImage *image = NULL; //原始图像  
	//if (image) cvReleaseImage(&image);
	//image = cvLoadImage("f:\\cpp\\MFC\\001\\MFC_DEMO01\\MFC_DEMO01\\1.jpg", 1); //显示图片  
	//DrawPicToHDC(image, IDC_PIC_STATIC);

}


void CMFC_DEMO01Dlg::OnBnClickedOk()			//退出
{
	// TODO:  在此添加控件通知处理程序代码
	KillTimer(1);							//关闭定时器
	CDialogEx::OnOK();
}




void CMFC_DEMO01Dlg::OnBnClickedStartButton1()
{
	// TODO:  在此添加控件通知处理程序代码

	
	HRESULT hr = myKinect.InitializeDefaultSensor();		//初始化默认Kinect
	if (SUCCEEDED(hr))
	{	
			myKinect.Update();								//刷新骨骼和深度数据	
			
	}
	SetTimer(1, 5, NULL);				//定时器
}

//定时器
void CMFC_DEMO01Dlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO:  在此添加消息处理程序代码和/或调用默认值

	myKinect.Update();
	CDialogEx::OnTimer(nIDEvent);
}

//选择文件夹
void CMFC_DEMO01Dlg::OnBnClickedChooseButton2()
{
	// TODO:  在此添加控件通知处理程序代码
	CString strFolderPath = CSelectFolderDlg::Show();
	//m_edit.SetWindowText(strFolderPath.GetString());		
	//UpdateData(FALSE);
	SetDlgItemText(IDC_PATH_EDIT1, strFolderPath);
}

//保存深度图像
void CMFC_DEMO01Dlg::OnBnClickedSaveButton1()
{
	// TODO:  在此添加控件通知处理程序代码
	UpdateData(TRUE);
	for (int i = 0; i < m_numedit; i++)
	{
		myKinect.SaveDepthImg();
		//myKinect.SaveSkeletonImg();
	}
}

//选择当前要显示的图片：深度图，骨骼图，RGB图，IR图
void CMFC_DEMO01Dlg::OnCbnSelchangePicCombo1()
{
	// TODO:  在此添加控件通知处理程序代码
}

//保存骨骼图
void CMFC_DEMO01Dlg::OnBnClickedSave2Button1()
{
	// TODO:  在此添加控件通知处理程序代码
	UpdateData(TRUE);
	for (int i = 0; i < m_numedit; i++)
	{
		//myKinect.SaveDepthImg();
		myKinect.SaveSkeletonImg();
	}
}

//保存深度和骨骼图
void CMFC_DEMO01Dlg::OnBnClickedSave3Button2()
{
	// TODO:  在此添加控件通知处理程序代码
	UpdateData(TRUE);
	for (int i = 0; i < m_numedit; i++)
	{
		myKinect.SaveDepthImg();
		myKinect.SaveSkeletonImg();
	}
}


void CMFC_DEMO01Dlg::OnBnClickedStopButton3()
{
	// TODO:  在此添加控件通知处理程序代码
	KillTimer(1);					//关闭计时器
	myKinect.m_pKinectSensor->Close();	//关闭Kinect
	IplImage *image = NULL; //原始图像  
	if (image) cvReleaseImage(&image);
	image = cvLoadImage("1.jpg", 1); //显示图片  
	DrawPicToHDC(image, IDC_PIC_STATIC);
	DrawPicToHDC(image, IDC_PIC2_STATIC);

}


void CMFC_DEMO01Dlg::OnBnClickedGetButton3()
{
	// TODO:  在此添加控件通知处理程序代码
}


void CMFC_DEMO01Dlg::OnEnChangePathEdit1()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
}
