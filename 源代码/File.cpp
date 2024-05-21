#include "File.h"
#include "User.h"
#include "Kernel.h"

File::File()
{
	this->f_count = 0;
	this->f_flag = 0;
	this->f_offset = 0;
	this->f_inode = NULL;
}

File::~File()
{
	//nothing to do here
}

OpenFiles::OpenFiles()
{

}

OpenFiles::~OpenFiles()
{

}

/* 【进程请求打开文件】时，在【打开文件描述符表】中分配一个空闲表项 */
int OpenFiles::AllocFreeSlot()
{
	int i;
	User& u = Kernel::Instance().GetUser();

	for (i = 0; i < OpenFiles::NOFILES; i++)
	{
		/* 进程打开文件描述符表中找到空闲项，则返回之 */
		if (this->ProcessOpenFileTable[i] == NULL)
		{
			/* 设置核心栈现场保护区中的EAX寄存器的值，即系统调用返回值 */
			u.u_ar0 = i;	/* 使用eax寄存器传递表项编号 */
			return i;		/* 分配成功，返回表项编号 */
		}
	}

	u.u_ar0 = -1;   /* Open1，需要一个标志。当打开文件结构创建失败时，可以回收系统资源 */
	u.u_error = User::emfile;
	return -1;
}

/* 根据fd找到对应的FIle文件指针 */
File* OpenFiles::GetF(int fd)
{
	File* pFile;	/* 存放打开文件表项的指针 */
	//User& u = Kernel::Instance().GetUser();	//获得当前的user结构

	/* 如果打开文件描述符的值超出了范围 */
	if (fd < 0 || fd >= OpenFiles::NOFILES)
	{
		cout << "【错误】文件描述符的值超出了范围！" << endl;
		return NULL;
	}

	/* 获得fd对应在进程打开文件描述符表中相应的File结构 */
	pFile = this->ProcessOpenFileTable[fd];
	if (pFile == NULL)
		cout << "【错误】pFile为空！" << endl;

	return pFile;	/* 即使pFile==NULL也返回它，由调用GetF的函数来判断返回值 */
}

/* 为已分配到的空闲描述符fd和已分配的打开文件表中空闲File对象建立勾连关系 */
void OpenFiles::SetF(int fd, File* pFile)
{
	if (fd < 0 || fd >= OpenFiles::NOFILES)
		cout << "【错误】文件描述符的值超出了范围！" << endl;

	/* 进程打开文件描述符指向系统打开文件表中相应的File结构 */
	this->ProcessOpenFileTable[fd] = pFile;
}

IOParameter::IOParameter()
{
	this->m_Base = 0;
	this->m_Count = 0;
	this->m_Offset = 0;
}

IOParameter::~IOParameter()
{
	//nothing to do here
}
