#include"DeviceDriver.h"
#include"FileSystem.h"
#include"FileManager.h"
#include"Utility.h"
#include"Inode.h"
#include<cstdio>
#include<time.h>


const char* DeviceDriver::disk_name = "ytli.img";
const char* DeviceDriver::shared_name = "ytli";


//ȫ��DeviceDriver
DeviceDriver g_DeviceDriver;

DeviceDriver::DeviceDriver()
{

}

DeviceDriver::~DeviceDriver()
{

}

void DeviceDriver::Initialize()
{
	if (_access(disk_name, 0))	//������̲�����
	{
		cout << "���̲����ڣ����ڴ�������..." << endl;
		format();
		cout << "���̴����ɹ�" << endl;
	}
}

/*==============================class DeviceDriver===================================*/
void DeviceDriver::format()
{
	char* now_disk_cur;
	remove(disk_name);
	MapView disk_mapview = Utility::get_mapview(disk_name, shared_name);//���ӳ����ͼ
	SuperBlock sb;
	FileSystem fs;
	sb.s_isize = fs.INODE_ZONE_SIZE;//���inode��������
	sb.s_fsize = fs.DATA_ZONE_END_SECTOR + 1;//�̿�����
	sb.s_ninode = 100;
	int top_inode = 6 + 2;//���ڳ�ʼ��5��Ŀ¼�ļ��������û������ļ������Դ�8#inode��ʼ����
	for (int i = 99; i >= 0; i--)
	{
		sb.s_inode[i] = top_inode;
		top_inode++;
	}
	sb.s_flock = 0;	/* ���������̿��������־ */
	sb.s_ilock = 0;	/* ��������Inode���־ */
	sb.s_fmod = 0;	/* �ڴ���super block�������޸ı�־����ζ����Ҫ��������Ӧ��Super Block */
	sb.s_ronly = 0;	/* ���ļ�ϵͳֻ�ܶ��� */
	sb.s_time = time(0);	/* ���һ�θ���ʱ�� */
	sb.s_nfree = 0;	//һ��ʼû�п��п�

	//�����һ����������ʼ������̿������������п����̿鶼����һ��
	int data_block_cur = fs.DATA_ZONE_END_SECTOR;
	while (data_block_cur >= fs.DATA_ZONE_START_SECTOR + 5 + 2)
	{
		//s_free����
		if (sb.s_nfree == 100)
		{
			//�����Ҫ����s_free������̿����ʵ��ַ
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

	//д��superblock
	now_disk_cur = disk_mapview.disk_base_address;
	memcpy(now_disk_cur, &sb, sizeof(sb));

	/*��ʽ����Ŀ¼,д��1#inode��1024#���ݿ�*/
	DiskInode* DI = new(nothrow) DiskInode;
	if (!DI)
	{
		cout << "�ڴ�����ʧ��" << endl;
		exit(100);
	}
	DI->d_mode = (1 << 14) | (0b111111111);	//Ŀ¼�ļ�
	DI->d_nlink = 1;	//�ļ�����ֻ��������һ��
	DI->d_uid = 0;	//0��ʾ�����û�
	DI->d_gid = 0;
	DI->d_size = 6 * 32;	//6��Ŀ¼�ÿ��Ŀ¼��32�ֽ�
	DI->d_addr[0] = 1024;	//д��1024#���ݿ�
	DI->d_atime = time(0);	//������ʱ��
	DI->d_mtime = DI->d_atime;	//����޸�ʱ��
	//д���Ŀ¼��DiskInode
	now_disk_cur = disk_mapview.disk_base_address + fs.INODE_ZONE_START_SECTOR * BLOCK_SIZE + 1 * fs.INODE_SIZE;
	memcpy(now_disk_cur, DI, sizeof(DiskInode));
	delete DI;
	//int all_inode_num[7] = { 0,1,2,3,4,5,6 };	//��ʽ��ʱ�漰��������inode��
	const char* all_dir_name[] = { ".","..","bin","etc","home","dev" };	//��ʽ��ʱ�漰��������Ŀ¼����
	//д���Ŀ¼�����ݿ�
	now_disk_cur = disk_mapview.disk_base_address + fs.DATA_ZONE_START_SECTOR * BLOCK_SIZE;	//1024����ʼ��ַ
	DirectoryEntry DE0[6];
	DE0[0].m_ino = 1;	strcpy_s(DE0[0].m_name, all_dir_name[0]);	//1	.\0
	DE0[1].m_ino = 1;	strcpy_s(DE0[1].m_name, all_dir_name[1]);	//1 ..\0
	DE0[2].m_ino = 2;	strcpy_s(DE0[2].m_name, all_dir_name[2]);	//2	bin\0
	DE0[3].m_ino = 3;	strcpy_s(DE0[3].m_name, all_dir_name[3]);	//3	etc\0
	DE0[4].m_ino = 4;	strcpy_s(DE0[4].m_name, all_dir_name[4]);	//4 home\0
	DE0[5].m_ino = 5;	strcpy_s(DE0[5].m_name, all_dir_name[5]);	//5	dev\0
	memcpy(now_disk_cur, DE0, sizeof(DE0));

	/*��ʽ��bin*/
	DI = new(nothrow) DiskInode;
	if (!DI)
	{
		cout << "�ڴ�����ʧ��" << endl;
		exit(101);
	}
	DI->d_mode = (1 << 14) | 0b111111111;	//Ŀ¼�ļ�
	DI->d_nlink = 1;	//�ļ�����ֻ��������һ��
	DI->d_uid = 0;	//0��ʾ�����û�
	DI->d_gid = 0;
	DI->d_size = 2 * 32;	//2��Ŀ¼�ÿ��Ŀ¼��32�ֽ�
	DI->d_addr[0] = 1025;	//д��1025#���ݿ�
	DI->d_atime = time(0);	//������ʱ��
	DI->d_mtime = DI->d_atime;	//����޸�ʱ��
	//д��bin��DiskInode
	now_disk_cur = disk_mapview.disk_base_address + fs.INODE_ZONE_START_SECTOR * BLOCK_SIZE + 2 * fs.INODE_SIZE;
	memcpy(now_disk_cur, DI, sizeof(DiskInode));
	delete DI;
	//д��bin�����ݿ�
	now_disk_cur = disk_mapview.disk_base_address + (fs.DATA_ZONE_START_SECTOR + 1) * BLOCK_SIZE;
	DirectoryEntry DE1[2];
	DE1[0].m_ino = 2;	strcpy_s(DE1[0].m_name, all_dir_name[0]);	//2	.\0
	DE1[1].m_ino = 1;	strcpy_s(DE1[1].m_name, all_dir_name[1]);	//1	..\0
	memcpy(now_disk_cur, DE1, sizeof(DE1));

	/*��ʽ��etc*/
	DI = new(nothrow) DiskInode;
	if (!DI)
	{
		cout << "�ڴ�����ʧ��" << endl;
		exit(102);
	}
	DI->d_mode = (1 << 14) | 0b111111111;	//Ŀ¼�ļ�
	DI->d_nlink = 1;	//�ļ�����ֻ��������һ��
	DI->d_uid = 0;	//0��ʾ�����û�
	DI->d_gid = 0;
	DI->d_size = 4 * 32;	//2��Ŀ¼�ÿ��Ŀ¼��32�ֽ�
	DI->d_addr[0] = 1026;	//д��1026#���ݿ�
	DI->d_atime = time(0);	//������ʱ��
	DI->d_mtime = DI->d_atime;	//����޸�ʱ��
	//д��etc��DiskInode
	now_disk_cur = disk_mapview.disk_base_address + fs.INODE_ZONE_START_SECTOR * BLOCK_SIZE + 3 * fs.INODE_SIZE;
	memcpy(now_disk_cur, DI, sizeof(DiskInode));
	delete DI;
	//д��etc�����ݿ�
	now_disk_cur = disk_mapview.disk_base_address + (fs.DATA_ZONE_START_SECTOR + 2) * BLOCK_SIZE;
	DirectoryEntry DE2[4];
	DE2[0].m_ino = 3;	strcpy_s(DE2[0].m_name, all_dir_name[0]);	//3	.\0
	DE2[1].m_ino = 1;	strcpy_s(DE2[1].m_name, all_dir_name[1]);	//1	..\0
	DE2[2].m_ino = 6;	strcpy_s(DE2[2].m_name, "user.txt");	//6 user.txt\0
	DE2[3].m_ino = 7;	strcpy_s(DE2[3].m_name, "group.txt");	//7 group.txt\0
	memcpy(now_disk_cur, DE2, sizeof(DE2));

	/*��ʽ��home*/
	DI = new(nothrow) DiskInode;
	if (!DI)
	{
		cout << "�ڴ�����ʧ��" << endl;
		exit(103);
	}
	DI->d_mode = (1 << 14) | 0b111111111;	//Ŀ¼�ļ�
	DI->d_nlink = 1;	//�ļ�����ֻ��������һ��
	DI->d_uid = 0;	//0��ʾ�����û�
	DI->d_gid = 0;
	DI->d_size = 2 * 32;	//2��Ŀ¼�ÿ��Ŀ¼��32�ֽ�
	DI->d_addr[0] = 1027;	//д��1027#���ݿ�
	DI->d_atime = time(0);	//������ʱ��
	DI->d_mtime = DI->d_atime;	//����޸�ʱ��
	//д��home��DiskInode
	now_disk_cur = disk_mapview.disk_base_address + fs.INODE_ZONE_START_SECTOR * BLOCK_SIZE + 4 * fs.INODE_SIZE;
	memcpy(now_disk_cur, DI, sizeof(DiskInode));
	delete DI;
	//д��home�����ݿ�
	now_disk_cur = disk_mapview.disk_base_address + (fs.DATA_ZONE_START_SECTOR + 3) * BLOCK_SIZE;
	DirectoryEntry DE3[2];
	DE3[0].m_ino = 4;	strcpy_s(DE3[0].m_name, all_dir_name[0]);	//4	.\0
	DE3[1].m_ino = 1;	strcpy_s(DE3[1].m_name, all_dir_name[1]);	//1	..\0
	memcpy(now_disk_cur, DE3, sizeof(DE3));

	/*��ʽ��dev*/
	DI = new(nothrow) DiskInode;
	if (!DI)
	{
		cout << "�ڴ�����ʧ��" << endl;
		exit(104);
	}
	DI->d_mode = (1 << 14) | 0b111111111;	//Ŀ¼�ļ�
	DI->d_nlink = 1;	//�ļ�����ֻ��������һ��
	DI->d_uid = 0;	//0��ʾ�����û�
	DI->d_gid = 0;
	DI->d_size = 2 * 32;	//2��Ŀ¼�ÿ��Ŀ¼��32�ֽ�
	DI->d_addr[0] = 1028;	//д��1028#���ݿ�
	DI->d_atime = time(0);	//������ʱ��
	DI->d_mtime = DI->d_atime;	//����޸�ʱ��
	//д��dev��DiskInode
	now_disk_cur = disk_mapview.disk_base_address + fs.INODE_ZONE_START_SECTOR * BLOCK_SIZE + 5 * fs.INODE_SIZE;
	memcpy(now_disk_cur, DI, sizeof(DiskInode));
	delete DI;
	//д��dev�����ݿ�
	now_disk_cur = disk_mapview.disk_base_address + (fs.DATA_ZONE_START_SECTOR + 4) * BLOCK_SIZE;
	DirectoryEntry DE4[2];
	DE4[0].m_ino = 5;	strcpy_s(DE4[0].m_name, all_dir_name[0]);	//5	.\0
	DE4[1].m_ino = 1;	strcpy_s(DE4[1].m_name, all_dir_name[1]);	//1	..\0
	memcpy(now_disk_cur, DE4, sizeof(DE4));

	/*��ʼ��user.txt*/
	const char* root_log = "root-1-0-root_group-1\n";
	DI = new(nothrow) DiskInode;
	if (!DI)
	{
		cout << "�ڴ�����ʧ��" << endl;
		exit(104);
	}
	DI->d_mode = 0b111111111;	//��ͨ�ļ�
	DI->d_nlink = 1;	//�ļ�����ֻ��������һ��
	DI->d_uid = 0;	//0��ʾ�����û�
	DI->d_gid = 0;
	DI->d_size = strlen(root_log);	//root_log�ĳ���
	DI->d_addr[0] = 1029;	//д��1029#���ݿ�
	DI->d_atime = time(0);	//������ʱ��
	DI->d_mtime = DI->d_atime;	//����޸�ʱ��
	now_disk_cur = disk_mapview.disk_base_address + fs.INODE_ZONE_START_SECTOR * BLOCK_SIZE + 6 * fs.INODE_SIZE;
	memcpy(now_disk_cur, DI, sizeof(DiskInode));
	delete DI;
	now_disk_cur = disk_mapview.disk_base_address + (fs.DATA_ZONE_START_SECTOR + 5) * BLOCK_SIZE;
	memcpy(now_disk_cur, root_log, strlen(root_log));

	/*��ʼ��group.txt*/
	const char* root_group = "root_group-1\n";
	DI = new(nothrow) DiskInode;
	if (!DI)
	{
		cout << "�ڴ�����ʧ��" << endl;
		exit(104);
	}
	DI->d_mode = 0b111111111;	//��ͨ�ļ�
	DI->d_nlink = 1;	//�ļ�����ֻ��������һ��
	DI->d_uid = 0;	//0��ʾ�����û�
	DI->d_gid = 0;
	DI->d_size = strlen(root_group);	//root_log�ĳ���
	DI->d_addr[0] = 1030;	//д��1030#���ݿ�
	DI->d_atime = time(0);	//������ʱ��
	DI->d_mtime = DI->d_atime;	//����޸�ʱ��
	now_disk_cur = disk_mapview.disk_base_address + fs.INODE_ZONE_START_SECTOR * BLOCK_SIZE + 7 * fs.INODE_SIZE;
	memcpy(now_disk_cur, DI, sizeof(DiskInode));
	delete DI;
	now_disk_cur = disk_mapview.disk_base_address + (fs.DATA_ZONE_START_SECTOR + 6) * BLOCK_SIZE;
	memcpy(now_disk_cur, root_group, strlen(root_group));

	Utility::unmap_mapview(disk_mapview);//ж��ӳ�䣬�ر��ļ�

}

//��ȡ��������
const char* DeviceDriver::GetDiskName()
{
	return this->disk_name;
}

//��ȡ�ļ�ӳ���������
const char* DeviceDriver::GetSharedName()
{
	return this->shared_name;
}

//���̵��ڴ�Ķ�д
void DeviceDriver::IO(Buf* bp)
{
	MapView disk_mapview = Utility::get_mapview(this->GetDiskName(), this->GetSharedName());
	char* disk_cur = disk_mapview.disk_base_address + bp->b_blkno * BLOCK_SIZE;

	//cout << "��ǰд��b_blknoΪ��" << bp->b_blkno << endl;

	if ((bp->b_flags & Buf::B_WRITE) && (bp->b_flags & Buf::B_READ))
	{
		cout << "��Ӧ�ô������ܶ�����д��Buf" << endl;
		exit(11);
	}
	if (bp->b_flags & Buf::B_WRITE)//д����
	{
		map_write(disk_cur, bp->b_addr, (size_t)BLOCK_SIZE);
	}
	else if (bp->b_flags & Buf::B_READ)
	{
		map_read(disk_cur, bp->b_addr, BLOCK_SIZE);
	}
	Utility::unmap_mapview(disk_mapview);
}