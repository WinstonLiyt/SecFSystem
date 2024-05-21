#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include "Kernel.h"
#include <windows.h>
#include <limits>
#include <algorithm>

using namespace std;

#define ROOT_DIR_SYMBOL "~"
#define BUFFER_SIZE 512
#define USER_FILE_SIZE 5120

/* ������ɫ���� */
enum ConsoleColor {
	BLACK = 0,
	BLUE = 1,
	GREEN = 2,
	CYAN = 3,
	RED = 4,
	PURPLE = 5,
	DARK_YELLOW = 6,
	LIGHT_GRAY = 7,
	DARK_GRAY = 8,
	LIGHT_BLUE = 9,
	LIGHT_GREEN = 10,
	LIGHT_CYAN = 11,
	LIGHT_RED = 12,
	LIGHT_MAGENTA = 13,
	YELLOW = 14,
	WHITE = 15
};

/* ���ฺ������û����룬�������н����ʾ���û� */
class UI
{
private:
	string cur_user;	// ��ǰ���û�
	string cur_group;	// ��ǰ����
	string cur_host;	// ��ǰ������
	string cur_dir;		// ��ǰ��·��
	int cur_uid;		// ��ǰ���û�id
	int cur_gid;		// ��ǰ����id
	bool is_login;		// Ŀǰ�Ƿ��¼
	int fd;

public:
	UI();
	~UI();
	void InitSystem();
	void run();
	void UI_help();
	void UI_format();
	void UI_cls();

	int Fopen(char* name, int mode);
	int Fcreat(char* name, int mode);
	int Fread(int fd, char* buffer, int length);
	int Fwrite(int fd, char* buffer, int length);
	void Fdelete(char* name);
	int Flseek(int fd, int position, int whence);
	void Fclose(int fd);
	void Ls();
	void Mkdir(char* name);
	void Cd(char* name);
	void FRead(char* name, int length);
	void FWrite(char* name, int length);
	void FLseek(char* name, int length, int type);
	void Fin(char* extername, char* intername);
	void Fout(char* extername, char* intername);
	void FOper(char* name);
	void Login(string user_name,string password);
	void Logout();
	void adduser(string user_name,string group_name,string password);
	void addgroup(string group_name);
	void deluser(string user_name);
	void delgroup(string group_name);
	void showusers();
	void showgroups();
};