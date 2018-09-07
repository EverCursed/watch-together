/*
Version information
*/

#define APP_TITLE "WatchTogether"

#define WT_MAJ_VER 0
#define WT_MIN_VER 0
#define WT_REV_NUM 1


#ifdef _UNICODE
#define _VN(maj,min,rev) #maj L"." #min L"." #rev
#define __VN(maj,min,rev) _VN(maj,min,rev)
#define WT_FULL_VERSION L""__VN(WT_MAJ_VER,WT_MIN_VER,WT_REV_NUM)
#define WT_WINDOW_TITLE L"" APP_TITLE L" v" WT_FULL_VERSION
#else
#define _VN(maj,min,rev) #maj "." #min "." #rev
#define __VN(maj,min,rev) _VN(maj,min,rev)
#define WT_FULL_VERSION __VN(WT_MAJ_VER,WT_MIN_VER,WT_REV_NUM)
#define WT_WINDOW_TITLE APP_TITLE " v" WT_FULL_VERSION 
#endif