#include "Inode.h"
#include "FileSystem.h"
#include "Utility.h"
#include "DeviceDriver.h"
#include "User.h"
#include "Kernel.h"

/* �ڴ�� i�ڵ�*/
Inode::Inode()
{
	/* ���Inode�����е����� */
	// this->Clean(); 
	/* ȥ��this->Clean();�����ɣ�
	 * Inode::Clean()�ض�����IAlloc()������·���DiskInode��ԭ�����ݣ�
	 * �����ļ���Ϣ��Clean()�����в�Ӧ�����i_dev, i_number, i_flag, i_count,
	 * ���������ڴ�Inode����DiskInode�����ľ��ļ���Ϣ����Inode�๹�캯����Ҫ
	 * �����ʼ��Ϊ��Чֵ��
	 */
	 /* ��Inode����ĳ�Ա������ʼ��Ϊ��Чֵ */
	this->i_flag = 0;
	this->i_mode = 0;
	this->i_count = 0;
	this->i_nlink = 0;
	this->i_number = -1;
	this->i_uid = -1;
	this->i_gid = -1;
	this->i_size = 0;
	this->i_lastr = -1;
	for (int i = 0; i < 10; i++)
		this->i_addr[i] = 0;
}

Inode::~Inode()
{
	//nothing to do here
}

/* ����inode�����е�������̿���������ȡ��Ӧ�ļ����ݣ����Ƕ���Inode�У����Ƕ��� u.u_IOParam.m_Base�� */
/* �Ӵ��̶�ȡ�ض��ļ������ݵ��û�ָ�����ڴ����� */
void Inode::ReadI()
{
	int lbn;	/* �ļ��߼���� */
	int bn;		/* lbn��Ӧ�������̿�� */
	int offset;	/* ��ǰ�ַ�������ʼ����λ�� */
	int nbytes;	/* �������û�Ŀ�����ֽ����� */
	short dev;	/* �豸�� */
	Buf* pBuf;	/* ����ָ�� */
	User& u = Kernel::Instance().GetUser();
	BufferManager& bufMgr = Kernel::Instance().GetBufferManager();
	DeviceDriver& devMgr = Kernel::Instance().GetDeviceDriver();

	if (0 == u.u_IOParam.m_Count)
	{
		/* ��Ҫ���ֽ���Ϊ�㣬�򷵻� */
		return;
	}
	this->i_flag |= Inode::IACC;  // ���÷��ʱ�־��д��ʱ��Ҫ�޸ķ���ʱ��
	if (u.u_error != User::noerror)
	{
		//cout << "��ReadI��ʱ��u_error�쳣����ע����" << endl;
	}

	/* һ��һ���ַ���ض�������ȫ�����ݣ�ֱ�������ļ�β */
	while (User::noerror == u.u_error && u.u_IOParam.m_Count != 0)
	{
		/* ���㵱ǰ�ַ������ļ��е��߼���� lbn���Լ����ַ����е���ʼλ�� offset */
		lbn = bn = u.u_IOParam.m_Offset / BLOCK_SIZE;
		offset = u.u_IOParam.m_Offset % BLOCK_SIZE;
		/* ���͵��û������ֽ�������ȡ�������ʣ���ֽ����뵱ǰ�ַ�������Ч�ֽ�����Сֵ */
		nbytes =min(BLOCK_SIZE - offset /* ������Ч�ֽ��� */, u.u_IOParam.m_Count);
		
		int remain = this->i_size - u.u_IOParam.m_Offset;
		/* ����Ѷ��������ļ���β */
		if (remain <= 0)
		{
			return;
		}
		/* ���͵��ֽ�������ȡ����ʣ���ļ��ĳ��� */
		nbytes = min(nbytes, remain);

		/* ���߼����lbnת���������̿��bn ��Bmap������Inode::rablock����UNIX��Ϊ��ȡԤ����Ŀ���̫��ʱ��
		 * �����Ԥ������ʱ Inode::rablock ֵΪ 0��
		 * */
		if ((bn = this->Bmap(lbn)) == 0)
		{
			return;
		}

		pBuf = bufMgr.Bread(bn);
		unsigned char* start = pBuf->b_addr + offset;  /* ������������ʼ��λ�� */

		memcpy(u.u_IOParam.m_Base, start, nbytes);
		u.u_IOParam.m_Base += nbytes;	/* �����û�Ŀ�� */
		u.u_IOParam.m_Offset += nbytes;	/* ���¶�дλ�� */
		u.u_IOParam.m_Count -= nbytes;	/* ����ʣ���ֽ��� */

		bufMgr.Brelse(pBuf);	/* ʹ���껺�棬�ͷŸ���Դ */
	}
}

/* ����Inode�����е�������̿�������������д���ļ� */
void Inode::WriteI()
{
	int lbn;	/* �ļ��߼���� */
	int bn;		/* lbn��Ӧ�������̿�� */
	int offset;	/* ��ǰ�ַ�������ʼ����λ�� */
	int nbytes;	/* �����ֽ����� */
	short dev;	/* �豸�� */
	Buf* pBuf;	/* ����ָ�� */
	User& u = Kernel::Instance().GetUser();
	BufferManager& bufMgr = Kernel::Instance().GetBufferManager();
	DeviceDriver& devMgr = Kernel::Instance().GetDeviceDriver();

	/* ����Inode�����ʱ�־λ */
	this->i_flag |= (Inode::IACC | Inode::IUPD);

	if (0 == u.u_IOParam.m_Count)
	{
		/* ��Ҫ���ֽ���Ϊ�㣬�򷵻� */
		return;
	}

	/* ֻҪû�д����������û���������Ҫд�룬ѭ���ͻ���� */
	while (User::noerror == u.u_error && u.u_IOParam.m_Count != 0)
	{
		/* ���㵱ǰ��ַ���߼���ţ�lbn�����ڸõ�ַ�е���ʼλ�ã�offset����Ȼ�󣬼���Ҫ������ֽ�����nbytes����
		   ���ǵ�ǰ��ʣ���С���û�Ҫд��Ĵ�С֮��Ľ�Сֵ */
		/* ���㵱ǰ�ַ�����߼����lbn���Լ��ڸ��ַ����е���ʼ����λ��offset */
		/* �߼����������ڵ�ǰ��Inode�ģ���0��ʼ */
		/* �����ž������ǵĴ��� */
		/* �߼����ת��������ʵ����ȥthis_addr���Ҷ�Ӧ������ֵ */
		lbn = u.u_IOParam.m_Offset / BLOCK_SIZE;		// �ڼ��飨�߼���ţ�
		offset = u.u_IOParam.m_Offset % BLOCK_SIZE;		// ����ƫ����
		/* ���㴫���ֽ�����nbytes,�Ǹÿ���ʣ���С���û�Ҫ���͵Ĵ�С�Ľ�Сֵ */
		nbytes = min(BLOCK_SIZE - offset, u.u_IOParam.m_Count);

		/* �߼����ת������ */
		if ((bn = this->Bmap(lbn)) == 0)
		{
			return;
		}

		if (BLOCK_SIZE == nbytes)
		{
			/* ���д������������һ���ַ��飬��Ϊ����仺�� */
			pBuf = bufMgr.GetBlk(bn);
		}
		else
		{
			/* д�����ݲ���һ���ַ��飬���ȶ���д�����������ַ����Ա�������Ҫ��д�����ݣ� */
			pBuf = bufMgr.Bread(bn);
		}

		/* ���������ݵ���ʼдλ�� */
		unsigned char* start = pBuf->b_addr + offset;

		/* д����: ���û�Ŀ�����������ݵ������� */
		memcpy(start, u.u_IOParam.m_Base, nbytes);

		/* �ô����ֽ���nbytes���¶�дλ�� */
		u.u_IOParam.m_Base += nbytes;	/* �����û�Ŀ���� */
		u.u_IOParam.m_Offset += nbytes;	/* ���¶�дλ�� */
		u.u_IOParam.m_Count -= nbytes;	/* ����ʣ���ֽ��� */

		if (u.u_error != User::noerror)	/* д�����г��� */
		{
			bufMgr.Brelse(pBuf);	/* �ͷŻ��� */
		}
		else if ((u.u_IOParam.m_Offset % BLOCK_SIZE) == 0)	/* ���д��һ���ַ��� */
		{
			/* ���첽��ʽ���ַ���д����̣����̲���ȴ�I/O�������������Լ�������ִ�� */
			bufMgr.Bwrite(pBuf);
			bufMgr.Brelse(pBuf);
		}
		else /* ���������δд�� */
		{
			/* ��������Ϊ�ӳ�д�������ڽ���I/O�������ַ�������������� */
			bufMgr.Bwrite(pBuf);
			bufMgr.Brelse(pBuf);
		}

		/* ��ͨ�ļ��������� */
		if ((this->i_size < u.u_IOParam.m_Offset) && (this->i_mode & (Inode::IFBLK & Inode::IFCHR)) == 0)
		{
			/* ����ļ�����С��д��λ�ã�������ļ����� */
			this->i_size = u.u_IOParam.m_Offset;
		}

		/*
		 * ֮ǰ�����ж��̿��ܵ��½����л����ڽ���˯���ڼ䵱ǰ�ڴ�Inode����
		 * ��ͬ�������Inode���ڴ���Ҫ�������ø��±�־λ��
		 * ����û�б�Ҫѽ����ʹwriteϵͳ����û��������iput����i_count����0֮��ŻὫ�ڴ�i�ڵ�ͬ���ش��̡�������
		 * �ļ�û��close֮ǰ�ǲ��ᷢ���ġ�
		 * ���ǵ�ϵͳ��writeϵͳ���������͸������ܳ�����������ˡ�
		 * ��������ȥ����
		 */
		this->i_flag |= Inode::IUPD;
	}
}

/* ���ļ����߼����ת���ɶ�Ӧ�������̿�� */
int Inode::Bmap(int lbn)
{
	Buf* pFirstBuf;		/* ���ڷ���һ�μ�������� */
	Buf* pSecondBuf;	/* ���ڷ��ʶ��μ�������� */
	int phyBlkno;		/* ת����������̿�� */
	int* iTable;		/* ���ڷ��������̿���һ�μ�ӡ����μ�������� */
	int index;			/* ���ڷ��������̿���һ�μ�ӡ����μ���������е�����ֵ */
	User& u = Kernel::Instance().GetUser();
	BufferManager& bufMgr = Kernel::Instance().GetBufferManager();
	FileSystem& fileSys = Kernel::Instance().GetFileSystem();

	/*
	 * Unix V6++���ļ������ṹ��(С�͡����ͺ;����ļ�)
	 * (1) i_addr[0] - i_addr[5]Ϊֱ���������ļ����ȷ�Χ��0 - 6���̿飻
	 *
	 * (2) i_addr[6] - i_addr[7]���һ�μ�����������ڴ��̿�ţ�ÿ���̿�
	 * �ϴ��128���ļ������̿�ţ������ļ����ȷ�Χ��7 - (128 * 2 + 6)���̿飻
	 *
	 * (3) i_addr[8] - i_addr[9]��Ŷ��μ�����������ڴ��̿�ţ�ÿ�����μ��
	 * �������¼128��һ�μ�����������ڴ��̿�ţ������ļ����ȷ�Χ��
	 * (128 * 2 + 6 ) < size <= (128 * 128 * 2 + 128 * 2 + 6)
	 */

	if (lbn >= Inode::HUGE_FILE_BLOCK)
	{
		u.u_error = User::efbig;
		return 0;
	}
	/* �����С���ļ����ӻ���������i_addr[0-5]�л�������̿�ż��� */
	if (lbn < 6)
	{
		phyBlkno = this->i_addr[lbn];	/* �ӻ����������л�������̿�� */

		/*
		 * ������߼���Ż�û����Ӧ�������̿����֮��Ӧ�������һ������顣
		 * ��ͨ�������ڶ��ļ���д�룬��д��λ�ó����ļ���С�����Ե�ǰ
		 * �ļ���������д�룬����Ҫ�������Ĵ��̿飬��Ϊ֮�����߼����
		 * �������̿��֮���ӳ�䡣
		 */
		if (phyBlkno == 0 && (pFirstBuf = fileSys.Alloc()) != NULL)
		{
			/*
			 * ��Ϊ����ܿ������ϻ�Ҫ�õ��˴��·�������ݿ飬���Բ��������������
			 * �����ϣ����ǽ�������Ϊ�ӳ�д��ʽ���������Լ���ϵͳ��I/O������
			 */
			bufMgr.Bdwrite(pFirstBuf);		/* ���·�������ݿ�д����� */
			phyBlkno = pFirstBuf->b_blkno;	/* ����·�������ݿ�������̿�� */
			/* ���߼����lbnӳ�䵽�����̿��phyBlkno */
			this->i_addr[lbn] = phyBlkno;	/* ���·�������ݿ�������̿��д����������� */
			this->i_flag |= Inode::IUPD;	/* ���ø��±�־λ */
		}

		return phyBlkno;
	}
	/* lbn >= 6 ���͡������ļ� */
	else
	{
		/* �����߼����lbn��Ӧi_addr[]�е����� */

		if (lbn < Inode::LARGE_FILE_BLOCK)	/* �����ļ�: ���Ƚ���7 - (128 * 2 + 6)���̿�֮�� */
		{
			// ���������������ʱi_addr�е��±�
			index = (lbn - Inode::SMALL_FILE_BLOCK) / Inode::ADDRESS_PER_INDEX_BLOCK + 6;
		}
		else	/* �����ļ�: ���Ƚ���263 - (128 * 128 * 2 + 128 * 2 + 6)���̿�֮�� */
		{
			// ��������
			index = (lbn - Inode::LARGE_FILE_BLOCK) / (Inode::ADDRESS_PER_INDEX_BLOCK * Inode::ADDRESS_PER_INDEX_BLOCK) + 8;
		}
		phyBlkno = this->i_addr[index];

		/* ������Ϊ�㣬���ʾ��������Ӧ�ļ��������� */
		if (0 == phyBlkno)
		{
			this->i_flag |= Inode::IUPD;	// ��־λ�������޸�
			/* ����һ�����̿��ż�������� */
			if ((pFirstBuf = fileSys.Alloc()) == NULL)
			{
				return 0;	/* ����ʧ�� */
			}
			/* i_addr[index]�м�¼���������������̿�� */
			this->i_addr[index] = pFirstBuf->b_blkno;
		}
		else
		{
			/* �����洢�����������ַ��� */
			pFirstBuf = bufMgr.Bread( phyBlkno);
		}
		/* ��ȡ��������ַ */
		iTable = (int*)pFirstBuf->b_addr;

		if (index >= 8)	/* ASSERT: 8 <= index <= 9 */
		{
			/*
			 * ���ھ����ļ��������pFirstBuf���Ƕ��μ��������
			 * ��������߼���ţ����ɶ��μ���������ҵ�һ�μ��������
			 */
			 // ��ȡ����
			index = ((lbn - Inode::LARGE_FILE_BLOCK) / Inode::ADDRESS_PER_INDEX_BLOCK) % Inode::ADDRESS_PER_INDEX_BLOCK;

			/* iTableָ�򻺴��еĶ��μ������������Ϊ�㣬������һ�μ�������� */
			phyBlkno = iTable[index];
			if (0 == phyBlkno)
			{
				/* �����һ�μ����������δ�����䣬����һ�����̿���һ�μ�������� */
				if ((pSecondBuf = fileSys.Alloc()) == NULL)
				{
					/* ����һ�μ����������̿�ʧ�ܣ��ͷŻ����еĶ��μ��������Ȼ�󷵻� */
					bufMgr.Brelse(pFirstBuf);
					return 0;
				}
				/* ���·����һ�μ����������̿�ţ�������μ����������Ӧ�� */
				iTable[index] = pSecondBuf->b_blkno;
				/* �����ĺ�Ķ��μ���������ӳ�д��ʽ��������� */
				bufMgr.Bdwrite(pFirstBuf);
			}
			else
			{
				/* �ͷŶ��μ��������ռ�õĻ��棬������һ�μ�������� */
				bufMgr.Brelse(pFirstBuf);
				pSecondBuf = bufMgr.Bread(phyBlkno);
			}

			/* ��pFirstBufָ��һ�μ�������� */
			pFirstBuf = pSecondBuf;
			/* ��iTableָ��һ�μ�������� */
			iTable = (int*)pSecondBuf->b_addr;
		}

		if (lbn < Inode::LARGE_FILE_BLOCK)
			index = (lbn - Inode::SMALL_FILE_BLOCK) % Inode::ADDRESS_PER_INDEX_BLOCK;
		else
			index = (lbn - Inode::LARGE_FILE_BLOCK) % Inode::ADDRESS_PER_INDEX_BLOCK;

		if ((phyBlkno = iTable[index]) == 0 && (pSecondBuf = fileSys.Alloc()) != NULL)
		{
			/* �����䵽���ļ������̿�ŵǼ���һ�μ���������� */
			phyBlkno = pSecondBuf->b_blkno;
			iTable[index] = phyBlkno;

			/* �������̿顢���ĺ��һ�μ�����������ӳ�д��ʽ��������� */
			bufMgr.Bdwrite(pSecondBuf);
			bufMgr.Bdwrite(pFirstBuf);
		}
		else
		{
			/* �ͷ�һ�μ��������ռ�û��� */
			bufMgr.Brelse(pFirstBuf);
		}
		
		return phyBlkno;
	}
}

// ���ڸ������Inode�����ķ���ʱ�䡢�޸�ʱ�䡾�ļ�ϵͳ�������������
void Inode::IUpdate(int time)
{
	Buf* pBuf;			/* �����ָ�� */
	DiskInode dInode;	/* ����Inode */
	FileSystem& filesys = Kernel::Instance().GetFileSystem();		/* �ļ�ϵͳ */
	BufferManager& bufMgr = Kernel::Instance().GetBufferManager();	/* ��������� */

	/* ��IUPD��IACC��־֮һ�����ã�����Ҫ������ӦDiskInode
	 * Ŀ¼����������������;����Ŀ¼�ļ���IACC��IUPD��־ */
	if ((this->i_flag & (Inode::IUPD | Inode::IACC)) != 0)
	{
		/* ���ص�ע�ͣ��ڻ�������ҵ�������i�ڵ㣨this->i_number���Ļ����
		 * ����һ�������Ļ���飬���δ����е�Bwrite()�ڽ������д�ش��̺���ͷŸû���顣
		 * ���ô�Ÿ�DiskInode���ַ�����뻺���� */
		pBuf = bufMgr.Bread(FileSystem::INODE_ZONE_START_SECTOR + this->i_number / FileSystem::INODE_NUMBER_PER_SECTOR);

		/* ���ڴ�Inode�����е���Ϣ���Ƶ�dInode�У�Ȼ��dInode���ǻ����оɵ����Inode */
		dInode.d_mode = this->i_mode;		/* �ļ����ͺͷ���Ȩ�� */
		dInode.d_nlink = this->i_nlink;		/* ������ */
		dInode.d_uid = this->i_uid;			/* �ļ������ߵ��û���ʶ�� */
		dInode.d_gid = this->i_gid;			/* �ļ������ߵ����ʶ�� */
		dInode.d_size = this->i_size;		/* �ļ����� */
		for (int i = 0; i < 10; i++)
			dInode.d_addr[i] = this->i_addr[i];	/* �ļ�ռ�õĴ��̿�� */

		if (this->i_flag & Inode::IACC)
		{
			/* ����������ʱ�� */
			dInode.d_atime = time;
		}
		if (this->i_flag & Inode::IUPD)
		{
			/* ����������ʱ�� */
			dInode.d_mtime = time;
		}

		/* ��pָ�򻺴����о����Inode��ƫ��λ�� */
		unsigned char* p = pBuf->b_addr + (this->i_number % FileSystem::INODE_NUMBER_PER_SECTOR) * sizeof(DiskInode);
		DiskInode* pNode = &dInode;
		/* ��dInode�е������ݸ��ǻ����еľ����Inode */
		memcpy(p, pNode, sizeof(DiskInode));

		/* ������д�������̣��ﵽ���¾����Inode��Ŀ�� */
		bufMgr.Bwrite(pBuf);
	}
}

/* �ͷ�Inode��ռ�õĴ��̿� */
void Inode::ITrunc()
{
	/* ���ɴ��̸��ٻ����ȡ���һ�μ�ӡ����μ��������Ĵ��̿� */
	BufferManager& bm = Kernel::Instance().GetBufferManager();
	/* ��ȡg_FileSystem��������ã�ִ���ͷŴ��̿�Ĳ��� */
	FileSystem& filesys = Kernel::Instance().GetFileSystem();

	/* ������ַ��豸���߿��豸���˳� */
	if (this->i_mode & (Inode::IFCHR & Inode::IFBLK))
	{
		return;
	}

	/* ����FILO��ʽ�ͷţ��Ծ���ʹ��SuperBlock�м�¼�Ŀ����̿��������
	 *
	 * Unix V6++���ļ������ṹ��(С�͡����ͺ;����ļ�)
	 * (1) i_addr[0] - i_addr[5]Ϊֱ���������ļ����ȷ�Χ��0 - 6���̿飻
	 *
	 * (2) i_addr[6] - i_addr[7]���һ�μ�����������ڴ��̿�ţ�ÿ���̿�
	 * �ϴ��128���ļ������̿�ţ������ļ����ȷ�Χ��7 - (128 * 2 + 6)���̿飻
	 *
	 * (3) i_addr[8] - i_addr[9]��Ŷ��μ�����������ڴ��̿�ţ�ÿ�����μ��
	 * �������¼128��һ�μ�����������ڴ��̿�ţ������ļ����ȷ�Χ��
	 * (128 * 2 + 6 ) < size <= (128 * 128 * 2 + 128 * 2 + 6)
	 */
	for (int i = 9; i >= 0; i--)		/* ��i_addr[9]��i_addr[0] */
	{
		/* ���i_addr[]�е�i��������� */
		if (this->i_addr[i] != 0)
		{
			/* �����i_addr[]�е�һ�μ�ӡ����μ�������� */
			if (i >= 6 && i <= 9)
			{
				/* �������������뻺�� */
				Buf* pFirstBuf = bm.Bread(this->i_addr[i]);
				/* ��ȡ��������ַ */
				int* pFirst = (int*)pFirstBuf->b_addr;

				/* ÿ�ż���������¼ 512/sizeof(int) = 128�����̿�ţ�������ȫ��128�����̿� */
				for (int j = 128 - 1; j >= 0; j--)
				{
					if (pFirst[j] != 0)	/* �������������� */
					{
						/*
						 * ��������μ��������i_addr[8]��i_addr[9]�
						 * ��ô���ַ����¼����128��һ�μ���������ŵĴ��̿��
						 */
						if (i >= 8 && i <= 9)
						{
							Buf* pSecondBuf = bm.Bread(pFirst[j]);  // ��һ�μ����������뻺��,ͨ��ѭ�����������ͷ�
							int* pSecond = (int*)pSecondBuf->b_addr;  // ��ȡ��������ַ

							for (int k = 128 - 1; k >= 0; k--)
							{
								if (pSecond[k] != 0)
								{
									/* �ͷ�ָ���Ĵ��̿� */
									filesys.Free(pSecond[k]);
								}
							}
							/* ����ʹ����ϣ��ͷ��Ա㱻��������ʹ�� */
							bm.Brelse(pSecondBuf);
						}
						//�ͷŴ��̿�
						filesys.Free(pFirst[j]);
					}
				}
				//�ͷŴ��̿�
				bm.Brelse(pFirstBuf);
			}
			/* �ͷ���������ռ�õĴ��̿� */
			filesys.Free(this->i_addr[i]);
			/* 0��ʾ����������� */
			this->i_addr[i] = 0;
		}
	}
	/* �̿��ͷ���ϣ��ļ���С���� */
	this->i_size = 0;
	/* ����IUPD��־λ����ʾ���ڴ�Inode��Ҫͬ������Ӧ���Inode */
	this->i_flag |= Inode::IUPD;
	/* ����ļ���־ ��ԭ����RWXRWXRWX����*/
	this->i_mode &= ~(Inode::ILARG & Inode::IRWXU & Inode::IRWXG & Inode::IRWXO);
	this->i_nlink = 1;
}

void Inode::Clean()
{
	/*
	 * Inode::Clean()�ض�����IAlloc()������·���DiskInode��ԭ�����ݣ�
	 * �����ļ���Ϣ��Clean()�����в�Ӧ�����i_dev, i_number, i_flag, i_count,
	 * ���������ڴ�Inode����DiskInode�����ľ��ļ���Ϣ����Inode�๹�캯����Ҫ
	 * �����ʼ��Ϊ��Чֵ��
	 */

	this->i_mode = 0;	//����ļ����ͺͷ���Ȩ��
	this->i_nlink = 0;	//����ļ�������
	this->i_uid = -1;	//����ļ�������
	this->i_gid = -1;	//����ļ���������
	this->i_size = 0;	//����ļ���С
	this->i_lastr = -1;	//������һ�ζ�ȡ���ļ����
	for (int i = 0; i < 10; i++)
		this->i_addr[i] = 0;	//����ļ����ݿ��
}

/* ���������Inode�ַ���Ļ����е�Inode��Ϣ�������ڴ�Inode�� */
void Inode::ICopy(Buf* bp, int inumber)
{
	DiskInode dInode;  // ��ʱ���������ڴ�����Inode��Ϣ
	DiskInode* pNode = &dInode;	// ָ����ʱ����dInode

	unsigned char* p = bp->b_addr + (inumber % FileSystem::INODE_NUMBER_PER_SECTOR) * sizeof(DiskInode);
	// �����������Inode���ݿ�������ʱ������
	memcpy(pNode, p, sizeof(DiskInode));

	/* �����Inode����dInode����Ϣ���Ƶ��ڴ�Inode�� */
	this->i_mode = dInode.d_mode;	//	�ļ����ͺͷ���Ȩ��
	this->i_nlink = dInode.d_nlink;	//	�ļ�������
	this->i_uid = dInode.d_uid;		// �ļ�������
	this->i_gid = dInode.d_gid;		// �ļ���������
	this->i_size = dInode.d_size;	// �ļ���С
	for (int i = 0; i < 10; i++)
		this->i_addr[i] = dInode.d_addr[i];	// �ļ����ݿ��
}

/* ����Inode */
void Inode::Prele()
{
	/* ����pipe��Inode,���һ�����Ӧ���� */
	this->i_flag &= ~Inode::ILOCK;
}


DiskInode::DiskInode()
{
	/*
	 * ���DiskInodeû�й��캯�����ᷢ�����½��Ѳ���Ĵ���
	 * DiskInode��Ϊ�ֲ�����ռ�ݺ���Stack Frame�е��ڴ�ռ䣬����
	 * ��οռ�û�б���ȷ��ʼ�����Ծɱ�������ǰջ���ݣ����ڲ�����
	 * DiskInode�����ֶζ��ᱻ���£���DiskInodeд�ص�������ʱ������
	 * ����ǰջ����һͬд�أ�����д�ؽ������Ī����������ݡ�
	 */
	this->d_mode = 0;	/* �ļ����ͺͷ���Ȩ�� */
	this->d_nlink = 0;	/* �ļ������� */
	this->d_uid = -1;	/* �ļ������� */
	this->d_gid = -1;	/* �ļ��������� */
	this->d_size = 0;	/* �ļ���С */
	for (int i = 0; i < 10; i++)
		this->d_addr[i] = 0;	/* �ļ����ݿ�� */
	this->d_atime = 0;	/* �ļ����һ�η���ʱ�� */
	this->d_mtime = 0;	/* �ļ����һ���޸�ʱ�� */
}

DiskInode::~DiskInode()
{
	//nothing to do 
}