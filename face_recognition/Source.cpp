/** FaceDetection.cpp **/
#include <opencv2/opencv.hpp>
#include <iostream>
#include <stdio.h>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <vector>

using namespace std;
using namespace cv;
int getframe(VideoCapture capture);
/** Print error message **/
void PANIC(char* msg);
#define PANIC(msg){perror(msg); exit(-1);}

/** Function for face detection **/
void DetectAndDraw(Mat frame);
/** Global variables **/
String face_cascade_name = "C:/opencv/build/etc/haarcascades/haarcascade_frontalface_alt.xml";
String face_prof_cascade_name = "C:/opencv/build/etc/haarcascades/haarcascade_profileface.xml";
String eye_cascade_name = "C:/opencv/build/etc/haarcascades/haarcascade_eye_tree_eyeglasses.xml";

CascadeClassifier face_cascade; 
CascadeClassifier eyes_cascade;
CascadeClassifier face_prof_cascade;
String window_name = "Face detection";
void DetectAndDraw_without_eyes(Mat single_frame, int i);
vector<Mat> frame;

int getframe(VideoCapture capture) {
	long totalFrameNumber = capture.get(CAP_PROP_FRAME_COUNT);
	cout << "total frame number of video is (" << totalFrameNumber << ")." << endl;
	long frameToStart = 1000;
	capture.set(CAP_PROP_POS_FRAMES, frameToStart);
	cout << "Start at " << frameToStart << "'th fps." << endl;
	int frameToStop = 2000;
	if (frameToStop < frameToStart)
	{
		cout << "Stop fps smaller then Start's!" << endl;
		return -1;
	}
	else
	{
		cout << "Stop at " << frameToStop << "'th fps." << endl;
	}
	double rate = capture.get(CAP_PROP_FPS);
	cout << "Frame rate " << rate << endl;

	bool stop = false; // stop get frame
	Mat single_frame;
	double delay = 1000 / rate; // time space between both fps

	long currentFrame = frameToStart;
	while (!stop)
	{
		if (!capture.read(single_frame))
		{
			cout << "some problem in getting frame" << endl;
			return -1;
		}
		if (currentFrame % 200 == 0) //禎數間格 跳禎
		{
			cout << "Getting " << currentFrame << "'s fps" << endl;
			stringstream str;
			str << "get_fps/" << currentFrame << ".png";        /*圖片儲存位置*/
			cout << str.str() << endl;	
			frame.push_back(single_frame.clone());
			imwrite(str.str(), single_frame);			
		}
		int c = waitKey(delay);
		if ((char)c == 27 || currentFrame > frameToStop)
		{
			stop = true;
		}
		if (c >= 0)
		{
			waitKey(0);
		}
		currentFrame++;
	}
}

void use_camera_0(VideoCapture capture) {
	if (capture.isOpened()) {
	cout << "Face Detection Started..." << endl;
	Mat single_frame;

	for (;;) {
		/* Get image from camera */
		capture >> single_frame;
		if (single_frame.empty())
			PANIC("Error capture frame");
		/* Start the face detection function */
		frame.push_back(single_frame);
		/** If you press ESC, q, or Q , the process will end **/
		char ch = (char)waitKey(1);
		if (ch == 27 || ch == 'q' || ch == 'Q')
			break;
	}
}
else
	PANIC("Error open camera");
}

int main(int argc, char* argv[]) {
	VideoCapture capture = VideoCapture("test.mp4");
	//frame = imread("test_1.jpg");
	frame.clear();
	int feeback = getframe(capture);
	//use_camera_0(capture);
	if (feeback == -1)
		return -1;
	if (!face_cascade.load(face_cascade_name))
		PANIC("Error loading face cascade");
	if (!face_prof_cascade.load(face_prof_cascade_name))
		PANIC("Error loading face prof cascade");
	if (!eyes_cascade.load(eye_cascade_name))
		PANIC("Error loading eye cascade");
	//DetectAndDraw(frame);
	cout << frame.size() << '\n';
	for (int i = 0; i < frame.size(); i++)
	{
		DetectAndDraw_without_eyes(frame[i], i);
	}	
	capture.release();
	return 0;
}

void DetectAndDraw(vector<Mat> frame) {
	std::vector<Rect> faces, face_prof, eyes, output;
	Mat frame_gray, frame_resize;
	Mat single_frame = frame.front();
	int radius;
	cvtColor(single_frame, frame_gray, COLOR_BGR2GRAY);	/* Convert to gray scale */
	equalizeHist(frame_gray, frame_gray); 			/* Histogram equalization */
	face_cascade.detectMultiScale(frame_gray, faces, 1.05, 5, CASCADE_SCALE_IMAGE, Size(30, 30));
	//face_prof_cascade.detectMultiScale(frame_gray, face_prof, 1.1, 2, CASCADE_SCALE_IMAGE, Size(30, 30));
	for (size_t i = 0; i < faces.size(); i++)
	{
		Point center;
		rectangle(single_frame, faces[i], Scalar(0, 255, 0), 3, 8, 0);
		Mat faceROI = frame_gray(faces[i]);
		eyes_cascade.detectMultiScale(faceROI, eyes, 1.1, 1, CASCADE_SCALE_IMAGE, Size(20, 20));		/* Detection of eyes int the input image */

		/** Draw circles around eyes **/
		int temp = 0, iner = 1;
		for (size_t j = 0; j < eyes.size(); j++)
		{
			center.x = cvRound((faces[i].x + eyes[j].x + eyes[j].width * 0.5));
			center.y = cvRound((faces[i].y + eyes[j].y + eyes[j].height * 0.5));
			radius = cvRound((eyes[j].width + eyes[j].height) * 0.25);
			if (faces[i].contains(center))
			{
				temp++;
			}
			circle(single_frame, center, radius, Scalar(0, 0, 255), 3, 8, 0);
		}
		if (temp >= 1)
		{
			output.push_back(faces[i]);
		}
	}
	cout << output.size() << '\n';
	for (size_t i = 0; i < output.size(); i++)
	{
		rectangle(single_frame, output[i], Scalar(255, 0, 0), 3, 8, 0);
	}
	for (size_t i = 0; i < face_prof.size(); i++)
	{
		Point center;
		rectangle(single_frame, face_prof[i], Scalar(0, 255, 0), 3, 8, 0);
	}
	// resize(frame, frame, Size(), 1.2, 1.2, INTER_CUBIC);  //放大
	resize(single_frame, single_frame, Size(), 0.3, 0.3, INTER_AREA);  //縮小

	waitKey(0);
}

void DetectAndDraw_without_eyes(Mat single_frame, int i) {
	std::vector<Rect> faces, face_prof, eyes, output;
	Mat frame_gray, frame_resize;

	int radius;
	resize(single_frame, single_frame, Size(), 1.6, 1.6, INTER_AREA);
	/* Convert to gray scale */
	cvtColor(single_frame, frame_gray, COLOR_BGR2GRAY);
	/* Histogram equalization */
	equalizeHist(frame_gray, frame_gray);
	/* Detect faces of different sizes using cascade classifier */
	face_cascade.detectMultiScale(frame_gray, faces, 1.03, 3, CASCADE_SCALE_IMAGE, Size(10, 10));
	//(1.03每次影象尺寸減小的比例, 2:目標至少要被檢測到2次, CV_HAAR_SCALE_IMAGE:分類器, Size(0.4, 0.4)為目標的最小最大尺寸
	
	for (size_t i = 0; i < faces.size(); i++)
	{
		rectangle(single_frame, faces[i], Scalar(255, 0, 0), 3, 8, 0);
	}
	//resize(frame, frame, Size(), 0, 0, INTER_CUBIC);  //放大
	//resize(frame, frame, Size(), 0.5, 0.5, INTER_AREA);  //縮小
	stringstream str;
	str << "get_fps/" << i << ".png";        /*圖片儲存位置*/
	cout << str.str() << endl;
	imwrite(str.str(), single_frame);
	waitKey(0);
}