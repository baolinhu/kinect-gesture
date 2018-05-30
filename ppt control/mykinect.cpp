//--------------------------------------【程序说明】-------------------------------------------
//* 程序描述：基于Kinect的ppt演示系统
//* 开发测试所用IDE版本：Visual Studio 2013
//* 开发测试所用OpenCV版本：	3.0 beta
//* 开发测试所使用硬件：	KinectV2 Xbox
//* 操作系统：Windows 10
//* Kinect SDK版本：KinectSDK-v2.0-PublicPreview1409-Setup 
//* 2017年4月 Created by @胡保林  hu_nobuone@163.com
//------------------------------------------------------------------------------------------------

//--------------------------------------头文件和命名空间-------------------------------------------
#include "mykinect.h"
#include <iostream>
//#include <sphelper.h>//语音头文件
//#include <sapi.h>
#include<Windows.h>
#include<time.h>	//时间头文件
#include<opencv2/opencv.hpp>
#include<math.h>	//常用数学公式
using namespace std;
using namespace cv;

//--------------------------------------全局变量定义-------------------------------------------
bool leftDetection = FALSE;
bool rightDetection = FALSE;
bool beginDetection = FALSE;
bool vDetection = FALSE;				//速度达到阈值 标志位
bool IsDetection = FALSE;			//成功检测跌倒标志位
bool HeightDetection = FALSE;		//高度达到阈值标志位
int detecttime;							//跌倒后的计时
long  framenumber;			//骨骼帧编号
long  depthnumber;			//深度帧编号
static long  irnumber = 0;				//IR帧编号
static long  colornumber = 0;			//RGB帧编号
float  SpineHeightin, SpineHeightout;	//Spine的高度
float  SpineV;							//Spine的速度
static DWORD tout;						//时间变量

/// Initializes the default Kinect sensor
HRESULT CBodyBasics::InitializeDefaultSensor()
{
	//用于判断每次读取操作的成功与否
	HRESULT hr;

	//搜索kinect
	hr = GetDefaultKinectSensor(&m_pKinectSensor);
	if (FAILED(hr)){
		return hr;
	}

	//找到kinect设备
	if (m_pKinectSensor)
	{
		// Initialize the Kinect and get coordinate mapper and the body reader
		IBodyFrameSource* pBodyFrameSource = NULL;//读取骨架
		IDepthFrameSource* pDepthFrameSource = NULL;//读取深度信息
		IColorFrameSource* pColorFrameSource = NULL;//读取彩色信息
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
		//color frame
		if (SUCCEEDED(hr)){
			hr = m_pKinectSensor->get_ColorFrameSource(&pColorFrameSource);
		}

		if (SUCCEEDED(hr)){
			hr = pColorFrameSource->OpenReader(&m_pColorFrameReader);
		}

		//depth frame
		if (SUCCEEDED(hr)){
			hr = m_pKinectSensor->get_DepthFrameSource(&pDepthFrameSource);
		}

		if (SUCCEEDED(hr)){
			hr = pDepthFrameSource->OpenReader(&m_pDepthFrameReader);
		}

		//body index frame
		if (SUCCEEDED(hr)){
			hr = m_pKinectSensor->get_BodyIndexFrameSource(&pBodyIndexFrameSource);
		}

		if (SUCCEEDED(hr)){
			hr = pBodyIndexFrameSource->OpenReader(&m_pBodyIndexFrameReader);
		}

		SafeRelease(pBodyFrameSource);
		SafeRelease(pDepthFrameSource);
		SafeRelease(pColorFrameSource);
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

	//colorImg,用于画彩色信息的MAT
	colorImg.create(cColorHeight, cColorWidth, CV_8UC3);
	colorImg.setTo(0);

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
	IColorFrame* pColorFrame = NULL;
	IBodyIndexFrame* pBodyIndexFrame = NULL;//背景二值图

	//记录每次操作的成功与否
	HRESULT hr = S_OK;

	//---------------------------------------获取背景二值图并显示---------------------------------
	//if (SUCCEEDED(hr)){
	//	hr = m_pBodyIndexFrameReader->AcquireLatestFrame(&pBodyIndexFrame);//获得背景二值图信息
	//}
	//if (SUCCEEDED(hr)){
	//	BYTE *bodyIndexArray = new BYTE[cDepthHeight * cDepthWidth];//背景二值图是8为uchar，有人是黑色，没人是白色
	//	pBodyIndexFrame->CopyFrameDataToArray(cDepthHeight * cDepthWidth, bodyIndexArray);

	//	//把背景二值图画到MAT里
	//uchar* skeletonData = (uchar*)skeletonImg.data;
	//	for (int j = 0; j < cDepthHeight * cDepthWidth; ++j){
	//		*skeletonData = bodyIndexArray[j]; ++skeletonData;
	//		*skeletonData = bodyIndexArray[j]; ++skeletonData;
	//		*skeletonData = bodyIndexArray[j]; ++skeletonData;
	//	}
	//	delete[] bodyIndexArray;
	//}
	//SafeRelease(pBodyIndexFrame);//必须要释放，否则之后无法获得新的frame数据

	////-----------------------获取深度数据并显示--------------------------
	//if (SUCCEEDED(hr)){
	//	hr = m_pDepthFrameReader->AcquireLatestFrame(&pDepthFrame);//获得深度数据
	//}
	//if (SUCCEEDED(hr)){
	//	UINT16 *depthArray = new UINT16[cDepthHeight * cDepthWidth];//深度数据是16位unsigned int
	//	pDepthFrame->CopyFrameDataToArray(cDepthHeight * cDepthWidth, depthArray);

	//	//把深度数据画到MAT中
	//	uchar* depthData = (uchar*)depthImg.data;
	//	for (int j = 0; j < cDepthHeight * cDepthWidth; ++j){
	//		*depthData = depthArray[j];
	//		++depthData;
	//	}
	//	delete[] depthArray;
	//}
	//SafeRelease(pDepthFrame);//必须要释放，否则之后无法获得新的frame数据
	//imshow("depthImg", depthImg);
	//cv::waitKey(5);


	//-----------------------获取彩色数据并显示--------------------------

	UINT nBufferSize_coloar = 0;
	RGBQUAD *pBuffer_color = NULL;
	
	if (SUCCEEDED(hr)){
		hr = m_pColorFrameReader->AcquireLatestFrame(&pColorFrame);//获得彩色数据
	}
	if (SUCCEEDED(hr)){
		ColorImageFormat imageFormat = ColorImageFormat_None;
		RGBQUAD* m_pColorRGBX = new RGBQUAD[cColorWidth * cColorHeight];
		if (SUCCEEDED(hr))
		{
			hr = pColorFrame->get_RawColorImageFormat(&imageFormat);
		}
		if (SUCCEEDED(hr))
		{
			hr = pColorFrame->get_RawColorImageFormat(&imageFormat);
		}
		if (SUCCEEDED(hr))
		{
			if (imageFormat == ColorImageFormat_Bgra)//这里有两个format，不知道具体含义，大概一个预先分配内存，一个需要自己开空间吧  
			{
				hr = pColorFrame->AccessRawUnderlyingBuffer(&nBufferSize_coloar, reinterpret_cast<BYTE**>(&pBuffer_color));
			}
			else if (m_pColorRGBX)
			{
				pBuffer_color = m_pColorRGBX;
				nBufferSize_coloar = cColorWidth * cColorHeight * sizeof(RGBQUAD);
				hr = pColorFrame->CopyConvertedFrameDataToArray(nBufferSize_coloar, reinterpret_cast<BYTE*>(pBuffer_color), ColorImageFormat_Bgra);
			}
			else
			{
				hr = E_FAIL;
			}
			uchar* p_mat = colorImg.data;
			
			const RGBQUAD* pBufferEnd = pBuffer_color + (cColorWidth * cColorHeight);

			while (pBuffer_color < pBufferEnd)
			{
				*p_mat = pBuffer_color->rgbBlue;
				p_mat++;
				*p_mat = pBuffer_color->rgbGreen;
				p_mat++;
				*p_mat = pBuffer_color->rgbRed;
				p_mat++;

				++pBuffer_color;
			}
			
			//colorImg = ConvertMat(pBuffer_color, cColorWidth, cColorHeight);
		}
		//BYTE *colorArray = new BYTE[cColorHeight * cColorWidth];//彩色数据是8位BYTE
		//pColorFrame->CopyRawFrameDataToArray(cColorHeight * cColorWidth, colorArray);

		////把彩色数据画到MAT中
		//uchar* colorData = (uchar*)colorImg.data;
		//for (int j = 0; j < cColorHeight * cColorWidth; ++j){
		//	*colorData = colorArray[j];
		//	++colorData;
		//	
		//}
		//cout << *colorData << "\t";
		//delete[] colorArray;

		delete[] m_pColorRGBX;

	}
	SafeRelease(pColorFrame);//必须要释放，否则之后无法获得新的frame数据
	namedWindow("colorImg", 0);
	resizeWindow("colorImg", 1024, 720);
	imshow("colorImg", colorImg);

	cv::waitKey(5);
	//-----------------------------获取骨架并显示----------------------------
	if (SUCCEEDED(hr)){
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
			//Joint joints[JointType_Count];//存储关节点类
			//DepthSpacePoint *depthSpacePosition = new DepthSpacePoint[_countof(joints)];
			//Detection(depthSpacePosition);
		}

		for (int i = 0; i < _countof(ppBodies); ++i)
		{
			SafeRelease(ppBodies[i]);//释放所有
		}
	}
	SafeRelease(pBodyFrame);//必须要释放，否则之后无法获得新的frame数据

}

/// Handle new body data
void CBodyBasics::ProcessBody(int nBodyCount, IBody** ppBodies)
{
	//记录操作结果是否成功
	HRESULT hr;

	//对于每一个IBody
	for (int i = 0; i < nBodyCount; ++i)
	{
		IBody* pBody = ppBodies[i];
		if (pBody)//还没有搞明白这里pBody和下面的bTracked有什么区别
		{
			BOOLEAN bTracked = false;
			hr = pBody->get_IsTracked(&bTracked);

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
				ColorSpacePoint *colorSpacePosition = new ColorSpacePoint[_countof(joints)];
				//获得关节点类
				hr = pBody->GetJoints(_countof(joints), joints);
				if (SUCCEEDED(hr))
				{
					for (int j = 0; j < _countof(joints); ++j)
					{
						//将关节点坐标从摄像机坐标系（-1~1）转到深度坐标系（424*512）
						m_pCoordinateMapper->MapCameraPointToDepthSpace(joints[j].Position, &depthSpacePosition[j]);
						m_pCoordinateMapper->MapCameraPointToColorSpace(joints[j].Position, &colorSpacePosition[j]);
					}


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

				/****************************PPT动作控制******************************************/
					//检测跌倒。距离超过0.5米时才检测，否则不检测
					if (fabs(joints[JointType_SpineMid].Position.Z) > 0.5)
						//Detection(joints);
						PPTControl(joints);
					



					cDrawHandState(colorSpacePosition[JointType_HandLeft], false);
					cDrawHandState(colorSpacePosition[JointType_HandRight], false);
					cDrawHandState(colorSpacePosition[JointType_HandLeft], leftDetection);
					cDrawHandState(colorSpacePosition[JointType_HandRight], rightDetection);
				}
				delete[] depthSpacePosition;
				delete[] colorSpacePosition;
			}
		}
	}
	namedWindow("skeletonImg", 0);
	resizeWindow("skeletonImg", 640, 480);
	cv::imshow("skeletonImg", skeletonImg);
	cv::waitKey(5);
}
///ppt控制
void CBodyBasics::PPTControl(Joint joints[])
{
	//向右
	if (fabs(joints[JointType_Head].Position.X - joints[JointType_HandRight].Position.X) > 0.45)
	{
		if (!leftDetection&&!rightDetection)
		{
			keybd_event(39, 0, 0, 0);
			rightDetection = TRUE;

		}

	}
	else
		rightDetection = false;
	//向左
	if (fabs(joints[JointType_Head].Position.X - joints[JointType_HandLeft].Position.X) > 0.45)
	{
		if (!leftDetection&&!rightDetection)
		{
			keybd_event(37, 0, 0, 0);
			leftDetection = TRUE;

		}
	}
	else
		leftDetection = false;

	if (fabs(joints[JointType_Head].Position.X - joints[JointType_HandLeft].Position.X) > 0.45)
	{
		if (!leftDetection&&!rightDetection)
		{
			keybd_event(37, 0, 0, 0);
			leftDetection = TRUE;

		}
	}
	else
		leftDetection = false;
}

//检测程序
void CBodyBasics::Detection(Joint joints[])
{
	static double tin, tout;
	//double tframe;

	//计算每相邻10帧的高度差，从而计算速度，1,11,12,22
	//大概30帧每秒，那么10帧就是0.33秒，
	if (framenumber % 11 == 1)		//framenumber是帧序列号，自己定义的
	{
		tin = static_cast<double>(GetTickCount());
		//cout << "tin是" << tin << endl;
		SpineHeightin = joints[JointType_SpineMid].Position.Y;
		//cout << "当前帧号为：" << frmamenumber << endl;
		//cout << "当前SpineHeightin的高度为" << SpineHeightin << "  m"<<endl;
	}
	if (!(framenumber % 11))
	{
		tout = static_cast<double>(GetTickCount());
		//cout << frmamenumber << endl;
		//cout <<"tout是"<< tout << endl;
		//cout << "每10帧计算一次下降的速度" << endl;
		SpineHeightout = joints[JointType_SpineMid].Position.Y;
		//cout << "当前帧号为：" << frmamenumber << endl;
		//  cout << "***********************************" << endl;
		//  cout << "当前SpineHeightin的高度为" << SpineHeightin << "  m" << endl;
		//tframe = (tout - tin) / getTickFrequency();
		// cout <<tframe << endl;
		//  cout << getTickFrequency()<<endl;
		//cout << "当前SpineHeightout的高度为" << SpineHeightout << "  m" << endl;
		//SpineV = (SpineHeightin - SpineHeightout) / tframe;
		SpineV = SpineHeightin - SpineHeightout;
		//cout << "SpineV是多少？？" << SpineV << endl;
		if ((SpineV) > 0.35)	//文献中给定的数据是1.35m/s，这个可能要根据实际情况略有调整	
		{
			vDetection = true;
			stringstream stream0;
			string str, str1;
			stream0 << SpineV;
			stream0 >> str;
			str1 = "身体中心向下的速度是： " + str + " m/s\r\n";
			//CString cstr = str1.c_str();
			//pDlg0->m_outedit.ReplaceSel(cstr);
			cout << "身体中心向下的速度是：   " << (SpineV) << "    m/s" << endl;

		}
		else vDetection = false;
	}
	//检测高度特征，对地面的检测，离地面的距离。这里转化为spine和foot之间的高度。
	if ((joints[JointType_SpineBase].Position.Y - joints[JointType_FootRight].Position.Y) <= 0.28)
	{

		cv::waitKey(15);
		if ((joints[JointType_SpineBase].Position.Y - joints[JointType_FootRight].Position.Y) <= 0.28)
		{
			if (vDetection)
			{

				HeightDetection = TRUE;
				//IsDetection = TRUE;
				vDetection = false;

				stringstream stream0, stream1;
				string str, str1, str2;
				//CString cstr, cstr1, cstr2;
				//cstr1 = "成功检测跌倒\r\n";

				stream0 << joints[JointType_SpineBase].Position.Y;
				stream0 >> str;
				str1 = "JointType_SpineBase的高度是： " + str + " m\r\n";
				//cstr = str1.c_str();

				stream1 << joints[JointType_FootRight].Position.Y;
				stream1 >> str;
				str2 = "JointType_FootRight的高度是： " + str + " m\r\n";
				//cstr2 = str1.c_str();


				cout << "***************" << "成功检测跌倒" << "***************" << endl;
				cout << "JointType_SpineBase的高度是  " << joints[JointType_SpineBase].Position.Y << "\tm" << endl;
				//cout << "***************" << endl;
				cout << "JointType_FootRight的高度是  " << joints[JointType_FootRight].Position.Y << "\tm" << endl;
				//SaveSkeletonImg();	//检测到跌倒事件，保存当前图片信息。
				//SaveDepthImg();
				//SpeechDetection();		//语音询问

			}
		}
		else
		{
			HeightDetection = FALSE;
		}
	}
}

//求2个骨骼点之间的距离
double CBodyBasics::Distance(Joint p1, Joint p2)
{
	double dist = 0;
	dist = sqrt(pow(p2.Position.X - p1.Position.X, 2) +
		pow(p2.Position.Y - p1.Position.Y, 2));
	return dist;
}

//画手的状态
void CBodyBasics::DrawHandState(const DepthSpacePoint depthSpacePosition, HandState handState)
{
	//给不同的手势分配不同颜色BGR
	CvScalar color;
	switch (handState){
	case HandState_Open:
		color = cvScalar(255, 0, 0);
		break;
	case HandState_Closed:
		color = cvScalar(0, 255, 0);
		break;
	case HandState_Lasso:
		color = cvScalar(0, 0, 255);
		break;
	default://如果没有确定的手势，就不要画
		return;
	}

	circle(skeletonImg,
		cvPoint(depthSpacePosition.X, depthSpacePosition.Y),
		20, color, -1);


}

void CBodyBasics::cDrawHandState(const ColorSpacePoint colorSpacePosition, bool isHighlighted)
{
	if (isHighlighted)
	circle(colorImg,
		cvPoint(colorSpacePosition.X, colorSpacePosition.Y),
		100, cvScalar(255, 0, 0), -1);
	else
		circle(colorImg,
		cvPoint(colorSpacePosition.X, colorSpacePosition.Y),
		60, cvScalar(255, 0, 0), -1);
}

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
		line(skeletonImg, p1, p2, cvScalar(255, 255, 255), 8);
	}
	else
	{
		//不确定的骨架，用红色直线
		line(skeletonImg, p1, p2, cvScalar(0, 0, 255), 8);
	}
}


/// Constructor
CBodyBasics::CBodyBasics() :
m_pKinectSensor(NULL),
m_pCoordinateMapper(NULL),
m_pBodyFrameReader(NULL){}
//m_pColorFrameReader(NULL)

/// Destructor
CBodyBasics::~CBodyBasics()
{
	SafeRelease(m_pBodyFrameReader);
	SafeRelease(m_pCoordinateMapper);
	//SafeRelease(m_pColorFrameReader);

	if (m_pKinectSensor)
	{
		m_pKinectSensor->Close();
	}
	SafeRelease(m_pKinectSensor);
}



