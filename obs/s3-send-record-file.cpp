#include "obs-app.hpp"
#include "s3-send-record-file.hpp"
#include <algorithm>
#include <functional>
#include <windows.h>

using namespace std;

void S3SendRecordFileThread::GetRecordFiles(std::vector<std::string>& files)
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

void S3SendRecordFileThread::run()
{
	// 获取时间最小的录像文件
	string tailfile("");
	string content_type("video/x-flv");
	bool exists = false;
	vector<string> files;
	GetRecordFiles(files);
	int nTail = files.size() - 1;
	int nFailCnt = 0;
	while (nTail >= 0) {
		tailfile = files[nTail];
		exists = os_file_exists(tailfile.c_str());
		if (!exists) {
			files.pop_back();
			continue;
		}

		// key 可以是[文件名称]或者 [目录/文件名称]
		int n = tailfile.rfind("/") + 1;
		string sname = tailfile.substr(n);
		string onekey = "";
		if (key.empty() || key == "") {
			onekey = sname.substr(0, sname.length() - 4);
		}
		else {
			if (!((key[key.length() - 1] == char("/")) || (key[key.length() - 1] == char("\\")))) {
				onekey = key + "/";
			}
			onekey += sname.substr(0, sname.length() - 4);
		}
		
		// .avi  video/avi 
		// .flv  video/x-flv
		// .mp4  video/mpeg4
		// .mov  
		// .mkv  
		// .ts   
		// .m3u8 
		// movie video/x-sgi-movie
		string fix = tailfile.substr(tailfile.length() - 4);
		if (fix.compare(".flv") == 0) {
			content_type = "video/x-flv";
		}
		else if (fix.compare(".mp4") == 0) {
			content_type = "video/mpeg4";
		}
		else if (fix.compare(".avi") == 0) {
			content_type = "video/avi";
		}

		typedef int (*PFUN_SendFile)(const char* hostname, const char* access_key, const char* secret_key, const char* bucket, const char* key, const char* filename, const char* content_type);
		
		HMODULE hMod = GetModuleHandleA("libs3.dll");
		if (hMod) {
			PFUN_SendFile pfunc = (PFUN_SendFile)GetProcAddress(hMod, "SendFile");
			if (pfunc) {
				int nRet = pfunc(url.c_str(), 
					access_key.c_str(),
					access_secret.c_str(), 
					bucket.c_str(),
					onekey.c_str(),
					tailfile.c_str(),
					content_type.c_str()
					);

				if (0 == nRet) {  // 返回0标示成功
					// 删除已上传文件，一定要确保删除成功
					bool exists = os_file_exists(tailfile.c_str());
					if (exists) {
						int nret = os_unlink(tailfile.c_str());
						if (nret != 0) {
							int retry = 0;
							for (retry = 0; retry < 5; retry++) {
								Sleep(5000);
								nret = os_unlink(tailfile.c_str());
								if (nret == 0) {
									break;
								}
							}
							if (retry == 5) {
								string newname = tailfile.substr(0, tailfile.length() - 4);
								os_rename(tailfile.c_str(), newname.c_str());
							}
						}
					}
					files.pop_back();
					nFailCnt = 0;
				}
				else {
					nFailCnt++;
					if (nFailCnt < 5) {
						// 如果文件发送失败，重试；做多发送5次
						continue;
					}
					else {
						// 如果5次发送失败，不物理删除文件，以后启动检查还会尝试发送
						files.pop_back();
						nFailCnt = 0;
					}
				}
			}
		}
		nTail = files.size() - 1;
	}
}
