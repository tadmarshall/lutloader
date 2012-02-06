// Profile.cpp -- Profile class for handling display profiles.
//

#include "stdafx.h"
#include "Profile.h"
#include <icm.h>
#include <strsafe.h>

// For viewing ...
//

// Tad's note -- this seems to match the ICC spec, but don't forget big-endian issues ...
////
//// ICC profile header
////
//
//typedef struct tagPROFILEHEADER {
//    DWORD   phSize;             // profile size in bytes
//    DWORD   phCMMType;          // CMM for this profile
//    DWORD   phVersion;          // profile format version number
//    DWORD   phClass;            // type of profile
//    DWORD   phDataColorSpace;   // color space of data
//    DWORD   phConnectionSpace;  // PCS
//    DWORD   phDateTime[3];      // date profile was created
//    DWORD   phSignature;        // magic number
//    DWORD   phPlatform;         // primary platform
//    DWORD   phProfileFlags;     // various bit settings
//    DWORD   phManufacturer;     // device manufacturer
//    DWORD   phModel;            // device model number
//    DWORD   phAttributes[2];    // device attributes
//    DWORD   phRenderingIntent;  // rendering intent
//    CIEXYZ  phIlluminant;       // profile illuminant
//    DWORD   phCreator;          // profile creator
//    BYTE    phReserved[44];     // reserved for future use
//} PROFILEHEADER;
//typedef PROFILEHEADER *PPROFILEHEADER, *LPPROFILEHEADER;
//
////
//// Profile class values
////
//
//#define CLASS_MONITOR           'mntr'
//#define CLASS_PRINTER           'prtr'
//#define CLASS_SCANNER           'scnr'
//#define CLASS_LINK              'link'
//#define CLASS_ABSTRACT          'abst'
//#define CLASS_COLORSPACE        'spac'
//#define CLASS_NAMED             'nmcl'
//#if NTDDI_VERSION >= NTDDI_VISTA
//#define CLASS_CAMP              'camp'
//#define CLASS_GMMP              'gmmp'
//#endif // NTDDI_VERSION >= NTDDI_VISTA
//
////
//// Color space values
////
//
//#define SPACE_XYZ               'XYZ '
//#define SPACE_Lab               'Lab '
//#define SPACE_Luv               'Luv '
//#define SPACE_YCbCr             'YCbr'
//#define SPACE_Yxy               'Yxy '
//#define SPACE_RGB               'RGB '
//#define SPACE_GRAY              'GRAY'
//#define SPACE_HSV               'HSV '
//#define SPACE_HLS               'HLS '
//#define SPACE_CMYK              'CMYK'
//#define SPACE_CMY               'CMY '
//#define SPACE_2_CHANNEL         '2CLR'
//#define SPACE_3_CHANNEL         '3CLR'
//#define SPACE_4_CHANNEL         '4CLR'
//#define SPACE_5_CHANNEL         '5CLR'
//#define SPACE_6_CHANNEL         '6CLR'
//#define SPACE_7_CHANNEL         '7CLR'
//#define SPACE_8_CHANNEL         '8CLR'
//
////
//// Profile flag bitfield values
////
//
//#define FLAG_EMBEDDEDPROFILE    0x00000001
//#define FLAG_DEPENDENTONDATA    0x00000002


//typedef struct tagPROFILE {
//    DWORD   dwType;             // profile type
//    PVOID   pProfileData;       // filename or buffer containing profile
//    DWORD   cbDataSize;         // size of profile data
//} PROFILE;
//typedef PROFILE *PPROFILE, *LPPROFILE;

//HPROFILE   WINAPI OpenColorProfileA(__in PPROFILE pProfile, DWORD dwDesiredAccess, DWORD dwShareMode, DWORD dwCreationMode);
//bool bRet = GetColorDirectoryW(pMachineName, pBuffer, pdWORD);
//bool bRet = EnumColorProfilesW(pMachineName, pEnumRecord, pBuffer, pdwSizeOfBuffer, pDWORDprofiles);

//BOOL
//WINAPI
//WcsGetUsePerUserProfiles(
//    __in LPCWSTR pDeviceName,
//    __in DWORD dwDeviceClass,
//    __out PBOOL pUsePerUserProfiles
//    );

// From wingdi.h ...
//
//WINGDIAPI int         WINAPI EnumICMProfilesW( __in HDC hdc, __in ICMENUMPROCW proc, __in_opt LPARAM param);
//typedef int (CALLBACK* ICMENUMPROCW)(LPWSTR, LPARAM);
//WINGDIAPI BOOL        WINAPI GetICMProfileW(    __in HDC hdc,
//                                                __inout LPDWORD pBufSize,
//                                                __out_ecount_opt(*pBufSize) LPWSTR pszFilename);
//WINGDIAPI BOOL        WINAPI GetDeviceGammaRamp( __in HDC hdc, __out_bcount(3*256*2) LPVOID lpRamp);
//WINGDIAPI BOOL        WINAPI SetDeviceGammaRamp( __in HDC hdc, __in_bcount(3*256*2)  LPVOID lpRamp);
///* Color Management caps */
//#define CM_NONE             0x00000000
//#define CM_DEVICE_ICM       0x00000001
//#define CM_GAMMA_RAMP       0x00000002
//#define CM_CMYK_COLOR       0x00000004
////
//// Support for named color profiles
////
//
//typedef char COLOR_NAME[32];
//typedef COLOR_NAME *PCOLOR_NAME, *LPCOLOR_NAME;
//
