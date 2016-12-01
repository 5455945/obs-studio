#include <curl/curl.h>
#include "obs-app.hpp"
#include "qt-wrappers.hpp"
#include "check-record-file.hpp"
#include "remote-data.hpp"
#include <algorithm>
#include <functional>

using namespace std;

void CheckRecordFileThread::GetRecordFiles(std::vector<std::string>& files)
{
	char path[MAX_PATH];
	memset(path, 0, sizeof(MAX_PATH));
	memcpy(path, record_path.c_str(), record_path.length() + 1);
	struct os_dirent *entry;

	os_dir_t *dir = os_opendir(path);
	if (dir) {	
		while ((entry = os_readdir(dir)) != NULL) {
			if (entry->directory || *entry->d_name == '.')
				continue;

			// 如果文件名称的后缀名称重复，是需要上传的文件
			string sname = entry->d_name;
			if (sname.length() <=8)
				continue;
			string tail = sname.substr(sname.length() - 4, 4);
			string last_tail = sname.substr(sname.length() - 8, 4);
			if (tail.compare(last_tail) == 0) {
				string filename = record_path + "/" ;
				filename += sname;
				files.push_back(filename);
			}
		}
	}
	os_closedir(dir);

	// 按降序排序，方便pop_back时间最小文件
	sort(files.begin(), files.end(), greater<string>());
}

void CheckRecordFileThread::run()
{
	// 获取时间最小的录像文件
	string tailfile("");
	bool exists = false;
	vector<string> files;
	GetRecordFiles(files);
	size_t nTail = files.size() - 1;
	while (nTail >= 0) {
		tailfile = files[nTail];
		exists = os_file_exists(tailfile.c_str());
		if (!exists) {
			files.pop_back();
			continue;
		}

		// key 可以是[文件名称]或者 [目录/文件名称]
		size_t n = tailfile.rfind("/") + 1;
		string sname = tailfile.substr(n);
		if (key.empty() || key == "") {
			key = sname.substr(0, sname.length() - 4);
		}
		else {
			char ch = key[key.length() - 1];
			if (!((ch == '/') || (ch == '\\'))) {
				key += "/";
			}
			key += sname.substr(0, sname.length() - 4);
		}
		
		// 开启上传进程
		if (SendThread) {
			SendThread->wait();
			delete SendThread;
		}

		RemoteDataThread *thread = new RemoteDataThread(
			url,
			std::string()
		);
		thread->PrepareData(string("access_key"), access_key);
		thread->PrepareData(string("access_secret"), access_secret);
		thread->PrepareData(string("opt"), opt);
		thread->PrepareData(string("bucket"), bucket);
		thread->PrepareData(string("key"), key);
		thread->PrepareDataFromFile(string("file"), tailfile);
		SendThread = thread;
		connect(thread, &RemoteDataThread::Result, this, &CheckRecordFileThread::SendFinished);
		current_send_file = tailfile;
		SendThread->start();

		// 结束本线程，一次只处理一个文件
		break;
	}
}

// 处理发送每个文件后，web返回结果
void CheckRecordFileThread::SendFinished(const QString& header, const QString& body, const QString& error)
{
	if (header.isEmpty() && body.isEmpty()) {
		blog(LOG_ERROR, "发送断流录像时，web服务端返回内容为空！");
	}

	obs_data_t* returnData = obs_data_create_from_json(QT_TO_UTF8(body));
	const char *json = obs_data_get_json(returnData);  // test
	json = QT_TO_UTF8(error);  // test
	bool bResult = false;
	bResult = obs_data_get_bool(returnData, "rt");
	const char* sError = obs_data_get_string(returnData, "error");

	if (bResult) {
		//std::string md5 = obs_data_get_string(returnData, "md5");
		//std::string url = obs_data_get_string(returnData, "objUrl");
		obs_data_release(returnData);

		// 删除已上传文件，一定要确保删除成功
		bool exists = os_file_exists(current_send_file.c_str());
		if (exists) {
			int nret = os_unlink(current_send_file.c_str());
			if (nret != 0) {
				int retry = 0;
				for (retry = 0; retry < 5; retry++) {
					Sleep(5000);
					nret = os_unlink(current_send_file.c_str());
					if (nret == 0) {
						break;
					}
				}
				if (retry == 5) {
					string newname = current_send_file.substr(0, current_send_file.length() - 4);
					os_rename(current_send_file.c_str(), newname.c_str());
				}
			}
		}
		
		// 继续运行检查
		this->run();
	}
	else {
		blog(LOG_WARNING, "Bad JSON file received from server, error:%s", sError);
		obs_data_release(returnData);
	}
}
