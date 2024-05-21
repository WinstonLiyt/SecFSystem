#include"User.h"
#include"Kernel.h"
#include"Utility.h"
#include<ctime>
#include"OpenFileManager.h"

FileSystem g_FileSystem;
SuperBlock g_SuperBlock;

/*构造函数*/
SuperBlock::SuperBlock()
{
	//nothing to do
}
/*析构函数*/
SuperBlock::~SuperBlock()
{
	//nothing to do
}


FileSystem::FileSystem()
{
	this->m_BufferManager = NULL;
	this->m_SuperBlock = NULL;
}

FileSystem::~FileSystem()
{
	//nothing to do here
}
void FileSystem::Initialize()
{
	this->m_BufferManager = &Kernel::Instance().GetBufferManager();
}

/* 从硬盘中读取超级块 */
void FileSystem::LoadSuperBlock()
{
	SuperBlock* sb = &g_SuperBlock;
	char* now_disk_cur;
	const char* disk_name = Kernel::Instance().GetDeviceDriver().GetDiskName();  // 获取磁盘名
	const char* shared_name = Kernel::Instance().GetDeviceDriver().GetSharedName();	 // 获取文件映射对象名称
	this->m_SuperBlock = &g_SuperBlock;
	MapView disk_mapview = Utility::get_mapview(disk_name, shared_name);
	now_disk_cur = disk_mapview.disk_base_address;
	map_read(now_disk_cur, this->m_SuperBlock, sizeof(SuperBlock));
	Utility::unmap_mapview(disk_mapview);  // 解除文件映射
}

/* 将内存SuperBlock更新到外存SuperBlock中 */
void FileSystem::Update()
{
	int i;  // 计数器
	SuperBlock* sb;  // 指向SuperBlock的指针
	Buf* pBuf;  // 缓存指针

	sb = this->m_SuperBlock;

	/* 清SuperBlock修改标志 */
	sb->s_fmod = 0;
	/* 写入SuperBlock最后存访时间 */
	sb->s_time =time(NULL);

	/*
	 * 为将要写回到磁盘上去的SuperBlock申请一块缓存，由于缓存块大小为512字节，
	 * SuperBlock大小为1024字节，占据2个连续的扇区，所以需要2次写入操作。
	 */
	for (int j = 0; j < 2; j++)
	{
		/* 第一次p指向SuperBlock的第0字节，第二次p指向第512字节 */
		int* p = (int*)sb + j * 128;

		/* 将要写入到设备dev上的SUPER_BLOCK_SECTOR_NUMBER + j扇区中去 */
		pBuf = this->m_BufferManager->GetBlk( FileSystem::SUPER_BLOCK_SECTOR_NUMBER + j);

		/* 将SuperBlock中第0 - 511字节写入缓存区 */
		//Utility::DWordCopy(p, (int*)pBuf->b_addr, 128);
		memcpy(pBuf->b_addr, p, 512);

		/* 将缓冲区中的数据写到磁盘上 */
		this->m_BufferManager->Bwrite(pBuf);
	}

	/* 同步修改过的内存Inode到对应外存Inode */
	g_InodeTable.UpdateInodeTable();

	/* 将延迟写的缓存块写到磁盘上 */
	this->m_BufferManager->Bflush();
}

/*
   * 在存储设备 dev 上分配一个空闲
   * 外存 INode，一般用于创建新的文件。
   */
Inode* FileSystem::IAlloc()
{
	SuperBlock* sb;	// 指向SuperBlock的指针
	Buf* pBuf;		// 指向缓冲区的指针
	Inode* pNode;	// 指向inode的指针
	User& u = Kernel::Instance().GetUser();
	int ino;	/* 分配到的空闲外存Inode编号 */

	/* 获取相应设备的SuperBlock内存副本 */
	sb = this->m_SuperBlock;

	/* 如果SuperBlock空闲Inode表被上锁，则睡眠等待至解锁 */
	while (sb->s_ilock)
	{
		cout << "【错误】SuperBlock空闲Inode表被上锁！" << endl;
		//u.u_procp->Sleep((unsigned long)&sb->s_ilock, ProcessManager::PINOD);
	}

	/*
	 * SuperBlock直接管理的空闲Inode索引表已空，
	 * 必须到磁盘上搜索空闲Inode。先对inode列表上锁，
	 * 因为在以下程序中会进行读盘操作可能会导致进程切换，
	 * 其他进程有可能访问该索引表，将会导致不一致性。
	 */
	if (sb->s_ninode <= 0)
	{
		/* 空闲Inode索引表上锁 */
		sb->s_ilock++;

		/* 外存Inode编号从0开始，这不同于Unix V6中外存Inode从1开始编号 */
		ino = -1;

		/* 依次读入磁盘Inode区中的磁盘块，搜索其中空闲外存Inode，记入空闲Inode索引表 */
		for (int i = 0; i < sb->s_isize; i++)
		{
			/* 读入磁盘inode区的第i个磁盘块 */
			pBuf = this->m_BufferManager->Bread(FileSystem::INODE_ZONE_START_SECTOR + i);

			/* 获取缓冲区首址 */
			int* p = (int*)pBuf->b_addr;

			/* 检查该缓冲区中每个外存Inode的i_mode != 0，表示已经被占用 */
			for (int j = 0; j < FileSystem::INODE_NUMBER_PER_SECTOR; j++)
			{
				ino++;	// 外存inode编号加1

				// 获取外存inode的i_mode
				int mode = *(p + j * sizeof(DiskInode) / sizeof(int));

				/* 该外存Inode已被占用，不能记入空闲Inode索引表 */
				if (mode != 0)
				{
					continue;
				}

				/*
				 * 如果外存inode的i_mode==0，此时并不能确定
				 * 该inode是空闲的，因为有可能是内存inode没有写到
				 * 磁盘上,所以要继续搜索内存inode中是否有相应的项
				 */
				if (g_InodeTable.IsLoaded(ino) == -1)
				{
					/* 该外存Inode没有对应的内存拷贝，将其记入空闲Inode索引表 */
					sb->s_inode[sb->s_ninode++] = ino;

					/* 如果空闲索引表已经装满，则不继续搜索 */
					if (sb->s_ninode >= 100)  // 空闲Inode数量
					{
						break;
					}
				}
			}

			/* 至此已读完当前磁盘块，释放相应的缓存 */
			this->m_BufferManager->Brelse(pBuf);

			/* 如果空闲索引表已经装满，则不继续搜索 */
			if (sb->s_ninode >= 100)
			{
				break;
			}
		}
		/* 解除对空闲外存Inode索引表的锁，唤醒因为等待锁而睡眠的进程 */
		sb->s_ilock = 0;
		// Kernel::Instance().GetProcessManager().WakeUpAll((unsigned long)&sb->s_ilock);

		/* 如果在磁盘上没有搜索到任何可用外存Inode，返回NULL */
		if (sb->s_ninode <= 0)
		{
			cout << "【错误】没有可用的外存Inode用来分配了！" << endl;
			u.u_error = User::enospc;	// 设置错误码，没有剩余可用的空闲inode
			return NULL;
		}
	}

	/*
	 * 上面部分已经保证，除非系统中没有可用外存Inode，
	 * 否则空闲Inode索引表中必定会记录可用外存Inode的编号。
	 */
	while (true)
	{
		/* 从索引表“栈顶”获取空闲外存Inode编号 */
		ino = sb->s_inode[--sb->s_ninode];  // 空闲inode栈

		/* 将空闲Inode读入内存 */
		pNode = g_InodeTable.IGet(ino);
		/* 未能分配到内存inode */
		if (NULL == pNode)
		{
			return NULL;
		}

		/* 如果该Inode空闲,清空Inode中的数据 */
		if (0 == pNode->i_mode)
		{
			pNode->Clean();
			/* 设置SuperBlock被修改标志 */
			sb->s_fmod = 1;
			return pNode;
		}
		else	/* 如果该Inode已被占用 */
		{
			g_InodeTable.IPut(pNode);
			continue;	/* while循环 */
		}
	}
	return NULL;	/* GCC likes it! */
}

void FileSystem::IFree(int number)
{
	SuperBlock* sb;  // SuperBlock内存副本

	sb = this->m_SuperBlock;	/* 获取相应设备的SuperBlock内存副本 */

	/*
	 * 如果超级块直接管理的空闲Inode表上锁，
	 * 则释放的外存Inode散落在磁盘Inode区中。
	 */
	if (sb->s_ilock)
	{
		return;
	}

	/*
	 * 如果超级块直接管理的空闲外存Inode超过100个，
	 * 同样让释放的外存Inode散落在磁盘Inode区中。
	 */
	if (sb->s_ninode >= 100)
	{
		return;
	}

	/* 将释放的外存inode编号记录到空闲inode索引表中 */
	sb->s_inode[sb->s_ninode++] = number;

	/* 设置SuperBlock被修改标志 */
	sb->s_fmod = 1;
}

Buf* FileSystem::Alloc()
{
	int blkno;	/* 分配到的空闲磁盘块编号 */
	SuperBlock* sb;
	Buf* pBuf;
	User& u = Kernel::Instance().GetUser();

	sb = this->m_SuperBlock;
	if (sb->s_flock)
	{
		cout << "【错误】发生异常，超级块上锁了！" << endl;
		exit(16);
	}
	/* 从索引表“栈顶”获取空闲磁盘块编号 */
	blkno = sb->s_free[--sb->s_nfree];
	
	/*
	 * 若获取磁盘块编号为零，则表示已分配尽所有的空闲磁盘块。
	 * 或者分配到的空闲磁盘块编号不属于数据盘块区域中(由BadBlock()检查)，
	 * 都意味着分配空闲磁盘块操作失败。
	 */
	if (0 == blkno)
	{
		sb->s_nfree = 0;	// 已经没有空闲磁盘块
		cout << "【错误】没有空闲块可以分配了！" << endl;
		u.u_error = User::enospc;
		return NULL;
	}
	/*
	 * 栈已空，新分配到空闲磁盘块中记录了下一组空闲磁盘块的编号,
	 * 将下一组空闲磁盘块的编号读入SuperBlock的空闲磁盘块索引表s_free[100]中。
	 */
	if (sb->s_nfree <= 0)
	{
		/*
		 * 此处加锁，因为以下要进行读盘操作，有可能发生进程切换，
		 * 新上台的进程可能对SuperBlock的空闲盘块索引表访问，会导致不一致性。
		 */
		sb->s_flock++;

		/* 读入该空闲磁盘块 */
		pBuf = this->m_BufferManager->Bread(blkno);

		/* 从该磁盘块的0字节开始记录，共占据4(s_nfree)+400(s_free[100])个字节 */
		int* p = (int*)pBuf->b_addr;

		/* 首先读出空闲盘块数s_nfree */
		sb->s_nfree = *p++;

		/* 读取缓存中后续位置的数据，写入到SuperBlock空闲盘块索引表s_free[100]中 */
		memcpy(sb->s_free, p, 400);

		/* 缓存使用完毕，释放以便被其它进程使用 */
		this->m_BufferManager->Brelse(pBuf);

		/* 解除对空闲磁盘块索引表的锁，唤醒因为等待锁而睡眠的进程 */
		sb->s_flock = 0;
	}
	/* 普通情况下成功分配到一空闲磁盘块 */
	pBuf = this->m_BufferManager->GetBlk(blkno);	/* 为该磁盘块申请缓存 */
	this->m_BufferManager->ClrBuf(pBuf);	/* 清空缓存中的数据 */
	sb->s_fmod = 1;	/* 设置SuperBlock被修改标志 */

	return pBuf;
}

/* 释放磁盘块blkno */
void FileSystem::Free(int blkno)
{
	SuperBlock* sb;
	Buf* pBuf;
	User& u = Kernel::Instance().GetUser();
	sb = this->m_SuperBlock;
	sb->s_fmod = 1;  // 设置修改标志
	if (sb->s_flock)
	{
		cout << "【错误】超级块被上锁！" << endl;
		exit(15);
	}
	/*
	 * 如果先前系统中已经没有空闲盘块，
	 * 现在释放的是系统中第1块空闲盘块
	 */
	//没问题，这就是实实在在的第一块，只要还有空闲块，s_nfree就不可能为0，因为在分配的时候
	//如果s_nfree为0，就立即补充上
	if (sb->s_nfree <= 0)
	{
		sb->s_nfree = 1;
		sb->s_free[0] = 0;	/* 使用0标记空闲盘块链结束标志 */
	}
	/* SuperBlock中直接管理空闲磁盘块号的栈已满 */
	if (sb->s_nfree >= 100)
	{
		/*
		 * 上锁，因为在下面要进行写盘操作，有可能发生进程切换。
		 * 如果不上锁，其他进程可能访问SuperBlock的空闲盘块表，会导致不一致性
		 */
		sb->s_flock++;

		/*
		 * 使用当前Free()函数正要释放的磁盘块，存放前一组100个空闲
		 * 磁盘块的索引表
		 */
		pBuf = this->m_BufferManager->GetBlk(blkno);	/* 为当前正要释放的磁盘块分配缓存 */

		/* 从该磁盘块的0字节开始记录，共占据4(s_nfree)+400(s_free[100])个字节 */
		int* p = (int*)pBuf->b_addr;

		/* 首先写入空闲盘块数，除了第一组为99块，后续每组都是100块 */
		*p++ = sb->s_nfree;

		/* 将SuperBlock的空闲盘块索引表s_free[100]写入缓存中后续位置 */
		memcpy(p, sb->s_free, 4 * 100);

		sb->s_nfree = 0;	//下面马上就加到1
		/* 将存放空闲盘块索引表的“当前释放盘块”写入磁盘，即实现了空闲盘块记录空闲盘块号的目标 */
		this->m_BufferManager->Bwrite(pBuf);
		// 解除上锁
		sb->s_flock = 0;
		// 唤醒因为该上锁而入睡的进程
		//Kernel::Instance().GetProcessManager().WakeUpAll((unsigned long)&sb->s_flock);
	}
	sb->s_free[sb->s_nfree++] = blkno;	/* SuperBlock中记录下当前释放盘块号 */
	sb->s_fmod = 1;  // 设置SuperBlock被修改标志
}

SuperBlock* FileSystem::GetSuperBlock()
{
	return this->m_SuperBlock ;
}
