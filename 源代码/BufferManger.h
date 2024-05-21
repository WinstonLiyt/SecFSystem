#ifndef BUFFER_MANAGER_H
#define BUFFER_MANAGER_H

#include"Buf.h"
#include"DeviceDriver.h"

class BufferManager
{
public:
	/* static const member */
	static const int NBUF = 15;			/* ������ƿ顢������������ */
	static const int BUFFER_SIZE = 512;	/* ��������С�� ���ֽ�Ϊ��λ */

public:
	BufferManager();			/* ���캯�� */
	~BufferManager();			/* �������� */

	void Initialize();			/* ������ƿ���еĳ�ʼ������������ƿ���b_addrָ����Ӧ�������׵�ַ��*/

	Buf* GetBlk(int blkno);		/* ����һ�黺�棬���ڶ�д�豸dev�ϵ��ַ���blkno�������ﲻҪdev�ˡ�*/
	void Brelse(Buf* bp);		/* �ͷŻ�����ƿ�buf */

	Buf* Bread(int blkno);		/* ��һ�����̿顣devΪ�������豸�ţ�blknoΪĿ����̿��߼���š������ﲻҪdev�ˡ� */

	void Bwrite(Buf* bp);		/* дһ�����̿� */
	void Bdwrite(Buf* bp);		/* �ӳ�д���̿� */

	void ClrBuf(Buf* bp);		/* ��ջ��������� */
	void Bflush();				/* ��devָ���豸�������ӳ�д�Ļ���ȫ����������� */
	Buf& GetBFreeList();		/* ��ȡ���ɻ�����п��ƿ� Buf �������� */

	bool CheckInFree(Buf* bp);	/* ���ĳ��������Ƿ������ɻ�������� */
	void NotAvail(Buf* bp);		/* ��ĳ�����������ɻ��������ȡ�� */

private:
	Buf bFreeList;				/* ���ɻ�����п��ƿ� */
	Buf m_Buf[NBUF];			/* ������ƿ����� */
	unsigned char Buffer[NBUF][BUFFER_SIZE];	/* ���������� */

	DeviceDriver* m_DeviceDriver;				/* ָ���������ģ��ȫ�ֶ��� */
};

extern BufferManager g_BufferManager;			/*  �����ȫ�ֱ������������ļ�ϵͳ�Ļ������ */

#endif