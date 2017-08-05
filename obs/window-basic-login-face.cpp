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
static string window_title = "开始工作--请正面对着摄像头来验证您的身份信息！";

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

	// 这里两个消息使用异步方式Qt::QueuedConnection，否则，要等用户关闭登陆框后，这个LoginFace的 close才会被调用
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
			OutputDebugString(_T("\n无账号信息\n"));
			bRet = false;
			break;
		}

		const char* ptoken = config_get_string(GetGlobalConfig(), "BasicLoginWindow", "token");
		if (!ptoken) {
			OutputDebugString(_T("\ntoken读取失败\n"));
			bRet = false;
			break;
		}
		
		HMODULE hModule = GetModuleHandleA("win-dshow.dll");
		if (!hModule) {
			OutputDebugString(_T("\n获得win-dshow.dll模块失败\n"));
			bRet = false;
			break;
		}

		typedef bool(__stdcall *PFUN_SaveCapturePicture)(bool isSave, wchar_t* filename);

		PFUN_SaveCapturePicture pfuncSaveCapturePicture = (PFUN_SaveCapturePicture)GetProcAddress(hModule, "SaveCapturePicture");
		if (!pfuncSaveCapturePicture) {
			OutputDebugString(_T("\n获得win-dshow.dll模块的SaveCapturePicture地址失败\n"));
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
			OutputDebugString(_T("\n登陆图片文件名称获取错误！\n"));
			bRet = false;
			break;
		}

		hModule = GetModuleHandleA("face-detect.dll");
		if (!hModule) {
			OutputDebugString(_T("\n获得face-detect.dll模块失败\n"));
			bRet = false;
			break;
		}
		typedef int(__stdcall *PFUNC_FaceDetect)(const char* filename, const char* window_title, void* parent_hwnd);
		PFUNC_FaceDetect pfuncFaceDetect = nullptr;
		pfuncFaceDetect = (PFUNC_FaceDetect)GetProcAddress(hModule, "FaceDetect");
		if (!pfuncFaceDetect) {
			OutputDebugString(_T("\n获得face-detect.dll模块的FaceDetect地址失败\n"));
			bRet = false;
			break;
		}
		
		for (int i = 0; i < 60; i++) {
			if (os_file_exists(loginFile.c_str())) {
				os_unlink(loginFile.c_str());
			}

			// ***************** 异步的，只能检测 **********************
			pfuncSaveCapturePicture(true, filename);
			for (int j = 0; j < 10; j++) {
				Sleep(20);  // 第一次获取文件比较慢，大约100+毫秒，第二次后，一般50毫秒内
				if (os_file_exists(loginFile.c_str())) {
					break;
				}
			}
			// ***************** 异步的，只能检测 **********************

			if (os_file_exists(loginFile.c_str())) {
				if (isOpenCVLoginFaceStop) {
					break;
				}
				int face_num = pfuncFaceDetect(loginFile.c_str(), window_title.c_str(), (void*)winId());
				if (face_num >= 2) {
					bWebLoginThread = true;
					// 如果文件存在，发送文件和用户名到web端验证
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
		sLoginFailInfo = QStringLiteral("刷脸登陆：本地认证失败");
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
		sLoginFailInfo = QStringLiteral("刷脸登陆：服务端认证失败");  // 认证失败
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
				// 如果没有手动关闭过，并且没有开启向web登陆线程
				if ((!isOpenCVLoginFaceStop) && (!bWebLoginThread)) {
					isOpenCVLoginFaceStop = true;
					sLoginFailInfo = QStringLiteral("刷脸登陆：手动关闭刷脸认证窗口");
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
	// 如果成功关闭刷脸窗口
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