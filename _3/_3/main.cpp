#include <iostream>

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include <Windows.h>
#include <string>
#include <math.h>
#define PI 3.14159

using std::cout;
using std::endl;
using std::vector;
using std::distance;
using std::string;
using std::to_string;
using namespace cv;
float distanceP2P(Point a, Point b){
	return sqrt(fabs(pow((a).x - (b).x, 2) + pow(a.y - b.y, 2)));
};
int main(int argc, char** argv)
{
	cv::VideoCapture cap(0); //capture the video from web cam

	if (!cap.isOpened())  // if not success, exit program
	{
		cout << "Cannot open the web cam" << endl;
		return -1;
	}
	int iLowH = 0;	int iHighH = 179;	int iLowS = 0;	int iHighS = 255;	int iLowV = 0;	int iHighV = 255;
	//Create trackbars in "Control" window
	namedWindow("Control", CV_WINDOW_AUTOSIZE); //create a window called "Control"
	cvCreateTrackbar("LowH", "Control", &iLowH, 179); //Hue (0 - 179)
	cvCreateTrackbar("HighH", "Control", &iHighH, 179);

	cvCreateTrackbar("LowS", "Control", &iLowS, 255); //Saturation (0 - 255)
	cvCreateTrackbar("HighS", "Control", &iHighS, 255);

	cvCreateTrackbar("LowV", "Control", &iLowV, 255); //Value (0 - 255)
	cvCreateTrackbar("HighV", "Control", &iHighV, 255);

	while (true)
	{
		Sleep(50);
		cv::Mat imgOriginal;
		bool bSuccess = cap.read(imgOriginal); // read a new frame from video

		if (!bSuccess) //if not success, break loop
		{
			cout << "Cannot read a frame from video stream" << endl;
			break;
		}
	//	cvNot(imgOriginal, imgOriginal);



		cv::Mat imgHSV;
		cvtColor(imgOriginal, imgHSV, cv::COLOR_BGR2HSV); //Convert the captured frame from BGR to HSV
		cv::Mat imgThresholded;

		inRange(imgHSV, cv::Scalar(iLowH, iLowS, iLowV), cv::Scalar(iHighH, iHighS, iHighV), imgThresholded); //Threshold the image

		//morphological opening (remove small objects from the foreground)
		erode(imgThresholded, imgThresholded, cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5, 5)), cv::Point(-1, -1), 1);
		dilate(imgThresholded, imgThresholded, cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5, 5)), cv::Point(-1, -1), 1);
		
		//morphological closing (fill small holes in the foreground)
		dilate(imgThresholded, imgThresholded, cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5, 5)), cv::Point(-1, -1), 3);
		erode(imgThresholded, imgThresholded, cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5, 5)), cv::Point(-1, -1), 3);
		

		vector<vector<cv::Point>> contours;
		findContours(imgThresholded, contours, cv::noArray(), cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
		int maxK = 0;
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
		for (int k = 0; k < hull.size(); k++){
			int i = hull[k];
			ptsHull.push_back(handContour[i]);
		}
		drawContours(imgOriginal, vector<vector<cv::Point>>(1, ptsHull), 0, cv::Scalar(255, 0, 0), 2);
		for (int k = 0; k < contours.size(); k++){
			cv::Point2f center;
			cv::Moments M = moments(contours[k]);
			center.x = M.m10 / M.m00;
			center.y = M.m01 / M.m00;
			//printf("Center of hull -> x: %.3f\t y: %.3f\n", center.x, center.y);
		}
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
				line(imgOriginal, ptStart, ptFar, cv::Scalar(0, 255, 0), 2);
				line(imgOriginal, ptEnd, ptFar, cv::Scalar(0, 255, 0), 2);
				circle(imgOriginal, ptStart, 6, cv::Scalar(k * 10, 150, 150), 2);
				circle(imgOriginal, ptEnd, 6, cv::Scalar(0, 0, 255), 2);
				circle(imgOriginal, ptFar, 6, cv::Scalar(0, 0, 255), 2);


				//	putText(imgOriginal, string(to_string(angle)), ptFar, cv::FONT_HERSHEY_PLAIN, 2.0f, cv::Scalar(0, 0, 0));
				//	putText(imgOriginal, string(to_string(l1)), ptStart, cv::FONT_HERSHEY_PLAIN, 3.0f, cv::Scalar(0, 0, 0));
				//	putText(imgOriginal, string(to_string(l2)), ptEnd, cv::FONT_HERSHEY_PLAIN, 3.0f, cv::Scalar(0, 0, 0));
			}
		}
		//redundant just for test
			inRange(imgHSV, Scalar(iLowH, iLowS, iLowV), Scalar(iHighH, iHighS, iHighV), imgThresholded); //Threshold the image
			//morphological opening (remove small objects from the foreground)
			erode(imgThresholded, imgThresholded, cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5, 5)), cv::Point(-1, -1), 1);
			dilate(imgThresholded, imgThresholded, cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5, 5)), cv::Point(-1, -1), 1);

			//morphological closing (fill small holes in the foreground)
			dilate(imgThresholded, imgThresholded, cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5, 5)), cv::Point(-1, -1), 3);
			erode(imgThresholded, imgThresholded, cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5, 5)), cv::Point(-1, -1), 3);
//
		//making mask
			Point2f center;
			float radius;
			minEnclosingCircle(handContour, center, radius);
		//	imgThresholded = ~imgThresholded;
		// get the circle's bounding rect
			
			Rect boundingRect(center.x - radius, center.y - radius, radius * 2, radius * 2);
			Mat mask = Mat::zeros(imgThresholded.size(), CV_8UC1);
			circle(mask, center, radius*4/5, CV_RGB(255, 255, 255),15, 8);
			// imagePart : hand focused image
			Mat imagePart = Mat::zeros(imgOriginal.size(), imgOriginal.type());
			imgOriginal.copyTo(imagePart, mask);
		
		circle(imgOriginal, center, radius, Scalar(255, 0, 0), 2);
		imshow("Original", imgThresholded); //show the original image
		imshow("Parted image Image", imagePart); //show the thresholded image
		if (cv::waitKey(30) == 27) //wait for 'esc' key press for 30ms. If 'esc' key is pressed, break loop
		{
			cout << "esc key is pressed by user" << endl;
			break;
		}
	}

	return 0;

}
//