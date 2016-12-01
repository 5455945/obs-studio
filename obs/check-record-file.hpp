#pragma once

#include <QThread>
#include <string>
#include <vector>

// ʹ��php�ӿ��ϴ�����¼���ļ����ļ�̫��ᱻ����˾ܾ���һ���Լ15MB���Ͳ�����
// ���ڸ���libs3�ӿ��ϴ�
class CheckRecordFileThread : public QThread {
	//Q_OBJECT

private:
	std::string url;
	std::string access_key;
	std::string access_secret;
	std::string opt;
	std::string bucket;
	std::string key;
	std::string record_path;
	std::string current_send_file;
	QPointer<QThread> SendThread;

	void GetRecordFiles(std::vector<std::string>& files);

private slots:
	void SendFinished(const QString& header, const QString& body, const QString& error);

protected:
	void run() override;

public:
	inline CheckRecordFileThread(
		std::string url_,
		std::string access_key_,
		std::string access_secret_,
		std::string opt_,
		std::string bucket_,
		std::string key_,
		std::string record_path_
		)
		: url(url_), access_key(access_key_), access_secret(access_secret_),
		opt(opt_), bucket(bucket_), key(key_), record_path(record_path_)
	{}
};
