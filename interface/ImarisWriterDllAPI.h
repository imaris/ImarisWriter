/***************************************************************************
 *   Copyright (c) 2020-present Bitplane AG Zuerich                        *
 *                                                                         *
 *   Licensed under the Apache License, Version 2.0 (the "License");       *
 *   you may not use this file except in compliance with the License.      *
 *   You may obtain a copy of the License at                               *
 *                                                                         *
 *       http://www.apache.org/licenses/LICENSE-2.0                        *
 *                                                                         *
 *   Unless required by applicable law or agreed to in writing, software   *
 *   distributed under the License is distributed on an "AS IS" BASIS,     *
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or imp   *
 *   See the License for the specific language governing permissions and   *
 *   limitations under the License.                                        *
 ***************************************************************************/
#ifndef __BP_IMARISWRITERDLL_API_H__
#define __BP_IMARISWRITERDLL_API_H__

// DLL stuff
#ifdef _WIN32
#ifndef BP_IMARISWRITER_DLL_EXPORT
#define BP_IMARISWRITER_DLL_EXPORT __declspec( dllexport )
#endif
#ifndef BP_IMARISWRITER_DLL_IMPORT
#define BP_IMARISWRITER_DLL_IMPORT __declspec( dllimport )
#endif
#else //_WIN32
#ifndef BP_IMARISWRITER_DLL_EXPORT
#define BP_IMARISWRITER_DLL_EXPORT __attribute__((visibility("default")))
#endif
#ifndef BP_IMARISWRITER_DLL_IMPORT
#define BP_IMARISWRITER_DLL_IMPORT __attribute__((visibility("default")))
#endif
#endif


#ifdef IMARISWRITER_DLL

// For unit tests we build a static lib
#ifdef BP_UNIT_TEST
#define BP_IMARISWRITER_DLL_API
#else
#define BP_IMARISWRITER_DLL_API BP_IMARISWRITER_DLL_IMPORT
#endif

#else // building the dll

// For unit tests we build a static lib
#ifdef BP_UNIT_TEST
#define BP_IMARISWRITER_DLL_API
#else
#define BP_IMARISWRITER_DLL_API BP_IMARISWRITER_DLL_EXPORT
#endif

#endif // IMARISWRITER_DLL

#endif // __BP_IMARISWRITERDLL_API_H__
