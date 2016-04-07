#include <iostream>

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include <Windows.h>
using namespace cv;
using namespace std;

int main(int argc, char** argv)
{
	VideoCapture cap(0); //capture the video from web cam
	
	if (!cap.isOpened())  // if not success, exit program
	{
		cout << "Cannot open the web cam" << endl;
		return -1;
	}
	
	namedWindow("Control", CV_WINDOW_AUTOSIZE); //create a window called "Control"

	int iLowH = 0;
	int iHighH = 179;

	int iLowS = 0;
	int iHighS = 255;

	int iLowV = 0;
	int iHighV = 255;

	/*
	//Create trackbars in "Control" window
	cvCreateTrackbar("LowH", "Control", &iLowH, 179); //Hue (0 - 179)
	cvCreateTrackbar("HighH", "Control", &iHighH, 179);

	cvCreateTrackbar("LowS", "Control", &iLowS, 255); //Saturation (0 - 255)
	cvCreateTrackbar("HighS", "Control", &iHighS, 255);

	cvCreateTrackbar("LowV", "Control", &iLowV, 255); //Value (0 - 255)
	cvCreateTrackbar("HighV", "Control", &iHighV, 255);
	*/
	
	while (true)
	{
		Mat imgOriginal;
		bool bSuccess = cap.read(imgOriginal); // read a new frame from video

		if (!bSuccess) //if not success, break loop
		{
			cout << "Cannot read a frame from video stream" << endl;
			break;
		}

		Mat imgHSV;

		cvtColor(imgOriginal, imgHSV, COLOR_BGR2HSV); //Convert the captured frame from BGR to HSV

		Mat imgThresholded;
		 //set color that close to human body
		iLowH = 0;
		iLowS = 50;
		iLowV = 0;
		iHighH = 40;
		iHighS = 255;
		iHighV = 255;
		
		inRange(imgHSV, Scalar(iLowH, iLowS, iLowV), Scalar(iHighH, iHighS, iHighV), imgThresholded); //Threshold the image

		//morphological opening (remove small objects from the foreground)
		erode(imgThresholded, imgThresholded, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)));
		dilate(imgThresholded, imgThresholded, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)));

		//morphological closing (fill small holes in the foreground)
		dilate(imgThresholded, imgThresholded, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)),Point(-1,-1), 3);
		erode(imgThresholded, imgThresholded, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)), Point(-1, -1), 3);
		imshow("Thresholded Image", imgThresholded); //show the thresholded image
		vector<vector<cv::Point>> contours;
		findContours(imgThresholded, contours, noArray(), RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
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
		vector<Point> handContour = contours[maxK];
		convexHull(handContour, hull);
		
		vector<Point> ptsHull;
		for (int k = 0; k < hull.size(); k++){
			int i = hull[k];
			ptsHull.push_back(handContour[i]);
		}
		drawContours(imgOriginal, vector<vector<Point>>(1, ptsHull), 0, Scalar(255, 0, 0), 2);
		for (int k = 0; k < contours.size(); k++){
			Point2f center;
			Moments M = moments(contours[k]);
			center.x = M.m10 / M.m00;
			center.y = M.m01 / M.m00;
			//printf("Center of hull -> x: %.3f\t y: %.3f\n", center.x, center.y);
		}
		Sleep(50);
		 //convex hull and points
		vector<Vec4i> defects;
		convexityDefects(handContour, hull, defects);
		for (int k = 1; k < defects.size(); k++){
			Vec4i v = defects[k];
			Point ptStart = handContour[v[0]];
			Point ptEnd = handContour[v[1]];
			Point ptFar = handContour[v[2]];
			float depth = v[3] / 256.0;
			if (depth > 10){
				line(imgOriginal, ptStart, ptFar, Scalar(0, 255, 0), 2);
				line(imgOriginal, ptEnd, ptFar, Scalar(0, 255, 0), 2);
				circle(imgOriginal, ptStart, 6, Scalar(k*10, 150, 150), 2);
				circle(imgOriginal, ptEnd, 6, Scalar(0, 0, 255), 2);
				circle(imgOriginal, ptFar, 6, Scalar(0, 0, 255), 2);
				printf("%d\n", k);
			}
		}
		//inRange(imgHSV, Scalar(iLowH, iLowS, iLowV), Scalar(iHighH, iHighS, iHighV), imgThresholded); //Threshold the image
		
		imshow("Original", imgOriginal); //show the original image

		if (waitKey(30) == 27) //wait for 'esc' key press for 30ms. If 'esc' key is pressed, break loop
		{
			cout << "esc key is pressed by user" << endl;
			break;
		}
	}

	return 0;

}