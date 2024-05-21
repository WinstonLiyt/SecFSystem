#include "Kernel.h"
#include"User.h"
#include"OpenFileManager.h"

Kernel Kernel::instance;
User g_user;

Kernel::Kernel()
{

}

Kernel::~Kernel()
{

}

Kernel& Kernel::Instance()
{
    return Kernel::instance;
}

/* �ں˳�ʼ��
   ÿ����ʼ���Ĺ��̶�����
   ��ָ��ָ���Ӧ��ȫ�ֱ�����
   �ٵ��ö�Ӧ�ĳ�ʼ������ */
void Kernel::Initialize()
{
    InitDeviceDriver();
    InitBuffer();
    InitFileSystem();
    InitUser();
}

BufferManager& Kernel::GetBufferManager()
{
    return *(this->m_BufferManager);
}

FileSystem& Kernel::GetFileSystem()
{
    return *(this->m_FileSystem);
}

FileManager& Kernel::GetFileManager()
{
    return *(this->m_FileManager);
}

DeviceDriver& Kernel::GetDeviceDriver()
{
    return *(this->m_DeviceDriver);
}

User& Kernel::GetUser()
{
    return *(this->m_User);
}

void Kernel::InitDeviceDriver()
{
    this->m_DeviceDriver = &g_DeviceDriver;
    this->m_DeviceDriver->Initialize();
}

void Kernel::InitBuffer()
{
    this->m_BufferManager = &g_BufferManager;
    this->GetBufferManager().Initialize();
}

void Kernel::InitFileSystem()
{
    this->m_FileSystem = &g_FileSystem;
    this->GetFileSystem().Initialize();

    this->m_FileManager = &g_FileManager;
    this->GetFileManager().Initialize();
}

void Kernel::InitUser()
{
    this->m_User = &g_user;
    this->m_User->u_curdir[0] = '/';                /* ��ʼ��ʱ�ڸ�Ŀ¼�� */
    this->m_User->u_cdir = g_InodeTable.IGet(1);    /* ��Ŀ¼ */
    this->m_User->u_pdir = NULL;                    /* һ��ʼû�и���Ŀ¼ */  
}
