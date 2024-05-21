#include "Utility.h"
#include "FileSystem.h"
#include "DeviceDriver.h"


/* 获得映射视图 */
MapView Utility::get_mapview(const char* disk_name, const char* shared_name)
{
	MapView disk_mapview;

	DWORD access_mode = (GENERIC_READ | GENERIC_WRITE);  // 文件存取模式
	DWORD share_mode = FILE_SHARE_READ | FILE_SHARE_WRITE;  // 文件共享模式
	DWORD flags = FILE_FLAG_SEQUENTIAL_SCAN;  // 文件属性	

	FileSystem fs;
	const DWORD disk_size = (fs.DATA_ZONE_END_SECTOR + 1) * BLOCK_SIZE;  // 磁盘大小，数据区结束位置+1再乘块大小
	DWORD error_code;  // 错误码
	// 打开磁盘
	disk_mapview.disk_handle = CreateFileA(
		disk_name,
		access_mode,
		share_mode,
		NULL,
		OPEN_ALWAYS,
		flags,
		NULL);
	// 检查是否创建成功
	if (disk_mapview.disk_handle == INVALID_HANDLE_VALUE)
	{
		error_code = GetLastError();
		cout << "格式化磁盘失败，错误码为: " << error_code << endl;
		exit(1);
	}
	DWORD size_high = 0;  // 为0使文件映射对象的最大大小等于 hFile 标识的文件的当前大小
	// 创建文件映射，如果要创建内存页面文件的映射，第一个参数设置为INVALID_HANDLE_VALUE
	disk_mapview.diskfm = CreateFileMappingA(
		disk_mapview.disk_handle,
		NULL,
		PAGE_READWRITE,
		size_high,
		disk_size,
		shared_name
	);
	error_code = GetLastError();
	if (error_code)
	{
		cout << "文件映射创建失败，错误码为: " <<error_code<< endl;
		exit(2);
	}
	else
	{
		if (disk_mapview.diskfm == NULL)
		{
			if (disk_mapview.disk_handle != INVALID_HANDLE_VALUE)
			{
				CloseHandle(disk_mapview.disk_handle);
			}
		}
		else
		{
			DWORD view_access = FILE_MAP_ALL_ACCESS;
			disk_mapview.disk_base_address = (char*)MapViewOfFile(disk_mapview.diskfm, view_access, 0, 0, 0);  // 把磁盘全部映射过来
		}
	}
	return disk_mapview;
}

void Utility::unmap_mapview(MapView& mapview)
{
	UnmapViewOfFile(mapview.disk_base_address);  // 卸载映射
	CloseHandle(mapview.diskfm);	// 关闭内存映射文件
	CloseHandle(mapview.disk_handle);	// 关闭文件
}

vector<string> Utility::splitstr(string str, string pattern)
{
	vector<string> res;
	char* pTmp = NULL;
	char* temp = strtok_s((char*)str.c_str(), pattern.c_str(), &pTmp);
	while (temp != NULL)
	{
		res.push_back(string(temp));
		temp = strtok_s(NULL, pattern.c_str(), &pTmp);
	}
	return res;
}

string Utility::wstring2string(wstring wstr)
{
	string result;
	// 获取缓冲区大小，并申请空间，缓冲区大小事按字节计算的  
	int len = WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), wstr.size(), NULL, 0, NULL, NULL);
	char* buffer = new char[len + 1];
	// 宽字节编码转换成多字节编码  
	WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), wstr.size(), buffer, len, NULL, NULL);
	buffer[len] = '\0';
	// 删除缓冲区并返回值  
	result.append(buffer);
	delete[] buffer;
	return result;
}

void Utility::StringCopy(char* src, char* dst)
{
	while ((*dst++ = *src++) != 0);
}

int Utility::StringLength(char* pString)
{
	int length = 0;
	char* pChar = pString;

	while (*pChar++)
	{
		length++;
	}

	/* 返回字符串长度 */
	return length;
}

void Utility::DWordCopy(int* src, int* dst, int count)
{
	while (count--)
	{
		*dst++ = *src++;
	}
	return;
}

vector<string> splitstr(string str, string pattern)
{
	vector<string> res;
	char* pTmp = NULL;
	char* temp = strtok_s((char*)str.c_str(), pattern.c_str(), &pTmp);
	while (temp != NULL)
	{
		res.push_back(string(temp));
		temp = strtok_s(NULL, pattern.c_str(), &pTmp);
	}
	return res;
}

MapView::MapView()
{
	this->diskfm = NULL;
	this->disk_base_address = NULL;
	this->disk_handle = NULL;
}

MapView::~MapView()
{

}