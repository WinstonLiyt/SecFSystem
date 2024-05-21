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

/* ������������ļ���ʱ���ڡ����ļ����������з���һ�����б��� */
int OpenFiles::AllocFreeSlot()
{
	int i;
	User& u = Kernel::Instance().GetUser();

	for (i = 0; i < OpenFiles::NOFILES; i++)
	{
		/* ���̴��ļ������������ҵ�������򷵻�֮ */
		if (this->ProcessOpenFileTable[i] == NULL)
		{
			/* ���ú���ջ�ֳ��������е�EAX�Ĵ�����ֵ����ϵͳ���÷���ֵ */
			u.u_ar0 = i;	/* ʹ��eax�Ĵ������ݱ����� */
			return i;		/* ����ɹ������ر����� */
		}
	}

	u.u_ar0 = -1;   /* Open1����Ҫһ����־�������ļ��ṹ����ʧ��ʱ�����Ի���ϵͳ��Դ */
	u.u_error = User::emfile;
	return -1;
}

/* ����fd�ҵ���Ӧ��FIle�ļ�ָ�� */
File* OpenFiles::GetF(int fd)
{
	File* pFile;	/* ��Ŵ��ļ������ָ�� */
	//User& u = Kernel::Instance().GetUser();	//��õ�ǰ��user�ṹ

	/* ������ļ���������ֵ�����˷�Χ */
	if (fd < 0 || fd >= OpenFiles::NOFILES)
	{
		cout << "�������ļ���������ֵ�����˷�Χ��" << endl;
		return NULL;
	}

	/* ���fd��Ӧ�ڽ��̴��ļ�������������Ӧ��File�ṹ */
	pFile = this->ProcessOpenFileTable[fd];
	if (pFile == NULL)
		cout << "������pFileΪ�գ�" << endl;

	return pFile;	/* ��ʹpFile==NULLҲ���������ɵ���GetF�ĺ������жϷ���ֵ */
}

/* Ϊ�ѷ��䵽�Ŀ���������fd���ѷ���Ĵ��ļ����п���File������������ϵ */
void OpenFiles::SetF(int fd, File* pFile)
{
	if (fd < 0 || fd >= OpenFiles::NOFILES)
		cout << "�������ļ���������ֵ�����˷�Χ��" << endl;

	/* ���̴��ļ�������ָ��ϵͳ���ļ�������Ӧ��File�ṹ */
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
