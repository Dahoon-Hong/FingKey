#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/videoio/videoio.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/video/background_segm.hpp"
#include <opencv2/core/core.hpp>
#include <stdio.h>
#include <string>
#include "CamShift.h"
#include <iostream>
#include <ios>
#include <limits>
#include <exception>
#include <ctype.h>
#define PI 3.14159
using namespace std;
using namespace cv;
static void help()
{
	printf("\n"
		"This program demonstrated a simple method of connected components clean up of background subtraction\n"
		"When the program starts, it begins learning the background.\n"
		"You can toggle background learning on and off by hitting the space bar.\n"
		"Call\n"
		"./segment_objects [video file, else it reads camera 0]\n\n");
}
float distanceP2P(Point a, Point b){
	return sqrt(fabs(pow((a).x - (b).x, 2) + pow(a.y - b.y, 2)));
};
int main(int argc, char** argv)
{
	VideoCapture cap;
	bool update_bg_model = true;

	cap.open(0);

	Mat tmp_frame, bgmask, out_frame,init ,original;
	cap >> tmp_frame;
	cap >> init;

	if (tmp_frame.empty())
	{
		printf("can not read data from the video source\n");
		return -1;
	}
	//namedWindow("segmented", 1);
	//namedWindow("Original", 1);

	vector<cv::Point> trace;
	int trace_cnt = 0;

	try {
		for (;;)
		{
			cap >> tmp_frame;
			cap >> original;
			if (tmp_frame.empty())
				break;
			
//back substraction
			absdiff(tmp_frame, init, tmp_frame);

//make binaryMat
			cv::Mat grayscaleMat(tmp_frame.size(), CV_8U);
			cv::cvtColor(tmp_frame, grayscaleMat, CV_BGR2GRAY);
			cv::Mat binaryMat(grayscaleMat.size(), grayscaleMat.type());
			cv::threshold(grayscaleMat, binaryMat, 30, 255, cv::THRESH_BINARY);
			
			cv::Mat temp(binaryMat.size(), binaryMat.type());
			binaryMat.copyTo(temp);
			

			Rect boundingRect1(0, 0, temp.cols, temp.rows *5/6);
			Mat mask1 = Mat::zeros(temp.size(), temp.type());
			rectangle(mask1, boundingRect1, CV_RGB(255, 255, 255), -1);
			temp = temp & mask1;
		

			
			vector<vector<cv::Point>> contours;
			findContours(temp, contours, cv::noArray(), cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
			
			int maxK = 0;
			if (contours.size() == 0)
				continue;
			
			double maxArea = contourArea(contours[0]);
			for (int k = 0; k < contours.size(); k++){
				double area = contourArea(contours[k]);
				if (area > maxArea){
					maxK = k;
					maxArea = area;
				}
			}
			
			vector<int> hull;
			vector<cv::Point> handContour = contours[maxK];
			cv::convexHull(handContour, hull);

			vector<cv::Point> ptsHull;
			int min = temp.rows;
			int min_idx=0;
			for (int k = 0; k < hull.size(); k++){
				int i = hull[k];
				if (min > handContour[i].y){
					min_idx = i;
					min = handContour[i].y;
				}
				ptsHull.push_back(handContour[i]);
			}
			circle(original, handContour[min_idx], 6, cv::Scalar(0, 100, 255), -1);
			drawContours(original, vector<vector<cv::Point>>(1, ptsHull), 0, cv::Scalar(255, 0, 0), 2);
			for (int k = 0; k < contours.size(); k++){
				cv::Point2f center;
				cv::Moments M = moments(contours[k]);
				center.x = M.m10 / M.m00;
				center.y = M.m01 / M.m00;
				//printf("Center of hull -> x: %.3f\t y: %.3f\n", center.x, center.y);
			}
			/*
			//convex hull and points 
			vector<cv::Vec4i> defects;
			convexityDefects(handContour, hull, defects);
			for (int k = 1; k < defects.size(); k++){
				cv::Vec4i v = defects[k];
				cv::Point ptStart = handContour[v[0]];
				cv::Point ptEnd = handContour[v[1]];
				cv::Point ptFar = handContour[v[2]];
				float depth = v[3] / 256.0;


				float l1 = distanceP2P(ptStart, ptFar);
				float l2 = distanceP2P(ptEnd, ptFar);
				float dot = (ptStart.x - ptFar.x)*(ptEnd.x - ptFar.x) + (ptStart.y - ptFar.y)*(ptEnd.y - ptFar.y);
				float angle = acosf(dot / (l1*l2));
				angle = angle * 180 / PI;

				if (angle <95 && depth > 10){
					line(original, ptStart, ptFar, cv::Scalar(0, 255, 0), 2);
					line(original, ptEnd, ptFar, cv::Scalar(0, 255, 0), 2);
					circle(original, ptStart, 6, cv::Scalar(k * 10, 150, 150), 2);
					circle(original, ptEnd, 6, cv::Scalar(0, 0, 255), 2);
					circle(original, ptFar, 6, cv::Scalar(0, 0, 255), 2);
				}
			}
			*/
				
			//making mask
			Point2f center;
			float radius;
			minEnclosingCircle(handContour, center, radius);
		//	circle(original, center, radius, Scalar(255, 255, 255), -1);
			line(original, center, handContour[min_idx], Scalar(0, 100, 255), 5);
		//	imgThresholded = ~imgThresholded;
		//get the circle's bounding rect

		//	Rect boundingRect(center.x - radius, center.y - radius, radius * 2, radius * 2);
			Mat mask = Mat::zeros(binaryMat.size(), binaryMat.type());
			circle(mask, center, radius * 5 / 6, CV_RGB(255, 255, 255), 10);
		//	rectangle(mask, boundingRect, CV_RGB(255, 255, 255), -1);
		//	rectangle(mask1, boundingRect1, CV_RGB(255, 255, 255), -1);
				
			Mat img_parted(binaryMat.size(), binaryMat.type());
			img_parted = binaryMat & mask;
			Mat label;
			int num_label = connectedComponents(img_parted, label) - 1;
			if (num_label == 2) {
				trace.push_back(handContour[min_idx]);
				trace_cnt = 0;
			}
				
			
			else {
				printf("%d\n", num_label);
				trace_cnt++;
				if (trace_cnt >= 7)
					trace.clear();
			}
			

			for (int i = 1; i < trace.size(); i++)
			{
				//cout << "[" << trace[i].x << ", " << trace[i].y << "] ";
				line(original, trace[i-1], trace[i], Scalar(255, 100, 0), 5);
			}
			cout << endl;
			


			flip(original, original, 1);
			flip(binaryMat, binaryMat, 1);
			flip(img_parted, img_parted, 1);
			
			imshow("Original", original);
			//imshow("Segmented", temp);
			//imshow("binary", binaryMat);
			imshow("Parted", img_parted);
			
			
			int keycode = waitKey(30);
			if (keycode == 27)
				break;
		}
	}
	catch (exception& e) {
		cout << e.what() << endl;
	}
	

	
	return 0;
}