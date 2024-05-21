#include"User.h"
#include"Kernel.h"
#include"Utility.h"
#include<ctime>
#include"OpenFileManager.h"

FileSystem g_FileSystem;
SuperBlock g_SuperBlock;

/*���캯��*/
SuperBlock::SuperBlock()
{
	//nothing to do
}
/*��������*/
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

/* ��Ӳ���ж�ȡ������ */
void FileSystem::LoadSuperBlock()
{
	SuperBlock* sb = &g_SuperBlock;
	char* now_disk_cur;
	const char* disk_name = Kernel::Instance().GetDeviceDriver().GetDiskName();  // ��ȡ������
	const char* shared_name = Kernel::Instance().GetDeviceDriver().GetSharedName();	 // ��ȡ�ļ�ӳ���������
	this->m_SuperBlock = &g_SuperBlock;
	MapView disk_mapview = Utility::get_mapview(disk_name, shared_name);
	now_disk_cur = disk_mapview.disk_base_address;
	map_read(now_disk_cur, this->m_SuperBlock, sizeof(SuperBlock));
	Utility::unmap_mapview(disk_mapview);  // ����ļ�ӳ��
}

/* ���ڴ�SuperBlock���µ����SuperBlock�� */
void FileSystem::Update()
{
	int i;  // ������
	SuperBlock* sb;  // ָ��SuperBlock��ָ��
	Buf* pBuf;  // ����ָ��

	sb = this->m_SuperBlock;

	/* ��SuperBlock�޸ı�־ */
	sb->s_fmod = 0;
	/* д��SuperBlock�����ʱ�� */
	sb->s_time =time(NULL);

	/*
	 * Ϊ��Ҫд�ص�������ȥ��SuperBlock����һ�黺�棬���ڻ�����СΪ512�ֽڣ�
	 * SuperBlock��СΪ1024�ֽڣ�ռ��2��������������������Ҫ2��д�������
	 */
	for (int j = 0; j < 2; j++)
	{
		/* ��һ��pָ��SuperBlock�ĵ�0�ֽڣ��ڶ���pָ���512�ֽ� */
		int* p = (int*)sb + j * 128;

		/* ��Ҫд�뵽�豸dev�ϵ�SUPER_BLOCK_SECTOR_NUMBER + j������ȥ */
		pBuf = this->m_BufferManager->GetBlk( FileSystem::SUPER_BLOCK_SECTOR_NUMBER + j);

		/* ��SuperBlock�е�0 - 511�ֽ�д�뻺���� */
		//Utility::DWordCopy(p, (int*)pBuf->b_addr, 128);
		memcpy(pBuf->b_addr, p, 512);

		/* ���������е�����д�������� */
		this->m_BufferManager->Bwrite(pBuf);
	}

	/* ͬ���޸Ĺ����ڴ�Inode����Ӧ���Inode */
	g_InodeTable.UpdateInodeTable();

	/* ���ӳ�д�Ļ����д�������� */
	this->m_BufferManager->Bflush();
}

/*
   * �ڴ洢�豸 dev �Ϸ���һ������
   * ��� INode��һ�����ڴ����µ��ļ���
   */
Inode* FileSystem::IAlloc()
{
	SuperBlock* sb;	// ָ��SuperBlock��ָ��
	Buf* pBuf;		// ָ�򻺳�����ָ��
	Inode* pNode;	// ָ��inode��ָ��
	User& u = Kernel::Instance().GetUser();
	int ino;	/* ���䵽�Ŀ������Inode��� */

	/* ��ȡ��Ӧ�豸��SuperBlock�ڴ渱�� */
	sb = this->m_SuperBlock;

	/* ���SuperBlock����Inode����������˯�ߵȴ������� */
	while (sb->s_ilock)
	{
		cout << "������SuperBlock����Inode��������" << endl;
		//u.u_procp->Sleep((unsigned long)&sb->s_ilock, ProcessManager::PINOD);
	}

	/*
	 * SuperBlockֱ�ӹ���Ŀ���Inode�������ѿգ�
	 * ���뵽��������������Inode���ȶ�inode�б�������
	 * ��Ϊ�����³����л���ж��̲������ܻᵼ�½����л���
	 * ���������п��ܷ��ʸ����������ᵼ�²�һ���ԡ�
	 */
	if (sb->s_ninode <= 0)
	{
		/* ����Inode���������� */
		sb->s_ilock++;

		/* ���Inode��Ŵ�0��ʼ���ⲻͬ��Unix V6�����Inode��1��ʼ��� */
		ino = -1;

		/* ���ζ������Inode���еĴ��̿飬�������п������Inode���������Inode������ */
		for (int i = 0; i < sb->s_isize; i++)
		{
			/* �������inode���ĵ�i�����̿� */
			pBuf = this->m_BufferManager->Bread(FileSystem::INODE_ZONE_START_SECTOR + i);

			/* ��ȡ��������ַ */
			int* p = (int*)pBuf->b_addr;

			/* ���û�������ÿ�����Inode��i_mode != 0����ʾ�Ѿ���ռ�� */
			for (int j = 0; j < FileSystem::INODE_NUMBER_PER_SECTOR; j++)
			{
				ino++;	// ���inode��ż�1

				// ��ȡ���inode��i_mode
				int mode = *(p + j * sizeof(DiskInode) / sizeof(int));

				/* �����Inode�ѱ�ռ�ã����ܼ������Inode������ */
				if (mode != 0)
				{
					continue;
				}

				/*
				 * ������inode��i_mode==0����ʱ������ȷ��
				 * ��inode�ǿ��еģ���Ϊ�п������ڴ�inodeû��д��
				 * ������,����Ҫ���������ڴ�inode���Ƿ�����Ӧ����
				 */
				if (g_InodeTable.IsLoaded(ino) == -1)
				{
					/* �����Inodeû�ж�Ӧ���ڴ濽��������������Inode������ */
					sb->s_inode[sb->s_ninode++] = ino;

					/* ��������������Ѿ�װ�����򲻼������� */
					if (sb->s_ninode >= 100)  // ����Inode����
					{
						break;
					}
				}
			}

			/* �����Ѷ��굱ǰ���̿飬�ͷ���Ӧ�Ļ��� */
			this->m_BufferManager->Brelse(pBuf);

			/* ��������������Ѿ�װ�����򲻼������� */
			if (sb->s_ninode >= 100)
			{
				break;
			}
		}
		/* ����Կ������Inode�����������������Ϊ�ȴ�����˯�ߵĽ��� */
		sb->s_ilock = 0;
		// Kernel::Instance().GetProcessManager().WakeUpAll((unsigned long)&sb->s_ilock);

		/* ����ڴ�����û���������κο������Inode������NULL */
		if (sb->s_ninode <= 0)
		{
			cout << "������û�п��õ����Inode���������ˣ�" << endl;
			u.u_error = User::enospc;	// ���ô����룬û��ʣ����õĿ���inode
			return NULL;
		}
	}

	/*
	 * ���沿���Ѿ���֤������ϵͳ��û�п������Inode��
	 * �������Inode�������бض����¼�������Inode�ı�š�
	 */
	while (true)
	{
		/* ��������ջ������ȡ�������Inode��� */
		ino = sb->s_inode[--sb->s_ninode];  // ����inodeջ

		/* ������Inode�����ڴ� */
		pNode = g_InodeTable.IGet(ino);
		/* δ�ܷ��䵽�ڴ�inode */
		if (NULL == pNode)
		{
			return NULL;
		}

		/* �����Inode����,���Inode�е����� */
		if (0 == pNode->i_mode)
		{
			pNode->Clean();
			/* ����SuperBlock���޸ı�־ */
			sb->s_fmod = 1;
			return pNode;
		}
		else	/* �����Inode�ѱ�ռ�� */
		{
			g_InodeTable.IPut(pNode);
			continue;	/* whileѭ�� */
		}
	}
	return NULL;	/* GCC likes it! */
}

void FileSystem::IFree(int number)
{
	SuperBlock* sb;  // SuperBlock�ڴ渱��

	sb = this->m_SuperBlock;	/* ��ȡ��Ӧ�豸��SuperBlock�ڴ渱�� */

	/*
	 * ���������ֱ�ӹ���Ŀ���Inode��������
	 * ���ͷŵ����Inodeɢ���ڴ���Inode���С�
	 */
	if (sb->s_ilock)
	{
		return;
	}

	/*
	 * ���������ֱ�ӹ���Ŀ������Inode����100����
	 * ͬ�����ͷŵ����Inodeɢ���ڴ���Inode���С�
	 */
	if (sb->s_ninode >= 100)
	{
		return;
	}

	/* ���ͷŵ����inode��ż�¼������inode�������� */
	sb->s_inode[sb->s_ninode++] = number;

	/* ����SuperBlock���޸ı�־ */
	sb->s_fmod = 1;
}

Buf* FileSystem::Alloc()
{
	int blkno;	/* ���䵽�Ŀ��д��̿��� */
	SuperBlock* sb;
	Buf* pBuf;
	User& u = Kernel::Instance().GetUser();

	sb = this->m_SuperBlock;
	if (sb->s_flock)
	{
		cout << "�����󡿷����쳣�������������ˣ�" << endl;
		exit(16);
	}
	/* ��������ջ������ȡ���д��̿��� */
	blkno = sb->s_free[--sb->s_nfree];
	
	/*
	 * ����ȡ���̿���Ϊ�㣬���ʾ�ѷ��価���еĿ��д��̿顣
	 * ���߷��䵽�Ŀ��д��̿��Ų����������̿�������(��BadBlock()���)��
	 * ����ζ�ŷ�����д��̿����ʧ�ܡ�
	 */
	if (0 == blkno)
	{
		sb->s_nfree = 0;	// �Ѿ�û�п��д��̿�
		cout << "������û�п��п���Է����ˣ�" << endl;
		u.u_error = User::enospc;
		return NULL;
	}
	/*
	 * ջ�ѿգ��·��䵽���д��̿��м�¼����һ����д��̿�ı��,
	 * ����һ����д��̿�ı�Ŷ���SuperBlock�Ŀ��д��̿�������s_free[100]�С�
	 */
	if (sb->s_nfree <= 0)
	{
		/*
		 * �˴���������Ϊ����Ҫ���ж��̲������п��ܷ��������л���
		 * ����̨�Ľ��̿��ܶ�SuperBlock�Ŀ����̿���������ʣ��ᵼ�²�һ���ԡ�
		 */
		sb->s_flock++;

		/* ����ÿ��д��̿� */
		pBuf = this->m_BufferManager->Bread(blkno);

		/* �Ӹô��̿��0�ֽڿ�ʼ��¼����ռ��4(s_nfree)+400(s_free[100])���ֽ� */
		int* p = (int*)pBuf->b_addr;

		/* ���ȶ��������̿���s_nfree */
		sb->s_nfree = *p++;

		/* ��ȡ�����к���λ�õ����ݣ�д�뵽SuperBlock�����̿�������s_free[100]�� */
		memcpy(sb->s_free, p, 400);

		/* ����ʹ����ϣ��ͷ��Ա㱻��������ʹ�� */
		this->m_BufferManager->Brelse(pBuf);

		/* ����Կ��д��̿������������������Ϊ�ȴ�����˯�ߵĽ��� */
		sb->s_flock = 0;
	}
	/* ��ͨ����³ɹ����䵽һ���д��̿� */
	pBuf = this->m_BufferManager->GetBlk(blkno);	/* Ϊ�ô��̿����뻺�� */
	this->m_BufferManager->ClrBuf(pBuf);	/* ��ջ����е����� */
	sb->s_fmod = 1;	/* ����SuperBlock���޸ı�־ */

	return pBuf;
}

/* �ͷŴ��̿�blkno */
void FileSystem::Free(int blkno)
{
	SuperBlock* sb;
	Buf* pBuf;
	User& u = Kernel::Instance().GetUser();
	sb = this->m_SuperBlock;
	sb->s_fmod = 1;  // �����޸ı�־
	if (sb->s_flock)
	{
		cout << "�����󡿳����鱻������" << endl;
		exit(15);
	}
	/*
	 * �����ǰϵͳ���Ѿ�û�п����̿飬
	 * �����ͷŵ���ϵͳ�е�1������̿�
	 */
	//û���⣬�����ʵʵ���ڵĵ�һ�飬ֻҪ���п��п飬s_nfree�Ͳ�����Ϊ0����Ϊ�ڷ����ʱ��
	//���s_nfreeΪ0��������������
	if (sb->s_nfree <= 0)
	{
		sb->s_nfree = 1;
		sb->s_free[0] = 0;	/* ʹ��0��ǿ����̿���������־ */
	}
	/* SuperBlock��ֱ�ӹ�����д��̿�ŵ�ջ���� */
	if (sb->s_nfree >= 100)
	{
		/*
		 * ��������Ϊ������Ҫ����д�̲������п��ܷ��������л���
		 * ������������������̿��ܷ���SuperBlock�Ŀ����̿���ᵼ�²�һ����
		 */
		sb->s_flock++;

		/*
		 * ʹ�õ�ǰFree()������Ҫ�ͷŵĴ��̿飬���ǰһ��100������
		 * ���̿��������
		 */
		pBuf = this->m_BufferManager->GetBlk(blkno);	/* Ϊ��ǰ��Ҫ�ͷŵĴ��̿���仺�� */

		/* �Ӹô��̿��0�ֽڿ�ʼ��¼����ռ��4(s_nfree)+400(s_free[100])���ֽ� */
		int* p = (int*)pBuf->b_addr;

		/* ����д������̿��������˵�һ��Ϊ99�飬����ÿ�鶼��100�� */
		*p++ = sb->s_nfree;

		/* ��SuperBlock�Ŀ����̿�������s_free[100]д�뻺���к���λ�� */
		memcpy(p, sb->s_free, 4 * 100);

		sb->s_nfree = 0;	//�������Ͼͼӵ�1
		/* ����ſ����̿�������ġ���ǰ�ͷ��̿顱д����̣���ʵ���˿����̿��¼�����̿�ŵ�Ŀ�� */
		this->m_BufferManager->Bwrite(pBuf);
		// �������
		sb->s_flock = 0;
		// ������Ϊ����������˯�Ľ���
		//Kernel::Instance().GetProcessManager().WakeUpAll((unsigned long)&sb->s_flock);
	}
	sb->s_free[sb->s_nfree++] = blkno;	/* SuperBlock�м�¼�µ�ǰ�ͷ��̿�� */
	sb->s_fmod = 1;  // ����SuperBlock���޸ı�־
}

SuperBlock* FileSystem::GetSuperBlock()
{
	return this->m_SuperBlock ;
}
