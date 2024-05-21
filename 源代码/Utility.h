#pragma once
#include<iostream>
#include <assert.h>
#include <Windows.h>
#include <WinBase.h>
#include<vector>

using namespace std;

#define VERSION 1.1

/* �����ļ�ӳ����ͼ */
class MapView
{
public:
	MapView();
	~MapView();

	HANDLE disk_handle;			/* ���1 */
	HANDLE diskfm;				/* ���2 */
	char* disk_base_address;	/* ����ӳ�����ַ */
};

/* �洢һЩ���ܺ��� */
class Utility
{
public:
	static void StringCopy(char* src, char* dst);				/* �ַ������� */
	static MapView get_mapview(const char* disk_name, const char* shared_name);		/* ��ô���ӳ����ͼ */
	static void unmap_mapview(MapView& mapview);				/* ж��ӳ��͹ر��ļ��Ĳ��� */
	static vector<string> splitstr(string str, string pattern);	/* �ָ����������� */
	static string wstring2string(wstring wstr);					/* ��stringתstring */
	static void DWordCopy(int* src, int* dst, int count);		/* �����ֽ� */
	static int StringLength(char* pString);						/* ��ȡString�ĳ��� */
	
};

/* �ڴ�ӳ������� */
template<class T>
void map_read(char*& cur, T& mem, size_t len)
{
	memcpy(mem, cur, len);
	cur += len;
}

/* �ڴ�ӳ��д���� */
template<class T>
void map_write(char*& cur, T& mem, size_t len)
{
	memcpy(cur, mem, len);
	cur += len;
}