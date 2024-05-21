#ifndef FILE_SYSTEM_H
#define FILE_SYSTEM_H

#include "Inode.h"
#include "BufferManger.h"

/*
 * �ļ�ϵͳ�洢��Դ�����(Super Block)�Ķ��塣
 * SuperBlock �г��˴洢��һЩ��־�����ͼ����������⣬
 * �������ſ����̿�Ϳ������ inode �ķ���ʹ�á�
 */
class SuperBlock
{
	/* Functions */
public:
	/* Constructors */
	SuperBlock();
	/* Destructors */
	~SuperBlock();

	/* Members */
public:
	int		s_isize;		/* ���Inode��ռ�õ��̿��� */
	int		s_fsize;		/* �̿����� */

	/* д����һ�����ļ��Ӷ���Ҫ�����Ŀ����̿�ĺ����㷨 */
	int		s_nfree;		/* ֱ�ӹ���Ŀ����̿����� */
	int		s_free[100];	/* ֱ�ӹ���Ŀ����̿������� */

	int		s_ninode;		/* ֱ�ӹ���Ŀ������Inode���� */
	int		s_inode[100];	/* ֱ�ӹ���Ŀ������Inode������ */

	int		s_flock;		/* ���������̿��������־ */
	int		s_ilock;		/* ��������Inode���־ */

	int		s_fmod;			/* �ڴ���super block�������޸ı�־����ζ����Ҫ��������Ӧ��Super Block */
	int		s_ronly;		/* ���ļ�ϵͳֻ�ܶ��� */
	int		s_time;			/* ���һ�θ���ʱ�� */
	int		padding[47];	/* ���ʹSuperBlock���С����1024�ֽڣ�ռ��2������ */
};

/*
 * �ļ�ϵͳ��(FileSystem)�����ļ��洢�豸��
 * �ĸ���洢��Դ�����̿顢���INode�ķ��䡢
 * �ͷš�
 */
class FileSystem
{
public:
	static const int SUPER_BLOCK_SECTOR_NUMBER = 0;		/* ����SuperBlockλ�ڴ����ϵ������ţ�ռ��0��1�������������޸ġ� */

	static const int ROOTINO = 1;						/* �ļ�ϵͳ��Ŀ¼���Inode��š��޸ġ� */

	static const int INODE_NUMBER_PER_SECTOR = 8;		/* ���INode���󳤶�Ϊ64�ֽڣ�ÿ�����̿���Դ��512/64 = 8�����Inode */
	static const int INODE_ZONE_START_SECTOR = 2;		/* ���Inode��λ�ڴ����ϵ���ʼ�����š��޸ġ� */
	static const int INODE_ZONE_SIZE = 1024 - 2;		/* ���������Inode��ռ�ݵ����������޸ġ� */
	static const int INODE_SIZE = 64;					/*���Inode����ĳ���*/

	static const int DATA_ZONE_START_SECTOR = 1024;		/* ����������ʼ������ */
	static const int DATA_ZONE_END_SECTOR = 18000 - 1;	/* �������Ľ��������� */
	static const int DATA_ZONE_SIZE = 18000 - DATA_ZONE_START_SECTOR;	/* ������ռ�ݵ��������� */

	/* Functions */
public:
	/* Constructors */
	FileSystem();
	/* Destructors */
	~FileSystem();

	/*
	 * @comment ��ʼ����Ա����
	 */
	void Initialize();

	/*
	* @comment ϵͳ��ʼ��ʱ����SuperBlock
	*/
	void LoadSuperBlock();

	/*
	 * @comment ��SuperBlock������ڴ渱�����µ�
	 * �洢�豸��SuperBlock��ȥ
	 */
	void Update();

	/*
	 * @comment  �ڴ洢�豸dev�Ϸ���һ������
	 * ���INode��һ�����ڴ����µ��ļ���
	 */
	Inode* IAlloc();

	/*
	 * @comment  �ͷŴ洢�豸dev�ϱ��Ϊnumber
	 * �����INode��һ������ɾ���ļ���
	 */
	void IFree(int number);

	/*
	 * @comment �ڴ洢�豸dev�Ϸ�����д��̿�
	 */
	Buf* Alloc();

	/*
	 * @comment �ͷŴ洢�豸dev�ϱ��Ϊblkno�Ĵ��̿�
	 */
	void Free(int blkno);

	SuperBlock* GetSuperBlock();

private:
	BufferManager* m_BufferManager;		/* FileSystem����Ҫ�������ģ��(BufferManager)�ṩ�Ľӿ� */

	SuperBlock* m_SuperBlock;			/* ָ��ȫ��SuperBlock */
};

extern FileSystem g_FileSystem;			/*  �����ȫ�ֱ������������ļ�ϵͳ���� */

#endif