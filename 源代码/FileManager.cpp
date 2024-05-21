#include "FileManager.h"
#include"Kernel.h"
#include"Utility.h"

FileManager g_FileManager;

/*==============================class DirectoryEntry===================================*/
DirectoryEntry::DirectoryEntry()
{
	this->m_ino = 0;
	this->m_name[0] = '\0';
}

DirectoryEntry::~DirectoryEntry()
{
	//nothing to do 
}
/*==============================class DirectoryEntry===================================*/

FileManager::FileManager()
{
	//nothing to do here
}

FileManager::~FileManager()
{
	//nothing to do here
}

void FileManager::Initialize()
{
	this->m_FileSystem = &Kernel::Instance().GetFileSystem();
	this->m_InodeTable = &g_InodeTable;
	this->m_OpenFileTable = &g_OpenFileTable;
	this->rootDirInode = this->m_InodeTable->IGet(1);	//1#Inode���Ǹ�Ŀ¼
	this->rootDirInode->i_flag &= ~Inode::ILOCK;
	this->m_InodeTable->Initialize();
}

/*
 * ���ܣ����ļ�
 * Ч�����������ļ��ṹ���ڴ�i�ڵ㿪�� ��i_count Ϊ������i_count ++��
 * */
void FileManager::Open()
{
	Inode* pInode;
	User& u = Kernel::Instance().GetUser();	

	pInode = this->NameI(NextChar, FileManager::OPEN);	/* 0 = Open, not create */
	/* û���ҵ���Ӧ��Inode */
	if (NULL == pInode)
	{
		u.u_ar0 = -1;  /* �Ҳ�����Ӧ���ļ�fdΪ-1 */
		cout << "���������ļ��򿪵Ĺ������Ҳ�����Ӧ��pInode��" << endl;
		return;	
	}
	this->Open1(pInode, u.u_arg[1], 0);  /* ttf = 0����Open���á�u_arg[1]������mode��ֵ */
}

/*
 * ���ܣ�����һ���µ��ļ�
 * Ч�����������ļ��ṹ���ڴ�i�ڵ㿪�� ��i_count Ϊ������Ӧ���� 1��
 * */
void FileManager::Creat()
{
	Inode* pInode;
	User& u = Kernel::Instance().GetUser();

	/* ����д������Ȩ�� */
	unsigned int newACCMode = u.u_arg[1] & (Inode::IRWXU | Inode::IRWXG | Inode::IRWXO);

	/* ����Ŀ¼��ģʽΪ1����ʾ����������Ŀ¼����д�������� */
	pInode = this->NameI(NextChar, FileManager::CREATE);
	/* û���ҵ���Ӧ��Inode����NameI���� */
	if (NULL == pInode)
	{
		if (u.u_error)
			return;
		/* ������Ҫ������Ŀ¼�����Դ���Inode */
		pInode = this->MakNode(newACCMode & (~Inode::ISVTX));
		/* ����ʧ�� */
		if (NULL == pInode)
		{
			return;
		}

		/*
		 * �����ϣ�������ֲ����ڣ�ʹ�ò���trf = 2������open1()��
		 * ����Ҫ����Ȩ�޼�飬��Ϊ�ոս������ļ���Ȩ�޺ʹ������mode
		 * ����ʾ��Ȩ��������һ���ġ�
		 */
		this->Open1(pInode, File::FWRITE, 2);
	}
	else
	{
		/* ���NameI()�������Ѿ�����Ҫ�������ļ�������ո��ļ������㷨ITrunc()����UIDû�иı�
		 * ԭ��UNIX��������������ļ�����ȥ�����½����ļ�һ����Ȼ�������ļ������ߺ����Ȩ��ʽû�䡣
		 * Ҳ����˵creatָ����RWX������Ч��
		 * ������Ϊ���ǲ�����ģ�Ӧ�øı䡣
		 * ���ڵ�ʵ�֣�creatָ����RWX������Ч */
		this->Open1(pInode, File::FWRITE, 1);
		pInode->i_mode |= newACCMode;
	}
}


/*
* trf == 0��open����
* trf == 1��creat���ã�creat�ļ���ʱ��������ͬ�ļ������ļ�
* trf == 2��creat���ã�creat�ļ���ʱ��δ������ͬ�ļ������ļ��������ļ�����ʱ��һ������
* mode���������ļ�ģʽ����ʾ�ļ������� ����д���Ƕ�д
*/
void FileManager::Open1(Inode* pInode, int mode, int trf)
{
	User& u = Kernel::Instance().GetUser();

	/*
	 * ����ϣ�����ļ��Ѵ��ڵ�����£���trf == 0��trf == 1����Ȩ�޼��
	 * �����ϣ�������ֲ����ڣ���trf == 2������Ҫ����Ȩ�޼�飬��Ϊ�ս���
	 * ���ļ���Ȩ�޺ʹ���Ĳ���mode������ʾ��Ȩ��������һ���ġ�
	 */
	if (trf != 2)
	{
		/* trf��Ϊ2����Ҫ����ļ����� */
		if (mode & File::FREAD)
		{
			/* ����Ȩ�� */
			if (this->Access(pInode, Inode::IREAD))
			{
				cout << "�����󡿵�ǰ�û�û�ж�ȡ���ļ���Ȩ�ޣ�" << endl;
			}
		}
		if (mode & File::FWRITE)
		{
			/* ���дȨ�� */
			this->Access(pInode, Inode::IWRITE);
		}
	}

	if (u.u_error)
	{
		this->m_InodeTable->IPut(pInode);  /* �д��ͷ��ڴ�inode */
		return;
	}

	/* ��creat�ļ���ʱ��������ͬ�ļ������ļ����ͷŸ��ļ���ռ�ݵ������̿� */
	if (1 == trf)
	{
		//����ļ�����
		pInode->ITrunc();
	}

	/* ����inode!
	 * ����Ŀ¼�����漰�����Ĵ��̶�д�������ڼ���̻���˯��
	 * ��ˣ����̱������������漰��i�ڵ㡣�����NameI��ִ�е�IGet����������
	 * �����ˣ����������п��ܻ���������л��Ĳ��������Խ���i�ڵ㡣
	 */
	pInode->Prele();

	/* ������ļ����ƿ�File�ṹ */
	File* pFile = this->m_OpenFileTable->FAlloc();
	if (NULL == pFile)
	{
		this->m_InodeTable->IPut(pInode);  /* �ͷ��ڴ�inode */
		return;
	}
	/* ���ô��ļ���ʽ������File�ṹ���ڴ�Inode�Ĺ�����ϵ */
	pFile->f_flag = mode & (File::FREAD | File::FWRITE);
	pFile->f_inode = pInode;  /* f_inodeָ���Ӧ�ļ���inode */


	/* Ϊ�򿪻��ߴ����ļ��ĸ�����Դ���ѳɹ����䣬�������� */
	if (u.u_error == 0)
	{
		return;
	}
	else	/* ����������ͷ���Դ����ʵ�������ó��� */
	{
		cout << "�쳣����:  " << u.u_error << endl;
		/* �ͷŴ��ļ������� */
		int fd = u.u_ar0;
		if (fd != -1)
		{
			u.u_ofiles.SetF(fd, NULL);  /* �ͷŴ��ļ�������fd���ݼ�File�ṹ���ü��� */
			/* �ݼ�File�ṹ��Inode�����ü�����File�ṹû���� f_countΪ0�����ͷ�File�ṹ��*/
			pFile->f_count--;
		}
		this->m_InodeTable->IPut(pInode);
	}
}

/* Close()ϵͳ���õĴ������ */
void FileManager::Close()
{
	User& u = Kernel::Instance().GetUser();
	int fd = u.u_arg[0];

	/* ��ȡ���ļ����ƿ�File�ṹ */
	File* pFile = u.u_ofiles.GetF(fd);
	if (NULL == pFile)
	{
		cout << "������Close������File�ṹ��ʧ�ܣ�" << endl;
		return;
	}

	/* �ͷŴ��ļ�������fd���ݼ�File�ṹ���ü��� */
	u.u_ofiles.SetF(fd, NULL);
	/* �ݼ�File�ṹ��Inode�����ü�����File�ṹû���� f_countΪ0�����ͷ�File�ṹ��*/
	this->m_OpenFileTable->CloseF(pFile);
}

void FileManager::Seek()
{
	File* pFile;
	User& u = Kernel::Instance().GetUser();
	int fd = u.u_arg[0];  /* ��ȡ�ļ������� */

	pFile = u.u_ofiles.GetF(fd);  /* ��ȡFile�ṹ */
	if (NULL == pFile)
	{
		cout << "������Seek�л�ȡFile�ṹʧ�ܣ�" << endl;
		return;  /* ��FILE�����ڣ�GetF��������� */
	}

	/* �ܵ��ļ�������seek */
	if (pFile->f_flag & File::FPIPE)
	{
		u.u_error = User::espipe;
		return;
	}

	int offset = u.u_arg[1];

	/* ���u.u_arg[2]��3 ~ 5֮�䣬��ô���ȵ�λ���ֽڱ�Ϊ512�ֽ� */
	if (u.u_arg[2] > 2)
	{
		offset = offset << 9;  // ����512
		u.u_arg[2] -= 3;  // arg2��3
	}

	switch (u.u_arg[2])
	{
		/* ��дλ������Ϊoffset */
		case 0:
			pFile->f_offset = offset;
			break;
			/* ��дλ�ü�offset(�����ɸ�) */
		case 1:
			pFile->f_offset += offset;
			break;
			/* ��дλ�õ���Ϊ�ļ����ȼ�offset */
		case 2:
			pFile->f_offset = pFile->f_inode->i_size + offset;
			break;
	}
}

/* ��ɻ�ȡ�ļ���Ϣ������� */
void FileManager::FStat()
{
	File* pFile;
	User& u = Kernel::Instance().GetUser();
	int fd = u.u_arg[0];  // �ļ�������

	pFile = u.u_ofiles.GetF(fd);  // ���ļ����ƿ�File�ṹ

	if (NULL == pFile)
	{
		cout << "��������FStat�д��ļ����ƿ�File�ṹʧ�ܣ�" << endl;
		return;
	}

	/* u.u_arg[1] = pStatBuf */
	this->Stat1(pFile->f_inode, u.u_arg[1]);  // �ļ���Ϣ
}

void FileManager::Stat()
{
	Inode* pInode;
	User& u = Kernel::Instance().GetUser();
	pInode = this->NameI(FileManager::NextChar, FileManager::OPEN);

	if (NULL == pInode)
	{
		return;
	}

	this->Stat1(pInode, u.u_arg[1]);	/* ��ȡ�ļ���Ϣ */
	this->m_InodeTable->IPut(pInode);	/* �ͷ�inode */
}

void FileManager::Stat1(Inode* pInode, unsigned long statBuf)
{
	Buf* pBuf;  // ������ָ��
	BufferManager& bufMgr = Kernel::Instance().GetBufferManager();

	pInode->IUpdate(time(NULL));  // ����Inode��ʱ���
	pBuf = bufMgr.Bread(FileSystem::INODE_ZONE_START_SECTOR + pInode->i_number / FileSystem::INODE_NUMBER_PER_SECTOR);  // ��ȡ���Inode���ڵ�����

	/* ��pָ�򻺴����б��Ϊinumber���Inode��ƫ��λ�� */
	unsigned char* p = pBuf->b_addr + (pInode->i_number % FileSystem::INODE_NUMBER_PER_SECTOR) * sizeof(DiskInode);
	/* ����Inode��Ϣ���û��ռ� */
	Utility::DWordCopy((int*)p, (int*)statBuf, sizeof(DiskInode) / sizeof(int));

	bufMgr.Brelse(pBuf);  // �ͷŻ�����
}

void FileManager::Read()
{
	/* ֱ�ӵ���Rdwr()�������� */
	this->Rdwr(File::FREAD);
}

void FileManager::Write()
{
	/* ֱ�ӵ���Rdwr()�������� */
	this->Rdwr(File::FWRITE);
}

/* ����NULL��ʾĿ¼����ʧ�ܣ������Ǹ�ָ�룬ָ���ļ����ڴ��i�ڵ� ���������ڴ�i�ڵ�  */
Inode* FileManager::NameI(char(*func)(), DirectorySearchMode mode)
{
	Inode* pInode;	/* ָ��ǰ������Ŀ¼��Inode */
	Buf* pBuf;		/* ָ��ǰ������Ŀ¼�Ļ����� */
	char curchar;	/* ��ǰ�ַ� */
	char* pChar;	/* ָ��ǰ�ַ���ָ�� */
	int freeEntryOffset;	/* �Դ����ļ�ģʽ����Ŀ¼ʱ����¼����Ŀ¼���ƫ���� */
	User& u = Kernel::Instance().GetUser();
	BufferManager& bufMgr = Kernel::Instance().GetBufferManager();

	/*
	 * �����·����'/'��ͷ�ģ��Ӹ�Ŀ¼��ʼ������
	 * ����ӽ��̵�ǰ����Ŀ¼��ʼ������
	 */
	pInode = u.u_cdir;  // ָ��ǰ���̵ĵ�ǰ����Ŀ¼
	if ('/' == (curchar = (*func)()))
	{
		pInode = this->rootDirInode;  // ��Ŀ¼
	}

	/* ����Inode�Ƿ����ڱ�ʹ�ã��Լ���֤������Ŀ¼���������и�Inode�����ͷ� */
	this->m_InodeTable->IGet(pInode->i_number);

	/* �������////a//b ����·�� ����·���ȼ���/a/b */
	while ('/' == curchar)
	{
		curchar = (*func)();	//��ͣ��ȡ��һ���ַ�
	}
	/* �����ͼ���ĺ�ɾ����ǰĿ¼�ļ������ */
	if ('\0' == curchar && mode != FileManager::OPEN)
	{
		u.u_error = User::enoent;
		goto out;  // �ͷŵ�ǰ��������Ŀ¼�ļ�Inode�����˳�
	}

	/* ���ѭ��ÿ�δ���pathname��һ��·������ */
	while (true)
	{
		/* ����������ͷŵ�ǰ��������Ŀ¼�ļ�Inode�����˳� */
		if (u.u_error != User::noerror)
		{
			break;	/* goto out; */
		}
		
		/* ����·��������ϣ�������ӦInodeָ�롣Ŀ¼�����ɹ����ء� */
		if ('\0' == curchar)
		{
			return pInode;	// �����������Inode
		}

		/* ���Ҫ���������Ĳ���Ŀ¼�ļ����ͷ����Inode��Դ���˳� */
		if ((pInode->i_mode & Inode::IFMT) != Inode::IFDIR)
		{
			break;	/* goto out; */
		}

		/* ����Ŀ¼����Ȩ�޼�飬IEXEC��Ŀ¼�ļ��б�ʾ����Ȩ�� */
		if (this->Access(pInode, Inode::IEXEC))
		{
			u.u_error = User::eacces;
			break;	/* ���߱�Ŀ¼����Ȩ�ޣ�goto out; */
		}

		/*
		 * ��Pathname�е�ǰ׼������ƥ���·������������u.u_dbuf[]�У�
		 * ���ں�Ŀ¼����бȽϡ�
		 */
		pChar = &(u.u_dbuf[0]);	//ָ��u.u_dbuf[]���׵�ַ
		while ('/' != curchar && '\0' != curchar && u.u_error == User::noerror)
		{
			if (pChar < &(u.u_dbuf[DirectoryEntry::DIRSIZ]))
			{
				// ����ǰ�ַ�������u.u_dbuf[]��
				*pChar = curchar;
				pChar++;
			}
			curchar = (*func)();  // ��ȡ��һ���ַ�
		}
		/* ��u_dbufʣ��Ĳ������Ϊ'\0' */
		while (pChar < &(u.u_dbuf[DirectoryEntry::DIRSIZ]))
		{
			*pChar = '\0';
			pChar++;
		}

		/* �������////a//b ����·�� ����·���ȼ���/a/b */
		while ('/' == curchar)
		{
			curchar = (*func)();  // ��ȡ��һ���ַ�,���Զ���
		}

		if (u.u_error != User::noerror)
		{
			break; /* goto out; */
		}
		

		/* �ڲ�ѭ�����ֶ���u.u_dbuf[]�е�·���������������Ѱƥ���Ŀ¼�� */
		u.u_IOParam.m_Offset = 0;
		/* ����ΪĿ¼����� �����հ׵�Ŀ¼��*/
		u.u_IOParam.m_Count = pInode->i_size / (DirectoryEntry::DIRSIZ + 4);
		freeEntryOffset = 0;  // �Դ����ļ�ģʽ����Ŀ¼ʱ����¼����Ŀ¼���ƫ����
		pBuf = NULL;  // ָ�򻺳���

		while (true)
		{
			/* ��Ŀ¼���Ѿ�������� */
			if (0 == u.u_IOParam.m_Count)
			{
				if (NULL != pBuf)
				{
					// �ͷŻ�����
					bufMgr.Brelse(pBuf);
				}
				/* ����Ǵ������ļ� */
				if (FileManager::CREATE == mode && curchar == '\0')
				{
					/* �жϸ�Ŀ¼�Ƿ��д */
					if (this->Access(pInode, Inode::IWRITE))
					{
						u.u_error = User::eacces;
						goto out;	/* Failed */
					}

					/* ����Ŀ¼Inodeָ�뱣���������Ժ�дĿ¼��WriteDir()�������õ� */
					u.u_pdir = pInode;

					if (freeEntryOffset)	/* �˱�������˿���Ŀ¼��λ��Ŀ¼�ļ��е�ƫ���� */
					{
						/* ������Ŀ¼��ƫ��������u���У�дĿ¼��WriteDir()���õ� */
						u.u_IOParam.m_Offset = freeEntryOffset - (DirectoryEntry::DIRSIZ + 4);
					}
					else  /* ���⣺Ϊ��if��֧û����IUPD��־��  ������Ϊ�ļ��ĳ���û�б�ѽ */
					{
						pInode->i_flag |= Inode::IUPD;	//����IUPD��־����ʾĿ¼�ļ����޸�
					}
					/* �ҵ�����д��Ŀ���Ŀ¼��λ�ã�NameI()�������� */
					return NULL;
				}

				/* Ŀ¼��������϶�û���ҵ�ƥ����ͷ����Inode��Դ�����˳� */
				u.u_error = User::enoent;
				goto out;
			}

			/* �Ѷ���Ŀ¼�ļ��ĵ�ǰ�̿飬��Ҫ������һĿ¼�������̿� */
			if (0 == u.u_IOParam.m_Offset % BLOCK_SIZE)
			{
				if (NULL != pBuf)
				{
					bufMgr.Brelse(pBuf);	//�ͷŻ�����
				}
				/* ����Ҫ���������̿�� */
				int phyBlkno = pInode->Bmap(u.u_IOParam.m_Offset / BLOCK_SIZE);
				pBuf = bufMgr.Bread(phyBlkno);	//��ȡĿ¼�������̿�
			}

			/* û�ж��굱ǰĿ¼���̿飬���ȡ��һĿ¼����u.u_dent */
			int* src = (int*)(pBuf->b_addr + (u.u_IOParam.m_Offset % BLOCK_SIZE));

			memcpy(&u.u_dent, src, sizeof(DirectoryEntry));

			// ����ƫ�����ͼ�����
			u.u_IOParam.m_Offset += (DirectoryEntry::DIRSIZ + 4);  // 4�ֽ���Ŀ¼���е�ino
			u.u_IOParam.m_Count--;  // Ŀ¼�������1

			/* ����ǿ���Ŀ¼���¼����λ��Ŀ¼�ļ���ƫ���� */
			if (0 == u.u_dent.m_ino)
			{
				if (0 == freeEntryOffset)
				{
					freeEntryOffset = u.u_IOParam.m_Offset;  // ����Ŀ¼���ƫ����
				}
				/* ��������Ŀ¼������Ƚ���һĿ¼�� */
				continue;
			}

			int i;
			bool yes = 0;
			for (i = 0; i < DirectoryEntry::DIRSIZ; i++)
			{
				if (u.u_dbuf[i] == '\0')
				{
					yes = 1;
					break;
				}
				// �Ƚ�pathname�е�ǰ·��������Ŀ¼���е��ļ����Ƿ���ͬ
				if (u.u_dbuf[i] != u.u_dent.m_name[i])
				{
					break;	/* ƥ����ĳһ�ַ�����������forѭ�� */
				}
			}

			if (!yes)
			{
				continue;
			}
			else
			{
				/* Ŀ¼��ƥ��ɹ����ص����While(true)ѭ�� */
				break;
			}
		}

		/*
		 * ���ڲ�Ŀ¼��ƥ��ѭ�������˴���˵��pathname��
		 * ��ǰ·������ƥ��ɹ��ˣ�����ƥ��pathname����һ·��
		 * ������ֱ������'\0'������
		 */
		if (NULL != pBuf)
		{
			//�ͷŻ�����
			bufMgr.Brelse(pBuf);
		}

		/* �����ɾ���������򷵻ظ�Ŀ¼Inode����Ҫɾ���ļ���Inode����u.u_dent.m_ino�� */
		if (FileManager::MYDELETE == mode && '\0' == curchar)
		{
			/* ����Ը�Ŀ¼û��д��Ȩ�� */
			if (this->Access(pInode, Inode::IWRITE))
			{
				u.u_error = User::eacces;//���ô�����Ϊû��Ȩ��
				break;	/* goto out; */
			}
			return pInode;	//���ظ�Ŀ¼Inode
		}

		/*
		 * ƥ��Ŀ¼��ɹ������ͷŵ�ǰĿ¼Inode������ƥ��ɹ���
		 * Ŀ¼��m_ino�ֶλ�ȡ��Ӧ��һ��Ŀ¼���ļ���Inode��
		 */
		this->m_InodeTable->IPut(pInode);  // �ͷŵ�ǰĿ¼Inode
		pInode = this->m_InodeTable->IGet(u.u_dent.m_ino);
		/* �ص����While(true)ѭ��������ƥ��Pathname����һ·������ */

		if (NULL == pInode)	/* ��ȡʧ�� */
		{
			return NULL;
		}
	}
	// �����While(true)ѭ�������˴���˵��ƥ��ʧ��
out:
	// �ͷŵ�ǰĿ¼Inode
	this->m_InodeTable->IPut(pInode);
	return NULL;
}

/*
 * ����ֵ��0����ʾӵ�д��ļ���Ȩ�ޣ�1��ʾû������ķ���Ȩ�ޡ��ļ�δ�ܴ򿪵�ԭ���¼��u.u_error�����С�
 */
int FileManager::Access(Inode* pInode, unsigned int mode)
{
	User& u = Kernel::Instance().GetUser();  // ��ȡ��ǰ���̵�user�ṹ
	/*
	 * ���ڳ����û�����д�κ��ļ����������
	 * ��Ҫִ��ĳ�ļ�ʱ��������i_mode�п�ִ�б�־
	 */
	if (u.u_uid == 0)
	{
		// �����û�
		if (Inode::IEXEC == mode && (pInode->i_mode & (Inode::IEXEC | (Inode::IEXEC >> 3) | (Inode::IEXEC >> 6))) == 0)
		{
			// �����û�ִ���ļ�ʱ��������i_mode�п�ִ�б�־
			u.u_error = User::eacces;
			return 1;
		}
		return 0;	/* Permission Check Succeed! */
	}
	if (u.u_uid != pInode->i_uid)
	{
		// ��������ļ������ߣ������ļ�������
		mode = mode >> 3;	// ����ļ�������
		if (u.u_gid != pInode->i_gid)
		{
			// ��������ļ������飬���������û�
			mode = mode >> 3;
		}
	}
	if ((pInode->i_mode & mode) != 0)
	{
		// ����ļ���i_mode��������ķ���Ȩ�ޣ��򷵻�0
		return 0;
	}

	// ����ļ���i_mode��û������ķ���Ȩ�ޣ��򷵻�1
	u.u_error = User::eacces;
	return 1;
}

/* �ı䵱ǰ����Ŀ¼ */
void FileManager::ChDir()
{
	Inode* pInode;
	User& u = Kernel::Instance().GetUser();

	// ���ļ�
	pInode = this->NameI(FileManager::NextChar, FileManager::OPEN);
	if (NULL == pInode)  // �ļ�������
	{
		return;
	}
	/* ���������ļ�����Ŀ¼�ļ� */
	if ((pInode->i_mode & Inode::IFMT) != Inode::IFDIR)
	{
		u.u_error = User::enotdir;
		this->m_InodeTable->IPut(pInode);  // �ͷ��ļ�Inode
		return;
	}
	// �ж�����ִ��Ȩ��
	if (this->Access(pInode, Inode::IEXEC))
	{
		// �ͷ��ļ�Inode
		this->m_InodeTable->IPut(pInode);
		return;
	}
	u.u_pdir = u.u_cdir;				// �����л���ĸ�Inode
	this->m_InodeTable->IPut(u.u_cdir);	// �ͷŵ�ǰĿ¼��Inode
	u.u_cdir = pInode;					// ���õ�ǰĿ¼��Inode
	pInode->Prele();					// �ͷŵ�ǰinode��
	// ���õ�ǰ����Ŀ¼
	this->SetCurDir((char*)u.u_arg[0] /* pathname */);
}

void FileManager::UnLink()
{
	Inode* pInode;
	Inode* pDeleteInode;
	User& u = Kernel::Instance().GetUser();
	// ���ļ�����ȡ�ļ�Inodeָ��
	pDeleteInode = this->NameI(FileManager::NextChar, FileManager::MYDELETE);
	if (NULL == pDeleteInode)  // �ļ�������
	{
		return;
	}
	if (Access(pDeleteInode, File::FWRITE))
	{
		cout << "ɾ��ʧ��,��ǰ�û�û��ɾ�����ļ���Ȩ��" << endl;
		return;
	}
	pDeleteInode->Prele();//�����ļ�Inode
	// ��ȡĿ¼�ļ�Inode
	pInode = this->m_InodeTable->IGet(u.u_dent.m_ino);
	if (NULL == pInode)
	{
		cout << "��������ɾ���ļ��Ĺ����У���ȡ�ļ���Inodeʧ���ˣ�" << endl;
	}

	/* д��������Ŀ¼�� */
	u.u_IOParam.m_Offset -= (DirectoryEntry::DIRSIZ + 4);	// ��������ƫ����
	u.u_IOParam.m_Base = (unsigned char*)&u.u_dent;			// �������û���ַ
	u.u_IOParam.m_Count = DirectoryEntry::DIRSIZ + 4;		// ���������ֽ���
	u.u_dent.m_ino = 0;										// ����Ŀ¼���е�inode��
	pDeleteInode->WriteI();  // д��Ŀ¼��

	/* �޸�inode�� */
	pInode->i_nlink=0;  // ��������1
	pInode->i_flag |= Inode::IUPD;  // �����ļ�Inode���޸ı�־

	this->m_InodeTable->IPut(pDeleteInode);  // �ͷ��ļ�Inode
	this->m_InodeTable->IPut(pInode);  // �ͷ�Ŀ¼�ļ�Inode
}

void FileManager::MkNod()
{
	Inode* pInode;  // ָ��Ҫ��������·��newPathname
	User& u = Kernel::Instance().GetUser();

	pInode = this->NameI(FileManager::NextChar, FileManager::CREATE);
	/* Ҫ�������ļ��Ѿ�����,���ﲢ����ȥ���Ǵ��ļ� */
	if (pInode != NULL)
	{
		cout << "Ŀ¼�Ѵ���!" << endl;
		this->m_InodeTable->IPut(pInode);//�ͷ��ļ�Inode
		return;
	}

	// ��ȡ�ļ�Inode
	pInode = this->MakNode(u.u_arg[1]);
	if (NULL == pInode)  // ��ȡʧ��
	{
		return;
	}
	u.u_ar0 = pInode->i_number;
	this->m_InodeTable->IPut(pInode);
}


/* ��ɶ�дϵͳ���ù������ִ��� */
void FileManager::Rdwr(File::FileFlags mode)
{
	File* pFile;
	User& u = Kernel::Instance().GetUser();

	/* ����Read()/Write()��ϵͳ���ò���fd��ȡ���ļ����ƿ�ṹ */
	pFile = u.u_ofiles.GetF(u.u_arg[0]);	/* fd */
	if (NULL == pFile)
	{
		/* �����ڸô��ļ���GetF�Ѿ����ù������룬�������ﲻ��Ҫ�������� */
		/*	u.u_error = User::EBADF;	*/
		u.u_ar0 = 0;
		return;
	}

	/* ��д��ģʽ����ȷ */
	if ((pFile->f_flag & mode) == 0)
	{
		cout << "�����󡿸��û�����ʹ�������Ȩ�ޣ�" << endl;
		u.u_error = User::eacces;	//���ô����룬�޷���Ȩ��
		return;
	}

	u.u_IOParam.m_Base = (unsigned char*)u.u_arg[1];	/* Ŀ�껺������ַ */
	
	u.u_IOParam.m_Count = u.u_arg[2];		/* Ҫ���/д���ֽ��� */
	//u.u_segflg = 0;		/* User Space I/O�����������Ҫ�����ݶλ��û�ջ�� */

	
	/* ��ͨ�ļ���д �����д�����ļ������ļ�ʵʩ������ʣ���������ȣ�ÿ��ϵͳ���á�
	Ϊ��Inode����Ҫ��������������NFlock()��NFrele()��
	�ⲻ��V6����ơ�read��writeϵͳ���ö��ڴ�i�ڵ�������Ϊ�˸�ʵʩIO�Ľ����ṩһ�µ��ļ���ͼ��*/
	
	/* �����ļ���ʼ��λ�� */
	u.u_IOParam.m_Offset = pFile->f_offset;
	if (File::FREAD == mode)
		pFile->f_inode->ReadI();
	else
		pFile->f_inode->WriteI();

	/* ���ݶ�д�������ƶ��ļ���дƫ��ָ�� */
	pFile->f_offset += (u.u_arg[2] - u.u_IOParam.m_Count);
	
	/* ����ʵ�ʶ�д���ֽ������޸Ĵ��ϵͳ���÷���ֵ�ĺ���ջ��Ԫ */
	u.u_ar0 = u.u_arg[2] - u.u_IOParam.m_Count;
}

char FileManager::NextChar()
{
	User& u = Kernel::Instance().GetUser();

	/* u.u_dirpָ��pathname�е��ַ� */
	return *u.u_dirp++;
}

/* ��creat���á�
 * Ϊ�´������ļ�д�µ�i�ڵ���µ�Ŀ¼��
 * ���ص�pInode�����������ڴ�i�ڵ㣬���е�i_count�� 1��
 *
 * �ڳ����������� WriteDir��������������Լ���Ŀ¼��д����Ŀ¼���޸ĸ�Ŀ¼�ļ���i�ڵ� ������д�ش��̡�
 *
 */
Inode* FileManager::MakNode(unsigned int mode)
{
	Inode* pInode;  // ���ڱ����´����ļ���Inode
	User& u = Kernel::Instance().GetUser();

	/* ����һ������DiskInode������������ȫ����� */
	pInode = this->m_FileSystem->IAlloc();

	if (NULL == pInode)
	{
		cout << "������MakNode�з���Inodeʧ�ܣ�" << endl;
		return NULL;
	}

	pInode->i_flag |= (Inode::IACC | Inode::IUPD);	// ����IACC��IUPD��־
	pInode->i_mode = mode | Inode::IALLOC;			// �����ļ����ͺ�IALLOC��־
	pInode->i_nlink = 1;		// ����Ӳ������Ϊ1
	pInode->i_uid = u.u_uid;	// �����ļ�������
	pInode->i_gid = u.u_gid;	// �����ļ�������

	/* ��Ŀ¼��д��u.u_dent�����д��Ŀ¼�ļ� */
	this->WriteDir(pInode);

	return pInode;	//�����´����ļ���Inode
}

/* ��Ŀ¼��Ŀ¼�ļ�д��һ��Ŀ¼�� */
void FileManager::WriteDir(Inode* pInode)
{
	User& u = Kernel::Instance().GetUser();

	/* ����Ŀ¼����Inode��Ų��� */
	u.u_dent.m_ino = pInode->i_number;

	/* ����Ŀ¼����pathname�������� */
	for (int i = 0; i < DirectoryEntry::DIRSIZ; i++)
	{
		// ��pathname����������Ŀ¼����
		u.u_dent.m_name[i] = u.u_dbuf[i];
	}

	u.u_IOParam.m_Count = DirectoryEntry::DIRSIZ + 4;  // ���ö�д�ֽ���, 4��Inode��ŵ��ֽ���
	u.u_IOParam.m_Base = (unsigned char*)&u.u_dent;  // ���ö�д������

	/* ��Ŀ¼��д�븸Ŀ¼�ļ� */
	u.u_pdir->WriteI();
	
	this->m_InodeTable->IPut(u.u_pdir);	 // �ͷŸ�Ŀ¼Inode
}

void FileManager::SetCurDir(char* pathname)
{
	User& u = Kernel::Instance().GetUser();

	/* ·�����ǴӸ�Ŀ¼'/'��ʼ����������u.u_curdir������ϵ�ǰ·������ */
	if (pathname[0] != '/')
	{
		if (strcmp((const char*)pathname, ".") && strcmp((const char*)pathname, ".."))
		{
			// ���㵱ǰ����Ŀ¼�ĳ���
			int length = Utility::StringLength(u.u_curdir);
			if (u.u_curdir[length - 1] != '/')
			{
				// �����ǰ����Ŀ¼������'/'��β�����ں������'/'
				u.u_curdir[length] = '/';
				length++;
			}
			// ����ǰ·������������u.u_curdir����
			Utility::StringCopy(pathname, u.u_curdir + length);
		}
		else if (!strcmp((const char*)pathname, ".."))  // cd ..�����
		{
			int length = Utility::StringLength(u.u_curdir);
			int cur = length - 1;
			while (cur >= 0)
			{
				if (cur == 0)
					break;
				if (u.u_curdir[cur] == '/')
				{
					u.u_curdir[cur] = '\0';
					break;
				}
				else
				{
					u.u_curdir[cur] = '\0';
					cur--;
				}
				
			}
		}
	}
	else	/* ����ǴӸ�Ŀ¼'/'��ʼ����ȡ��ԭ�й���Ŀ¼ */
	{
		Utility::StringCopy(pathname, u.u_curdir);
	}
}
