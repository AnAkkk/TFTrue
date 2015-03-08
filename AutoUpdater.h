#pragma once

#include <string>
#include <stddef.h>
#include <time.h>

class CAutoUpdater
{
public:
	static bool StartAutoUpdater();
	void OnGameFrame();

	std::string GetCurrentModulePath();

private:
	bool IsModuleValid(std::string strFileName );
	bool GetFileMD5(std::string strFileName, char ucMD5[] );
	void Base64ToHex(const char *szBase64, size_t length, char *szResult );

	int m_iStatus = 0;

	time_t m_tLastMessageTime = 0;
};

extern CAutoUpdater g_AutoUpdater;
