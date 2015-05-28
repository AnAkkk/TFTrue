#pragma once

#include <string>
//#include <unordered_map>
#include <map>
#include <cstdio>
class CModuleScanner
{
public:
	// Pass any address in the module or the module handle itself to this function.
	CModuleScanner(void* hModule);

	void* FindSignature(const char* pubSignature, const char* cszMask) const;
	void* FindSignature(const unsigned char* pubSignature, const char* cszMask) const;

	void* FindSymbol(const char* cszSymbol) const;

private:
	void CacheSymbols();

	char m_szFilename[FILENAME_MAX];
	const void* m_pAllocationBase;
	unsigned int m_uSize;

	// Used to be std::unordered_map, but changed to std::map due to new libstdc++ symbols
	// making the plugin fail to load as Valve's libstdc++ don't have these symbols
	std::map<std::string, void*> m_symbols;
};

inline void* CModuleScanner::FindSignature(const char* pubSignature, const char* cszMask) const
{
	return FindSignature((const unsigned char*)pubSignature, cszMask);
}
