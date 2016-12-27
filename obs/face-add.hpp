#pragma once

#include <QThread>
#include <string>
#include <vector>

// �ϴ���֤������ͼƬ�ļ�
class FaceAddThread : public QThread {
	//Q_OBJECT

private:
	bool force;
	int nTryCanFaceLoginTimes;  // �����ϴ���֤ͷ�����(�Ѿ���ȷ��ȡͷ�񣬿�������������˴���)
	QPointer<QThread> OpenCVFaceAddThread;
	bool OpenCVFaceAdd(bool force = false);

private slots:
	void OpenCVFaceAddFinished(const QString& header, const QString& body, const QString& error);

protected:
	void run() override;

public:
	inline FaceAddThread(
		bool force_ = false       // ǿ���ϴ�
		)
		: force(force_), nTryCanFaceLoginTimes(0), OpenCVFaceAddThread(nullptr)
	{}
};
