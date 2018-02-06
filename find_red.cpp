#include "opencv2/objdetect.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include <iostream>
#include <errno.h>
#include <wiringPi.h>
#include <wiringSerial.h>

using namespace cv;

/*
H为色调
S为饱和度
V为亮度
*/

#define STEP 5
#define CONTINUES 10

int main()
{
	int fd ;
    	char info[12];
	CvScalar point;
	Mat3b bgr, hsv;
	uchar *p;
	Mat1b mask1, mask2, mask;
	int x[4] = {0};//存放上下左右四个点的横坐标
	int y[4] = {0};//存放上下左右四个点的纵坐标
	int i, j, count=0,flag=0;//一些数组下标与循环变量

	//打开摄像头
	VideoCapture cap(0); 
	if (!cap.isOpened()) 
		return -1;
	
	if ((fd = serialOpen ("/dev/ttySAC0", 9600)) < 0)
	{
	      fprintf (stderr, "Unable to open serial device: %s\n", strerror (errno)) ;
	      return 1 ;
        }
        if (wiringPiSetup () == -1)
        {
              fprintf (stdout, "Unable to start wiringPi: %s\n", strerror (errno)) ;
	      return 1 ;
	}

	while (1)
	{
		cap >> bgr;

		cvtColor(bgr, hsv, COLOR_BGR2HSV);
		
		//滤出
		inRange(hsv, Scalar(0, 150, 0), Scalar(10, 255, 255), mask1);
		inRange(hsv, Scalar(170, 150, 0), Scalar(180, 255, 255), mask2);

		mask = mask1 | mask2;

		//腐蚀
		erode(mask,mask,getStructuringElement(MORPH_ELLIPSE,Size(5,5)));
		dilate(mask, mask, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)));
		
		//膨胀
		dilate(mask, mask, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)));
		erode(mask, mask, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)));

		//row为纵坐标，col为横坐标
		for(i = 1; i < mask.rows ; i += STEP)
		{
			p = mask.ptr<uchar>(i);

			//从上到下扫描,记住最上面的像素点
			for (j = 1; j < mask.cols; j ++)
			{
				if (*(p+j)>128)
				{
					count++;//记录连续点数
					if (count > CONTINUES)//检测到为上部第一个红色点
					{
						x[0] = j;//记录横坐标
						y[0] = i;//记录纵坐标
					}
				}

				//一旦出现黑点
				if (*(p + j) < 128)
					count = 0;//清除连续点数

				if (count>CONTINUES)
					break;//上点找到，跳出此循环
			}

			if (count>CONTINUES)
			{
				count = 0;
				break;//上点找到，跳出此循环
			}
		}

		for (i = mask.rows; i >1; i -= STEP)
		{
			p = mask.ptr<uchar>(i-1);

			for (j = 1; j < mask.cols; j++)
			{
				if (*(p + j) > 128)
				{
					count++;//记录连续点数
					if (count > CONTINUES)
					{
						x[1] = j;//记录横坐标
						y[1] = i;//记录纵坐标
					}
				}

				//一旦出现黑点
				if (*(p + j) < 128)
					count = 0;//清除连续点数

				if (count>CONTINUES)
					break;//下点找到，跳出此循环
			}
			if (count>CONTINUES)
			{
				count = 0;
				break;//上下点都找到，跳出此循环
			}
		}
		
		p = mask.ptr<uchar>((y[0] + y[1]) / 2);

		for (j = 1; j < mask.cols; j++)
		{
			if (*(p + j) > 128)
				count++;

			if (count > 10)
			{
				if (!flag)
				{
					flag = 1;
					x[2] = j;
					y[2] = (y[0] + y[1]) / 2;
				}
				else
				{
					x[3] = j;
					y[3] = (y[0] + y[1]) / 2;
				}
			}

			if (*(p + j) < 128)
				count = 0;
		}

		flag = 0;

		line(bgr,Point(x[0],y[0]),Point(x[2],y[2]),Scalar(0,0,0),3);
		line(bgr, Point(x[2], y[2]), Point(x[1], y[1]), Scalar(0, 0, 0), 3);
		line(bgr, Point(x[1], y[1]), Point(x[3], y[3]), Scalar(0, 0, 0), 3);
		line(bgr, Point(x[3], y[3]), Point(x[0], y[0]), Scalar(0, 0, 0), 3);

		arrowedLine(bgr,Point((x[0]+x[1])/2,(y[2]+y[3])/2),Point(bgr.cols/2,bgr.rows/2), Scalar(0, 255, 255),5);

		printf("centerPoint(%d,%d),cneterLineLen:%d\n",(x[0]+x[1])/2,(y[2]+y[3])/2,x[3]-x[2]);

		imshow("bgr",bgr);

		char c = waitKey(30);

#if 0
	        if(serialDataAvail (fd))
	        {
	            c = serialGetchar (fd);
		}
#endif
		
#if 1
//this is uart test
		info[0] = ((x[0]+x[1])/2)/100+'0';
		info[1] = ((x[0]+x[1])/2)/10%10+'0';
		info[2] = ((x[0]+x[1])/2)%10+'0';
		info[3] = ',';
		info[4] = ((y[2]+y[3])/2)/100+'0';
		info[5] = ((y[2]+y[3])/2)/10%10+'0';
		info[6] = ((y[2]+y[3])/2)%10+'0';
		info[7] = '|';
		info[8] = (x[3]-x[2])/100+'0';
		info[9] = (x[3]-x[2])/10%10+'0';
		info[10] = (x[3]-x[2])%10+'0';
		info[11] = '\n';	
		
		for(i = 0; i < 12; i++)
            serialPutchar (fd, info[i]) ;		
#endif


		if (27 == c)
			break;
	}

	cvDestroyWindow("bgr");
	cvDestroyWindow("Mask");
	return 0;
}
