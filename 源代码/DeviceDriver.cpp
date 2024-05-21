#include"DeviceDriver.h"
#include"FileSystem.h"
#include"FileManager.h"
#include"Utility.h"
#include"Inode.h"
#include<cstdio>
#include<time.h>


const char* DeviceDriver::disk_name = "ytli.img";
const char* DeviceDriver::shared_name = "ytli";


//全局DeviceDriver
DeviceDriver g_DeviceDriver;

DeviceDriver::DeviceDriver()
{

}

DeviceDriver::~DeviceDriver()
{

}

void DeviceDriver::Initialize()
{
	if (_access(disk_name, 0))	//如果磁盘不存在
	{
		cout << "磁盘不存在，正在创建磁盘..." << endl;
		format();
		cout << "磁盘创建成功" << endl;
	}
}

/*==============================class DeviceDriver===================================*/
void DeviceDriver::format()
{
	char* now_disk_cur;
	remove(disk_name);
	MapView disk_mapview = Utility::get_mapview(disk_name, shared_name);//获得映射视图
	SuperBlock sb;
	FileSystem fs;
	sb.s_isize = fs.INODE_ZONE_SIZE;//外存inode的扇区数
	sb.s_fsize = fs.DATA_ZONE_END_SECTOR + 1;//盘块总数
	sb.s_ninode = 100;
	int top_inode = 6 + 2;//由于初始有5个目录文件和两个用户管理文件，所以从8#inode开始空闲
	for (int i = 99; i >= 0; i--)
	{
		sb.s_inode[i] = top_inode;
		top_inode++;
	}
	sb.s_flock = 0;	/* 封锁空闲盘块索引表标志 */
	sb.s_ilock = 0;	/* 封锁空闲Inode表标志 */
	sb.s_fmod = 0;	/* 内存中super block副本被修改标志，意味着需要更新外存对应的Super Block */
	sb.s_ronly = 0;	/* 本文件系统只能读出 */
	sb.s_time = time(0);	/* 最近一次更新时间 */
	sb.s_nfree = 0;	//一开始没有空闲块

	//从最后一块数据区开始填空闲盘块索引表，把所有空闲盘块都回收一遍
	int data_block_cur = fs.DATA_ZONE_END_SECTOR;
	while (data_block_cur >= fs.DATA_ZONE_START_SECTOR + 5 + 2)
	{
		//s_free满了
		if (sb.s_nfree == 100)
		{
			//获得正要放入s_free的这块盘块的其实地址
			char* now_disk_cur = disk_mapview.disk_base_address + data_block_cur * BLOCK_SIZE;

			memcpy(now_disk_cur, &sb.s_nfree, sizeof(sb.s_nfree));
			now_disk_cur += sizeof(sb.s_nfree);

			for (int i = 0; i < 100; i++)
			{
				memcpy(now_disk_cur, &sb.s_free[i], sizeof(sb.s_free[i]));
				now_disk_cur += sizeof(sb.s_free[i]);
			}
			sb.s_nfree = 0;
		}
		sb.s_free[sb.s_nfree] = data_block_cur;
		sb.s_nfree++;
		data_block_cur--;
	}

	//写入superblock
	now_disk_cur = disk_mapview.disk_base_address;
	memcpy(now_disk_cur, &sb, sizeof(sb));

	/*格式化根目录,写在1#inode，1024#数据块*/
	DiskInode* DI = new(nothrow) DiskInode;
	if (!DI)
	{
		cout << "内存申请失败" << endl;
		exit(100);
	}
	DI->d_mode = (1 << 14) | (0b111111111);	//目录文件
	DI->d_nlink = 1;	//文件联结只有现在这一个
	DI->d_uid = 0;	//0表示超级用户
	DI->d_gid = 0;
	DI->d_size = 6 * 32;	//6个目录项，每个目录项32字节
	DI->d_addr[0] = 1024;	//写在1024#数据块
	DI->d_atime = time(0);	//最后访问时间
	DI->d_mtime = DI->d_atime;	//最后修改时间
	//写入根目录的DiskInode
	now_disk_cur = disk_mapview.disk_base_address + fs.INODE_ZONE_START_SECTOR * BLOCK_SIZE + 1 * fs.INODE_SIZE;
	memcpy(now_disk_cur, DI, sizeof(DiskInode));
	delete DI;
	//int all_inode_num[7] = { 0,1,2,3,4,5,6 };	//格式化时涉及到的所有inode号
	const char* all_dir_name[] = { ".","..","bin","etc","home","dev" };	//格式化时涉及到的所有目录项名
	//写入根目录的数据块
	now_disk_cur = disk_mapview.disk_base_address + fs.DATA_ZONE_START_SECTOR * BLOCK_SIZE;	//1024块起始地址
	DirectoryEntry DE0[6];
	DE0[0].m_ino = 1;	strcpy_s(DE0[0].m_name, all_dir_name[0]);	//1	.\0
	DE0[1].m_ino = 1;	strcpy_s(DE0[1].m_name, all_dir_name[1]);	//1 ..\0
	DE0[2].m_ino = 2;	strcpy_s(DE0[2].m_name, all_dir_name[2]);	//2	bin\0
	DE0[3].m_ino = 3;	strcpy_s(DE0[3].m_name, all_dir_name[3]);	//3	etc\0
	DE0[4].m_ino = 4;	strcpy_s(DE0[4].m_name, all_dir_name[4]);	//4 home\0
	DE0[5].m_ino = 5;	strcpy_s(DE0[5].m_name, all_dir_name[5]);	//5	dev\0
	memcpy(now_disk_cur, DE0, sizeof(DE0));

	/*格式化bin*/
	DI = new(nothrow) DiskInode;
	if (!DI)
	{
		cout << "内存申请失败" << endl;
		exit(101);
	}
	DI->d_mode = (1 << 14) | 0b111111111;	//目录文件
	DI->d_nlink = 1;	//文件联结只有现在这一个
	DI->d_uid = 0;	//0表示超级用户
	DI->d_gid = 0;
	DI->d_size = 2 * 32;	//2个目录项，每个目录项32字节
	DI->d_addr[0] = 1025;	//写在1025#数据块
	DI->d_atime = time(0);	//最后访问时间
	DI->d_mtime = DI->d_atime;	//最后修改时间
	//写入bin的DiskInode
	now_disk_cur = disk_mapview.disk_base_address + fs.INODE_ZONE_START_SECTOR * BLOCK_SIZE + 2 * fs.INODE_SIZE;
	memcpy(now_disk_cur, DI, sizeof(DiskInode));
	delete DI;
	//写入bin的数据块
	now_disk_cur = disk_mapview.disk_base_address + (fs.DATA_ZONE_START_SECTOR + 1) * BLOCK_SIZE;
	DirectoryEntry DE1[2];
	DE1[0].m_ino = 2;	strcpy_s(DE1[0].m_name, all_dir_name[0]);	//2	.\0
	DE1[1].m_ino = 1;	strcpy_s(DE1[1].m_name, all_dir_name[1]);	//1	..\0
	memcpy(now_disk_cur, DE1, sizeof(DE1));

	/*格式化etc*/
	DI = new(nothrow) DiskInode;
	if (!DI)
	{
		cout << "内存申请失败" << endl;
		exit(102);
	}
	DI->d_mode = (1 << 14) | 0b111111111;	//目录文件
	DI->d_nlink = 1;	//文件联结只有现在这一个
	DI->d_uid = 0;	//0表示超级用户
	DI->d_gid = 0;
	DI->d_size = 4 * 32;	//2个目录项，每个目录项32字节
	DI->d_addr[0] = 1026;	//写在1026#数据块
	DI->d_atime = time(0);	//最后访问时间
	DI->d_mtime = DI->d_atime;	//最后修改时间
	//写入etc的DiskInode
	now_disk_cur = disk_mapview.disk_base_address + fs.INODE_ZONE_START_SECTOR * BLOCK_SIZE + 3 * fs.INODE_SIZE;
	memcpy(now_disk_cur, DI, sizeof(DiskInode));
	delete DI;
	//写入etc的数据块
	now_disk_cur = disk_mapview.disk_base_address + (fs.DATA_ZONE_START_SECTOR + 2) * BLOCK_SIZE;
	DirectoryEntry DE2[4];
	DE2[0].m_ino = 3;	strcpy_s(DE2[0].m_name, all_dir_name[0]);	//3	.\0
	DE2[1].m_ino = 1;	strcpy_s(DE2[1].m_name, all_dir_name[1]);	//1	..\0
	DE2[2].m_ino = 6;	strcpy_s(DE2[2].m_name, "user.txt");	//6 user.txt\0
	DE2[3].m_ino = 7;	strcpy_s(DE2[3].m_name, "group.txt");	//7 group.txt\0
	memcpy(now_disk_cur, DE2, sizeof(DE2));

	/*格式化home*/
	DI = new(nothrow) DiskInode;
	if (!DI)
	{
		cout << "内存申请失败" << endl;
		exit(103);
	}
	DI->d_mode = (1 << 14) | 0b111111111;	//目录文件
	DI->d_nlink = 1;	//文件联结只有现在这一个
	DI->d_uid = 0;	//0表示超级用户
	DI->d_gid = 0;
	DI->d_size = 2 * 32;	//2个目录项，每个目录项32字节
	DI->d_addr[0] = 1027;	//写在1027#数据块
	DI->d_atime = time(0);	//最后访问时间
	DI->d_mtime = DI->d_atime;	//最后修改时间
	//写入home的DiskInode
	now_disk_cur = disk_mapview.disk_base_address + fs.INODE_ZONE_START_SECTOR * BLOCK_SIZE + 4 * fs.INODE_SIZE;
	memcpy(now_disk_cur, DI, sizeof(DiskInode));
	delete DI;
	//写入home的数据块
	now_disk_cur = disk_mapview.disk_base_address + (fs.DATA_ZONE_START_SECTOR + 3) * BLOCK_SIZE;
	DirectoryEntry DE3[2];
	DE3[0].m_ino = 4;	strcpy_s(DE3[0].m_name, all_dir_name[0]);	//4	.\0
	DE3[1].m_ino = 1;	strcpy_s(DE3[1].m_name, all_dir_name[1]);	//1	..\0
	memcpy(now_disk_cur, DE3, sizeof(DE3));

	/*格式化dev*/
	DI = new(nothrow) DiskInode;
	if (!DI)
	{
		cout << "内存申请失败" << endl;
		exit(104);
	}
	DI->d_mode = (1 << 14) | 0b111111111;	//目录文件
	DI->d_nlink = 1;	//文件联结只有现在这一个
	DI->d_uid = 0;	//0表示超级用户
	DI->d_gid = 0;
	DI->d_size = 2 * 32;	//2个目录项，每个目录项32字节
	DI->d_addr[0] = 1028;	//写在1028#数据块
	DI->d_atime = time(0);	//最后访问时间
	DI->d_mtime = DI->d_atime;	//最后修改时间
	//写入dev的DiskInode
	now_disk_cur = disk_mapview.disk_base_address + fs.INODE_ZONE_START_SECTOR * BLOCK_SIZE + 5 * fs.INODE_SIZE;
	memcpy(now_disk_cur, DI, sizeof(DiskInode));
	delete DI;
	//写入dev的数据块
	now_disk_cur = disk_mapview.disk_base_address + (fs.DATA_ZONE_START_SECTOR + 4) * BLOCK_SIZE;
	DirectoryEntry DE4[2];
	DE4[0].m_ino = 5;	strcpy_s(DE4[0].m_name, all_dir_name[0]);	//5	.\0
	DE4[1].m_ino = 1;	strcpy_s(DE4[1].m_name, all_dir_name[1]);	//1	..\0
	memcpy(now_disk_cur, DE4, sizeof(DE4));

	/*初始化user.txt*/
	const char* root_log = "root-1-0-root_group-1\n";
	DI = new(nothrow) DiskInode;
	if (!DI)
	{
		cout << "内存申请失败" << endl;
		exit(104);
	}
	DI->d_mode = 0b111111111;	//普通文件
	DI->d_nlink = 1;	//文件联结只有现在这一个
	DI->d_uid = 0;	//0表示超级用户
	DI->d_gid = 0;
	DI->d_size = strlen(root_log);	//root_log的长度
	DI->d_addr[0] = 1029;	//写在1029#数据块
	DI->d_atime = time(0);	//最后访问时间
	DI->d_mtime = DI->d_atime;	//最后修改时间
	now_disk_cur = disk_mapview.disk_base_address + fs.INODE_ZONE_START_SECTOR * BLOCK_SIZE + 6 * fs.INODE_SIZE;
	memcpy(now_disk_cur, DI, sizeof(DiskInode));
	delete DI;
	now_disk_cur = disk_mapview.disk_base_address + (fs.DATA_ZONE_START_SECTOR + 5) * BLOCK_SIZE;
	memcpy(now_disk_cur, root_log, strlen(root_log));

	/*初始化group.txt*/
	const char* root_group = "root_group-1\n";
	DI = new(nothrow) DiskInode;
	if (!DI)
	{
		cout << "内存申请失败" << endl;
		exit(104);
	}
	DI->d_mode = 0b111111111;	//普通文件
	DI->d_nlink = 1;	//文件联结只有现在这一个
	DI->d_uid = 0;	//0表示超级用户
	DI->d_gid = 0;
	DI->d_size = strlen(root_group);	//root_log的长度
	DI->d_addr[0] = 1030;	//写在1030#数据块
	DI->d_atime = time(0);	//最后访问时间
	DI->d_mtime = DI->d_atime;	//最后修改时间
	now_disk_cur = disk_mapview.disk_base_address + fs.INODE_ZONE_START_SECTOR * BLOCK_SIZE + 7 * fs.INODE_SIZE;
	memcpy(now_disk_cur, DI, sizeof(DiskInode));
	delete DI;
	now_disk_cur = disk_mapview.disk_base_address + (fs.DATA_ZONE_START_SECTOR + 6) * BLOCK_SIZE;
	memcpy(now_disk_cur, root_group, strlen(root_group));

	Utility::unmap_mapview(disk_mapview);//卸载映射，关闭文件

}

//获取磁盘名字
const char* DeviceDriver::GetDiskName()
{
	return this->disk_name;
}

//获取文件映射对象名称
const char* DeviceDriver::GetSharedName()
{
	return this->shared_name;
}

//磁盘到内存的读写
void DeviceDriver::IO(Buf* bp)
{
	MapView disk_mapview = Utility::get_mapview(this->GetDiskName(), this->GetSharedName());
	char* disk_cur = disk_mapview.disk_base_address + bp->b_blkno * BLOCK_SIZE;

	//cout << "当前写入b_blkno为：" << bp->b_blkno << endl;

	if ((bp->b_flags & Buf::B_WRITE) && (bp->b_flags & Buf::B_READ))
	{
		cout << "不应该存在又能读又能写的Buf" << endl;
		exit(11);
	}
	if (bp->b_flags & Buf::B_WRITE)//写操作
	{
		map_write(disk_cur, bp->b_addr, (size_t)BLOCK_SIZE);
	}
	else if (bp->b_flags & Buf::B_READ)
	{
		map_read(disk_cur, bp->b_addr, BLOCK_SIZE);
	}
	Utility::unmap_mapview(disk_mapview);
}