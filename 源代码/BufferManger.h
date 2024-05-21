#ifndef BUFFER_MANAGER_H
#define BUFFER_MANAGER_H

#include"Buf.h"
#include"DeviceDriver.h"

class BufferManager
{
public:
	/* static const member */
	static const int NBUF = 15;			/* 缓存控制块、缓冲区的数量 */
	static const int BUFFER_SIZE = 512;	/* 缓冲区大小。 以字节为单位 */

public:
	BufferManager();			/* 构造函数 */
	~BufferManager();			/* 析构函数 */

	void Initialize();			/* 缓存控制块队列的初始化。将缓存控制块中b_addr指向相应缓冲区首地址。*/

	Buf* GetBlk(int blkno);		/* 申请一块缓存，用于读写设备dev上的字符块blkno。【这里不要dev了】*/
	void Brelse(Buf* bp);		/* 释放缓存控制块buf */

	Buf* Bread(int blkno);		/* 读一个磁盘块。dev为主、次设备号，blkno为目标磁盘块逻辑块号。【这里不要dev了】 */

	void Bwrite(Buf* bp);		/* 写一个磁盘块 */
	void Bdwrite(Buf* bp);		/* 延迟写磁盘块 */

	void ClrBuf(Buf* bp);		/* 清空缓冲区内容 */
	void Bflush();				/* 将dev指定设备队列中延迟写的缓存全部输出到磁盘 */
	Buf& GetBFreeList();		/* 获取自由缓存队列控制块 Buf 对象引用 */

	bool CheckInFree(Buf* bp);	/* 检查某个缓存块是否在自由缓存队列中 */
	void NotAvail(Buf* bp);		/* 将某个缓存块从自由缓存队列中取出 */

private:
	Buf bFreeList;				/* 自由缓存队列控制块 */
	Buf m_Buf[NBUF];			/* 缓存控制块数组 */
	unsigned char Buffer[NBUF][BUFFER_SIZE];	/* 缓冲区数组 */

	DeviceDriver* m_DeviceDriver;				/* 指向磁盘驱动模块全局对象 */
};

extern BufferManager g_BufferManager;			/*  定义成全局变量负责整个文件系统的缓存管理 */

#endif