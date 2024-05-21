#ifndef USER_H
#define USER_H

#include "Inode.h"
#include "FileSystem.h"
#include "FileManager.h"
#include "File.h"
//// added
//#include "UI.h"

/* 
	User 类存储有关于用户的相关信息，包括用户 id、用户组 id。
	User 类也负责存储有关于当前工作路径的信息，包括指向当前目录的 Inode 指针、
	指向父目录的 Inode 指针、当前目录的目录项、当前路径分量。
	基本所有 API 都要用到有关于路径的信息， 所以使用 User 类提供支持。
*/

/*
 * @comment 该类与Unixv6中 struct user结构对应，因此只改变
 * 类名，不修改成员结构名字，关于数据类型的对应关系如下:
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
	/* 系统调用相关成员 */
	int u_uid;			/* 有效用户ID */
	int u_gid;			/* 有效组ID */
	int u_ar0;			/* 存放系统调用返回值 */
	int u_arg[5];		/* 存放当前系统调用参数 */
	char* u_dirp;		/* 系统调用参数(一般用于Pathname)的指针 */

	/* 文件系统相关成员 */
	Inode* u_cdir;		/* 指向当前目录的Inode指针 */
	Inode* u_pdir;		/* 指向父目录的Inode指针 */

	DirectoryEntry u_dent;					/* 当前目录的目录项 */
	char u_dbuf[DirectoryEntry::DIRSIZ];	/* 当前路径分量 */
	char u_curdir[128];						/* 当前工作目录完整路径 */

	ErrorCode u_error;			/* 存放错误码 */

	// int userRead(char* fd, int len);


	/* 文件系统相关成员 */
	OpenFiles u_ofiles;		/* 进程打开文件描述符表对象 */

	/* 文件I/O操作 */
	IOParameter u_IOParam;	/* 记录当前读、写文件的偏移量，用户目标区域和剩余字节数参数 */
};

#endif