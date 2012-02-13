// VersionMacros.h -- A file to hold version, build and revision numbers, with helper macros
//

#include "buildnumber.h"
#define MAJOR_VERSION        0
#define MINOR_VERSION        7

//#define BUILD_NUMBER         100			// Uncomment to force specific build number
#ifndef BUILD_NUMBER
  #define BUILD_NUMBER       BUILDNUMBER
#endif

#define REVISION_NUMBER      0
#define BUILD_TAG            "alpha 1"

#ifdef BUILD_TAG
  #define BUILD_ANSI_VERSION_STRING(a,b,c,d,e) #a "." #b "." #c "." #d " " e
  #define BUILD_UNICODE_VERSION_STRING(a,b,c,d,e) L#a L"." L#b L"." L#c L"." L#d L" " L##e
#else
  #define BUILD_ANSI_VERSION_STRING(a,b,c,d,e) #a "." #b "." #c "." #d
  #define BUILD_UNICODE_VERSION_STRING(a,b,c,d,e) L#a L"." L#b L"." L#c L"." L#d
#endif
#define RCFILE_VERSION(a,b,c,d,e) BUILD_ANSI_VERSION_STRING(a,b,c,d,e)
#define PROGRAM_VERSION(a,b,c,d,e) BUILD_UNICODE_VERSION_STRING(a,b,c,d,e)
