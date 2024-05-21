#ifndef DEVICEDRIVER_H
#define DEVICEDRIVER_H

#include <iostream>
#include <assert.h>
#include <Windows.h>
#include <WinBase.h>
#include <io.h>
#include "Buf.h"

using namespace std;

#define BLOCK_SIZE 512		/* ���ݿ��СΪ512�ֽ� */

/*
* �ļ������豸�����࣬��Ҫ������̸�ʽ��
*/
class DeviceDriver
{
public:
	static const char* disk_name;		/* �������� */
	static const char* shared_name;		/* �ļ�ӳ��������� */

	/* Constructors */
	DeviceDriver();
	/* Destructors */
	~DeviceDriver();

	/* �豸������ʼ�� */
	void Initialize();

	/* ���̸�ʽ�� */
	void format();

	/* ��ȡ�������� */
	const char* GetDiskName();

	/* ��ȡ�ļ�ӳ��������� */
	const char* GetSharedName();

	/* �������bp�Ķ�д���� */
	void IO(Buf* bp);
};

extern DeviceDriver g_DeviceDriver;		/* ȫ���豸����ģ�� */

#endif