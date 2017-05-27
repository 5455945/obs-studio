#include <curl/curl.h>
#include "obs-app.hpp"
#include "qt-wrappers.hpp"
#include "remote-post.hpp"
#include <fstream>

using namespace std;

size_t RemotePostThread::WriteBodyCallback(char *ptr, size_t size, size_t nmemb, std::string &str)
{
	size_t total = size * nmemb;
	if (total)
		str.append(ptr, total);

	return total;
}

size_t RemotePostThread::WriteHeaderCallback(char *ptr, size_t size, size_t nmemb, std::string &str)
{
	size_t total = size * nmemb;
	if (total)
		str.append(ptr, total);

	return total;
}

size_t RemotePostThread::ReadCallback(void *ptr, size_t size, size_t nmemb, void *userp)
{
	PDATA_BUFFER databuf = (PDATA_BUFFER)userp;
	if (databuf == nullptr) {
		return 0;
	}
	size_t tocopy = size * nmemb;

	if (tocopy < 1 || !databuf->data_size) {
		//sleep(10000);
		return 0;
	}

	if (tocopy > databuf->data_size) {
		tocopy = databuf->data_size;
	}

	memcpy(ptr, databuf->readptr, tocopy);
	databuf->readptr += tocopy;
	databuf->data_size -= tocopy;
	return tocopy;
}

void RemotePostThread::run()
{
	CURLcode code;
	char error[CURL_ERROR_SIZE];
	memset(error, 0, sizeof(error));
	string versionString("User-Agent: obs-basic ");
	versionString += App()->GetVersionString();

	string contentTypeString;
	if (!content_type.empty() && content_type.length() > 0) {
		contentTypeString += "Content-Type: ";
		contentTypeString += content_type;
	}
	else {
		contentTypeString = "Content-Type: multipart/form-data; boundary=" + boundary;
	}

	auto curl_deleter = [] (CURL *curl) {curl_easy_cleanup(curl);};
	using Curl = unique_ptr<CURL, decltype(curl_deleter)>;
	Curl curl{curl_easy_init(), curl_deleter};
	if (curl) {
		struct curl_slist *header = nullptr;
		string sHeader, sBody;

		header = curl_slist_append(header, versionString.c_str());
		if (!contentTypeString.empty()) {
			header = curl_slist_append(header, contentTypeString.c_str());
		}
		header = curl_slist_append(header, "Expect:");  // 解决有些服务器需要 100 continue问题
		header = curl_slist_append(header, "Accept: */*");
		header = curl_slist_append(header, ContentLength.c_str());

		curl_easy_setopt(curl.get(), CURLOPT_SSL_VERIFYPEER, 0);
		curl_easy_setopt(curl.get(), CURLOPT_SSL_VERIFYHOST, 2);
		curl_easy_setopt(curl.get(), CURLOPT_URL, url.c_str());
		curl_easy_setopt(curl.get(), CURLOPT_HEADER, 0L);  // 0 sBody只包含body体，1 sBody 包含html的header+body
		curl_easy_setopt(curl.get(), CURLOPT_POST, 1L);
		curl_easy_setopt(curl.get(), CURLOPT_HTTPHEADER, header);
		curl_easy_setopt(curl.get(), CURLOPT_ERRORBUFFER, error);
		curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, WriteBodyCallback);
		curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &sBody);
		curl_easy_setopt(curl.get(), CURLOPT_HEADERFUNCTION, WriteHeaderCallback);
		curl_easy_setopt(curl.get(), CURLOPT_HEADERDATA, &sHeader);
		//curl_easy_setopt(curl.get(), CURLOPT_TIMEOUT, 30);  // 30秒

#ifdef CURL_DOES_CONVERSIONS
		curl_easy_setopt(curl.get(), CURLOPT_TRANSFERTEXT, 1L);
#endif
#if LIBCURL_VERSION_NUM >= 0x072400
		curl_easy_setopt(curl.get(), CURLOPT_SSL_ENABLE_ALPN, 0L);
#endif
		curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDSIZE, (long)data_buffer.data_size);
		curl_easy_setopt(curl.get(), CURLOPT_READFUNCTION, ReadCallback);
		curl_easy_setopt(curl.get(), CURLOPT_READDATA, &data_buffer);
		curl_easy_setopt(curl.get(), CURLOPT_VERBOSE, 1L);
		curl_easy_setopt(curl.get(), CURLOPT_FOLLOWLOCATION, 1L);
		//curl_easy_setopt(curl.get(), CURLOPT_NOBODY, 1L);  // sBody不接受内容
		
		code = curl_easy_perform(curl.get());
		if (code != CURLE_OK) {
			emit Result(QString(), QString(), QT_UTF8(error));
		} else {
			emit Result(QT_UTF8(sHeader.c_str()), QT_UTF8(sBody.c_str()), QT_UTF8(error));
		}

		curl_slist_free_all(header);

		if (data_buffer.delptr) {
			delete[] data_buffer.delptr;
			data_buffer.delptr = nullptr;
		}
	}
}

bool RemotePostThread::PrepareDataHeader()
{
	time_t t = time(NULL);
	char buf[128];
	memset(buf, 0, sizeof(128));
	sprintf_s(buf, 127, "%llX", t);
	ticks = buf;

	memset(buf, 0, 128);
	sprintf_s(buf, 127, "---------------------------%s", ticks.c_str());
	boundary = buf;

	return true;
}

bool RemotePostThread::PrepareData(const std::string& name, const std::string& value)
{
	std::string data("");
	data += "--";
	data += boundary;
	data += "\r\n";
	data += "Content-Disposition: form-data; name=\"" + name + "\"";
	data += "\r\n\r\n";
	data += value;
	data += "\r\n";

	char* pbuf = nullptr;
	size_t bufsize = data_buffer.data_size + data.length();

	pbuf = new char[bufsize + 1];
	memset(pbuf, 0, sizeof(char)*(bufsize + 1));
	if (data_buffer.readptr) {
		memcpy(pbuf, data_buffer.readptr, data_buffer.data_size);
		delete[] data_buffer.readptr;
		data_buffer.readptr = nullptr;
	}
	memcpy(pbuf + data_buffer.data_size, data.c_str(), data.length());

	data_buffer.readptr = pbuf;
	data_buffer.delptr = data_buffer.readptr;
	data_buffer.data_size = bufsize;

	return true;
}

bool RemotePostThread::PrepareDataFromFile(const std::string& name, const std::string& filename)
{
	fstream file;
	size_t filesize = 0;
	char* pfilebuf = nullptr;

	bool exists = os_file_exists(filename.c_str());
	if (!exists) {
		return false;
	}

	file.open(filename.c_str(), ios_base::in | ios_base::binary);
	if (!file.is_open()) {
		blog(LOG_INFO, "\n读文件[%s]打开失败！\n", filename.c_str());
		file.close();
		return false;
	}
	else {
		file.seekg(0, ios_base::end);
		filesize = file.tellg();
		file.seekg(0, ios_base::beg);
		pfilebuf = new char[filesize + 1];
		memset(pfilebuf, 0, filesize + 1);
		file.read(pfilebuf, filesize);
	}
	file.close();

	size_t r1 = filename.rfind('/');
	size_t r2 = filename.rfind('\\');
	size_t r = r1 > r2 ? r1 : r2;
	string sname("tmp.tmp");
	if (r > 0) {
		sname = filename.substr(r + 1);
	}
	
	string data("");
	data += "--";
	data += boundary;
	data += "\r\n";
	data += "Content-Disposition: form-data; name=\"" + name + "\"; filename=\"" + sname + "\"";
	data += "\r\n";
	data += "Content-Type: image/jpeg";
	data += "\r\n\r\n";

	string lastline = "\r\n";

	size_t bufsize = data_buffer.data_size + data.length() + filesize + lastline.length();
	char* pbuf = new char[bufsize + 1];
	memset(pbuf, 0, sizeof(char)*(bufsize + 1));
	if (data_buffer.readptr) {
		memcpy(pbuf, data_buffer.readptr, data_buffer.data_size);
		delete data_buffer.readptr;
		data_buffer.readptr = nullptr;
	}

	data_buffer.delptr = pbuf;
	data_buffer.readptr = pbuf;

	char* ptr = pbuf + data_buffer.data_size;
	memcpy(ptr, data.c_str(), data.length());
	data_buffer.data_size += data.length();

	ptr = ptr + data.length();
	memcpy(ptr, pfilebuf, filesize);
	data_buffer.data_size += filesize;

	ptr = ptr + filesize;
	memcpy(ptr, lastline.c_str(), lastline.length());
	data_buffer.data_size += lastline.length();
	
	delete [] pfilebuf;
	pfilebuf = nullptr;

	return true;
}

bool RemotePostThread::PrepareDataFoot(bool isFile)
{
	string foot = "";
	if (isFile) {
		foot += "\r\n";  // 如果最后一个是文件，要多加一个回车换行
	}
	foot += "--";
	foot += boundary;
	foot += "--\r\n";

	char* pbuf = nullptr;
	size_t bufsize = data_buffer.data_size + foot.length();
	pbuf = new char[bufsize + 1];
	memset(pbuf, 0, sizeof(char)*(bufsize + 1));
	if (data_buffer.readptr) {
		memcpy(pbuf, data_buffer.readptr, data_buffer.data_size);
		delete[] data_buffer.readptr;
		data_buffer.readptr = nullptr;
	}
	memcpy(pbuf + data_buffer.data_size, foot.c_str(), foot.length());

	data_buffer.readptr = pbuf;
	data_buffer.delptr = data_buffer.readptr;
	data_buffer.data_size = bufsize;

	char buf[128];
	memset(buf, 0, 128);
	sprintf_s(buf, 127, "Content-Length: %d", data_buffer.data_size);
	ContentLength = buf;

	return true;
}
