#include "BufferManger.h"
#include "Kernel.h"

BufferManager g_BufferManager;

BufferManager::BufferManager()
{
	//nothing to do here
}

BufferManager::~BufferManager()
{
	//nothing to do here
}

void BufferManager::Initialize()
{
	Buf* bp;

	/* ���ɻ������Ϊ�գ�ǰ��ָ��ͺ���ָ�붼ָ���Լ� */
	this->bFreeList.b_forw = this->bFreeList.b_back = &(this->bFreeList);

	/* �ѻ����ȫ�����������ɻ�������� */
	for (int i = 0; i < NBUF; i++)
	{
		bp = &(this->m_Buf[i]);	/* ������ƿ����� */
		bp->b_addr = this->Buffer[i];	// ָ���Ӧ�Ļ����

		/* ����黺���������ɻ�����еĶ��� */
		bp->b_back = &(this->bFreeList);
		bp->b_forw = this->bFreeList.b_forw;
		this->bFreeList.b_forw->b_back = bp;  // ԭ���׻�����ǰһ���Ϊ�¼ӵĻ���
		this->bFreeList.b_forw = bp;  // �¼ӵĻ����������ɻ�����еĵ�һ�黺��

		this->m_DeviceDriver = &Kernel::Instance().GetDeviceDriver();	//ָ���豸����
	}

}

Buf* BufferManager::GetBlk(int blkno)
{
	Buf* bp=NULL;
	User& u = Kernel::Instance().GetUser();

	/* ������豸�ų�����ϵͳ�п��豸����������Ҫ�ˣ� */

	// m_Buf�еĻ���鶼���豸�����еģ�NBUF = 15��
	for (int i = 0; i < this->NBUF; i++)
	{
		/* �ҵ�����飬�����ɶ�������ȡ����ָ��ȡ�������û������Ϊ�����ã� */
		if (this->m_Buf[i].b_blkno == blkno)
		{
			bp = &this->m_Buf[i];
			this->NotAvail(bp);
			break;
		}
	}
	/* ��ǰ�豸��������û�ҵ��������ɶ�����һ�� */
	if (!bp)
	{
		if (this->bFreeList.b_forw == &this->bFreeList)
		{
			cout << "���������ɶ���Ϊ�գ�" << endl;
			exit(1);
		}

		/* ȡ���ɶ��еĵ�һ�飬��ȡ���Ļ������Ϊ������ */
		bp = this->bFreeList.b_forw;
		this->NotAvail(bp);

		/* ���ӳ�д����Ҫ�ѻ�����ϵ�������д��Ӳ�� */
		if (bp->b_flags & Buf::B_DELWRI)
		{
			this->Bwrite(bp);  // ������������д��Ӳ��
			bp->b_flags = bp->b_flags & (~Buf::B_DELWRI);  // ����ӳ�д��־
		}

		/* ���»����Ŀ�ţ������û����ı�־λ */
		bp->b_blkno = blkno;
		bp->b_flags = 0;
	}

	return bp;
}

/* �ͷŻ����bp�����û������ñ�־���Ұѻ����������ɻ�����еĶ�β */
void BufferManager::Brelse(Buf* bp)
{

	int cnt = 0;
	Buf* cur1 = this->bFreeList.b_forw;
	/* �������б��ͷ���������ɶ��У��ҵ�����ĩβ���õ����ɶ����л�������� */
	while (cur1->b_forw != &(this->bFreeList))
	{
		cur1 = cur1->b_forw;
		cnt++;
	}

	this->bFreeList.b_back = cur1;  // �������ɶ��е�βָ��
	
	// ���bp������B_WANTED��B_BUSY��B_ASYNC��־
	bp->b_flags &= ~(Buf::B_WANTED | Buf::B_BUSY | Buf::B_ASYNC);


	/* ���bp�Ƿ��������ɶ����У���ֹ�ظ����� */
	if (!this->CheckInFree(bp))
	{
		(this->bFreeList.b_back)->b_forw = bp;	/* ��bp�嵽���ɻ�����еĶ�β */
		bp->b_back = this->bFreeList.b_back;	/* ����bp��ǰһ�������Ϊԭ�����ɻ�����ж�β�Ļ���� */
		bp->b_forw = &(this->bFreeList);		/* ����bp�ĺ��ָ�룬ָ�������б�ͷ���γɱջ� */
		this->bFreeList.b_back = bp;			/* �������ɻ�����еĶ�βΪbp */
	}
}

Buf* BufferManager::Bread(int blkno)
{
	Buf* bp;
	bp = this->GetBlk(blkno);

	/* ��黺����Ƿ��Ѿ���������Ҫ�����ݣ��������ݶ�ȡ��д������Ѿ���� */
	if (bp->b_flags & Buf::B_DONE)
		return bp;

	bp->b_flags = Buf::B_READ;
	bp->b_wcount = BufferManager::BUFFER_SIZE;	 // ���û����Ĺ�������Ϊ�������Ĵ�С������Ҫ���͵��ֽ���
	this->m_DeviceDriver->IO(bp);  // �����豸������ִ��IO�������������õı�־���ж�����
	bp->b_flags |= Buf::B_DONE;

	return bp;
}

void BufferManager::Bwrite(Buf* bp)
{
	/* ��������������ɻ��������ȡ�� */
	this->NotAvail(bp);
	bp->b_flags &= ~(Buf::B_READ | Buf::B_DONE | Buf::B_ERROR | Buf::B_DELWRI);
	bp->b_flags |= (Buf::B_WRITE|Buf::B_BUSY);
	this->m_DeviceDriver->IO(bp);

	this->Brelse(bp);
}

/* �ӳ�д����� */
void BufferManager::Bdwrite(Buf* bp)
{
	/* ����B_DONE������������ʹ�øô��̿����� */
	bp->b_flags |= (Buf::B_DELWRI | Buf::B_DONE);
	this->Brelse(bp);
}

void BufferManager::ClrBuf(Buf* bp)
{
	int* pInt = (int*)bp->b_addr;  // ���������׵�ַ

	/* ������������������ */
	for (unsigned int i = 0; i < BufferManager::BUFFER_SIZE / sizeof(int); i++)
	{
		pInt[i] = 0;
	}
	return;
}

/* ���ӳ�д�Ļ���ȫ����������� */
void BufferManager::Bflush()
{
	Buf* bp;
	/* ע�⣺����֮����Ҫ��������һ����֮�����¿�ʼ������
		* ��Ϊ��bwite()���뵽����������ʱ�п��жϵĲ���������
		* �ȵ�bwriteִ����ɺ�CPU�Ѵ��ڿ��ж�״̬�����Ժ�
		* �п��������ڼ���������жϣ�ʹ��bfreelist���г��ֱ仯��
		* �����������������������������¿�ʼ������ô�ܿ�����
		* ����bfreelist���е�ʱ����ִ���
		*/
	for (bp = this->bFreeList.b_forw; bp != &this->bFreeList; bp = bp->b_forw)
	{
		/* �ҳ����ɶ����������ӳ�д�Ŀ� */
		if (bp->b_flags & Buf::B_DELWRI)
			Bwrite(bp);
	}
}

/* ��ȡ���ɻ�����п��ƿ� Buf �������� */
Buf& BufferManager::GetBFreeList()
{
	return this->bFreeList;
}

/* ���ĳ��������Ƿ������ɻ�������� */
bool BufferManager::CheckInFree(Buf* bp)
{
	Buf* cur = this->bFreeList.b_forw;
	while (cur != &(this->bFreeList))
	{
		if (cur == bp)
			return true;
		cur = cur->b_forw;
	}
	return false;
}

/* ��ĳ�����������ɻ��������ȡ�� */
void BufferManager::NotAvail(Buf* bp)
{
	if (!this->CheckInFree(bp))
		/*cout << "�����󡿸û�����Ѿ��������ɻ���������ˣ��޷���ȡ����" << endl;*/
		;
	else
	{
		if (bp)
		{
			/* �����ɶ�����ȡ�� */
			bp->b_back->b_forw = bp->b_forw;
			bp->b_forw->b_back = bp->b_back;
			/* ����B_BUSY��־ */
			bp->b_flags |= Buf::B_BUSY;
		}
	}
}
