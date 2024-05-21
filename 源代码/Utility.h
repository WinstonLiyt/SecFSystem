#pragma once
#include<iostream>
#include <assert.h>
#include <Windows.h>
#include <WinBase.h>
#include<vector>

using namespace std;

#define VERSION 1.1

/* 管理文件映射视图 */
class MapView
{
public:
	MapView();
	~MapView();

	HANDLE disk_handle;			/* 句柄1 */
	HANDLE diskfm;				/* 句柄2 */
	char* disk_base_address;	/* 磁盘映射基地址 */
};

/* 存储一些功能函数 */
class Utility
{
public:
	static void StringCopy(char* src, char* dst);				/* 字符串复制 */
	static MapView get_mapview(const char* disk_name, const char* shared_name);		/* 获得磁盘映射视图 */
	static void unmap_mapview(MapView& mapview);				/* 卸载映射和关闭文件的操作 */
	static vector<string> splitstr(string str, string pattern);	/* 分割命令行输入 */
	static string wstring2string(wstring wstr);					/* 宽string转string */
	static void DWordCopy(int* src, int* dst, int count);		/* 复制字节 */
	static int StringLength(char* pString);						/* 获取String的长度 */
	
};

/* 内存映射读函数 */
template<class T>
void map_read(char*& cur, T& mem, size_t len)
{
	memcpy(mem, cur, len);
	cur += len;
}

/* 内存映射写函数 */
template<class T>
void map_write(char*& cur, T& mem, size_t len)
{
	memcpy(cur, mem, len);
	cur += len;
}