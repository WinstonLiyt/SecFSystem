#ifndef USER_H
#define USER_H

#include "Inode.h"
#include "FileSystem.h"
#include "FileManager.h"
#include "File.h"
//// added
//#include "UI.h"

/* 
	User ��洢�й����û��������Ϣ�������û� id���û��� id��
	User ��Ҳ����洢�й��ڵ�ǰ����·������Ϣ������ָ��ǰĿ¼�� Inode ָ�롢
	ָ��Ŀ¼�� Inode ָ�롢��ǰĿ¼��Ŀ¼���ǰ·��������
	�������� API ��Ҫ�õ��й���·������Ϣ�� ����ʹ�� User ���ṩ֧�֡�
*/

/*
 * @comment ������Unixv6�� struct user�ṹ��Ӧ�����ֻ�ı�
 * ���������޸ĳ�Ա�ṹ���֣������������͵Ķ�Ӧ��ϵ����:
 */

class User
{
public:
	enum ErrorCode
	{
		noerror = 0,	/* No error */
		eperm = 1,	/* Operation not permitted */
		enoent = 2,	/* No such file or directory */
		esrch = 3,	/* No such process */
		eintr = 4,	/* Interrupted system call */
		eio = 5,	/* I/O error */
		enxio = 6,	/* No such device or address */
		e2big = 7,	/* Arg list too long */
		enoexec = 8,	/* Exec format error */
		ebadf = 9,	/* Bad file number */
		echild = 10,	/* No child processes */
		eagain = 11,	/* Try again */
		enomem = 12,	/* Out of memory */
		eacces = 13,	/* Permission denied */
		efault = 14,	/* Bad address */
		enotblk = 15,	/* Block device required */
		ebusy = 16,	/* Device or resource busy */
		eexist = 17,	/* File exists */
		exdev = 18,	/* Cross-device link */
		enodev = 19,	/* No such device */
		enotdir = 20,	/* Not a directory */
		eisdir = 21,	/* Is a directory */
		einval = 22,	/* Invalid argument */
		enfile = 23,	/* File table overflow */
		emfile = 24,	/* Too many open files */
		enotty = 25,	/* Not a typewriter(terminal) */
		etxtbsy = 26,	/* Text file busy */
		efbig = 27,	/* File too large */
		enospc = 28,	/* No space left on device */
		espipe = 29,	/* Illegal seek */
		erofs = 30,	/* Read-only file system */
		emlink = 31,	/* Too many links */
		epipe = 32,	/* Broken pipe */
		enosys = 100,
	};
public:
	/* ϵͳ������س�Ա */
	int u_uid;			/* ��Ч�û�ID */
	int u_gid;			/* ��Ч��ID */
	int u_ar0;			/* ���ϵͳ���÷���ֵ */
	int u_arg[5];		/* ��ŵ�ǰϵͳ���ò��� */
	char* u_dirp;		/* ϵͳ���ò���(һ������Pathname)��ָ�� */

	/* �ļ�ϵͳ��س�Ա */
	Inode* u_cdir;		/* ָ��ǰĿ¼��Inodeָ�� */
	Inode* u_pdir;		/* ָ��Ŀ¼��Inodeָ�� */

	DirectoryEntry u_dent;					/* ��ǰĿ¼��Ŀ¼�� */
	char u_dbuf[DirectoryEntry::DIRSIZ];	/* ��ǰ·������ */
	char u_curdir[128];						/* ��ǰ����Ŀ¼����·�� */

	ErrorCode u_error;			/* ��Ŵ����� */

	// int userRead(char* fd, int len);


	/* �ļ�ϵͳ��س�Ա */
	OpenFiles u_ofiles;		/* ���̴��ļ������������ */

	/* �ļ�I/O���� */
	IOParameter u_IOParam;	/* ��¼��ǰ����д�ļ���ƫ�������û�Ŀ�������ʣ���ֽ������� */
};

#endif