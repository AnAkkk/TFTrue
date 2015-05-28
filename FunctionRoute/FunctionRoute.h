#pragma once

class CFunctionRoute
{
public:
	CFunctionRoute();
	~CFunctionRoute();

	//	Hook a function, all calls made to the original function will be routed to the new function.
	//	Usage: RouteFunction(&printf, &MyPrintf);
	bool RouteFunction(void* pOriginalFunction, void* pNewFunction);

	//	Hook a virtual function, all calls made to the original function for the passed instance will be routed to the new function.
	//	If bGlobal is true, all the objets that use the same vtable will be affected.
	//	If bGlobal is false, you must restore the function before the instance deletion, else the memory will be corrupted
	//	Usage: RouteVirtualFunction(pObject, &IObject::Function, &MyFunction);
	template <typename T, typename T2>
	bool RouteVirtualFunction(void* pInstance, T pMemberFunction, T2 pNewFunction, bool bGlobal = true);

	//	Remove the function hook
	bool RestoreFunction();

	//	Call the original function without triggering the hook
	template <typename T>
	T CallOriginalFunction() const;

	//	Get the address of a virtual function
	template <typename T>
	static const void* GetVirtualFunctionAddress(void* pInstance, T pMemberFunction);

	//	Return the address of the hooked function
	//	Never use this function to call the original function directly ! Its code may have been altered by the hook
	//	Use CallOriginalFunction() for that.
	const void* GetHookedFunctionAddress() const;

private:

	enum EMemoryProtection
	{
		k_EMemoryProtectionInvalid	= 1 << 0,

		k_EMemoryProtectionNoAccess	= 0,
		k_EMemoryProtectionRead		= 1 << 1,
		k_EMemoryProtectionWrite	= 1 << 2,
		k_EMemoryProtectionExecute	= 1 << 3,
	};

	const void* InternalCallOriginalFunction() const;
	bool InternalRouteVirtualFunction(void* pInstance, void* pMemberFunction, void* pNewFunction, bool bGlobal);

	static int GetVirtualFunctionOffset(void* pMemberFunction);

	unsigned int CopyAndFixOpCodes(const void* pFromAddr, unsigned int uLength, void* pToAddr = NULL);

	EMemoryProtection GetMemoryProtection(const void* pMemory) const;
	bool SetMemoryProtection(void* pMemory, unsigned int uSize, EMemoryProtection eProtection);

	static long long abs(long long n);

private:

	enum EHookType
	{
		k_EHookTypeNone = 0,
		k_EHookTypeNearJump,
		k_EHookTypeVTableLocal,
		k_EHookTypeVTableGlobal,
	};

	EHookType m_eHookType;

	void* m_pHookedFunction;

	void* m_pDataOriginalAddr;
	unsigned char* m_pubMemoryBackup;
	unsigned int m_cubMemoryBackup;

	unsigned char* m_pubTrampoline;
	unsigned char* m_pNewVTable;
};

template <typename T, typename T2>
inline bool CFunctionRoute::RouteVirtualFunction(void* pInstance, T pMemberFunction, T2 pNewFunction, bool bGlobal/* = true*/)
{
	return InternalRouteVirtualFunction(pInstance, *(void**)&pMemberFunction, (void*)pNewFunction, bGlobal);
}

template <typename T>
inline T CFunctionRoute::CallOriginalFunction() const
{
	return (T)InternalCallOriginalFunction();
}

inline const void* CFunctionRoute::GetHookedFunctionAddress() const
{
	return m_pHookedFunction;
}

template <typename T>
inline const void* CFunctionRoute::GetVirtualFunctionAddress(void* pInstance, T pMemberFunction)
{
	int iOffset = GetVirtualFunctionOffset(*(void**)&pMemberFunction);
	return iOffset >= 0 ? (*(void ***)pInstance)[iOffset] : NULL;
}

inline long long CFunctionRoute::abs(long long n)
{
	if(n & 0x8000000000000000ull)
	{
		return (n ^ -1) + 1; 
	}
	else
		return n;
}
