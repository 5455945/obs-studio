#pragma once

#include <QThread>
#include <string>
#include <vector>

// 上传认证人脸的图片文件
class FaceAddThread : public QThread {
	//Q_OBJECT

private:
	bool force;
	int nTryCanFaceLoginTimes;  // 尝试上传认证头像次数(已经正确获取头像，可能是网络或服务端错误)
	QPointer<QThread> OpenCVFaceAddThread;
	bool OpenCVFaceAdd(bool force = false);

private slots:
	void OpenCVFaceAddFinished(const QString& header, const QString& body, const QString& error);

protected:
	void run() override;

public:
	inline FaceAddThread(
		bool force_ = false       // 强制上传
		)
		: force(force_), nTryCanFaceLoginTimes(0), OpenCVFaceAddThread(nullptr)
	{}
};
