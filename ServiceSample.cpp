// ServiceSample.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <string>
#include <algorithm>
#include <windows.h>

using namespace std;
int GetOperate(char* szCmd);
bool ServiceOperate(int nOp);
int ServiceMain(int argc, char* argv[], char* envp[]);

int main(int argc, char* argv[], char* envp[])
{
	if (argc > 1 && ServiceOperate(GetOperate(argv[1]))) return 0;
	return ServiceMain(argc, argv, envp);
}

enum {
	INSTALL,
	DEL,
	STOP,
	RUN,
};

HANDLE hInstance;
#define SERVICE_NAME "Sample Service"

bool g_bQuit = false;
// doing something when press Ctrl+C or close
BOOL CALLBACK HandleCtrlEvent(DWORD ctrlType)
{
	g_bQuit = true;
	Sleep(INFINITE);
	return TRUE;
}

int ServiceMain(int argc, char* argv[], char* envp[]) {
	SetConsoleCtrlHandler(HandleCtrlEvent, TRUE);

	hInstance = CreateMutexA(NULL, TRUE, SERVICE_NAME "_INSTANCE");
	SetWindowTextA(GetConsoleWindow(), SERVICE_NAME);

	if (ERROR_ALREADY_EXISTS == GetLastError()) // has a running instance
	{
		return 0;
	}

	while (!g_bQuit) {
		// something to do in loop
		printf("Service Running...\n");
		Sleep(1000);
	}

	return 0;
}

int GetOperate(char* szCmd)
{
	string sCmd(szCmd);
	if (sCmd == "install") return INSTALL;
	if (sCmd == "delete") return DEL;
	if (sCmd == "stop") return STOP;
	if (sCmd == "run") return RUN;
	return -1;
}

string GetCurDirectory()
{
	char szModuleFile[1024] = "";
	GetModuleFileNameA(NULL, szModuleFile, MAX_PATH);
	string sModuleFile(szModuleFile);
	if (sModuleFile != "")
	{
		sModuleFile = sModuleFile.substr(0, sModuleFile.rfind("\\") + 1);
	}

	return sModuleFile;
}

string GetProgramName()
{
	char szModuleFile[1024] = "";
	GetModuleFileNameA(NULL, szModuleFile, MAX_PATH);
	string sModuleFile(szModuleFile);
	if (sModuleFile != "")
	{
		sModuleFile = sModuleFile.substr(sModuleFile.rfind("\\") + 1);
		sModuleFile = sModuleFile.substr(0, sModuleFile.length() - 4);
	}

	return sModuleFile;
}


bool InstallService(string sProgram)
{
#define CUR_DIR_MARK "{{CUR_DIR}}"
#define PROGRAM_MARK "{{PROGRAM}}"

	string sConfig = "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"
		"<service>\n"
		"	<id>" SERVICE_NAME "</id>\n"
		"	<name>" SERVICE_NAME "</name>\n"
		"	<description>" SERVICE_NAME "</description>\n"
		"	<executable>" CUR_DIR_MARK PROGRAM_MARK "</executable>\n"
		"	<stopargument>stop</stopargument>\n"
		"	<logpath>" CUR_DIR_MARK "</logpath>\n"
		"	<logmode>roll</logmode>\n"
		"	<depend></depend>\n"
		"	<startargument></startargument>\n"
		"	<stopexecutable>" CUR_DIR_MARK PROGRAM_MARK "</stopexecutable>\n"
		"	<stopargument>stop</stopargument>\n"
		"	<stoptimeout>10sec</stoptimeout>\n"
		"</service>";

	string::size_type pos = 0;
	string sCurrentDir = GetCurDirectory();
	while ((pos = sConfig.find(CUR_DIR_MARK)) != std::string::npos)
	{
		sConfig.replace(pos, strlen(CUR_DIR_MARK), sCurrentDir);
	}
	while ((pos = sConfig.find(PROGRAM_MARK)) != std::string::npos)
	{
		sConfig.replace(pos, strlen(PROGRAM_MARK), sProgram);
	}

	printf("Write Config...\n");

	FILE* file;
	fopen_s(&file, (GetCurDirectory() + "service-exec.xml").c_str(), "w+");
	if (NULL != file)
	{
		fwrite(sConfig.c_str(), sConfig.length(), 1, file);
		fclose(file);
	}
	else
	{
		printf("Write Config Failed...\n");
		return true;
	}

	printf("Register Service...\n");

	string sRegister = GetCurDirectory() + "service-exec.exe install";
	system(sRegister.c_str());

	printf("Start Service...\n");

	string sCmd = "sc start \"" SERVICE_NAME "\"";
	system(sCmd.c_str());
	sCmd = "sc failure \"" SERVICE_NAME "\" reset=5 actions=restart/3/restart/10/restart/60";
	system(sCmd.c_str());

	return true;
}

bool StopService(string sProgram)
{
	string sCmd = "sc stop \"" SERVICE_NAME "\"";
	system(sCmd.c_str());

	HWND hWnd = ::FindWindowA(NULL, SERVICE_NAME);
	::SendMessage(hWnd, WM_CLOSE, 0, 0);

	Sleep(500);
	sCmd = "taskkill /im " + sProgram + ".exe /f";
	system(sCmd.c_str());

	return true;
}

bool DeleteService(string sProgram)
{
	string sCmd = "sc stop \"" SERVICE_NAME "\"";
	system(sCmd.c_str());
	sCmd = "sc delete \"" SERVICE_NAME "\"";
	system(sCmd.c_str());

	return true;
}

bool ServiceOperate(int nOp)
{
	string sProgram = GetProgramName();
	string sCmd;
	switch (nOp) {
	case INSTALL: return InstallService(sProgram);
	case DEL: return DeleteService(sProgram);
	case STOP: return StopService(sProgram);
	case RUN: sCmd = "sc start \"" SERVICE_NAME "\""; break;
	default:
		return false;
	}

	system(sCmd.c_str());

	return true;
}
