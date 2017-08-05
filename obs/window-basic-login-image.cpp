#include "window-basic-login-image.hpp"
#include "ui_OBSBasicLoginImage.h"
#include "window-basic-main.hpp"
#include "md5.h"
#include "obs-scene.h"
#include "remote-post.hpp"
#include "qt-wrappers.hpp"
#include <QDesktopWidget>
#include <sstream>

using namespace std;

const string imgs_path("xmf/obs-studio/imgs");
QPointer<OBSBasic> OBSBasicLoginImage::main = nullptr;
QPointer<OBSQTDisplay> OBSBasicLoginImage::view = nullptr;

OBSBasicLoginImage::OBSBasicLoginImage(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::OBSBasicLoginImage)
{
    ui->setupUi(this);
	main = qobject_cast<OBSBasic*>(parent);
	setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
	connect(this, &OBSBasicLoginImage::LoginNormal, main, &OBSBasic::LoginNormal, Qt::QueuedConnection);
	connect(this, &OBSBasicLoginImage::LoginSucceeded, main, &OBSBasic::LoginSucceeded, Qt::QueuedConnection);
}

OBSBasicLoginImage::~OBSBasicLoginImage()
{
    delete ui;
}

void OBSBasicLoginImage::DrawPreview(void *data, uint32_t cx, uint32_t cy)
{
	obs_sceneitem_t *sceneitem;
	QString source_name;
	QDesktopWidget* desktopWidget = QApplication::desktop();
	QRect screenRect = desktopWidget->screenGeometry();
	int monitor_width = screenRect.width();
	source_name = QStringLiteral("视频捕获设备");
	sceneitem = obs_scene_find_source(main->GetCurrentScene(), QT_TO_UTF8(source_name));
	obs_source_t* source = NULL;
	if (!sceneitem) {
		source = obs_source_create("dshow_input", QT_TO_UTF8(source_name), NULL, nullptr);
		if (source) {
			sceneitem = obs_scene_add(main->GetCurrentScene(), source);
			//uint32_t dshow_width = obs_source_get_width(sceneitem->source);
			vec2_set(&sceneitem->scale, 0.4f, 0.4f);
			vec2_set(&sceneitem->pos, monitor_width - 640 * 0.4f, 0.0f);
			obs_sceneitem_set_visible(sceneitem, true);
			main->LoginSuccessSetSource(source);
			sceneitem = obs_scene_find_source(main->GetCurrentScene(), QT_TO_UTF8(source_name));
			obs_source_release(source);
			DrawPreview(NULL, cx, cy);
		}
	}
	else {
		source = sceneitem->source;
	}

	gs_viewport_push();
	gs_projection_push();
	//gs_ortho(0.0f, 640.0f, 0.0f, 480.0f, -100.0f, 100.0f);
	//gs_set_viewport(0, 0, 640, 480);
	obs_source_video_render(source);
	gs_projection_pop();
	gs_viewport_pop();
}

bool OBSBasicLoginImage::LoginImage()
{
	bool bRet = true;
	resize(660, 500);
	setMinimumSize(660, 500);
	setMaximumSize(QSize(660, 500));
	setWindowTitle(QString(QStringLiteral("开始工作 -- 请正面对着摄像头来验证您的身份信息！")));

	if (!view) {
		view = new OBSQTDisplay(this);
		setLayout(new QVBoxLayout(this));
		layout()->addWidget(view);
		layout()->setAlignment(view, Qt::AlignBottom);

		auto addDrawCallback = [this]()
		{
			obs_display_add_draw_callback(view->GetDisplay(), OBSBasicLoginImage::DrawPreview, this);
		};

		connect(view, &OBSQTDisplay::DisplayCreated, addDrawCallback);
	}
	view->setMinimumSize(640, 480);
	view->setMaximumSize(QSize(640, 480));
	//view->show();

	do {
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
		dst << imgs_path.c_str() << "/" << "admin_face.jpg";
		string loginFile = string(GetConfigPathPtr(dst.str().c_str()));
		wchar_t filename[MAX_PATH];
		memset(filename, 0, sizeof(wchar_t) * MAX_PATH);
		size_t len = os_utf8_to_wcs(loginFile.c_str(), 0, filename, MAX_PATH - 1);
		if (len <= 0) {
			OutputDebugString(_T("\n登陆图片文件名称获取错误！\n"));
			bRet = false;
			break;
		}

		// 尝试刷脸登陆
		int i = 0;
		for (i = 0; i < 10; i++) {
			pfuncSaveCapturePicture(true, filename);
			Sleep(200);
			if (os_file_exists(loginFile.c_str())) {
				// 如果文件存在，发送文件和用户名到web端验证
				string url = "https://xmfapi.cdnunion.com/user/index/face_login";
				if (loginImageThread) {
					loginImageThread->wait();
					delete loginImageThread;
				}
				RemotePostThread *thread = new RemotePostThread(url, string());
				loginImageThread = thread;
				connect(thread, &RemotePostThread::Result, this, &OBSBasicLoginImage::LoginImageFinished, Qt::QueuedConnection);
				thread->PrepareDataHeader();
				thread->PrepareData("token", ptoken);
				thread->PrepareData("user_id", pUserId);
				string timestamp = std::to_string(time(NULL));
				thread->PrepareData("timestamp", timestamp);
				thread->PrepareData("code", MD5(string(pUserId + timestamp + "E12AAD9E3CD85")).toString());
				thread->PrepareData("version", main->GetAppVersion());
				thread->PrepareDataFromFile("user_face", loginFile);
				thread->PrepareDataFoot(true);
				loginImageThread->start();
				bRet = true;
				break;
			}
		}
	} while (false);

	if (!bRet) {
		if (view) {
			delete view;
			view = nullptr;
		}
		//resize(340, 186);
		//setMinimumSize(340, 186);
		//setMaximumSize(QSize(340, 186));
		//setWindowTitle(QString(QStringLiteral("开始工作")));
		emit LoginNormal(QStringLiteral("图片登陆本地认证失败"));
		close();
	}

	return bRet;
}

void OBSBasicLoginImage::LoginImageFinished(const QString& header, const QString& body, const QString& error)
{
	bool bFaceLoginSuccess = false;
	obs_data_t* returnData = nullptr;

	do {
		if (body.isEmpty()) {
			blog(LOG_ERROR, "FaceAddFinished failed");
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
				bFaceLoginSuccess = true;
				LoginSucceeded(body);
			}
		}
		obs_data_release(returnData);
	} while (false);

	if (!bFaceLoginSuccess) {
		if (view) {
			delete view;
			view = nullptr;
		}
		//if (view) {
		//	view->hide();
		//	resize(340, 186);
		//	setMinimumSize(340, 186);
		//	setMaximumSize(QSize(340, 186));
		//	setWindowTitle(QString(QStringLiteral("开始工作")));
		//}
		LoginNormal(QStringLiteral("图片登陆服务端认证失败"));
	}

	close();
}

int OBSBasicLoginImage::exec()
{
	this->LoginImage();
	return QDialog::exec();
}