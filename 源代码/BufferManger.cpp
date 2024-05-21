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

	/* 自由缓存队列为空，前向指针和后向指针都指向自己 */
	this->bFreeList.b_forw = this->bFreeList.b_back = &(this->bFreeList);

	/* 把缓存块全部都丢到自由缓存队列中 */
	for (int i = 0; i < NBUF; i++)
	{
		bp = &(this->m_Buf[i]);	/* 缓存控制块数组 */
		bp->b_addr = this->Buffer[i];	// 指向对应的缓存块

		/* 把这块缓存块放在自由缓存队列的队首 */
		bp->b_back = &(this->bFreeList);
		bp->b_forw = this->bFreeList.b_forw;
		this->bFreeList.b_forw->b_back = bp;  // 原队首缓存块的前一块变为新加的缓存
		this->bFreeList.b_forw = bp;  // 新加的缓存变成了自由缓存队列的第一块缓存

		this->m_DeviceDriver = &Kernel::Instance().GetDeviceDriver();	//指向设备驱动
	}

}

Buf* BufferManager::GetBlk(int blkno)
{
	Buf* bp=NULL;
	User& u = Kernel::Instance().GetUser();

	/* 如果主设备号超出了系统中块设备数量（不需要了） */

	// m_Buf中的缓存块都是设备队列中的（NBUF = 15）
	for (int i = 0; i < this->NBUF; i++)
	{
		/* 找到缓存块，从自由队列里面取出（指针取出，将该缓冲块标记为不可用） */
		if (this->m_Buf[i].b_blkno == blkno)
		{
			bp = &this->m_Buf[i];
			this->NotAvail(bp);
			break;
		}
	}
	/* 当前设备队列里面没找到，从自由队列拿一个 */
	if (!bp)
	{
		if (this->bFreeList.b_forw == &this->bFreeList)
		{
			cout << "【错误】自由队列为空！" << endl;
			exit(1);
		}

		/* 取自由队列的第一块，将取出的缓冲块标记为不可用 */
		bp = this->bFreeList.b_forw;
		this->NotAvail(bp);

		/* 【延迟写】需要把缓存块上的内容先写回硬盘 */
		if (bp->b_flags & Buf::B_DELWRI)
		{
			this->Bwrite(bp);  // 将缓冲块的内容写回硬盘
			bp->b_flags = bp->b_flags & (~Buf::B_DELWRI);  // 清除延迟写标志
		}

		/* 更新缓冲块的块号，并重置缓冲块的标志位 */
		bp->b_blkno = blkno;
		bp->b_flags = 0;
	}

	return bp;
}

/* 释放缓存块bp，设置缓存块可用标志并且把缓存块放在自由缓存队列的队尾 */
void BufferManager::Brelse(Buf* bp)
{

	int cnt = 0;
	Buf* cur1 = this->bFreeList.b_forw;
	/* 从自由列表的头部遍历自由队列，找到队列末尾，得到自由队列中缓存块数量 */
	while (cur1->b_forw != &(this->bFreeList))
	{
		cur1 = cur1->b_forw;
		cnt++;
	}

	this->bFreeList.b_back = cur1;  // 设置自由队列的尾指针
	
	// 清除bp缓存块的B_WANTED、B_BUSY、B_ASYNC标志
	bp->b_flags &= ~(Buf::B_WANTED | Buf::B_BUSY | Buf::B_ASYNC);


	/* 检查bp是否已在自由队列中，防止重复插入 */
	if (!this->CheckInFree(bp))
	{
		(this->bFreeList.b_back)->b_forw = bp;	/* 把bp插到自由缓存队列的队尾 */
		bp->b_back = this->bFreeList.b_back;	/* 设置bp的前一个缓存块为原来自由缓存队列队尾的缓存块 */
		bp->b_forw = &(this->bFreeList);		/* 设置bp的后继指针，指向自由列表头，形成闭环 */
		this->bFreeList.b_back = bp;			/* 更新自由缓存队列的队尾为bp */
	}
}

Buf* BufferManager::Bread(int blkno)
{
	Buf* bp;
	bp = this->GetBlk(blkno);

	/* 检查缓存块是否已经包含了需要的数据，并且数据读取或写入操作已经完成 */
	if (bp->b_flags & Buf::B_DONE)
		return bp;

	bp->b_flags = Buf::B_READ;
	bp->b_wcount = BufferManager::BUFFER_SIZE;	 // 设置缓存块的工作计数为缓冲区的大小，即需要传送的字节数
	this->m_DeviceDriver->IO(bp);  // 调用设备驱动来执行IO操作，根据设置的标志进行读操作
	bp->b_flags |= Buf::B_DONE;

	return bp;
}

void BufferManager::Bwrite(Buf* bp)
{
	/* 把这个缓存块从自由缓存队列中取出 */
	this->NotAvail(bp);
	bp->b_flags &= ~(Buf::B_READ | Buf::B_DONE | Buf::B_ERROR | Buf::B_DELWRI);
	bp->b_flags |= (Buf::B_WRITE|Buf::B_BUSY);
	this->m_DeviceDriver->IO(bp);

	this->Brelse(bp);
}

/* 延迟写缓存块 */
void BufferManager::Bdwrite(Buf* bp)
{
	/* 置上B_DONE允许其它进程使用该磁盘块内容 */
	bp->b_flags |= (Buf::B_DELWRI | Buf::B_DONE);
	this->Brelse(bp);
}

void BufferManager::ClrBuf(Buf* bp)
{
	int* pInt = (int*)bp->b_addr;  // 缓冲区的首地址

	/* 将缓冲区中数据清零 */
	for (unsigned int i = 0; i < BufferManager::BUFFER_SIZE / sizeof(int); i++)
	{
		pInt[i] = 0;
	}
	return;
}

/* 将延迟写的缓存全部输出到磁盘 */
void BufferManager::Bflush()
{
	Buf* bp;
	/* 注意：这里之所以要在搜索到一个块之后重新开始搜索，
		* 因为在bwite()进入到驱动程序中时有开中断的操作，所以
		* 等到bwrite执行完成后，CPU已处于开中断状态，所以很
		* 有可能在这期间产生磁盘中断，使得bfreelist队列出现变化，
		* 如果这里继续往下搜索，而不是重新开始搜索那么很可能在
		* 操作bfreelist队列的时候出现错误。
		*/
	for (bp = this->bFreeList.b_forw; bp != &this->bFreeList; bp = bp->b_forw)
	{
		/* 找出自由队列中所有延迟写的块 */
		if (bp->b_flags & Buf::B_DELWRI)
			Bwrite(bp);
	}
}

/* 获取自由缓存队列控制块 Buf 对象引用 */
Buf& BufferManager::GetBFreeList()
{
	return this->bFreeList;
}

/* 检查某个缓存块是否在自由缓存队列中 */
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

/* 将某个缓存块从自由缓存队列中取出 */
void BufferManager::NotAvail(Buf* bp)
{
	if (!this->CheckInFree(bp))
		/*cout << "【错误】该缓存块已经不在自由缓存队列中了，无法再取出！" << endl;*/
		;
	else
	{
		if (bp)
		{
			/* 从自由队列中取出 */
			bp->b_back->b_forw = bp->b_forw;
			bp->b_forw->b_back = bp->b_back;
			/* 设置B_BUSY标志 */
			bp->b_flags |= Buf::B_BUSY;
		}
	}
}
