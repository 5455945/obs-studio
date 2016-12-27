#include <curl/curl.h>
#include "obs-app.hpp"
#include "qt-wrappers.hpp"
#include "face-add.hpp"
#include "remote-post.hpp"
#include <sstream>

using namespace std;

void FaceAddThread::run()
{
	bool bRet = false;
	int i = 0;
	do {
		bRet = OpenCVFaceAdd(force);
		wait(100);
	} while ((!bRet) && (i++ < 20));
}

bool FaceAddThread::OpenCVFaceAdd(bool force)
{
	bool bRet = true;
	if (!force) {
		// 如果不是强制上传，检查是否已经上传过，如果上传过，就不再上传
		bool isExists = config_get_bool(GetGlobalConfig(), "BasicLoginWindow", "can_face_login");
		if (isExists) {
			return false;
		}
	}

	const char* pAdminId = config_get_string(GetGlobalConfig(), "BasicLoginWindow", "admin_id");
	if (!pAdminId) {
		bRet = false;
		return bRet;
	}

	const char* ptoken = config_get_string(GetGlobalConfig(), "BasicLoginWindow", "token");
	if (!ptoken) {
		bRet = false;
		return bRet;
	}

	HMODULE hModule = GetModuleHandleA("win-dshow.dll");
	if (!hModule) {
		bRet = false;
		return bRet;
	}

	typedef bool(__stdcall *PFUN_SaveCapturePicture)(bool isSave, wchar_t* filename);

	PFUN_SaveCapturePicture pfuncSaveCapturePicture = (PFUN_SaveCapturePicture)GetProcAddress(hModule, "SaveCapturePicture");
	if (!pfuncSaveCapturePicture) {
		bRet = false;
		return bRet;
	}

	stringstream dst;
	dst << "vhome/obs-studio/imgs" << "/" << "admin_face.jpg";
	string loginFile = string(GetConfigPathPtr(dst.str().c_str()));
	wchar_t filename[MAX_PATH];
	memset(filename, 0, sizeof(wchar_t) * MAX_PATH);
	size_t len = os_utf8_to_wcs(loginFile.c_str(), 0, filename, MAX_PATH - 1);
	if (len <= 0) {
		bRet = false;
		return bRet;
	}

	// 尝试刷脸登陆
	typedef int(__stdcall *PFUNC_ImageFaceDetect)(const char* filename);
	PFUNC_ImageFaceDetect pfuncImageFaceDetect = nullptr;
	hModule = GetModuleHandleA("face-detect.dll");
	if (!hModule) {
		bRet = false;
		return bRet;
	}
	for (int i = 0; i < 50; i++) {
		if (os_file_exists(loginFile.c_str())) {
			os_unlink(loginFile.c_str());
		}
		pfuncSaveCapturePicture(true, filename);
		// 最多等3次 0.6秒，待文件生成
		for (int j = 0; j < 3; j++) {
			Sleep(200);
			if (os_file_exists(loginFile.c_str())) {
				break;
			}
		}

		if (os_file_exists(loginFile.c_str())) {
			pfuncImageFaceDetect = (PFUNC_ImageFaceDetect)GetProcAddress(hModule, "ImageFaceDetect");
			if (!pfuncImageFaceDetect) {
				bRet = false;
				break;
			}
			int face_num = pfuncImageFaceDetect(loginFile.c_str());
			if (face_num > 0) {
				string url = "https://app.cdnunion.com/admin/my/face_add";
				if (OpenCVFaceAddThread) {
					OpenCVFaceAddThread->wait();
					delete OpenCVFaceAddThread;
				}
				RemotePostThread *thread = new RemotePostThread(url, string());
				OpenCVFaceAddThread = thread;
				connect(thread, &RemotePostThread::Result, this, &FaceAddThread::OpenCVFaceAddFinished);
				thread->PrepareDataHeader();
				thread->PrepareData("token", ptoken);
				thread->PrepareData("admin_id", pAdminId);
				thread->PrepareDataFromFile("admin_face", loginFile);
				thread->PrepareDataFoot(true);
				OpenCVFaceAddThread->start();
				bRet = true;
				break;
			}
		}
	}
	if (!bRet) {
		blog(LOG_ERROR, "FaceAddFinished failed! image file get error!");
	}

	return bRet;
}

void FaceAddThread::OpenCVFaceAddFinished(const QString& header, const QString& body, const QString& error)
{
	bool bRet = false;
	do {
		if (body.isEmpty()) {
			blog(LOG_ERROR, "FaceAddFinished failed");
			break;
		}

		obs_data_t* returnData = obs_data_create_from_json(QT_TO_UTF8(body));
		const char *json = obs_data_get_json(returnData);
		bRet = obs_data_get_bool(returnData, "rt");
		const char* sError = obs_data_get_string(returnData, "error");
		obs_data_release(returnData);
		if (!bRet) {
			nTryCanFaceLoginTimes++;
			if (nTryCanFaceLoginTimes <= 5) {
				OpenCVFaceAdd(false);  // 如果返回false，认为是本地错误，不再尝试
				return;
			}
		}
		else {
			nTryCanFaceLoginTimes = 0;
		}
	} while (false);

	config_set_bool(GetGlobalConfig(), "BasicLoginWindow", "can_face_login", bRet);
}
