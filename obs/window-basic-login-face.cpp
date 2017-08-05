#include "window-basic-login-face.hpp"
#include "ui_OBSBasicLoginFace.h"
#include "window-basic-main.hpp"
#include "md5.h"
#include "obs-scene.h"
#include "remote-post.hpp"
#include "qt-wrappers.hpp"
#include <QDesktopWidget>
#include <sstream>

using namespace std;

const string imgs_path("xmf/obs-studio/imgs");
static string window_title = "��ʼ����--�������������ͷ����֤���������Ϣ��";

OBSBasicLoginFace::OBSBasicLoginFace(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::OBSBasicLoginFace),
	OpenCVLoginFaceThread(nullptr), 
	isOpenCVLoginFaceStop(false)
{
    ui->setupUi(this);
	bLoginStatus = false;
	bWebLoginThread = false;
	main = qobject_cast<OBSBasic*>(parent);
	setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);

	// ����������Ϣʹ���첽��ʽQt::QueuedConnection������Ҫ���û��رյ�½������LoginFace�� close�Żᱻ����
	connect(this, &OBSBasicLoginFace::LoginNormal, main, &OBSBasic::LoginNormal, Qt::QueuedConnection);
	connect(this, &OBSBasicLoginFace::LoginSucceeded, main, &OBSBasic::LoginSucceeded, Qt::QueuedConnection);

	string url = "https://xmfapi.cdnunion.com/user/index/face_login";
	OpenCVLoginFaceThread = new RemotePostThread(url, string());
	connect(OpenCVLoginFaceThread, &RemotePostThread::Result, this, &OBSBasicLoginFace::OpenCVLoginFaceFinished, Qt::QueuedConnection);
}

OBSBasicLoginFace::~OBSBasicLoginFace()
{
    delete ui;
}

void OBSBasicLoginFace::open()
{
	QDialog::open();
}

int OBSBasicLoginFace::exec()
{
	move(10000, 10000);
	QDialog::show();
	bool bRet = OpenCVLoginFace();
	if (bRet) {
		return QDialog::exec();
	}
	return 0;
}

void OBSBasicLoginFace::done(int status)
{
	QDialog::done(status);
}

void OBSBasicLoginFace::accept()
{
	QDialog::accept();
}
void OBSBasicLoginFace::reject()
{
	OpenCVLoginFaceEnd();
	QDialog::reject();
}

bool OBSBasicLoginFace::close()
{
	return QDialog::close();
}

void OBSBasicLoginFace::show()
{
	QDialog::show();
}

bool OBSBasicLoginFace::OpenCVLoginFace()
{
	bool bRet = false;
	do {
		bool can_face_login = config_get_bool(GetGlobalConfig(), "BasicLoginWindow", "can_face_login");
		if (!can_face_login) {
			bRet = false;
			break;
		}

		const char* pUserId = config_get_string(GetGlobalConfig(), "BasicLoginWindow", "user_id");
		if (!pUserId) {
			OutputDebugString(_T("\n���˺���Ϣ\n"));
			bRet = false;
			break;
		}

		const char* ptoken = config_get_string(GetGlobalConfig(), "BasicLoginWindow", "token");
		if (!ptoken) {
			OutputDebugString(_T("\ntoken��ȡʧ��\n"));
			bRet = false;
			break;
		}
		
		HMODULE hModule = GetModuleHandleA("win-dshow.dll");
		if (!hModule) {
			OutputDebugString(_T("\n���win-dshow.dllģ��ʧ��\n"));
			bRet = false;
			break;
		}

		typedef bool(__stdcall *PFUN_SaveCapturePicture)(bool isSave, wchar_t* filename);

		PFUN_SaveCapturePicture pfuncSaveCapturePicture = (PFUN_SaveCapturePicture)GetProcAddress(hModule, "SaveCapturePicture");
		if (!pfuncSaveCapturePicture) {
			OutputDebugString(_T("\n���win-dshow.dllģ���SaveCapturePicture��ַʧ��\n"));
			bRet = false;
			break;
		}

		stringstream dst;
		dst << "xmf/obs-studio/imgs/" << "user_face.jpg";
		string loginFile = string(GetConfigPathPtr(dst.str().c_str()));
		wchar_t filename[MAX_PATH];
		memset(filename, 0, sizeof(wchar_t) * MAX_PATH);
		size_t len = os_utf8_to_wcs(loginFile.c_str(), 0, filename, MAX_PATH - 1);
		if (len <= 0) {
			OutputDebugString(_T("\n��½ͼƬ�ļ����ƻ�ȡ����\n"));
			bRet = false;
			break;
		}

		hModule = GetModuleHandleA("face-detect.dll");
		if (!hModule) {
			OutputDebugString(_T("\n���face-detect.dllģ��ʧ��\n"));
			bRet = false;
			break;
		}
		typedef int(__stdcall *PFUNC_FaceDetect)(const char* filename, const char* window_title, void* parent_hwnd);
		PFUNC_FaceDetect pfuncFaceDetect = nullptr;
		pfuncFaceDetect = (PFUNC_FaceDetect)GetProcAddress(hModule, "FaceDetect");
		if (!pfuncFaceDetect) {
			OutputDebugString(_T("\n���face-detect.dllģ���FaceDetect��ַʧ��\n"));
			bRet = false;
			break;
		}
		
		for (int i = 0; i < 60; i++) {
			if (os_file_exists(loginFile.c_str())) {
				os_unlink(loginFile.c_str());
			}

			// ***************** �첽�ģ�ֻ�ܼ�� **********************
			pfuncSaveCapturePicture(true, filename);
			for (int j = 0; j < 10; j++) {
				Sleep(20);  // ��һ�λ�ȡ�ļ��Ƚ�������Լ100+���룬�ڶ��κ�һ��50������
				if (os_file_exists(loginFile.c_str())) {
					break;
				}
			}
			// ***************** �첽�ģ�ֻ�ܼ�� **********************

			if (os_file_exists(loginFile.c_str())) {
				if (isOpenCVLoginFaceStop) {
					break;
				}
				int face_num = pfuncFaceDetect(loginFile.c_str(), window_title.c_str(), (void*)winId());
				if (face_num >= 2) {
					bWebLoginThread = true;
					// ����ļ����ڣ������ļ����û�����web����֤
					string timestamp = std::to_string(time(NULL));
					if (OpenCVLoginFaceThread->isRunning()) {
						OpenCVLoginFaceThread->wait();
					}
					OpenCVLoginFaceThread->PrepareDataHeader();
					OpenCVLoginFaceThread->PrepareData("token", ptoken);
					OpenCVLoginFaceThread->PrepareData("user_id", pUserId);
					OpenCVLoginFaceThread->PrepareData("timestamp", timestamp);
					OpenCVLoginFaceThread->PrepareData("code", MD5(string(pUserId + timestamp + "E12AAD9E3CD85")).toString());
					OpenCVLoginFaceThread->PrepareData("version", main->GetAppVersion());
					OpenCVLoginFaceThread->PrepareDataFromFile("user_face", loginFile);
					OpenCVLoginFaceThread->PrepareDataFoot(true);
					OpenCVLoginFaceThread->start();
					bRet = true;
					break;
				}
			}
		}
	} while (false);

	if ((!bRet) && (!isOpenCVLoginFaceStop)) {
		sLoginFailInfo = QStringLiteral("ˢ����½��������֤ʧ��");
		config_set_bool(GetGlobalConfig(), "BasicLoginWindow", "can_face_login", false);
		close();
	}

	return bRet;
}

void OBSBasicLoginFace::OpenCVLoginFaceFinished(const QString& header, const QString& body, const QString& error)
{
	obs_data_t* returnData = nullptr;
	do {
		if (body.isEmpty()) {
			blog(LOG_ERROR, "FaceAddFinished failed");
			config_set_bool(GetGlobalConfig(), "BasicLoginWindow", "can_face_login", false);
			break;
		}

		returnData = obs_data_create_from_json(QT_TO_UTF8(body));
		const char *json = obs_data_get_json(returnData);
		bool bResult = false;
		bResult = obs_data_get_bool(returnData, "rt");
		const char* sError = obs_data_get_string(returnData, "error");
		if (bResult) {
			double confidence = obs_data_get_double(returnData, "confidence");
			bool flag = false;
			flag = obs_data_get_bool(returnData, "flag");
			if (flag) {
				sLoginData = body;
				bLoginStatus = true;
			}
			else {
				blog(LOG_INFO, "FaceAddFinished failed %s", sError);
			}
		}
	} while (false);

	obs_data_release(returnData);

	if (!bLoginStatus) {
		sLoginFailInfo = QStringLiteral("ˢ����½���������֤ʧ��");  // ��֤ʧ��
		config_set_bool(GetGlobalConfig(), "BasicLoginWindow", "can_face_login", false);
	}

	close();
}

bool OBSBasicLoginFace::nativeEvent(const QByteArray &eventType, void *message, long *result)
{
	if (eventType == "windows_generic_MSG")
	{
		MSG* msg = reinterpret_cast<MSG*>(message);
		switch (msg->message)
		{
		case WM_CLOSE:
		{
			if ((msg->hwnd == (HWND)winId()) && (msg->wParam == 1) && (msg->lParam == 2)) {
				// ���û���ֶ��رչ�������û�п�����web��½�߳�
				if ((!isOpenCVLoginFaceStop) && (!bWebLoginThread)) {
					isOpenCVLoginFaceStop = true;
					sLoginFailInfo = QStringLiteral("ˢ����½���ֶ��ر�ˢ����֤����");
					close();
					return true;
				}
			}
		}
		break;
		default:
		break;
		}
	}
	return QWidget::nativeEvent(eventType, message, result);
}

void OBSBasicLoginFace::OpenCVLoginFaceEnd()
{
	// ����ɹ��ر�ˢ������
	HMODULE hModule = GetModuleHandleA("face-detect.dll");
	if (hModule) {
		typedef bool(__stdcall *PFUNC_FaceDestroyWindow)(const char* window_title);
		PFUNC_FaceDestroyWindow pfuncFaceDestroyWindow = (PFUNC_FaceDestroyWindow)GetProcAddress(hModule, "FaceDestroyWindow");
		if (pfuncFaceDestroyWindow) {
			pfuncFaceDestroyWindow(window_title.c_str());
		}
	}

	if (bLoginStatus) {
		emit LoginSucceeded(sLoginData);
	}
	else {
		emit LoginNormal(sLoginFailInfo);
	}

	disconnect(this, &OBSBasicLoginFace::LoginNormal, main, &OBSBasic::LoginNormal);
	disconnect(this, &OBSBasicLoginFace::LoginSucceeded, main, &OBSBasic::LoginSucceeded);
	disconnect(OpenCVLoginFaceThread, &RemotePostThread::Result, this, &OBSBasicLoginFace::OpenCVLoginFaceFinished);

	if (OpenCVLoginFaceThread) {
		OpenCVLoginFaceThread->wait();
		delete OpenCVLoginFaceThread;
		OpenCVLoginFaceThread = nullptr;
	}
}