#define _CRT_SECURE_NO_WARNINGS
#include "UI.h"
#include "Utility.h"
#include "Kernel.h"
#include "DeviceDriver.h"
#include <stdlib.h>
#include "FileManager.h";
#include <conio.h>
#include "OpenFileManager.h"


void SetColor(ConsoleColor color)
{
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(hConsole, color);
}

UI::UI()
{
	is_login = false;
	cur_uid = -1;
	cur_user = "";
	cur_gid = -1;
	cur_group = "";
	cur_dir = "";
	fd = -1;
	DWORD size = 0;
	wstring wstr;
	GetComputerName(NULL, &size); //得到电脑名称长度
	wchar_t* name = new wchar_t[size];
	if (GetComputerName(name, &size))
	{
		wstr = name;
	}
	delete[] name;
	cur_host = Utility::wstring2string(wstr);
}

UI::~UI()
{

}

void UI::InitSystem()
{
	Kernel::Instance().Initialize();
	Kernel::Instance().GetFileSystem().LoadSuperBlock();
}

void UI::run()
{
	InitSystem();

	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED);

	User& u = Kernel::Instance().GetUser();
	u.u_uid = -1;
	SetColor(LIGHT_GRAY);
	cout <<
		"\
________  ________													\n\
|   ____| |   ____>    Unix-Like Secondary File System Windows		\n\
|  |____  |  |____     Program Verson: " << VERSION << "			\n\
|   ____| |____   |    Completion Date: 2024-04-05					\n\
|  |	   ____|  |    https://github.com/WinstonLiyt/SecFSystem	\n\
|__|	  <_______|    @Tongji University CS LiYuante				\n\
\n";

	cout << "Welcome to the Unix-Like Secondary File System platform! \nFor guidance on how to use this system, please type 'help'." << endl;
	cout << endl;
	while (1)
	{
		/* 一条指令执行完之后，u_error信息就没用了，否则会出bug */
		u.u_error = User::noerror;

		/* 输入缓冲区 */
		string input_buf;
		/* 输入参数 */
		vector<string> args;

		/* CMD提示栏 */
		SetColor(LIGHT_GREEN);
		cout << endl << cur_user << "@" << cur_host;
		SetColor(PURPLE);
		cout << " CMD";
		SetColor(DARK_YELLOW);
		cout << " " << ROOT_DIR_SYMBOL << u.u_curdir << endl;
		SetColor(LIGHT_GRAY);
		cout << (cur_user == "root" ? "# " : "$ ");
		/* 获取用户输入 */
		getline(cin, input_buf);
		args = Utility::splitstr(input_buf, " ");
		if (args.size() <= 0)
			continue;
		else
		{
			/* help */
			if (args[0] == "help")
				UI_help();
			/* exit */
			else if (args[0] == "exit")
			{
				cout << "文件系统退出成功！" << endl;
				return;
			}
			/* fformat */
			else if (args[0] == "fformat")
			{
				if (!is_login)
				{
					cout << "仍未登录，请先登录！" << endl;
					continue;
				}
				cout << "确定要进行磁盘格式化吗？[y]，格式化将清空磁盘上所有数据：";
				string confrim;
				getline(cin, confrim);
				if (confrim == "y" || confrim == "Y")
				{
					UI_format();
					cout << "请重启系统！" << endl;
					return;
				}
				else
					cout << "取消格式化！" << endl;
			}
			/* cls */
			else if (args[0] == "cls")
			{
				UI_cls();
			}
			/* ls */
			else if (args[0] == "ls")
			{
				if (!is_login)
				{
					cout << "仍未登录，请先登录！" << endl;
					continue;
				}
				Ls();
			}
			/* cd */
			else if (args[0] == "cd" && args.size() == 2)
			{
				if (!is_login)
				{
					cout << "仍未登录，请先登录！" << endl;
					continue;
				}
				Cd((char*)args[1].c_str());
			}
			/* mkdir */
			else if (args[0] == "mkdir" && args.size() == 2)
			{
				if (!is_login)
				{
					cout << "仍未登录，请先登录！" << endl;
					continue;
				}
				Mkdir((char*)args[1].c_str());
			}
			/* fcreat */
			else if (args[0] == "fcreat" && args.size() == 2)
			{
				if (!is_login)
				{
					cout << "仍未登录，请先登录！" << endl;
					continue;
				}
				Fcreat((char*)args[1].c_str(), 0b111100000);
			}
			/* fopen */
			else if (args[0] == "fopen" && args.size() == 2)
			{
				if (!is_login)
				{
					cout << "仍未登录，请先登录！" << endl;
					continue;
				}
				User& u = Kernel::Instance().GetUser();
				fd = Fopen((char*)args[1].c_str(), File::FREAD | File::FWRITE);
			}
			/* fclose */
			else if (args[0] == "fclose" && args.size() == 2)
			{
				if (!is_login)
				{
					cout << "仍未登录，请先登录！" << endl;
					continue;
				}
				Fclose(fd);
			}
			/* fread */
			else if (args[0] == "fread" && args.size() == 3)
			{
				if (!is_login)
				{
					cout << "仍未登录，请先登录！" << endl;
					continue;
				}
				FRead((char*)args[1].c_str(), std::stoi(args[2]));
			}
			/* fwrite */
			else if (args[0] == "fwrite" && args.size() == 3)
			{
				if (!is_login)
				{
					cout << "仍未登录，请先登录！" << endl;
					continue;
				}
				FWrite((char*)args[1].c_str(), std::stoi(args[2]));
			}
			/* flseek */
			else if (args[0] == "flseek" && args.size() == 4)
			{
				if (!is_login)
				{
					cout << "仍未登录，请先登录！" << endl;
					continue;
				}
				FLseek((char*)args[1].c_str(), std::stoi(args[2]), std::stoi(args[3]));
			}
			/*fOper*/
			else if (args[0] == "foper" && args.size() == 2)
			{
				if (!is_login)
				{
					cout << "仍未登录，请先登录！" << endl;
					continue;
				}
				FOper((char*)args[1].c_str());
			}
			/* fin */
			else if (args[0] == "fin" && args.size() == 3)
			{
				if (!is_login)
				{
					cout << "仍未登录，请先登录！" << endl;
					continue;
				}
				Fin((char*)args[1].c_str(), (char*)args[2].c_str());
			}
			/* fout */
			else if (args[0] == "fout" && args.size() == 3)
			{
				if (!is_login)
				{
					cout << "仍未登录，请先登录！" << endl;
					continue;
				}
				Fout((char*)args[1].c_str(), (char*)args[2].c_str());
			}
			/* login */
			else if (args[0] == "login" && args.size() == 2)
			{
				cout << "password: ";
				char password[256] = { 0 };
				char ch = '\0';
				int pos = 0;
				while ((ch = _getch()) != '\r' && pos < 255)
				{
					if (ch != 8)//不是回撤就录入
					{
						password[pos] = ch;
						putchar('*');//并且输出*号
						pos++;
					}
					else
					{
						putchar('\b');//这里是删除一个，我们通过输出回撤符 /b，回撤一格，
						putchar(' ');//再显示空格符把刚才的*给盖住，
						putchar('\b');//然后再 回撤一格等待录入。
						pos--;
					}
				}
				password[pos] = '\0';
				putchar('\n');
				Login(args[1], string(password));
			}
			/* logout */
			else if (args[0] == "logout" && args.size() == 1)
			{
				if (!is_login)
				{
					cout << "仍未登录，请先登录！" << endl;
					continue;
				}
				Logout();
			}
			/* adduser*/
			else if (args[0] == "adduser" && args.size() == 3)
			{
				if (!is_login)
				{
					cout << "仍未登录，请先登录！" << endl;
					continue;
				}
				cout << "password: ";
				char password[256] = { 0 };
				char ch = '\0';
				int pos = 0;
				while ((ch = _getch()) != '\r' && pos < 255)
				{
					if (ch != 8)//不是回撤就录入
					{
						password[pos] = ch;
						putchar('*');//并且输出*号
						pos++;
					}
					else
					{
						putchar('\b');//这里是删除一个，我们通过输出回撤符 /b，回撤一格，
						putchar(' ');//再显示空格符把刚才的*给盖住，
						putchar('\b');//然后再 回撤一格等待录入。
						pos--;
					}
				}
				password[pos] = '\0';
				putchar('\n');
				adduser(args[1], args[2], string(password));
			}
			/* addgroup */
			else if (args[0] == "addgroup" && args.size() == 2)
			{
				if (!is_login)
				{
					cout << "仍未登录，请先登录！" << endl;
					continue;
				}
				addgroup(args[1]);
			}
			/* fdelete */
			else if (args[0] == "fdelete" && args.size() == 2)
			{
				if (!is_login)
				{
					cout << "仍未登录，请先登录！" << endl;
					continue;
				}
				Fdelete((char*)args[1].c_str());
			}
			/* deluser */
			else if (args[0] == "deluser" && args.size() == 2)
			{
				if (!is_login)
				{
					cout << "仍未登录，请先登录！" << endl;
					continue;
				}
				deluser(args[1]);
			}
			/* delgroup */
			else if (args[0] == "delgroup" && args.size() == 2)
			{
				if (!is_login)
				{
					cout << "仍未登录，请先登录！" << endl;
					continue;
				}
				delgroup(args[1]);
			}
			/* showusers */
			else if (args[0] == "showusers" && args.size() == 1)
			{
				showusers();
			}
			/* showgroups */
			else if (args[0] == "showgroups" && args.size() == 1)
			{
				showgroups();
			}
			else
			{
				cout << "未能识别指令！" << endl;
			}
		}
		FileSystem fs = Kernel::Instance().GetFileSystem();
		fs.Update();
	}

}

//输出帮助指令
void UI::UI_help()
{
	cout << "help                                                          -显示所有可执行指令" << endl;
	cout << "fformat                                                       -格式化文件系统" << endl;
	cout << "exit                                                          -退出文件系统" << endl;
	cout << "cls                                                           -清空屏幕" << endl;
	cout << "ls                                                            -显示当前目录下的所有文件" << endl;
	cout << "cd           <<dir_name>>                                     -切换目录" << endl;
	cout << "mkdir        <<dir_name>>                                     -新建文件夹" << endl;
	cout << "fcreat       <<file_name>>                                    -新建文件" << endl;
	cout << "fdelete      <<dir_name/file_name>>                           -删除文件夹/文件" << endl;
	cout << "fopen        <<file_name>>                                    -打开文件（方能读、写文件）" << endl;
	cout << "fclose       <<file_name>>                                    -关闭文件" << endl;
	cout << "fread        <<file_name>>    <<length>>                      -读取文件内一定长度的字节到命令行" << endl;
	cout << "fwrite       <<file_name>>    <<length>>                      -写入一定长度的字节文本到文件中" << endl;
	cout << "flseek       <<file_name>>    <<length>>     <<type>>         -定位文件读写指针" << endl;
	cout << "             type: SEEK_SET [1]; SEEK_CUR [2]; SEEK_END [3]   -定位文件读写指针" << endl;
	cout << "foper        <<file_name>>                                    -程序文件读写测试" << endl;
	cout << "fin          <<outer_filename>>   <<inner_filename>>          -将外部文件输入到文件系统中的文件中" << endl;
	cout << "fout         <<outer_filename>>   <<inner_filename>>          -将内部文件输出到外部的文件中" << endl;
	cout << "login        <<user_name>>                                    -用户登录" << endl;
	cout << "logout       <<user_name>>                                    -用户登出" << endl;
	cout << "adduser      <<user_name>>    <<group_name>>                  -在用户组group_name中添加新用户user_name" << endl;
	cout << "addgroup     <<group_name>>                                   -添加新用户组" << endl;
	cout << "deluser      <<user_name>>                                    -删除用户" << endl;
	cout << "delgroups    <<group_name>>                                   -删除用户组" << endl;
	cout << "showusers                                                     -展示系统中的所有用户" << endl;
	cout << "showgroups                                                    -展示系统中的所有用户组" << endl;
}

/* 磁盘格式化 */
void UI::UI_format()
{
	User& u = Kernel::Instance().GetUser();
	//u.u_uid = 0;
	//添加用户文件和组文件
	if (u.u_uid != 0)
	{
		cout << "权限不足，只有root才有格式化权限！" << endl;
	}
	else
	{
		DeviceDriver& dedr = Kernel::Instance().GetDeviceDriver();
		dedr.format();
		cout << "磁盘格式化成功！" << endl;
	}
}

void UI::UI_cls()
{
	system("cls");
}

int UI::Fopen(char* name, int mode)
{
	User& u = Kernel::Instance().GetUser();
	u.u_dirp = name;
	u.u_arg[1] = mode;
	FileManager* fm = &Kernel::Instance().GetFileManager();
	fm->Open();
	return u.u_ar0;
}

int UI::Fcreat(char* name, int mode)
{
	User& u = Kernel::Instance().GetUser();
	u.u_dirp = name;
	u.u_arg[1] = mode;
	FileManager* fm = &Kernel::Instance().GetFileManager();
	fm->Creat();
	return u.u_ar0;
}

int UI::Fread(int fd, char* buffer, int length)
{
	User& u = Kernel::Instance().GetUser();
	u.u_arg[0] = fd;
	u.u_arg[1] = int(buffer);
	u.u_arg[2] = length;
	FileManager* fm = &Kernel::Instance().GetFileManager();
	fm->Read();
	return u.u_ar0;
}

int UI::Fwrite(int fd, char* buffer, int length)
{
	User& u = Kernel::Instance().GetUser();
	u.u_arg[0] = fd;
	u.u_arg[1] = int(buffer);
	u.u_arg[2] = length;
	FileManager* fm = &Kernel::Instance().GetFileManager();
	fm->Write();
	return u.u_ar0;
}

void UI::Fdelete(char* name)
{
	User& u = Kernel::Instance().GetUser();
	u.u_dirp = name;
	FileManager* fm = &Kernel::Instance().GetFileManager();
	fm->UnLink();
}

int UI::Flseek(int fd, int position, int whence)
{
	User& u = Kernel::Instance().GetUser();
	u.u_arg[0] = fd;
	u.u_arg[1] = position;
	u.u_arg[2] = whence;
	FileManager* fm = &Kernel::Instance().GetFileManager();
	fm->Seek();
	return u.u_ar0;
}

void UI::Fclose(int fd_t)
{
	User& u = Kernel::Instance().GetUser();
	u.u_arg[0] = fd_t;
	FileManager* fm = &Kernel::Instance().GetFileManager();
	fm->Close();
	fd = -1;
}

void UI::Ls()
{
	User& u = Kernel::Instance().GetUser();
	int fd = Fopen(u.u_curdir, File::FREAD);

	/* 存储从文件中读取到的所有数据 */
	char buffer[10000];
	/* 展示每一个项目的文件名 */
	char name[28];
	/* 跳过前两个目录项，即当前目录和父目录 */
	Flseek(fd, 64, 0);
	/* 从目录文件中读取32字节的数据到buffer中，每次读取对应一个目录项 */
	int length = Fread(fd, buffer, 32);
	int num = 0;
	/* 存储文件的inode编号 */
	int m_ino;

	while (length)
	{
		/* 每个目录项的前4字节存储inode编号，后28字节存储文件名 */
		memcpy(name, buffer + 4, 28);
		memcpy(&m_ino, buffer, 4);
		if (m_ino != 0)
		{
			cout << name << "   ";
			num++;
		}

		if (num == 5)
		{
			num = 0;
			cout << endl;
		}
		length = Fread(fd, buffer, 32);
	}
	cout << endl;
}

void UI::Mkdir(char* name)
{
	User& u = Kernel::Instance().GetUser();
	/* 将要创建的目录名称存储在用户状态中 */
	u.u_dirp = name;
	/* 设置新目录的权限，04：目录；
	 * 7 代表所有者具有读、写、执行权限；
	 * 5代表群组和其他用户具有读和执行权限，但无写权限 */
	u.u_arg[1] = 040755;	//设置默认

	FileManager* fm = &Kernel::Instance().GetFileManager();
	fm->MkNod();

	/* 初始化目录项 */
	/* 两个元素，表示新目录中的两个默认目录项：.（当前目录）和..（父目录） */
	DirectoryEntry DE0[2];
	DE0[0].m_ino = u.u_pdir->i_number;
	strcpy_s(DE0[0].m_name, "..");
	DE0[1].m_ino = u.u_ar0;
	strcpy_s(DE0[1].m_name, ".");

	/* 写入目录项 */
	int fd = Fopen(name, File::FWRITE);
	Fwrite(fd, (char*)DE0, sizeof(DE0));
	Fclose(fd);
}

void UI::Cd(char* name)
{
	User& u = Kernel::Instance().GetUser();
	u.u_dirp = name;
	u.u_arg[0] = (int)name;
	FileManager* fm = &Kernel::Instance().GetFileManager();
	fm->ChDir();
}

void UI::FRead(char* name, int length)
{
	///* 以二进制模式读取文件 */
	//FILE* file = fopen(name, "wb");
	/*int fd = Fopen(name, File::FREAD);*/
	cout << "fd: " << fd << endl;
	if (fd == -1)
	{
		cout << "指令运行失败！" << endl;
		return;
	}

	char buffer[USER_FILE_SIZE];

	// int len = Fread(fd, buffer, length);
	User& u = Kernel::Instance().GetUser();
	u.u_arg[0] = fd;
	u.u_arg[1] = int(buffer);
	u.u_arg[2] = length;
	FileManager* fm = &Kernel::Instance().GetFileManager();
	fm->Read();

	buffer[u.u_ar0] = '\0';
	for (int i = 0; i <= u.u_ar0; i++)
	{
		if ((int)buffer[i] == -1)
			cout << "*";
		else
			cout << buffer[i];
	}
	cout << endl;

	/*Fclose(fd);*/
}

void UI::FWrite(char* name, int length)
{
	//FILE* file = fopen(name, "wb");
	/*int fd = Fopen(name, File::FWRITE);*/
	cout << "fd: " << fd << endl;
	if (fd == -1)
	{
		cout << "指令运行失败！" << endl;
		return;
	}
	char buffer[USER_FILE_SIZE];
	cin.read(buffer, length);
	cin.ignore((std::numeric_limits<streamsize>::max)(), '\n');
	int real_length = Fwrite(fd, buffer, length);

	/*Fclose(fd);*/
}

void UI::FLseek(char* name, int length, int type)
{
	/*FILE* file = fopen(name, "wb");
	int fd = Fopen(name, File::FWRITE);*/
	// cout << "fd: " << fd << endl;
	if (fd == -1)
	{
		cout << "指令运行失败！" << endl;
		return;
	}
	Flseek(fd, length, type);

	/*Fclose(fd);*/
}

void UI::FOper(char* name)
{
	cout << "正在打开文件" << name << "..." << endl;
	int fd = Fopen(name, File::FWRITE | File::FREAD);
	if (fd == -1)
	{
		cout << "指令运行失败！" << endl;
		return;
	}
	cout << "打开文件" << name << "成功。" << endl << endl;
	cout << "请写入800个字符" << name << "：" << endl;
	char bufferW[USER_FILE_SIZE];
	cin.read(bufferW, 800);
	cin.ignore((std::numeric_limits<streamsize>::max)(), '\n');
	int real_length = Fwrite(fd, bufferW, 800);
	cout << "800个字符已写入文件。" << endl << endl;
	cout << "将该文件的文件指针跳转至第500个字符中..." << endl;
	Flseek(fd, 500, 0);	/* 从头 */
	cout << "该文件的文件指针已跳转至第500个字符。" << endl << endl;
	cout << "从该文件中读出500个字符至abc字符串中..." << endl;
	char buffer[USER_FILE_SIZE];
	User& u = Kernel::Instance().GetUser();
	u.u_arg[0] = fd;
	u.u_arg[1] = int(buffer);
	u.u_arg[2] = 500;
	FileManager* fm = &Kernel::Instance().GetFileManager();
	fm->Read();
	cout << "在该文件中读出字符至abc字符串成功，读出实际字符数目为" << u.u_ar0 << "。" << endl << endl;
	buffer[u.u_ar0] = '\0';
	cout << "输出abc字符串内容：" << endl;
	/* 字符串abc的内容*/
	for (int i = 0; i <= u.u_ar0; i++)
	{
		if ((int)buffer[i] == -1)
			cout << "*";
		else
			cout << buffer[i];
	}
	cout << endl << endl;
	cout << "将abc字符串内容再次写入文件..." << endl;
	real_length = Fwrite(fd, buffer, u.u_ar0);
	cout << "写入成功，您可以通过指令 fread " << name << " 再次查看文件内容。" << endl << endl;;

	Fclose(fd);
	cout << "关闭文件。" << endl;
}

void UI::Fin(char* extername, char* intername)
{
	/* 以二进制模式读取文件 */
	FILE* ex_file = fopen(extername, "rb");
	if (!ex_file)
	{
		cout << "文件系统外文件打开失败！" << endl;
		return;
	}

	/* 在内部文件系统中创建新文件 */
	/* 所有者有读、写、执行权限；群组成员有读权限；其他用户没有任何权限 */
	Fcreat(intername, 0b111100000);
	int fd = Fopen(intername, File::FWRITE);	/* 以写入模式打开刚创建的文件 */

	/* 从外部文件中读取数据到缓冲区buffer，每次最多读取512字节，直到达到文件末尾 */
	char buffer[BUFFER_SIZE];
	while (!feof(ex_file))
	{
		int length = fread(buffer, 1, sizeof(buffer), ex_file);
		Fwrite(fd, buffer, length);
	}

	/* 关闭外部文件和内部文件系统中的文件 */
	fclose(ex_file);
	Fclose(fd);
}

void UI::Fout(char* extername, char* intername)
{
	/* 以二进制模式读取文件 */
	FILE* ex_file = fopen(extername, "wb");
	int fd = Fopen(intername, File::FREAD);
	if (fd == -1)
	{
		cout << "指令运行失败！" << endl;
		return;
	}

	char buffer[BUFFER_SIZE];
	int length = Fread(fd, buffer, sizeof(buffer));
	if (length == -1)
	{
		cout << "指令执行失败，请检查内部文件是否存在！" << endl;
		return;
	}
	while (length)
	{
		fwrite(buffer, length, 1, ex_file);
		length = Fread(fd, buffer, sizeof(buffer));
	}

	fclose(ex_file);
	Fclose(fd);
}

void UI::Login(string user_name, string password)
{
	if (is_login)
	{
		cout << "您已登录，无法重复登录！" << endl;
		return;
	}
	/* 打开用户信息文件 */
	int fd = Fopen((char*)"/etc/user.txt", File::FREAD);
	char buffer[USER_FILE_SIZE];
	int length = Fread(fd, buffer, sizeof(buffer));
	if (length == -1)
	{
		cout << "指令执行失败，请检查内部文件是否存在！" << endl;
		return;
	}

	/* 解析用户信息 */
	string userdata = buffer;
	vector<string> users = Utility::splitstr(userdata, "\n");
	for (int i = 0; i < users.size(); i++)
	{
		vector<string>umsg = Utility::splitstr(users[i], "-");
		if (umsg[0] == user_name && umsg[1] == password && umsg[4] == "1")
		{
			User& u = Kernel::Instance().GetUser();
			u.u_uid = i;				/* uid按照顺序分配 */
			u.u_gid = stoi(umsg[2]);	/* 设置组号 */
			cur_user = user_name;
			cur_group = umsg[3];
			cur_uid = i;
			cur_gid = u.u_gid;
			is_login = 1;				/* 登录状态 */
			cout << "用户：" << umsg[0] << "登陆成功！" << endl;
			Fclose(fd);
			return;
		}
		else if (umsg[0] == user_name && umsg[1] != password)
		{
			cout << "用户密码输入错误！" << endl;
			Fclose(fd);
			return;
		}
	}
	cout << "不存在用户：" << user_name << endl;
	Fclose(fd);
}

void UI::Logout()
{
	User& u = Kernel::Instance().GetUser();
	string tmp = cur_user;
	u.u_uid = -1;
	u.u_gid = -1;
	cur_user = "";
	cur_group = "";
	cur_uid = -1;
	cur_gid = -1;
	is_login = 0;
	Cd((char*)"/");
	cout << "当前用户" << tmp << "已登出！" << endl;
}

void UI::adduser(string user_name, string group_name, string password)
{
	User& u = Kernel::Instance().GetUser();
	/* 检查当前用户是否有权限添加新用户 */
	if (u.u_uid != 0)
	{
		cout << "权限不足，普通用户无法添加新用户，添加失败！" << endl;
		return;
	}

	/* 读取用户文件 */
	int fd_user = Fopen((char*)"/etc/user.txt", File::FREAD | File::FWRITE);
	char buffer_user[USER_FILE_SIZE];
	int length = Fread(fd_user, buffer_user, sizeof(buffer_user));
	if (length == -1)
	{
		cout << "指令执行失败，请检查内部文件是否存在" << endl;
		return;
	}
	string userdata = buffer_user;
	vector<string> users = Utility::splitstr(userdata, "\n");

	/* 检查用户是否已存在 */
	int user_cur = 0;
	for (user_cur = 0; user_cur < users.size() - 1; user_cur++)
	{
		vector<string>umsg = Utility::splitstr(users[user_cur], "-");
		if (umsg[0] == user_name)
		{
			cout << "该用户已存在！" << endl;
			return;
		}
	}

	/* 读取用户组文件 */
	int fd_group = Fopen((char*)"/etc/group.txt", File::FREAD);
	char buffer_group[USER_FILE_SIZE];
	length = Fread(fd_group, buffer_group, sizeof(buffer_group));
	if (length == -1)
	{
		cout << "指令执行失败，请检查内部文件是否存在！" << endl;
		return;
	}
	string groupdata = buffer_group;
	vector<string>groups = Utility::splitstr(groupdata, "\n");

	/* 检查用户组是否存在 */
	bool group_exist = 0;
	int group_cur = 0;
	for (group_cur = 0; group_cur < groups.size() - 1; group_cur++)
	{
		vector<string> gmsg = Utility::splitstr(groups[group_cur], "-");
		if (gmsg[0] == group_name && gmsg[1] == "1")
		{
			group_exist = 1;
			break;
		}
	}
	if (!group_exist)
	{
		cout << "用户组不存在，添加失败！" << endl;
		return;
	}

	/* 添加新用户 */
	string new_user = user_name + "-" + password + "-" + to_string(group_cur) + "-" + group_name + "-" + "1" + "\n";
	int n = Fwrite(fd_user, (char*)new_user.data(), new_user.length());

	FileSystem fs = Kernel::Instance().GetFileSystem();
	fs.Update();
	Fclose(fd_user);
	Fclose(fd_group);

	cout << "新用户" << user_name << "添加成功！" << endl;
}

void UI::addgroup(string group_name)
{
	User& u = Kernel::Instance().GetUser();
	/* 检查当前用户是否有权限添加新组别 */
	if (u.u_uid != 0)
	{
		cout << "权限不足，普通用户无法添加新组别，添加失败！" << endl;
		return;
	}

	/* 读取用户组文件 */
	int fd_group = Fopen((char*)"/etc/group.txt", File::FREAD | File::FWRITE);
	char buffer_group[USER_FILE_SIZE];
	int length = Fread(fd_group, buffer_group, sizeof(buffer_group));
	if (length == -1)
	{
		cout << "指令执行失败，请检查内部文件是否存在！" << endl;
		return;
	}
	string groupdata = buffer_group;
	vector<string>groups = Utility::splitstr(groupdata, "\n");

	/* 检查用户组是否存在 */
	int group_cur = 0;
	for (group_cur = 0; group_cur < groups.size() - 1; group_cur++)
	{
		if (groups[group_cur] == group_name)
		{
			cout << "该组别已存在！" << endl;
			return;
		}
	}

	/* 添加新组别 */
	string new_group = group_name + "-" + "1" + "\n";
	Fwrite(fd_group, (char*)new_group.data(), new_group.length());
	FileSystem fs = Kernel::Instance().GetFileSystem();
	fs.Update();
	Fclose(fd_group);

	cout << "新组别" << group_name << "添加成功！" << endl;
}

void UI::deluser(string user_name)
{
	User& u = Kernel::Instance().GetUser();
	if (u.u_uid != 0)
	{
		cout << "权限不足，普通用户无法删除其他用户！" << endl;
		return;
	}
	if (user_name == "root")
	{
		cout << "[警告] root用户不可被删除！" << endl;
		return;
	}

	int fd_user = Fopen((char*)"/etc/user.txt", File::FREAD);
	char buffer_user[USER_FILE_SIZE];
	int length = Fread(fd_user, buffer_user, sizeof(buffer_user));
	if (length == -1)
	{
		cout << "指令执行失败，请检查内部文件是否存在！" << endl;
		return;
	}

	string userdata = buffer_user;
	vector<string> users = Utility::splitstr(userdata, "\n");
	vector<string> new_users;
	int user_cur = 0;
	bool del_flag = 0;
	for (user_cur = 0; user_cur < users.size() - 1; user_cur++)
	{
		vector<string>umsg = Utility::splitstr(users[user_cur], "-");
		if (umsg[0] == user_name && umsg[4] == " 1")
		{
			new_users.push_back(umsg[0] + "-" + umsg[1] + "-" + umsg[2] + "-" + umsg[3] + "-" + "0" + "\n");
			del_flag = 1;
		}
		else
		{
			new_users.push_back(users[user_cur] + "\n");
		}
	}
	Fclose(fd_user);
	fd_user = Fopen((char*)"/etc/user.txt", File::FWRITE);
	for (int i = 0; i < new_users.size(); i++)
	{
		Fwrite(fd_user, (char*)new_users[i].c_str(), new_users[i].length());
	}
	Fclose(fd_user);
	if (del_flag)
		cout << "用户：" << user_name << " 已删除！" << endl;
	else
		cout << "用户：" << user_name << "不存在！" << endl;
}

void UI::delgroup(string group_name)
{
	User& u = Kernel::Instance().GetUser();
	if (u.u_uid != 0)
	{
		cout << "权限不足，普通用户无法删除其他用户！" << endl;
		return;
	}
	if (group_name == "root_group")
	{
		cout << "[警告] root所在的用户组不可被删除！" << endl;
		return;
	}

	int fd_user = Fopen((char*)"/etc/user.txt", File::FREAD);
	char buffer_user[USER_FILE_SIZE];
	int length = Fread(fd_user, buffer_user, sizeof(buffer_user));
	if (length == -1)
	{
		cout << "指令执行失败，请检查内部文件是否存在！" << endl;
		return;
	}
	string userdata = buffer_user;
	vector<string> users = Utility::splitstr(userdata, "\n");
	int user_cur = 0;
	for (user_cur = 0; user_cur < users.size() - 1; user_cur++)
	{
		vector<string>umsg = Utility::splitstr(users[user_cur], "-");
		if (umsg[3] == group_name && umsg[4] == "1")
		{
			Fclose(fd_user);
			cout << "还有用户处于组：" << group_name << " 中，无法删除！" << endl;
			return;
		}
	}
	Fclose(fd_user);


	int fd_group = Fopen((char*)"/etc/group.txt", File::FREAD);
	char buffer_group[USER_FILE_SIZE];
	length = Fread(fd_group, buffer_group, sizeof(buffer_group));
	if (length == -1)
	{
		cout << "指令执行失败，请检查内部文件是否存在！" << endl;
		return;
	}

	string groupdata = buffer_group;
	vector<string> groups = Utility::splitstr(groupdata, "\n");
	vector<string> new_groups;
	int group_cur = 0;
	bool del_flag = 0;
	for (group_cur = 0; group_cur < groups.size() - 1; group_cur++)
	{
		vector<string>umsg = Utility::splitstr(groups[group_cur], "-");
		if (umsg[0] == group_name && umsg[1] == "1")
		{
			new_groups.push_back(umsg[0] + "-" + "0" + "\n");
			del_flag = 1;
		}
		else
		{
			new_groups.push_back(groups[group_cur] + "\n");
		}
	}
	Fclose(fd_group);
	fd_group = Fopen((char*)"/etc/group.txt", File::FWRITE);
	for (int i = 0; i < new_groups.size(); i++)
	{
		Fwrite(fd_group, (char*)new_groups[i].c_str(), new_groups[i].length());
	}
	Fclose(fd_group);
	if (del_flag)
		cout << "用户组：" << group_name << " 已删除！" << endl;
	else
		cout << "用户组：" << group_name << "不存在！" << endl;
}

void UI::showusers()
{
	User& u = Kernel::Instance().GetUser();
	if (u.u_uid != 0)
	{
		cout << "权限不足，普通用户无法查看所有用户！" << endl;
		return;
	}
	int fd_user = Fopen((char*)"/etc/user.txt", File::FREAD);
	char buffer_user[USER_FILE_SIZE];
	int length = Fread(fd_user, buffer_user, sizeof(buffer_user));
	if (length == -1)
	{
		cout << "指令执行失败，请检查内部文件是否存在！" << endl;
		return;
	}
	string userdata = buffer_user;
	vector<string> users = Utility::splitstr(userdata, "\n");
	int user_cur = 0;
	for (user_cur = 0; user_cur < users.size() - 1; user_cur++)
	{
		vector<string>umsg = Utility::splitstr(users[user_cur], "-");
		if (umsg[4] == "1")
		{
			cout << "用户名：" << umsg[0] << "  密码：" << umsg[1] << "  uid：" << user_cur << "  gid：" << umsg[2] << "  组名：" << umsg[3] << endl;
		}
	}
	Fclose(fd_user);
}

void UI::showgroups()
{
	User& u = Kernel::Instance().GetUser();
	if (u.u_uid != 0)
	{
		cout << "权限不足，普通用户无法查看所有用户！" << endl;
		return;
	}
	int fd_group = Fopen((char*)"/etc/group.txt", File::FREAD);
	char buffer_group[USER_FILE_SIZE];
	int length = Fread(fd_group, buffer_group, sizeof(buffer_group));
	if (length == -1)
	{
		cout << "指令执行失败，请检查内部文件是否存在！" << endl;
		return;
	}
	string groupdata = buffer_group;
	vector<string> groups = Utility::splitstr(groupdata, "\n");
	vector<string> new_groups;
	int group_cur = 0;
	bool del_flag = 0;
	for (group_cur = 0; group_cur < groups.size() - 1; group_cur++)
	{
		vector<string>umsg = Utility::splitstr(groups[group_cur], "-");
		if (umsg[1] == "1")
		{
			cout << "组名：" << umsg[0] << "  gid：" << group_cur << endl;
		}
	}
	Fclose(fd_group);
}