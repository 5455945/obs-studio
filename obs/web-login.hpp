
#pragma once

#include <QThread>
#include <string>

class WebLoginThread : public QThread {
	Q_OBJECT

	std::string url;
	std::string contentType;
	std::string postData;

	void run() override;

signals:
	void Result(const QString &text, const QString &error);

public:
	inline WebLoginThread(
			std::string url_,
			std::string contentType_ = std::string(),
			std::string postData_ = std::string())
		: url(url_), contentType(contentType_), postData(postData_)
	{}
};
