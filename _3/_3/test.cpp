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
using namespace std;
using namespace cv;
using namespace camShift;
static void help()
{
	printf("\n"
		"This program demonstrated a simple method of connected components clean up of background subtraction\n"
		"When the program starts, it begins learning the background.\n"
		"You can toggle background learning on and off by hitting the space bar.\n"
		"Call\n"
		"./segment_objects [video file, else it reads camera 0]\n\n");
}
static void refineSegments(const Mat& img, Mat& mask, Mat& dst)
{
	int niters = 3;
	vector<vector<Point> > contours;
	vector<Vec4i> hierarchy;
	Mat temp;
	dilate(mask, temp, Mat(), Point(-1, -1), niters);
	erode(temp, temp, Mat(), Point(-1, -1), niters * 2);
	dilate(temp, temp, Mat(), Point(-1, -1), niters);
	findContours(temp, contours, hierarchy, RETR_CCOMP, CHAIN_APPROX_SIMPLE);
	dst = Mat::zeros(img.size(), img.type());
	if (contours.size() == 0)
		return;
	// iterate through all the top-level contours,
	// draw each connected component with its own random color
	int idx = 0, largestComp = 0;
	double maxArea = 0;
	for (; idx >= 0; idx = hierarchy[idx][0])
	{
		const vector<Point>& c = contours[idx];
		double area = fabs(contourArea(Mat(c)));
		if (area > maxArea)
		{
			maxArea = area;
			largestComp = idx;
		}
	}
	Scalar color(255, 255, 255);

	drawContours(dst, contours, largestComp, color, FILLED, LINE_8, hierarchy);
	//drawContours(dst, vector<vector<cv::Point>>(1, ptsHull), 0, cv::Scalar(255, 0, 0), 2);
}
void mouseFunction(int event, int x, int y, int, void* parameter) {
	static cv::Point startPoint;
	camShift::CamShift& camShift = *((camShift::CamShift*)((void**)parameter)[0]);
	bool& selectionHasBeenSet = *((bool*)((void**)parameter)[1]);

	switch (event) {
	case cv::EVENT_LBUTTONDOWN:
		startPoint = cv::Point(x, y);
		break;
	case cv::EVENT_LBUTTONUP:
		camShift.setSelection(cv::Rect(startPoint, cv::Point(x, y)));
		selectionHasBeenSet = true;
		break;
	default: break;
	}
}
int main(int argc, char** argv)
{
	VideoCapture cap;
	bool update_bg_model = true;
	CommandLineParser parser(argc, argv, "{help h||}{@input||}");
	if (parser.has("help"))
	{
		help();
		return 0;
	}
	string input = parser.get<std::string>("@input");
	if (input.empty())
		cap.open(0);
	else
		cap.open(input);
	if (!cap.isOpened())
	{
		printf("\nCan not open camera or video file\n");
		return -1;
	}
	Mat tmp_frame, bgmask, out_frame,init;
	cap >> tmp_frame;
	cap >> init;
	if (tmp_frame.empty())
	{
		printf("can not read data from the video source\n");
		return -1;
	}
	namedWindow("video", 1);
	namedWindow("segmented", 1);
	cv::namedWindow("back", 1);
	Ptr<BackgroundSubtractorMOG2> bgsubtractor = createBackgroundSubtractorMOG2();
	bgsubtractor->setVarThreshold(10);
	try {
		/*-- Declarations --*/

		camShift::CamShift camShift;
		bool selectionHasBeenSet = false; void* sharedPointers[] = { &camShift, &selectionHasBeenSet };
		cv::setMouseCallback("segmented", mouseFunction, sharedPointers);

		for (;;)
		{
			cap >> tmp_frame;
			if (tmp_frame.empty())
				break;
			//	bgsubtractor->apply(tmp_frame, bgmask, update_bg_model ? -1 : 0);
			//	refineSegments(tmp_frame, bgmask, out_frame);
//back substraction
			vector<vector<cv::Point>> contours;
			absdiff(tmp_frame, init, tmp_frame);

//make binaryMat
			cv::Mat grayscaleMat(tmp_frame.size(), CV_8U);
			cv::cvtColor(tmp_frame, grayscaleMat, CV_BGR2GRAY);
			cv::Mat binaryMat(grayscaleMat.size(), grayscaleMat.type());
			cv::threshold(grayscaleMat, binaryMat, 30, 255, cv::THRESH_BINARY);


			binaryMat = ~binaryMat;
			camShift.setCapturedRawFrame(tmp_frame);
			
			if (selectionHasBeenSet) {
				/* Execute the CAMShift algorithm */
				camShift.runCamShift();
				/* Draw an ellipse on the captured raw frame, hopefully indicating where the tracked
				object is located in the frame */
				cv::ellipse(binaryMat, camShift.getRotatedTrack(), cv::Scalar(70, 100, 255), 3, CV_AA);

				/* Update the window that displays the backprojections */
				//cv::imshow("back", camShift.getBackprojection());
			}

			flip(binaryMat, binaryMat, 1);
			//imshow("video", imgThresholded);
			imshow("segmented", binaryMat);


			int keycode = waitKey(30);
			if (keycode == 27)
				break;
			if (keycode == ' ')
			{
				update_bg_model = !update_bg_model;
				printf("Learn background is in state = %d\n", update_bg_model);
			}
		}
	}
	catch (exception& e) {
		cout << e.what() << endl;
	}
	

	
	return 0;
}