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

/* 内核初始化
   每个初始化的过程都是先
   让指针指向对应的全局变量，
   再调用对应的初始化函数 */
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
    this->m_User->u_curdir[0] = '/';                /* 初始化时在根目录下 */
    this->m_User->u_cdir = g_InodeTable.IGet(1);    /* 根目录 */
    this->m_User->u_pdir = NULL;                    /* 一开始没有父级目录 */  
}
