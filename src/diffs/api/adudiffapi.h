/**
 * @file adudiffapi.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#ifdef WIN32
	#define CDECL __cdecl

	#ifdef ADUDIFFAPI_DLL_EXPORTS
		#define ADUAPI_LINKAGESPEC __declspec(dllexport)
	#else
		#define ADUAPI_LINKAGESPEC __declspec(dllimport)
	#endif
#else
	#define CDECL
	#define ADUAPI_LINKAGESPEC
#endif
