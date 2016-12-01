#pragma once

#include <QThread>
#include <string>
#include <vector>

class S3SendRecordFileThread : public QThread {
	//Q_OBJECT

private:
	std::string url;
	std::string access_key;
	std::string access_secret;
	std::string opt;
	std::string bucket;
	std::string key;
	std::string record_path;

	void GetRecordFiles(std::vector<std::string>& files);

protected:
	void run() override;

public:
	inline S3SendRecordFileThread(
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
