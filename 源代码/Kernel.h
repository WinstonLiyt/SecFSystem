#ifndef KERNEL_H
#define KERNEL_H

#include "FileSystem.h"
#include "BufferManger.h"
#include "FileManager.h"
#include "DeviceDriver.h"
#include "User.h"

/*
 * Kernel�����ڷ�װ�����ں���ص�ȫ����ʵ������
 * ����PageManager, ProcessManager�ȡ�
 *
 * Kernel�����ڴ���Ϊ����ģʽ����֤�ں��з�װ���ں�
 * ģ��Ķ���ֻ��һ��������
 */
class Kernel
{
public:
	Kernel();
	~Kernel();
	static Kernel& Instance();
	void Initialize();		/* �ú�����ɳ�ʼ���ں˴󲿷����ݽṹ�ĳ�ʼ�� */

	BufferManager& GetBufferManager();
	FileSystem& GetFileSystem();
	FileManager& GetFileManager();
	DeviceDriver& GetDeviceDriver();
	User& GetUser();		/* ��ȡ��ǰ���̵�User�ṹ */

private:
	void InitDeviceDriver();
	void InitBuffer();
	void InitFileSystem();
	void InitUser();

private:
	static Kernel instance;		/* Kernel������ʵ�� */

	BufferManager* m_BufferManager;
	FileSystem* m_FileSystem;
	FileManager* m_FileManager;
	DeviceDriver* m_DeviceDriver;
	User* m_User;
};

#endif