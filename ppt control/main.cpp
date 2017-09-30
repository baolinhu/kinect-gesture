#include "mykinect.h"
#include <iostream>
using namespace std;

int main()
{
	CBodyBasics myKinect;
	HRESULT hr = myKinect.InitializeDefaultSensor();
	if (SUCCEEDED(hr)){
		while (1){
			myKinect.Update();
		}
	}
	else{
		cout << "kinect initialization failed!" << endl;
		system("pause");
	}
}