#include "Utility.h"
#include "FileSystem.h"
#include "DeviceDriver.h"


/* ���ӳ����ͼ */
MapView Utility::get_mapview(const char* disk_name, const char* shared_name)
{
	MapView disk_mapview;

	DWORD access_mode = (GENERIC_READ | GENERIC_WRITE);  // �ļ���ȡģʽ
	DWORD share_mode = FILE_SHARE_READ | FILE_SHARE_WRITE;  // �ļ�����ģʽ
	DWORD flags = FILE_FLAG_SEQUENTIAL_SCAN;  // �ļ�����	

	FileSystem fs;
	const DWORD disk_size = (fs.DATA_ZONE_END_SECTOR + 1) * BLOCK_SIZE;  // ���̴�С������������λ��+1�ٳ˿��С
	DWORD error_code;  // ������
	// �򿪴���
	disk_mapview.disk_handle = CreateFileA(
		disk_name,
		access_mode,
		share_mode,
		NULL,
		OPEN_ALWAYS,
		flags,
		NULL);
	// ����Ƿ񴴽��ɹ�
	if (disk_mapview.disk_handle == INVALID_HANDLE_VALUE)
	{
		error_code = GetLastError();
		cout << "��ʽ������ʧ�ܣ�������Ϊ: " << error_code << endl;
		exit(1);
	}
	DWORD size_high = 0;  // Ϊ0ʹ�ļ�ӳ����������С���� hFile ��ʶ���ļ��ĵ�ǰ��С
	// �����ļ�ӳ�䣬���Ҫ�����ڴ�ҳ���ļ���ӳ�䣬��һ����������ΪINVALID_HANDLE_VALUE
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
		cout << "�ļ�ӳ�䴴��ʧ�ܣ�������Ϊ: " <<error_code<< endl;
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
			disk_mapview.disk_base_address = (char*)MapViewOfFile(disk_mapview.diskfm, view_access, 0, 0, 0);  // �Ѵ���ȫ��ӳ�����
		}
	}
	return disk_mapview;
}

void Utility::unmap_mapview(MapView& mapview)
{
	UnmapViewOfFile(mapview.disk_base_address);  // ж��ӳ��
	CloseHandle(mapview.diskfm);	// �ر��ڴ�ӳ���ļ�
	CloseHandle(mapview.disk_handle);	// �ر��ļ�
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
	// ��ȡ��������С��������ռ䣬��������С�°��ֽڼ����  
	int len = WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), wstr.size(), NULL, 0, NULL, NULL);
	char* buffer = new char[len + 1];
	// ���ֽڱ���ת���ɶ��ֽڱ���  
	WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), wstr.size(), buffer, len, NULL, NULL);
	buffer[len] = '\0';
	// ɾ��������������ֵ  
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

	/* �����ַ������� */
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