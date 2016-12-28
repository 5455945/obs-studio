#include "face-detect.h"
#include <opencv2/opencv.hpp>
#include <string>
#include "windows.h"
#include "CvTextFreeType.h"

using namespace cv;
using namespace std;

bool g_bStop = false;
HWND g_hParentHwnd = nullptr;
char g_window_title[256] = { 0 };
int PreWndProcCallback(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, int*  was_processed);

int __stdcall FaceDetect(const char* filename, const char* window_title, void* parent_hwnd)
{
	if (filename == nullptr || window_title == nullptr || parent_hwnd == nullptr) {
		return 0;
	}

	if (!g_hParentHwnd) {
		g_hParentHwnd = (HWND)parent_hwnd;  // �����ھ��
		memset(g_window_title, 0, sizeof(g_window_title));
		if (strlen(window_title) > 0) {
			memcpy(g_window_title, window_title, strlen(window_title));
		}

		cvSetPreprocessFuncWin32(PreWndProcCallback);
		//cvSetPostprocessFuncWin32(PreWndProcCallback);
	}

	int nRet = -1;
	if (filename == nullptr || window_title == nullptr) {
		return nRet;
	}

	int nFaces = 0;
	int nEyes = 0;
	int nNoses = 0;
	int nMouths = 0;
	std::vector<Rect> faces;
	CascadeClassifier face_cascade, eye_cascade, nose_cascade, mouth_cascade;
	string face_cascade_name = "../../data/obs-plugins/face-detect/haarcascades/haarcascade_frontalface_alt2.xml";
	string eye_cascade_name = "../../data/obs-plugins/face-detect/haarcascades/haarcascade_eye.xml";
	string nose_cascade_name = "../../data/obs-plugins/face-detect/haarcascades/haarcascade_mcs_nose.xml";
	string mouth_cascade_name = "../../data/obs-plugins/face-detect/haarcascades/haarcascade_mcs_mouth.xml";
	string imageName = filename;
	Mat image = imread(imageName.c_str(), 1);
	if (!image.empty()) {
		if (!face_cascade.load(face_cascade_name)) {
			return nRet;
		}
		if (!eye_cascade.load(eye_cascade_name)) {
			return nRet;
		}
		if (!nose_cascade.load(nose_cascade_name)) {
			return nRet;
		}
		if (!mouth_cascade.load(mouth_cascade_name)) {
			return nRet;
		}
		Mat face_gray;
		cvtColor(image, face_gray, CV_BGR2GRAY);
		equalizeHist(face_gray, face_gray);
		face_cascade.detectMultiScale(face_gray, faces, 1.1, 3, 0 | CV_HAAR_SCALE_IMAGE, Size(30, 30));
		nFaces = (int)faces.size();

		Point  center;
		int radius;
		for (size_t i = 0; i < faces.size(); i++) {
			vector<Rect> eyes;
			center.x = cvRound((faces[i].x + faces[i].width * 0.5));
			center.y = cvRound((faces[i].y + faces[i].height * 0.5));
			radius = cvRound((faces[i].width + faces[i].height) * 0.25);
			circle(image, center, radius, Scalar(255, 0, 0), 2);

			// �������
			Mat face = image(faces[i]);
			eye_cascade.detectMultiScale(face, eyes, 1.1, 2, 0 | CV_HAAR_SCALE_IMAGE, cv::Size(20, 20));
			// ���ۼ���������  
			if (eyes.size() >= 2)
			{
				nEyes = (nEyes >= (int)eyes.size()) ? nEyes : eyes.size();
				for (int e = 0; e < (int)eyes.size(); e++) {
					Rect rect = eyes[e] + cv::Point(faces[i].x, faces[i].y);
					Mat eye = image(rect);
					center.x = cvRound((rect.x + rect.width * 0.5));
					center.y = cvRound((rect.y + rect.height * 0.5));
					radius = cvRound((rect.width + rect.height) * 0.25);
					circle(image, center, radius, Scalar(0, 128, 128), 1);
				}
			}

			// ������
			Mat ROI = image(Rect(faces[i].x, faces[i].y, faces[i].width, faces[i].height));
			vector<Rect_<int> > nose;
			nose_cascade.detectMultiScale(ROI, nose, 1.20, 5, 0 | CV_HAAR_SCALE_IMAGE, Size(30, 30));
			if (nose.size() > 0) {
				nNoses = (nNoses >= (int)nose.size()) ? nNoses : nose.size();
				for (int j = 0; j < (int)nose.size(); j++) {
					Rect rect = nose[j] + cv::Point(faces[i].x, faces[i].y);
					center.x = cvRound((rect.x + rect.width * 0.5));
					center.y = cvRound((rect.y + rect.height * 0.5));
					radius = cvRound((rect.width + rect.height) * 0.25);
					circle(image, center, radius, Scalar(0, 0, 128), 1);
				}
			}

			// �����
			vector<Rect_<int> > mouth;
			mouth_cascade.detectMultiScale(ROI, mouth, 1.10, 3, 0 | CV_HAAR_SCALE_IMAGE, Size(20, 20));
			if (mouth.size() > 0) {
				nMouths = (nMouths >= (int)mouth.size()) ? nMouths : mouth.size();
				for (int j = 0; j < (int)nose.size(); j++) {
					Rect rect = mouth[j] + cv::Point(faces[i].x, faces[i].y);
					center.x = cvRound((rect.x + rect.width * 0.5));
					center.y = cvRound((rect.y + rect.height * 0.5));
					radius = cvRound((rect.width + rect.height) * 0.25);
					circle(image, center, radius, Scalar(0, 128, 0), 1);
				}
			}
		}
		
		CvTextFreeType text("../../data/obs-plugins/face-detect/font/simsun.ttc");
		float p = 1.0f;                  // ͸����
		CvScalar size(48, 0.5, 0.1, 0);  // �����С/�հױ���/�������/��ת�Ƕ�  
		text.setFont(NULL, &size, NULL, &p);
		if (nFaces * nEyes <= 1) {
			text.putText(image, "δ��⵽�����������뽫������������ͷ", cvPoint(20, 30), CV_RGB(255, 0, 0));
		}
		else {
			text.putText(image, "��⵽����������������������֤�����Ե�...", cvPoint(20, 30), CV_RGB(0, 255, 0));
		}

		Size image_size = image.size();
		int cx = GetSystemMetrics(SM_CXFULLSCREEN);
		int cy = GetSystemMetrics(SM_CYFULLSCREEN);
		LONG x = (cx - image_size.width) / 2;
		LONG y = (cy - image_size.height) / 2;
		//namedWindow(window_title, 1);
		imshow(window_title, image);
		HWND hwnd = (HWND)cvGetWindowHandle(window_title);
		HWND hParent = GetParent(hwnd);
		if (hParent) {
			::SetWindowLongA(hParent, GWL_STYLE, ::GetWindowLongA(hParent, GWL_STYLE) & ~WS_MAXIMIZEBOX & ~WS_MINIMIZEBOX);
			// ����ˢ�������ö� HWND_TOP  HWND_TOPMOST SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOMOVE
			::SetWindowPos(hParent, HWND_TOP, x, y, image_size.width, image_size.height, SWP_SHOWWINDOW | SWP_NOSIZE);
			
			// ���ô��ڵĴ�Сͼ��
			char szFileName[MAX_PATH];
			memset(szFileName, 0, sizeof(szFileName));
			HINSTANCE hInstance = ::GetModuleHandle(NULL);
			::GetModuleFileNameA(hInstance, szFileName, MAX_PATH);
			HICON hIcon = ExtractIconA(hInstance, szFileName, 0);
			if (hIcon) {
				// ��ͼ�꣺����alt+tab���л�����ʱ��Ӧ��ͼ��
				// Сͼ�꣺���Ǵ������ϽǶ�Ӧ���Ǹ�ͼ��
				::SendMessage(hParent, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
				::SendMessage(hParent, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
			}
		}
		waitKey(1);
	}
	//nRet = nFaces * nEyes * nNoses * nMouths;
	nRet = nFaces * nEyes;
	return nRet;
}

bool __stdcall FaceDestroyWindow(const char* window_title)
{
	if (window_title == nullptr) {
		return false;
	}
	//destroyWindow(window_title);
	destroyAllWindows();
	g_hParentHwnd = nullptr;

	return true;
}

int __stdcall ImageFaceDetect(const char* filename)
{
	int nRet = -1;
	if (filename == nullptr) {
		return nRet;
	}

	int nFaces = 0;
	int nEyes = 0;
	int nNoses = 0;
	int nMouths = 0;
	std::vector<Rect> faces;
	CascadeClassifier face_cascade, eye_cascade, nose_cascade, mouth_cascade;
	
	string face_cascade_name = "../../data/obs-plugins/face-detect/haarcascades/haarcascade_frontalface_alt2.xml";
	string eye_cascade_name = "../../data/obs-plugins/face-detect/haarcascades/haarcascade_eye.xml";
	string nose_cascade_name = "../../data/obs-plugins/face-detect/haarcascades/haarcascade_mcs_nose.xml";
	string mouth_cascade_name = "../../data/obs-plugins/face-detect/haarcascades/haarcascade_mcs_mouth.xml";
	string imageName = filename;
	Mat image = imread(imageName.c_str(), 1);
	if (!image.empty()) {
		if (!face_cascade.load(face_cascade_name)) {
			return nRet;
		}
		if (!eye_cascade.load(eye_cascade_name)) {
			return nRet;
		}
		if (!nose_cascade.load(nose_cascade_name)) {
			return nRet;
		}
		if (!mouth_cascade.load(mouth_cascade_name)) {
			return nRet;
		}

		Mat face_gray;
		cvtColor(image, face_gray, CV_BGR2GRAY);
		equalizeHist(face_gray, face_gray);

		face_cascade.detectMultiScale(face_gray, faces, 1.1, 3, 0 | CV_HAAR_SCALE_IMAGE, Size(30, 30));
		nFaces = (int)faces.size();
		for (size_t i = 0; i < faces.size(); i++) {
			// �������
			vector<Rect> eyes;
			cv::Mat face = image(faces[i]);
			eye_cascade.detectMultiScale(face, eyes, 1.1, 2, 0 | CV_HAAR_SCALE_IMAGE, cv::Size(20, 20));
			nEyes = (nEyes >= (int)eyes.size()) ? nEyes : eyes.size();

			// ������
			Mat ROI = image(Rect(faces[i].x, faces[i].y, faces[i].width, faces[i].height));
			vector<Rect_<int> > nose;
			nose_cascade.detectMultiScale(ROI, nose, 1.20, 3, 0 | CASCADE_SCALE_IMAGE, Size(20, 20));
			nNoses = (nNoses >= (int)nose.size()) ? nNoses : nose.size();

			// �����
			vector<Rect_<int> > mouth;
			mouth_cascade.detectMultiScale(ROI, mouth, 1.10, 3, 0 | CASCADE_SCALE_IMAGE, Size(20, 20));
			nMouths = (nMouths >= (int)mouth.size()) ? nMouths : mouth.size();
		}
		waitKey(1);
	}
	//nRet = nFaces * nEyes * nNoses * nMouths;
	nRet = nFaces * nEyes;

	return nRet;
}

int PreWndProcCallback(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, int* was_processed)
{
	switch (uMsg)
	{
	case WM_DESTROY:
		if (g_hParentHwnd) {
			::PostMessageA(g_hParentHwnd, WM_CLOSE, 1, 2);
			destroyAllWindows();
			g_hParentHwnd = nullptr;
		}
		break;
	default:
		break;
	}

	return *was_processed;
}
