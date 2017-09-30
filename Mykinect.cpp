//--------------------------------------【程序说明】-------------------------------------------
//		程序说明：本科毕业设计--基于Kinect的人体动作识别系统
//		程序描述：基于Kinect的人体动作识别系统
//		开发测试所用IDE版本：Visual Studio 2013
//		开发测试所用OpenCV版本：	3.0 beta
//		开发测试所使用硬件：	KinectV2 Xbox
//		操作系统：Windows 10
//		Kinect SDK版本：KinectSDK-v2.0-PublicPreview1409-Setup 
//		2017年4月 Created by @hu_nobuone@163.com
//------------------------------------------------------------------------------------------------

//---------------------------------【头文件、命名空间包含部分】-----------------------------
//		描述：包含程序所使用的头文件和命名空间
//-------------------------------------------------------------------------------------------------
#include "stdafx.h"
#include "Mykinect.h"
#include "MFC_DEMO01.h"
#include "MFC_DEMO01Dlg.h"
#include"KinectJointFilter.h"
#include "afxdialogex.h"
#include<Windows.h>
#include<opencv2/opencv.hpp>	//opencv头文件
#include <sstream>				//数字转字符串
#include<cstring>
//#include<afx.h>
 
using namespace std;			//命名空间
using namespace cv;
using namespace Sample;

//extern void CMFC_DEMO01Dlg::DrawPicToHDC(IplImage* img, UINT ID);

//全局变量定义
static DWORD  framenumber = 0,depthnumber=0;			//骨骼帧编号
float  spinemid_xin;		//重心
float  spinemid_yin;
float  spinemid_xout;
float  spinemid_yout;
float  spinemid_x;
float  spinemid_y;
float  spinebase_yin, spinebase_yout, spinebase_y;
float rightfoot_yin = 0, rightfoot_yout = 0, rightfoot_y = 0;
float rightAnkle_yin = 0, rightAnkle_yout = 0, rightAnkle_y = 0;
float leftfoot_yin = 0, leftfoot_yout = 0, leftfoot_y = 0;
float base_foot_in = 0, base_foot_out = 0, base_foot = 0;
float spinetemp = 0;

const double thresh_x = 0.15;		//阈值
const double thresh_y = 0.2;

int flag = 0;		//下蹲标志位
/// Initializes the default Kinect sensor
HRESULT CBodyBasics::InitializeDefaultSensor()
{
	//用于判断每次读取操作的成功与否
	HRESULT hr;

	//搜索kinect
	hr = GetDefaultKinectSensor(&m_pKinectSensor);
	if (FAILED(hr))
	{
		return hr;
	}

	//找到kinect设备
	if (m_pKinectSensor)
	{
		// Initialize the Kinect and get coordinate mapper and the body reader
		IBodyFrameSource* pBodyFrameSource = NULL;//读取骨架
		IDepthFrameSource* pDepthFrameSource = NULL;//读取深度信息
		IBodyIndexFrameSource* pBodyIndexFrameSource = NULL;//读取背景二值图

		//打开kinect
		hr = m_pKinectSensor->Open();

		//coordinatemapper
		if (SUCCEEDED(hr))
		{
			hr = m_pKinectSensor->get_CoordinateMapper(&m_pCoordinateMapper);
		}

		//bodyframe    每种图都是通过source，reader，frame三个类
		if (SUCCEEDED(hr))
		{
			hr = m_pKinectSensor->get_BodyFrameSource(&pBodyFrameSource);
		}

		if (SUCCEEDED(hr))
		{
			hr = pBodyFrameSource->OpenReader(&m_pBodyFrameReader);
		}

		//depth frame
		if (SUCCEEDED(hr))
		{
			hr = m_pKinectSensor->get_DepthFrameSource(&pDepthFrameSource);
		}

		if (SUCCEEDED(hr))
		{
			hr = pDepthFrameSource->OpenReader(&m_pDepthFrameReader);
		}

		//body index frame
		if (SUCCEEDED(hr))
		{
			hr = m_pKinectSensor->get_BodyIndexFrameSource(&pBodyIndexFrameSource);
		}

		if (SUCCEEDED(hr))
		{
			hr = pBodyIndexFrameSource->OpenReader(&m_pBodyIndexFrameReader);
		}

		SafeRelease(pBodyFrameSource);
		SafeRelease(pDepthFrameSource);
		SafeRelease(pBodyIndexFrameSource);
	}

	if (!m_pKinectSensor || FAILED(hr))
	{
		std::cout << "Kinect initialization failed!" << std::endl;
		return E_FAIL;
	}

	//skeletonImg,用于画骨架、背景二值图的MAT
	skeletonImg.create(cDepthHeight, cDepthWidth, CV_8UC3);
	skeletonImg.setTo(0);

	//depthImg,用于画深度信息的MAT
	depthImg.create(cDepthHeight, cDepthWidth, CV_8UC1);
	depthImg.setTo(0);

	return hr;
}


/// Main processing function
void CBodyBasics::Update()
{
	//每次先清空skeletonImg
	skeletonImg.setTo(0);

	//如果丢失了kinect，则不继续操作
	if (!m_pBodyFrameReader)
	{
		return;
	}

	IBodyFrame* pBodyFrame = NULL;//骨架信息
	IDepthFrame* pDepthFrame = NULL;//深度信息
	IBodyIndexFrame* pBodyIndexFrame = NULL;//背景二值图

	//记录每次操作的成功与否
	HRESULT hr = S_OK;

	//---------------------------------------获取背景二值图并显示---------------------------------
	if (SUCCEEDED(hr)){
		hr = m_pBodyIndexFrameReader->AcquireLatestFrame(&pBodyIndexFrame);//获得背景二值图信息
	}
	if (SUCCEEDED(hr)){
		BYTE *bodyIndexArray = new BYTE[cDepthHeight * cDepthWidth];//背景二值图是8为uchar，有人是黑色，没人是白色
		pBodyIndexFrame->CopyFrameDataToArray(cDepthHeight * cDepthWidth, bodyIndexArray);

		//把背景二值图画到MAT里
	uchar* skeletonData = (uchar*)skeletonImg.data;
		for (int j = 0; j < cDepthHeight * cDepthWidth; ++j){
			*skeletonData = bodyIndexArray[j]; ++skeletonData;
			*skeletonData = bodyIndexArray[j]; ++skeletonData;
			*skeletonData = bodyIndexArray[j]; ++skeletonData;
		}
		delete[] bodyIndexArray;
	}
	SafeRelease(pBodyIndexFrame);//必须要释放，否则之后无法获得新的frame数据

	//-----------------------获取深度数据并显示--------------------------
	if (SUCCEEDED(hr))
	{
		hr = m_pDepthFrameReader->AcquireLatestFrame(&pDepthFrame);//获得深度数据
	}
	if (SUCCEEDED(hr))
	{
		Mat temp(cDepthHeight, cDepthWidth, CV_16UC1);    //建立图像矩阵
		pDepthFrame->CopyFrameDataToArray(cDepthHeight * cDepthWidth, (UINT16 *)temp.data); //先把数据存入16位的图像矩阵中
		temp.convertTo(depthImg, CV_8UC1, 255.0 / 4500);   //再把16位转换为8位

		//pDepthFrame->CopyFrameDataToArray(cDepthHeight * cDepthWidth, depthArray);

	//	//把深度数据画到MAT中
		//uchar* depthData = (uchar*)depthImg.data;
		//for (int j = 0; j < cDepthHeight * cDepthWidth; ++j)
		//{
		//	*depthData = depthArray[j];
		//	++depthData;
		//}
		//delete[] depthArray;
	}
	depthnumber++;
	SafeRelease(pDepthFrame);		//必须要释放，否则之后无法获得新的frame数据
	IplImage *src;
	src = &IplImage(depthImg);
	CMFC_DEMO01Dlg *pDlg0 = CMFC_DEMO01Dlg::s_pDlg;
	pDlg0->DrawPicToHDC(src, IDC_PIC_STATIC);
	//输出测试
	//stringstream ostream;
	//ostream << "hello, world.";
	//GetDlgItem(IDC_OUT_EDIT1)->SetWindowText(ostream.str().c_str());
	//CString str=L"当前是jdhj";
	//HWND hWnd = AfxGetMainWnd()->m_hWnd;
	//SetDlgItemText(hWnd, IDC_OUT_EDIT1, str);
	//cv::imshow("depthImg", depthImg);			
	//cv::waitKey(5);

	//-----------------------------获取骨架并显示----------------------------
	if (SUCCEEDED(hr))
	{
		hr = m_pBodyFrameReader->AcquireLatestFrame(&pBodyFrame);//获取骨架信息
	}
	if (SUCCEEDED(hr))
	{
		IBody* ppBodies[BODY_COUNT] = { 0 };//每一个IBody可以追踪一个人，总共可以追踪六个人

		if (SUCCEEDED(hr))
		{
			//把kinect追踪到的人的信息，分别存到每一个IBody中
			hr = pBodyFrame->GetAndRefreshBodyData(_countof(ppBodies), ppBodies);
		}

		if (SUCCEEDED(hr))
		{
			//对每一个IBody，我们找到他的骨架信息，并且画出来
			ProcessBody(BODY_COUNT, ppBodies);
		}

		for (int i = 0; i < _countof(ppBodies); ++i)
		{
			SafeRelease(ppBodies[i]);//释放所有
		}
	}
	SafeRelease(pBodyFrame);//必须要释放，否则之后无法获得新的frame数据

}


//画手的状态
void CBodyBasics::DrawHandState(const DepthSpacePoint depthSpacePosition, HandState handState)
{
	//给不同的手势分配不同颜色
	CvScalar color;
	switch (handState)
	{
	case HandState_Open:
		color = cvScalar(255, 0, 0);	//手是张开的状态，用蓝色表示
		break;
	case HandState_Closed:
		color = cvScalar(0, 255, 0);	//手是闭合的状态，用绿色表示
		break;
	case HandState_Lasso:
		color = cvScalar(0, 0, 255);	//手是介于张开和闭合的状态，用红色表示
		break;
	default:							//如果没有确定的手势，就不要画
		return;
	}

	circle(skeletonImg,
		cvPoint(depthSpacePosition.X, depthSpacePosition.Y),
		20, color, -1);
}

/// Handle new body data
void CBodyBasics::ProcessBody(int nBodyCount, IBody** ppBodies)
{
	//记录操作结果是否成功
	HRESULT hr;
	FilterDoubleExponential filterKinect;	//霍尔特双指数滤波器
	//对于每一个IBody
	for (int i = 0; i < nBodyCount; ++i)
	{
		IBody* pBody = ppBodies[i];
		if (pBody)//还没有搞明白这里pBody和下面的bTracked有什么区别
		{
			BOOLEAN bTracked = false;
			hr = pBody->get_IsTracked(&bTracked);

			filterKinect.Update(pBody);		//平滑和稳定身体骨架

			if (SUCCEEDED(hr) && bTracked)
			{
				Joint joints[JointType_Count];//存储关节点类
				HandState leftHandState = HandState_Unknown;//左手状态
				HandState rightHandState = HandState_Unknown;//右手状态

				//获取左右手状态
				pBody->get_HandLeftState(&leftHandState);
				pBody->get_HandRightState(&rightHandState);

				//存储深度坐标系中的关节点位置
				DepthSpacePoint *depthSpacePosition = new DepthSpacePoint[_countof(joints)];

				//获得关节点类
				hr = pBody->GetJoints(_countof(joints), joints);
				if (SUCCEEDED(hr))
				{
					for (int j = 0; j < _countof(joints); ++j)
					{
						//将关节点坐标从摄像机坐标系（-1~1）转到深度坐标系（424*512）
						m_pCoordinateMapper->MapCameraPointToDepthSpace(joints[j].Position, &depthSpacePosition[j]);
						circle(skeletonImg, CvPoint(depthSpacePosition[j].X, depthSpacePosition[j].Y), 3, cvScalar(0, 255, 255), 1, 8, 0);
					}
					filterKinect.Update(joints);		//平滑身体每个骨骼点

					//------------------------hand state left  and  right-------------------------------
					DrawHandState(depthSpacePosition[JointType_HandLeft], leftHandState);
					DrawHandState(depthSpacePosition[JointType_HandRight], rightHandState);

					//---------------------------body-------------------------------
					DrawBone(joints, depthSpacePosition, JointType_Head, JointType_Neck);
					DrawBone(joints, depthSpacePosition, JointType_Neck, JointType_SpineShoulder);
					DrawBone(joints, depthSpacePosition, JointType_SpineShoulder, JointType_SpineMid);
					DrawBone(joints, depthSpacePosition, JointType_SpineMid, JointType_SpineBase);
					DrawBone(joints, depthSpacePosition, JointType_SpineShoulder, JointType_ShoulderRight);
					DrawBone(joints, depthSpacePosition, JointType_SpineShoulder, JointType_ShoulderLeft);
					DrawBone(joints, depthSpacePosition, JointType_SpineBase, JointType_HipRight);
					DrawBone(joints, depthSpacePosition, JointType_SpineBase, JointType_HipLeft);

					// -----------------------Right Arm ------------------------------------ 
					DrawBone(joints, depthSpacePosition, JointType_ShoulderRight, JointType_ElbowRight);
					DrawBone(joints, depthSpacePosition, JointType_ElbowRight, JointType_WristRight);
					DrawBone(joints, depthSpacePosition, JointType_WristRight, JointType_HandRight);
					DrawBone(joints, depthSpacePosition, JointType_HandRight, JointType_HandTipRight);
					DrawBone(joints, depthSpacePosition, JointType_WristRight, JointType_ThumbRight);

					//----------------------------------- Left Arm--------------------------
					DrawBone(joints, depthSpacePosition, JointType_ShoulderLeft, JointType_ElbowLeft);
					DrawBone(joints, depthSpacePosition, JointType_ElbowLeft, JointType_WristLeft);
					DrawBone(joints, depthSpacePosition, JointType_WristLeft, JointType_HandLeft);
					DrawBone(joints, depthSpacePosition, JointType_HandLeft, JointType_HandTipLeft);
					DrawBone(joints, depthSpacePosition, JointType_WristLeft, JointType_ThumbLeft);

					// ----------------------------------Right Leg--------------------------------
					DrawBone(joints, depthSpacePosition, JointType_HipRight, JointType_KneeRight);
					DrawBone(joints, depthSpacePosition, JointType_KneeRight, JointType_AnkleRight);
					DrawBone(joints, depthSpacePosition, JointType_AnkleRight, JointType_FootRight);

					// -----------------------------------Left Leg---------------------------------
					DrawBone(joints, depthSpacePosition, JointType_HipLeft, JointType_KneeLeft);
					DrawBone(joints, depthSpacePosition, JointType_KneeLeft, JointType_AnkleLeft);
					DrawBone(joints, depthSpacePosition, JointType_AnkleLeft, JointType_FootLeft);

					//动作检测函数
					if (joints[JointType_SpineBase].Position.Z > 0.5&&joints[JointType_SpineBase].Position.Z<3.5)
						Detection(joints);
					else
						cout << "为了检测的准确度，请尽量站在离Kinect 0.5--3.5 米之间，谢谢配合！" << endl;
				}
				delete[] depthSpacePosition;
			}
		}
	}
	framenumber++;
	//MFC的pic控件显示
	IplImage *src;
	src = &IplImage(skeletonImg);
	CMFC_DEMO01Dlg *pDlg1 = CMFC_DEMO01Dlg::s_pDlg;
	pDlg1->DrawPicToHDC(src, IDC_PIC2_STATIC);
	//Opencv显示
	//namedWindow("skeletonImg", 0);
	//resizeWindow("skeletonImg", 640, 480);
	//cv::imshow("skeletonImg", skeletonImg);
	//cv::waitKey(5);
}

//求2个骨骼点之间的距离
double CBodyBasics::Distance(Joint p1, Joint p2)
{
	double dist = 0;
	dist = sqrt(pow(p2.Position.X - p1.Position.X, 2) +
		pow(p2.Position.Y - p1.Position.Y, 2) + pow(p2.Position.Z - p1.Position.Z, 2));
	return dist;
}

//检测函数:为了检测准确请尽量站在合适位置，让Kinect获取全身骨骼点
void CBodyBasics::Detection(Joint joints[])
{
	static double tin, tout;
	//double tframe;
	CMFC_DEMO01Dlg *pDlg0 = CMFC_DEMO01Dlg::s_pDlg;		//实例化一个CMFC_DEMO01Dlg 指针
	//计算每相邻10帧的高度差，从而计算速度，1,11,12,22
	//大概30帧每秒，那么10帧就是0.33秒，
	if (framenumber % 11 == 1)		//framenumber是帧序列号，自己定义的
	{
		tin = static_cast<double>(GetTickCount());
		//cout << "tin是" << tin << endl;
		spinemid_xin = joints[JointType_SpineMid].Position.X;
		spinemid_yin = joints[JointType_SpineMid].Position.Y;
		rightfoot_yin = joints[JointType_KneeRight].Position.Y;
		leftfoot_yin = joints[JointType_KneeLeft].Position.Y;
		spinebase_yin = joints[JointType_SpineBase].Position.Y;
		rightAnkle_yin = joints[JointType_AnkleRight].Position.Y;
		base_foot_in = spinebase_yin - rightAnkle_yin;
		//cout << "basefootin为：" << base_foot_in << endl;
		//cout << "当前SpineHeightin的高度为" << SpineHeightin << "  m"<<endl;
	}
	if (!(framenumber % 11))
	{
		tout = static_cast<double>(GetTickCount());
		//cout << frmamenumber << endl;
		//cout <<"tout是"<< tout << endl;
		//cout << "每10帧计算一次下降的速度" << endl;
		spinemid_xout = joints[JointType_SpineMid].Position.X;
		spinemid_yout = joints[JointType_SpineMid].Position.Y;
		rightfoot_yout = joints[JointType_KneeRight].Position.Y;
		leftfoot_yout = joints[JointType_KneeLeft].Position.Y;
		rightAnkle_yout = joints[JointType_AnkleRight].Position.Y;
		spinebase_yout = joints[JointType_SpineBase].Position.Y;
		base_foot_out = spinebase_yout - rightAnkle_yout;
		//cout << "当前帧号为：" << base_foot_out << endl;
		//  cout << "***********************************" << endl;
		//  cout << "当前SpineHeightin的高度为" << SpineHeightin << "  m" << endl;
		//tframe = (tout - tin) / getTickFrequency();
		// cout <<tframe << endl;
		//  cout << getTickFrequency()<<endl;
		//cout << "当前SpineHeightout的高度为" << SpineHeightout << "  m" << endl;
		//SpineV = (SpineHeightin - SpineHeightout) / tframe;
		spinemid_x = spinemid_xout - spinemid_xin;
		spinemid_y = spinemid_yout - spinemid_yin;
		rightfoot_y = rightfoot_yout - rightfoot_yin;
		leftfoot_y = leftfoot_yout - leftfoot_yin;
		base_foot = base_foot_out - base_foot_in;
		//cout << "Spinemid_x是多少？？" << base_foot << endl;
		//cout << "Spinemid_y是多少？？" << spinemid_y << endl;
		//上蹦检测：双脚离地面超过0.15米，或者人体重心相对正常站立时上升超过0.15米
		if ((leftfoot_y>0.15&&rightfoot_y > 0.15) || (spinetemp>0.01&&spinetemp + 0.15<joints[JointType_SpineMid].Position.Y))		//y轴向上为正
		{
			string str1 = "上蹦\r\n " ;			//这里面是为了把数据输出到mfc显示框,你可以不用管，下同
			CString cstr = str1.c_str();		//删掉编辑框中的内容，方法一：GetDlgItem(IDC_EDIT1)->SetWindowText("");
												//方法二：给编辑框定义一个控件变量，m_edit1.SetWindowText("");
			pDlg0->m_outedit.SetSel(-1);
			pDlg0->m_outedit.ReplaceSel(cstr);
			cout << str1;						//控制台输出显示，下同。为了方便调试，这里同时会显示到控制台
		}	
		//else if (base_foot < -thresh_y)	//下蹲检测：主要检测腿部有弯曲即可，臀部与脚踝之间的距离减少超过0.2米
		else if (Distance(joints[JointType_HipLeft], joints[JointType_AnkleLeft])*(1 + 0.15) < Distance(joints[JointType_HipLeft], joints[JointType_KneeLeft]) + Distance(joints[JointType_KneeLeft], joints[JointType_AnkleLeft]))
		{
			flag++;
			if (flag == 2)			//下蹲状态需要时间，这里给了一下标志位，类似计时器，连续监测到两次才算下蹲，避免重复出现结果
			{
				flag = 0;
				//下蹲其他检测方法，还可以检测hipleft、knee、ankle三点之间的夹角关系和距离关系，夹角小于160度（可以多试几						
				//个值），说明有下蹲,或者两边之和大于第三边的1.15倍左右，也可以说明有下蹲
				string str1 = "下蹲\r\n ";
				CString cstr = str1.c_str();
				pDlg0->m_outedit.SetSel(-1);
				pDlg0->m_outedit.ReplaceSel(cstr);
				cout << str1;
			}
		}
		//x轴方向向右为正
		//重心向右移动超过阈值thresh_x，则判断右移
		if (spinemid_x > thresh_x)		
		{
			string str1 = "右移\r\n ";
			CString cstr = str1.c_str();
			pDlg0->m_outedit.SetSel(-1);
			pDlg0->m_outedit.ReplaceSel(cstr);
			cout << str1;
		}
		else if (spinemid_x < -thresh_x)		////重心向左移动超过阈值thresh_x，则判断左移
		{
			string str1 = "左移\r\n ";
			CString cstr = str1.c_str();
			pDlg0->m_outedit.SetSel(-1);
			pDlg0->m_outedit.ReplaceSel(cstr);
			cout << str1;
		}			
	}
	//根据勾股定理，计算HipLeft、AnkleLeft、AnkleLeft之间的距离关系。0.15是一个估计值，可根据实际情况略微调整
/*	if (Distance(joints[JointType_HipLeft], joints[JointType_AnkleLeft])*(1 + 0.15) < Distance(joints[JointType_HipLeft], joints[JointType_KneeLeft]) + Distance(joints[JointType_KneeLeft], joints[JointType_AnkleLeft]))
	{
		flag++;
		if (flag == 2)			//下蹲状态需要时间，这里给了一下标志位，类似计时器，连续监测到两次才算下蹲，避免重复出现结果
		{
			flag = 0;
			cout << "下蹲1111111\n";
		}
	}
*/

}

//把关节点之间用线连起来。确定的用白线，不确定的用红线。
/// Draws one bone of a body (joint to joint)
void CBodyBasics::DrawBone(const Joint* pJoints, const DepthSpacePoint* depthSpacePosition, JointType joint0, JointType joint1)
{
	TrackingState joint0State = pJoints[joint0].TrackingState;
	TrackingState joint1State = pJoints[joint1].TrackingState;

	// If we can't find either of these joints, exit
	if ((joint0State == TrackingState_NotTracked) || (joint1State == TrackingState_NotTracked))
	{
		return;
	}

	// Don't draw if both points are inferred
	if ((joint0State == TrackingState_Inferred) && (joint1State == TrackingState_Inferred))
	{
		return;
	}

	CvPoint p1 = cvPoint(depthSpacePosition[joint0].X, depthSpacePosition[joint0].Y),
		p2 = cvPoint(depthSpacePosition[joint1].X, depthSpacePosition[joint1].Y);

	// We assume all drawn bones are inferred unless BOTH joints are tracked
	if ((joint0State == TrackingState_Tracked) && (joint1State == TrackingState_Tracked))
	{
		//非常确定的骨架，用白色直线
		line(skeletonImg, p1, p2, cvScalar(255, 255, 255), 4);
	}
	else
	{
		//不确定的骨架，用红色直线
		line(skeletonImg, p1, p2, cvScalar(0, 0, 255), 4);
	}
}


//保存深度图像
void CBodyBasics::SaveDepthImg()
{
	stringstream stream0,stream1;
	string str,filepath,str1;
	CMFC_DEMO01Dlg *pDlg0 = CMFC_DEMO01Dlg::s_pDlg;
	//filepath = "D:\\pic\\";			//设置固定保存路径
	//从选择文件夹里面获取当前选择的路径
	filepath = (pDlg0->m_edit).GetBuffer(0);
	if (filepath.empty())
		filepath = "D:\\pic\\";			//设置默认固定保存路径
	else
		filepath += "/";

	//从选择文件夹里面获取当前选择的路径
	//stream0 << pDlg0->m_edit;
	//stream0 >> filepath;
	stream1 << depthnumber;        //从long型数据输入
	stream1 >> str;					//转换为 string
	str1 = "成功保存第 " + str + " 帧深度图\r\n";
	CString cstr = str1.c_str();
	
	if (depthImg.data)
	{
		imwrite(filepath + str + "depth.bmp", depthImg);
		pDlg0->m_outedit.SetSel(-1);
		pDlg0->m_outedit.ReplaceSel(cstr);
		//HWND hWnd = AfxGetMainWnd()->m_hWnd;
		//SetDlgItemText(hWnd, IDC_OUT_EDIT1,cstr);
		cout << str1 ;
		//cout << str + "depth.bmp" << endl;
	}
	else
	{
		CString cfail = "没有数据，保存失败\r\n";
		pDlg0->m_outedit.ReplaceSel(cfail);
	}
		
}

//保存骨骼图数据
void CBodyBasics::SaveSkeletonImg()
{
	stringstream stream0, stream1;
	string str, filepath, str1;
	CMFC_DEMO01Dlg *pDlg0 = CMFC_DEMO01Dlg::s_pDlg;
	//filepath = "D:\\pic\\";			//设置固定保存路径
	filepath = pDlg0->m_edit.GetBuffer(0);
	//stream0 << pDlg0->m_edit;
	//stream0 >> filepath;
	if (filepath.empty())
		filepath = "D:\\pic\\";			//设置默认固定保存路径
	else
		filepath += "/";

	//从选择文件夹里面获取当前选择的路径
	//stream0 << pDlg0->m_edit;
	//stream0 >> filepath;
	stream1 << framenumber;        //从long型数据输入
	stream1 >> str;					//转换为 string
	str1 = "成功保存第 " + str + " 帧骨骼图\r\n";
	CString cstr = str1.c_str();

	if (skeletonImg.data)
	{
		imwrite(filepath + str + "skeleton.bmp", skeletonImg);
		pDlg0->m_outedit.SetSel(-1);
		pDlg0->m_outedit.ReplaceSel(cstr);
		//HWND hWnd = AfxGetMainWnd()->m_hWnd;
		//SetDlgItemText(hWnd, IDC_OUT_EDIT1,cstr);
		cout << str1;
		//cout << str + "depth.bmp" << endl;
	}
	else
	{
		CString cfail = "没有数据，保存失败\r\n";
		pDlg0->m_outedit.ReplaceSel(cfail);
	}

}

/**************************************************************************************************/
//构造函数
CBodyBasics::CBodyBasics() :
m_pKinectSensor(NULL),
m_pCoordinateMapper(NULL),
m_pBodyFrameReader(NULL),
m_pDepthFrameReader(NULL),
m_pBodyIndexFrameReader(NULL)
{
	//pDlg = new CMFC_DEMO01Dlg();
}

// 析构函数
CBodyBasics::~CBodyBasics()
{
	SafeRelease(m_pBodyFrameReader);
	SafeRelease(m_pCoordinateMapper);
	SafeRelease(m_pDepthFrameReader);
	SafeRelease(m_pBodyIndexFrameReader);

	if (m_pKinectSensor)
	{
		m_pKinectSensor->Close();
	}
	SafeRelease(m_pKinectSensor);
}



