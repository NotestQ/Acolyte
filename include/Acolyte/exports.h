#pragma once
#ifdef ACOLYTE_EXPORTS
	#define ACOLYTE_API    __declspec(dllexport)
#else
	#define ACOLYTE_API    __declspec(dllimport)
#endif

// Quick footgun checks
#if !defined(_MSC_VER)
#  error "Acolyte requires MSVC"
#endif

#if defined(_MSC_VER) && _MSC_VER != 1944
#  pragma message("WARNING: Acolyte has only been tested with MSVC 1944 (VS2022)! \
You are compiling with a different MSVC version!"
#endif

#if _ITERATOR_DEBUG_LEVEL != 0
#  error "Acolyte requires mods to be compiled as Release"
#endif

#if !defined(_DLL)
#  error "Acolyte requires dynamic DLLs"
#endif

#if !defined(_M_IX86)
#  error "Acolyte is x86 only"
#endif

#if defined(_MSC_VER)
#  if _MSVC_LANG < 202302L
#    error "Acolyte requires C++23 or later"
#  endif
#else
#  if __cplusplus < 202302L
#    error "Acolyte requires C++23 or later"
#  endif
#endif