#ifndef DEVICEDRIVER_H
#define DEVICEDRIVER_H

#include <iostream>
#include <assert.h>
#include <Windows.h>
#include <WinBase.h>
#include <io.h>
#include "Buf.h"

using namespace std;

#define BLOCK_SIZE 512		/* 数据块大小为512字节 */

/*
* 文件管理设备驱动类，主要处理磁盘格式化
*/
class DeviceDriver
{
public:
	static const char* disk_name;		/* 磁盘名字 */
	static const char* shared_name;		/* 文件映射对象名称 */

	/* Constructors */
	DeviceDriver();
	/* Destructors */
	~DeviceDriver();

	/* 设备驱动初始化 */
	void Initialize();

	/* 磁盘格式化 */
	void format();

	/* 获取磁盘名字 */
	const char* GetDiskName();

	/* 获取文件映射对象名称 */
	const char* GetSharedName();

	/* 处理缓存块bp的读写操作 */
	void IO(Buf* bp);
};

extern DeviceDriver g_DeviceDriver;		/* 全局设备驱动模块 */

#endif