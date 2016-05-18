#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/videoio/videoio.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/video/background_segm.hpp"
#include <opencv2/core/core.hpp>
#include <stdio.h>
#include <string>
#include "CamShift.h"
#include <ctime>
#include <iostream>
#include <ios>
#include <limits>
#include <exception>
#include <ctype.h>
#define PI 3.14159
using namespace std;
using namespace cv;

#define	CHAR_RECOGNITION_MODE	100
#define NUM_RECOGNITION_MODE	200
#define SPACE_RECOGNITION_MODE	400
#define DELETE_RECOGNITION_MODE	800


float distanceP2P(Point a, Point b){
	return sqrt(fabs(pow((a).x - (b).x, 2) + pow(a.y - b.y, 2)));
};

bool checkPointInRect(Rect r, Point p){
	if ((p.x >= r.x && p.x <= r.x + r.width) && (p.y >= r.y && p.y <= r.y + r.height))
		return true;
	else
		return false;
}


int main(int argc, char** argv)
{
	VideoCapture cap;
	bool update_bg_model = true;

	cap.open(0);
	cap.set(CV_CAP_PROP_FPS, 30);
	Mat tmp_frame, bgmask, out_frame,init ,original;
	cap >> tmp_frame;
	cap >> init;

	if (tmp_frame.empty())
	{
		printf("can not read data from the video source\n");
		return -1;
	}

	// finger route
	vector<cv::Point> trace;
	int trace_cnt = 0;

	// Character or Number recognition mode
	// Add space, delete mode
	Rect NumRecogRect(10, 10, 90, 50);
	Rect CharRecogRect(110, 10, 90, 50);
	Rect SpaceRecogRect(210, 10, 90, 50);
	Rect DeleteRecogRect(310, 10, 90, 50);

	int RecogMode = NUM_RECOGNITION_MODE;
	int RecogModeChange_cnt = 0;
	int CAM_WIDTH = init.size().width;
	int CAM_HEIGHT = init.size().height;
	
	Point NumStringPoint(CAM_WIDTH - NumRecogRect.width + 10, 15 + NumRecogRect.height/2);
	Point CharStringPoint(CAM_WIDTH - (CharRecogRect.width + NumRecogRect.width) - 5, 15 + CharRecogRect.height / 2);
	Point SpaceStringPoint(CAM_WIDTH - (SpaceRecogRect.width + CharRecogRect.width + NumRecogRect.width) - 17, 15 + SpaceRecogRect.height / 2);
	Point DeleteStringPoint(CAM_WIDTH - (DeleteRecogRect.width + SpaceRecogRect.width + CharRecogRect.width + NumRecogRect.width) - 15, 15 + DeleteRecogRect.height / 2);

	//rect size for get char
	//int sizeofRect = int(CAM_WIDTH/7);
	int sizeofRect = 100;

	// properties of saved image
	vector<int> compression_params;
	compression_params.push_back(CV_IMWRITE_PNG_COMPRESSION);
	compression_params.push_back(9);
	int img_cnt = 0;

	int num_frame = 0;
	time_t start, end;
	time(&start);
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
		
			erode(binaryMat, binaryMat, Mat(), Point(-1, -1), 2, 1, 1);
			dilate(binaryMat, binaryMat, Mat(), Point(-1, -1), 5, 1, 1);
			erode(binaryMat, binaryMat, Mat(), Point(-1, -1), 2, 1, 1);
			
			cv::Mat temp(binaryMat.size(), binaryMat.type());
			binaryMat.copyTo(temp);
			
//making mask
			Rect boundingRect1(0, 0, temp.cols, temp.rows *5/6);
			Mat mask1 = Mat::zeros(temp.size(), temp.type());
			rectangle(mask1, boundingRect1, CV_RGB(255, 255, 255), -1);
			temp = temp & mask1;

			//contour
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
			// convex hull
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
			}
			
				
			//making mask
			Point2f center;
			float radius;
			minEnclosingCircle(handContour, center, radius);
			line(original, center, handContour[min_idx], Scalar(0, 100, 255), 5);

		//get the circle's bounding rect

		//	Rect boundingRect(center.x - radius, center.y - radius, radius * 2, radius * 2);
			Mat mask = Mat::zeros(binaryMat.size(), binaryMat.type());
			circle(mask, center, radius * 5 / 6, CV_RGB(255, 255, 255), 10);
		//	rectangle(mask, boundingRect, CV_RGB(255, 255, 255), -1);
		//	rectangle(mask1, boundingRect1, CV_RGB(255, 255, 255), -1);
				
//check number of finger
			Mat img_parted(binaryMat.size(), binaryMat.type());
			img_parted = binaryMat & mask;
			Mat label;
			int num_label = connectedComponents(img_parted, label) - 1;
			if (num_label == 2) {
				trace.push_back(handContour[min_idx]);
				trace_cnt = 0;
			}
			else {
				trace_cnt++;
				if (trace_cnt >= 7) {
					trace.clear();
					img_cnt++;
				}
			}
			
			imshow("bin", binaryMat);

			int idx = 0;
			int startFrame = 0;
			for (int i = 1; i < trace.size(); i++)
			{
				int init_x = trace[0].x - idx*sizeofRect + sizeofRect * 2 / 7, init_y = trace[0].y + 2/7*sizeofRect;
				line(original, trace[i-1], trace[i], Scalar(255, 100, 0), 5);
				line(binaryMat, trace[i - 1], trace[i], Scalar(255, 100, 0), 5);
				rectangle(original, Rect(init_x - sizeofRect, init_y, sizeofRect, sizeofRect), CV_RGB(255, 0, 0), 3);
				if (trace[i].x < (init_x - sizeofRect)){
					//send data frome startFrame to currentFrame(==i)
					//if delete mode, the number of idx represents the number of character will be deleting 
					idx++;
					startFrame = i;
					cv::Mat subImg = binaryMat(cv::Rect(init_x - sizeofRect, init_y, sizeofRect, sizeofRect));
					flip(subImg, subImg, 1);
					bitwise_not(subImg, subImg);
					imwrite("./images/" + to_string(idx + img_cnt * 4) + ".png", subImg, compression_params);
					imshow("./images/" + to_string(idx + img_cnt * 4) + ".png", subImg);
					
				}
			}

//Mode select
			Mat overlay(original.size(), original.type());
			original.copyTo(overlay);

			if (checkPointInRect(CharRecogRect, handContour[min_idx])){
				RecogModeChange_cnt++;
				if (RecogModeChange_cnt == 7) {
					RecogMode = CHAR_RECOGNITION_MODE;
					RecogModeChange_cnt = 0;
					trace.clear();
					printf("CHARACTER RECOGNITION MODE!\n");
				}
			}
			else if (checkPointInRect(NumRecogRect, handContour[min_idx])){
				RecogModeChange_cnt++;
				if (RecogModeChange_cnt == 7) {
					RecogMode = NUM_RECOGNITION_MODE;
					RecogModeChange_cnt = 0;
					trace.clear();
					printf("NUMBER RECOGNITION MODE!\n");
				}
			}
			else if (checkPointInRect(SpaceRecogRect, handContour[min_idx])){
				RecogModeChange_cnt++;
				if (RecogModeChange_cnt == 7) {
					RecogMode = SPACE_RECOGNITION_MODE;
					RecogModeChange_cnt = 0;
					trace.clear();
					printf("SPACE RECOGNITION MODE!\n");
				}
			}
			else if (checkPointInRect(DeleteRecogRect, handContour[min_idx])){
				RecogModeChange_cnt++;
				if (RecogModeChange_cnt == 7) {
					RecogMode = DELETE_RECOGNITION_MODE;
					RecogModeChange_cnt = 0;
					trace.clear();
					printf("DELETE RECOGNITION MODE!\n");
				}
			}
			else
				RecogModeChange_cnt = 0;


			if (RecogMode == CHAR_RECOGNITION_MODE) {
				rectangle(overlay, CharRecogRect, CV_RGB(50, 205, 50), -1);
				rectangle(overlay, NumRecogRect, CV_RGB(128, 128, 128), -1);
				rectangle(overlay, SpaceRecogRect, CV_RGB(128, 128, 128), -1);
				rectangle(overlay, DeleteRecogRect, CV_RGB(128, 128, 128), -1);
			}
			else if (RecogMode == NUM_RECOGNITION_MODE){
				rectangle(overlay, CharRecogRect, CV_RGB(128, 128, 128), -1);
				rectangle(overlay, NumRecogRect, CV_RGB(50, 205, 50), -1);
				rectangle(overlay, SpaceRecogRect, CV_RGB(128, 128, 128), -1);
				rectangle(overlay, DeleteRecogRect, CV_RGB(128, 128, 128), -1);
			}
			else if (RecogMode == SPACE_RECOGNITION_MODE){
				rectangle(overlay, CharRecogRect, CV_RGB(128, 128, 128), -1);
				rectangle(overlay, NumRecogRect, CV_RGB(128, 128, 128), -1);
				rectangle(overlay, SpaceRecogRect, CV_RGB(50, 205, 50), -1);
				rectangle(overlay, DeleteRecogRect, CV_RGB(128, 128, 128), -1);
			}
			else if (RecogMode == DELETE_RECOGNITION_MODE){
				rectangle(overlay, CharRecogRect, CV_RGB(128, 128, 128), -1);
				rectangle(overlay, NumRecogRect, CV_RGB(128, 128, 128), - 1);
				rectangle(overlay, SpaceRecogRect, CV_RGB(128, 128, 128), -1);
				rectangle(overlay, DeleteRecogRect, CV_RGB(50, 205, 50), -1);
			}

			addWeighted(overlay, 0.5, original, 1 - 0.5, 0, original);

			flip(original, original, 1);
			//flip(binaryMat, binaryMat, 1);
			flip(img_parted, img_parted, 1);

			putText(original, "NUM", NumStringPoint, 2, 0.7, Scalar::all(255));
			putText(original, "CHAR", CharStringPoint, 2, 0.7, Scalar::all(255));
			putText(original, "DEL", DeleteStringPoint, 2, 0.7, Scalar::all(255));
			putText(original, "SPACE", SpaceStringPoint, 2, 0.7, Scalar::all(255));
			
			time(&end);
			num_frame++;

			std::ostringstream strs;
			strs << num_frame / difftime(end, start);
			std::string fpsString = strs.str();
			putText(original, fpsString, Point(10, 30), 2, 0.7, Scalar::all(255));

			imshow("Original", original);
			//imshow("binary", binaryMat);
			
			
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