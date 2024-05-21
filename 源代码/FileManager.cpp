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
	this->rootDirInode = this->m_InodeTable->IGet(1);	//1#Inode就是根目录
	this->rootDirInode->i_flag &= ~Inode::ILOCK;
	this->m_InodeTable->Initialize();
}

/*
 * 功能：打开文件
 * 效果：建立打开文件结构，内存i节点开锁 、i_count 为正数（i_count ++）
 * */
void FileManager::Open()
{
	Inode* pInode;
	User& u = Kernel::Instance().GetUser();	

	pInode = this->NameI(NextChar, FileManager::OPEN);	/* 0 = Open, not create */
	/* 没有找到相应的Inode */
	if (NULL == pInode)
	{
		u.u_ar0 = -1;  /* 找不到对应的文件fd为-1 */
		cout << "【错误】在文件打开的过程中找不到对应的pInode！" << endl;
		return;	
	}
	this->Open1(pInode, u.u_arg[1], 0);  /* ttf = 0，由Open调用。u_arg[1]用来传mode的值 */
}

/*
 * 功能：创建一个新的文件
 * 效果：建立打开文件结构，内存i节点开锁 、i_count 为正数（应该是 1）
 * */
void FileManager::Creat()
{
	Inode* pInode;
	User& u = Kernel::Instance().GetUser();

	/* 读、写、运行权限 */
	unsigned int newACCMode = u.u_arg[1] & (Inode::IRWXU | Inode::IRWXG | Inode::IRWXO);

	/* 搜索目录的模式为1，表示创建；若父目录不可写，出错返回 */
	pInode = this->NameI(NextChar, FileManager::CREATE);
	/* 没有找到相应的Inode，或NameI出错 */
	if (NULL == pInode)
	{
		if (u.u_error)
			return;
		/* 不存在要创建的目录，所以创建Inode */
		pInode = this->MakNode(newACCMode & (~Inode::ISVTX));
		/* 创建失败 */
		if (NULL == pInode)
		{
			return;
		}

		/*
		 * 如果所希望的名字不存在，使用参数trf = 2来调用open1()。
		 * 不需要进行权限检查，因为刚刚建立的文件的权限和传入参数mode
		 * 所表示的权限内容是一样的。
		 */
		this->Open1(pInode, File::FWRITE, 2);
	}
	else
	{
		/* 如果NameI()搜索到已经存在要创建的文件，则清空该文件（用算法ITrunc()）。UID没有改变
		 * 原来UNIX的设计是这样：文件看上去就像新建的文件一样。然而，新文件所有者和许可权方式没变。
		 * 也就是说creat指定的RWX比特无效。
		 * 邓蓉认为这是不合理的，应该改变。
		 * 现在的实现：creat指定的RWX比特有效 */
		this->Open1(pInode, File::FWRITE, 1);
		pInode->i_mode |= newACCMode;
	}
}


/*
* trf == 0由open调用
* trf == 1由creat调用，creat文件的时候搜索到同文件名的文件
* trf == 2由creat调用，creat文件的时候未搜索到同文件名的文件，这是文件创建时更一般的情况
* mode参数：打开文件模式，表示文件操作是 读、写还是读写
*/
void FileManager::Open1(Inode* pInode, int mode, int trf)
{
	User& u = Kernel::Instance().GetUser();

	/*
	 * 对所希望的文件已存在的情况下，即trf == 0或trf == 1进行权限检查
	 * 如果所希望的名字不存在，即trf == 2，不需要进行权限检查，因为刚建立
	 * 的文件的权限和传入的参数mode的所表示的权限内容是一样的。
	 */
	if (trf != 2)
	{
		/* trf不为2，需要检查文件类型 */
		if (mode & File::FREAD)
		{
			/* 检查读权限 */
			if (this->Access(pInode, Inode::IREAD))
			{
				cout << "【错误】当前用户没有读取该文件的权限！" << endl;
			}
		}
		if (mode & File::FWRITE)
		{
			/* 检查写权限 */
			this->Access(pInode, Inode::IWRITE);
		}
	}

	if (u.u_error)
	{
		this->m_InodeTable->IPut(pInode);  /* 有错，释放内存inode */
		return;
	}

	/* 在creat文件的时候搜索到同文件名的文件，释放该文件所占据的所有盘块 */
	if (1 == trf)
	{
		//清空文件内容
		pInode->ITrunc();
	}

	/* 解锁inode!
	 * 线性目录搜索涉及大量的磁盘读写操作，期间进程会入睡。
	 * 因此，进程必须上锁操作涉及的i节点。这就是NameI中执行的IGet上锁操作。
	 * 行至此，后续不再有可能会引起进程切换的操作，可以解锁i节点。
	 */
	pInode->Prele();

	/* 分配打开文件控制块File结构 */
	File* pFile = this->m_OpenFileTable->FAlloc();
	if (NULL == pFile)
	{
		this->m_InodeTable->IPut(pInode);  /* 释放内存inode */
		return;
	}
	/* 设置打开文件方式，建立File结构和内存Inode的勾连关系 */
	pFile->f_flag = mode & (File::FREAD | File::FWRITE);
	pFile->f_inode = pInode;  /* f_inode指向对应文件的inode */


	/* 为打开或者创建文件的各种资源都已成功分配，函数返回 */
	if (u.u_error == 0)
	{
		return;
	}
	else	/* 如果出错则释放资源，其实根本不该出错 */
	{
		cout << "异常出错:  " << u.u_error << endl;
		/* 释放打开文件描述符 */
		int fd = u.u_ar0;
		if (fd != -1)
		{
			u.u_ofiles.SetF(fd, NULL);  /* 释放打开文件描述符fd，递减File结构引用计数 */
			/* 递减File结构和Inode的引用计数，File结构没有锁 f_count为0就是释放File结构了*/
			pFile->f_count--;
		}
		this->m_InodeTable->IPut(pInode);
	}
}

/* Close()系统调用的处理过程 */
void FileManager::Close()
{
	User& u = Kernel::Instance().GetUser();
	int fd = u.u_arg[0];

	/* 获取打开文件控制块File结构 */
	File* pFile = u.u_ofiles.GetF(fd);
	if (NULL == pFile)
	{
		cout << "【错误】Close调用中File结构打开失败！" << endl;
		return;
	}

	/* 释放打开文件描述符fd，递减File结构引用计数 */
	u.u_ofiles.SetF(fd, NULL);
	/* 递减File结构和Inode的引用计数，File结构没有锁 f_count为0就是释放File结构了*/
	this->m_OpenFileTable->CloseF(pFile);
}

void FileManager::Seek()
{
	File* pFile;
	User& u = Kernel::Instance().GetUser();
	int fd = u.u_arg[0];  /* 获取文件描述符 */

	pFile = u.u_ofiles.GetF(fd);  /* 获取File结构 */
	if (NULL == pFile)
	{
		cout << "【错误】Seek中获取File结构失败！" << endl;
		return;  /* 若FILE不存在，GetF有设出错码 */
	}

	/* 管道文件不允许seek */
	if (pFile->f_flag & File::FPIPE)
	{
		u.u_error = User::espipe;
		return;
	}

	int offset = u.u_arg[1];

	/* 如果u.u_arg[2]在3 ~ 5之间，那么长度单位由字节变为512字节 */
	if (u.u_arg[2] > 2)
	{
		offset = offset << 9;  // 乘以512
		u.u_arg[2] -= 3;  // arg2减3
	}

	switch (u.u_arg[2])
	{
		/* 读写位置设置为offset */
		case 0:
			pFile->f_offset = offset;
			break;
			/* 读写位置加offset(可正可负) */
		case 1:
			pFile->f_offset += offset;
			break;
			/* 读写位置调整为文件长度加offset */
		case 2:
			pFile->f_offset = pFile->f_inode->i_size + offset;
			break;
	}
}

/* 完成获取文件信息处理过程 */
void FileManager::FStat()
{
	File* pFile;
	User& u = Kernel::Instance().GetUser();
	int fd = u.u_arg[0];  // 文件描述符

	pFile = u.u_ofiles.GetF(fd);  // 打开文件控制块File结构

	if (NULL == pFile)
	{
		cout << "【错误】在FStat中打开文件控制块File结构失败！" << endl;
		return;
	}

	/* u.u_arg[1] = pStatBuf */
	this->Stat1(pFile->f_inode, u.u_arg[1]);  // 文件信息
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

	this->Stat1(pInode, u.u_arg[1]);	/* 获取文件信息 */
	this->m_InodeTable->IPut(pInode);	/* 释放inode */
}

void FileManager::Stat1(Inode* pInode, unsigned long statBuf)
{
	Buf* pBuf;  // 缓冲区指针
	BufferManager& bufMgr = Kernel::Instance().GetBufferManager();

	pInode->IUpdate(time(NULL));  // 更新Inode的时间戳
	pBuf = bufMgr.Bread(FileSystem::INODE_ZONE_START_SECTOR + pInode->i_number / FileSystem::INODE_NUMBER_PER_SECTOR);  // 读取外存Inode所在的扇区

	/* 将p指向缓存区中编号为inumber外存Inode的偏移位置 */
	unsigned char* p = pBuf->b_addr + (pInode->i_number % FileSystem::INODE_NUMBER_PER_SECTOR) * sizeof(DiskInode);
	/* 复制Inode信息到用户空间 */
	Utility::DWordCopy((int*)p, (int*)statBuf, sizeof(DiskInode) / sizeof(int));

	bufMgr.Brelse(pBuf);  // 释放缓冲区
}

void FileManager::Read()
{
	/* 直接调用Rdwr()函数即可 */
	this->Rdwr(File::FREAD);
}

void FileManager::Write()
{
	/* 直接调用Rdwr()函数即可 */
	this->Rdwr(File::FWRITE);
}

/* 返回NULL表示目录搜索失败，否则是根指针，指向文件的内存打开i节点 ，上锁的内存i节点  */
Inode* FileManager::NameI(char(*func)(), DirectorySearchMode mode)
{
	Inode* pInode;	/* 指向当前搜索的目录的Inode */
	Buf* pBuf;		/* 指向当前搜索的目录的缓冲区 */
	char curchar;	/* 当前字符 */
	char* pChar;	/* 指向当前字符的指针 */
	int freeEntryOffset;	/* 以创建文件模式搜索目录时，记录空闲目录项的偏移量 */
	User& u = Kernel::Instance().GetUser();
	BufferManager& bufMgr = Kernel::Instance().GetBufferManager();

	/*
	 * 如果该路径是'/'开头的，从根目录开始搜索，
	 * 否则从进程当前工作目录开始搜索。
	 */
	pInode = u.u_cdir;  // 指向当前进程的当前工作目录
	if ('/' == (curchar = (*func)()))
	{
		pInode = this->rootDirInode;  // 根目录
	}

	/* 检查该Inode是否正在被使用，以及保证在整个目录搜索过程中该Inode不被释放 */
	this->m_InodeTable->IGet(pInode->i_number);

	/* 允许出现////a//b 这种路径 这种路径等价于/a/b */
	while ('/' == curchar)
	{
		curchar = (*func)();	//不停读取下一个字符
	}
	/* 如果试图更改和删除当前目录文件则出错 */
	if ('\0' == curchar && mode != FileManager::OPEN)
	{
		u.u_error = User::enoent;
		goto out;  // 释放当前搜索到的目录文件Inode，并退出
	}

	/* 外层循环每次处理pathname中一段路径分量 */
	while (true)
	{
		/* 如果出错则释放当前搜索到的目录文件Inode，并退出 */
		if (u.u_error != User::noerror)
		{
			break;	/* goto out; */
		}
		
		/* 整个路径搜索完毕，返回相应Inode指针。目录搜索成功返回。 */
		if ('\0' == curchar)
		{
			return pInode;	// 返回上锁后的Inode
		}

		/* 如果要进行搜索的不是目录文件，释放相关Inode资源则退出 */
		if ((pInode->i_mode & Inode::IFMT) != Inode::IFDIR)
		{
			break;	/* goto out; */
		}

		/* 进行目录搜索权限检查，IEXEC在目录文件中表示搜索权限 */
		if (this->Access(pInode, Inode::IEXEC))
		{
			u.u_error = User::eacces;
			break;	/* 不具备目录搜索权限，goto out; */
		}

		/*
		 * 将Pathname中当前准备进行匹配的路径分量拷贝到u.u_dbuf[]中，
		 * 便于和目录项进行比较。
		 */
		pChar = &(u.u_dbuf[0]);	//指向u.u_dbuf[]的首地址
		while ('/' != curchar && '\0' != curchar && u.u_error == User::noerror)
		{
			if (pChar < &(u.u_dbuf[DirectoryEntry::DIRSIZ]))
			{
				// 将当前字符拷贝到u.u_dbuf[]中
				*pChar = curchar;
				pChar++;
			}
			curchar = (*func)();  // 获取下一个字符
		}
		/* 将u_dbuf剩余的部分填充为'\0' */
		while (pChar < &(u.u_dbuf[DirectoryEntry::DIRSIZ]))
		{
			*pChar = '\0';
			pChar++;
		}

		/* 允许出现////a//b 这种路径 这种路径等价于/a/b */
		while ('/' == curchar)
		{
			curchar = (*func)();  // 获取下一个字符,忽略多余
		}

		if (u.u_error != User::noerror)
		{
			break; /* goto out; */
		}
		

		/* 内层循环部分对于u.u_dbuf[]中的路径名分量，逐个搜寻匹配的目录项 */
		u.u_IOParam.m_Offset = 0;
		/* 设置为目录项个数 ，含空白的目录项*/
		u.u_IOParam.m_Count = pInode->i_size / (DirectoryEntry::DIRSIZ + 4);
		freeEntryOffset = 0;  // 以创建文件模式搜索目录时，记录空闲目录项的偏移量
		pBuf = NULL;  // 指向缓冲区

		while (true)
		{
			/* 对目录项已经搜索完毕 */
			if (0 == u.u_IOParam.m_Count)
			{
				if (NULL != pBuf)
				{
					// 释放缓冲区
					bufMgr.Brelse(pBuf);
				}
				/* 如果是创建新文件 */
				if (FileManager::CREATE == mode && curchar == '\0')
				{
					/* 判断该目录是否可写 */
					if (this->Access(pInode, Inode::IWRITE))
					{
						u.u_error = User::eacces;
						goto out;	/* Failed */
					}

					/* 将父目录Inode指针保存起来，以后写目录项WriteDir()函数会用到 */
					u.u_pdir = pInode;

					if (freeEntryOffset)	/* 此变量存放了空闲目录项位于目录文件中的偏移量 */
					{
						/* 将空闲目录项偏移量存入u区中，写目录项WriteDir()会用到 */
						u.u_IOParam.m_Offset = freeEntryOffset - (DirectoryEntry::DIRSIZ + 4);
					}
					else  /* 问题：为何if分支没有置IUPD标志？  这是因为文件的长度没有变呀 */
					{
						pInode->i_flag |= Inode::IUPD;	//设置IUPD标志，表示目录文件已修改
					}
					/* 找到可以写入的空闲目录项位置，NameI()函数返回 */
					return NULL;
				}

				/* 目录项搜索完毕而没有找到匹配项，释放相关Inode资源，并退出 */
				u.u_error = User::enoent;
				goto out;
			}

			/* 已读完目录文件的当前盘块，需要读入下一目录项数据盘块 */
			if (0 == u.u_IOParam.m_Offset % BLOCK_SIZE)
			{
				if (NULL != pBuf)
				{
					bufMgr.Brelse(pBuf);	//释放缓冲区
				}
				/* 计算要读的物理盘块号 */
				int phyBlkno = pInode->Bmap(u.u_IOParam.m_Offset / BLOCK_SIZE);
				pBuf = bufMgr.Bread(phyBlkno);	//读取目录项数据盘块
			}

			/* 没有读完当前目录项盘块，则读取下一目录项至u.u_dent */
			int* src = (int*)(pBuf->b_addr + (u.u_IOParam.m_Offset % BLOCK_SIZE));

			memcpy(&u.u_dent, src, sizeof(DirectoryEntry));

			// 更新偏移量和计数器
			u.u_IOParam.m_Offset += (DirectoryEntry::DIRSIZ + 4);  // 4字节是目录项中的ino
			u.u_IOParam.m_Count--;  // 目录项个数减1

			/* 如果是空闲目录项，记录该项位于目录文件中偏移量 */
			if (0 == u.u_dent.m_ino)
			{
				if (0 == freeEntryOffset)
				{
					freeEntryOffset = u.u_IOParam.m_Offset;  // 空闲目录项的偏移量
				}
				/* 跳过空闲目录项，继续比较下一目录项 */
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
				// 比较pathname中当前路径分量和目录项中的文件名是否相同
				if (u.u_dbuf[i] != u.u_dent.m_name[i])
				{
					break;	/* 匹配至某一字符不符，跳出for循环 */
				}
			}

			if (!yes)
			{
				continue;
			}
			else
			{
				/* 目录项匹配成功，回到外层While(true)循环 */
				break;
			}
		}

		/*
		 * 从内层目录项匹配循环跳至此处，说明pathname中
		 * 当前路径分量匹配成功了，还需匹配pathname中下一路径
		 * 分量，直至遇到'\0'结束。
		 */
		if (NULL != pBuf)
		{
			//释放缓冲区
			bufMgr.Brelse(pBuf);
		}

		/* 如果是删除操作，则返回父目录Inode，而要删除文件的Inode号在u.u_dent.m_ino中 */
		if (FileManager::MYDELETE == mode && '\0' == curchar)
		{
			/* 如果对父目录没有写的权限 */
			if (this->Access(pInode, Inode::IWRITE))
			{
				u.u_error = User::eacces;//设置错误码为没有权限
				break;	/* goto out; */
			}
			return pInode;	//返回父目录Inode
		}

		/*
		 * 匹配目录项成功，则释放当前目录Inode，根据匹配成功的
		 * 目录项m_ino字段获取相应下一级目录或文件的Inode。
		 */
		this->m_InodeTable->IPut(pInode);  // 释放当前目录Inode
		pInode = this->m_InodeTable->IGet(u.u_dent.m_ino);
		/* 回到外层While(true)循环，继续匹配Pathname中下一路径分量 */

		if (NULL == pInode)	/* 获取失败 */
		{
			return NULL;
		}
	}
	// 从外层While(true)循环跳至此处，说明匹配失败
out:
	// 释放当前目录Inode
	this->m_InodeTable->IPut(pInode);
	return NULL;
}

/*
 * 返回值是0，表示拥有打开文件的权限；1表示没有所需的访问权限。文件未能打开的原因记录在u.u_error变量中。
 */
int FileManager::Access(Inode* pInode, unsigned int mode)
{
	User& u = Kernel::Instance().GetUser();  // 获取当前进程的user结构
	/*
	 * 对于超级用户，读写任何文件都是允许的
	 * 而要执行某文件时，必须在i_mode有可执行标志
	 */
	if (u.u_uid == 0)
	{
		// 超级用户
		if (Inode::IEXEC == mode && (pInode->i_mode & (Inode::IEXEC | (Inode::IEXEC >> 3) | (Inode::IEXEC >> 6))) == 0)
		{
			// 超级用户执行文件时，必须在i_mode有可执行标志
			u.u_error = User::eacces;
			return 1;
		}
		return 0;	/* Permission Check Succeed! */
	}
	if (u.u_uid != pInode->i_uid)
	{
		// 如果不是文件所有者，则检查文件所属组
		mode = mode >> 3;	// 检查文件所属组
		if (u.u_gid != pInode->i_gid)
		{
			// 如果不是文件所属组，则检查其他用户
			mode = mode >> 3;
		}
	}
	if ((pInode->i_mode & mode) != 0)
	{
		// 如果文件的i_mode中有所需的访问权限，则返回0
		return 0;
	}

	// 如果文件的i_mode中没有所需的访问权限，则返回1
	u.u_error = User::eacces;
	return 1;
}

/* 改变当前工作目录 */
void FileManager::ChDir()
{
	Inode* pInode;
	User& u = Kernel::Instance().GetUser();

	// 打开文件
	pInode = this->NameI(FileManager::NextChar, FileManager::OPEN);
	if (NULL == pInode)  // 文件不存在
	{
		return;
	}
	/* 搜索到的文件不是目录文件 */
	if ((pInode->i_mode & Inode::IFMT) != Inode::IFDIR)
	{
		u.u_error = User::enotdir;
		this->m_InodeTable->IPut(pInode);  // 释放文件Inode
		return;
	}
	// 判断有无执行权限
	if (this->Access(pInode, Inode::IEXEC))
	{
		// 释放文件Inode
		this->m_InodeTable->IPut(pInode);
		return;
	}
	u.u_pdir = u.u_cdir;				// 设置切换后的父Inode
	this->m_InodeTable->IPut(u.u_cdir);	// 释放当前目录的Inode
	u.u_cdir = pInode;					// 设置当前目录的Inode
	pInode->Prele();					// 释放当前inode锁
	// 设置当前工作目录
	this->SetCurDir((char*)u.u_arg[0] /* pathname */);
}

void FileManager::UnLink()
{
	Inode* pInode;
	Inode* pDeleteInode;
	User& u = Kernel::Instance().GetUser();
	// 打开文件，获取文件Inode指针
	pDeleteInode = this->NameI(FileManager::NextChar, FileManager::MYDELETE);
	if (NULL == pDeleteInode)  // 文件不存在
	{
		return;
	}
	if (Access(pDeleteInode, File::FWRITE))
	{
		cout << "删除失败,当前用户没有删除该文件的权限" << endl;
		return;
	}
	pDeleteInode->Prele();//解锁文件Inode
	// 获取目录文件Inode
	pInode = this->m_InodeTable->IGet(u.u_dent.m_ino);
	if (NULL == pInode)
	{
		cout << "【错误】在删除文件的过程中，获取文件的Inode失败了！" << endl;
	}

	/* 写入清零后的目录项 */
	u.u_IOParam.m_Offset -= (DirectoryEntry::DIRSIZ + 4);	// 重新设置偏移量
	u.u_IOParam.m_Base = (unsigned char*)&u.u_dent;			// 重新设置基地址
	u.u_IOParam.m_Count = DirectoryEntry::DIRSIZ + 4;		// 重新设置字节数
	u.u_dent.m_ino = 0;										// 清零目录项中的inode号
	pDeleteInode->WriteI();  // 写入目录项

	/* 修改inode项 */
	pInode->i_nlink=0;  // 链接数减1
	pInode->i_flag |= Inode::IUPD;  // 设置文件Inode已修改标志

	this->m_InodeTable->IPut(pDeleteInode);  // 释放文件Inode
	this->m_InodeTable->IPut(pInode);  // 释放目录文件Inode
}

void FileManager::MkNod()
{
	Inode* pInode;  // 指向要创建的新路径newPathname
	User& u = Kernel::Instance().GetUser();

	pInode = this->NameI(FileManager::NextChar, FileManager::CREATE);
	/* 要创建的文件已经存在,这里并不能去覆盖此文件 */
	if (pInode != NULL)
	{
		cout << "目录已存在!" << endl;
		this->m_InodeTable->IPut(pInode);//释放文件Inode
		return;
	}

	// 获取文件Inode
	pInode = this->MakNode(u.u_arg[1]);
	if (NULL == pInode)  // 获取失败
	{
		return;
	}
	u.u_ar0 = pInode->i_number;
	this->m_InodeTable->IPut(pInode);
}


/* 完成读写系统调用公共部分代码 */
void FileManager::Rdwr(File::FileFlags mode)
{
	File* pFile;
	User& u = Kernel::Instance().GetUser();

	/* 根据Read()/Write()的系统调用参数fd获取打开文件控制块结构 */
	pFile = u.u_ofiles.GetF(u.u_arg[0]);	/* fd */
	if (NULL == pFile)
	{
		/* 不存在该打开文件，GetF已经设置过出错码，所以这里不需要再设置了 */
		/*	u.u_error = User::EBADF;	*/
		u.u_ar0 = 0;
		return;
	}

	/* 读写的模式不正确 */
	if ((pFile->f_flag & mode) == 0)
	{
		cout << "【错误】该用户无行使此类操作权限！" << endl;
		u.u_error = User::eacces;	//设置错误码，无访问权限
		return;
	}

	u.u_IOParam.m_Base = (unsigned char*)u.u_arg[1];	/* 目标缓冲区首址 */
	
	u.u_IOParam.m_Count = u.u_arg[2];		/* 要求读/写的字节数 */
	//u.u_segflg = 0;		/* User Space I/O，读入的内容要送数据段或用户栈段 */

	
	/* 普通文件读写 ，或读写特殊文件。对文件实施互斥访问，互斥的粒度：每次系统调用。
	为此Inode类需要增加两个方法：NFlock()、NFrele()。
	这不是V6的设计。read、write系统调用对内存i节点上锁是为了给实施IO的进程提供一致的文件视图。*/
	
	/* 设置文件起始读位置 */
	u.u_IOParam.m_Offset = pFile->f_offset;
	if (File::FREAD == mode)
		pFile->f_inode->ReadI();
	else
		pFile->f_inode->WriteI();

	/* 根据读写字数，移动文件读写偏移指针 */
	pFile->f_offset += (u.u_arg[2] - u.u_IOParam.m_Count);
	
	/* 返回实际读写的字节数，修改存放系统调用返回值的核心栈单元 */
	u.u_ar0 = u.u_arg[2] - u.u_IOParam.m_Count;
}

char FileManager::NextChar()
{
	User& u = Kernel::Instance().GetUser();

	/* u.u_dirp指向pathname中的字符 */
	return *u.u_dirp++;
}

/* 由creat调用。
 * 为新创建的文件写新的i节点和新的目录项
 * 返回的pInode是上了锁的内存i节点，其中的i_count是 1。
 *
 * 在程序的最后会调用 WriteDir，在这里把属于自己的目录项写进父目录，修改父目录文件的i节点 、将其写回磁盘。
 *
 */
Inode* FileManager::MakNode(unsigned int mode)
{
	Inode* pInode;  // 用于保存新创建文件的Inode
	User& u = Kernel::Instance().GetUser();

	/* 分配一个空闲DiskInode，里面内容已全部清空 */
	pInode = this->m_FileSystem->IAlloc();

	if (NULL == pInode)
	{
		cout << "【错误】MakNode中分配Inode失败！" << endl;
		return NULL;
	}

	pInode->i_flag |= (Inode::IACC | Inode::IUPD);	// 设置IACC和IUPD标志
	pInode->i_mode = mode | Inode::IALLOC;			// 设置文件类型和IALLOC标志
	pInode->i_nlink = 1;		// 设置硬链接数为1
	pInode->i_uid = u.u_uid;	// 设置文件所有者
	pInode->i_gid = u.u_gid;	// 设置文件所属组

	/* 将目录项写入u.u_dent，随后写入目录文件 */
	this->WriteDir(pInode);

	return pInode;	//返回新创建文件的Inode
}

/* 向父目录的目录文件写入一个目录项 */
void FileManager::WriteDir(Inode* pInode)
{
	User& u = Kernel::Instance().GetUser();

	/* 设置目录项中Inode编号部分 */
	u.u_dent.m_ino = pInode->i_number;

	/* 设置目录项中pathname分量部分 */
	for (int i = 0; i < DirectoryEntry::DIRSIZ; i++)
	{
		// 将pathname分量拷贝到目录项中
		u.u_dent.m_name[i] = u.u_dbuf[i];
	}

	u.u_IOParam.m_Count = DirectoryEntry::DIRSIZ + 4;  // 设置读写字节数, 4是Inode编号的字节数
	u.u_IOParam.m_Base = (unsigned char*)&u.u_dent;  // 设置读写缓冲区

	/* 将目录项写入父目录文件 */
	u.u_pdir->WriteI();
	
	this->m_InodeTable->IPut(u.u_pdir);	 // 释放父目录Inode
}

void FileManager::SetCurDir(char* pathname)
{
	User& u = Kernel::Instance().GetUser();

	/* 路径不是从根目录'/'开始，则在现有u.u_curdir后面加上当前路径分量 */
	if (pathname[0] != '/')
	{
		if (strcmp((const char*)pathname, ".") && strcmp((const char*)pathname, ".."))
		{
			// 计算当前工作目录的长度
			int length = Utility::StringLength(u.u_curdir);
			if (u.u_curdir[length - 1] != '/')
			{
				// 如果当前工作目录不是以'/'结尾，则在后面加上'/'
				u.u_curdir[length] = '/';
				length++;
			}
			// 将当前路径分量拷贝到u.u_curdir后面
			Utility::StringCopy(pathname, u.u_curdir + length);
		}
		else if (!strcmp((const char*)pathname, ".."))  // cd ..的情况
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
	else	/* 如果是从根目录'/'开始，则取代原有工作目录 */
	{
		Utility::StringCopy(pathname, u.u_curdir);
	}
}
