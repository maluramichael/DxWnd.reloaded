#define _WIN32_WINNT 0x0600
#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_DEPRECATE 1

#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include <psapi.h>
#include "dxwnd.h"
#include "dxwcore.hpp"
#include "shareddc.hpp"
#include "dxhook.h"
#include "glhook.h"
#include "glidehook.h"
#include "msvfwhook.h"
#define DXWDECLARATIONS 1
#include "syslibs.h"
#undef DXWDECLARATIONS
#include "dxhelper.h"
#include "Ime.h"
#include "Winnls32.h"
#include "Mmsystem.h"
#include "disasm.h"
#include "MinHook.h" 

#define SKIPIMEWINDOW TRUE

dxwCore dxw;
dxwSStack dxwss;
dxwWStack dxwws;
dxwSDC sdc;
GetWindowLong_Type pGetWindowLong;
SetWindowLong_Type pSetWindowLong;

extern LRESULT CALLBACK MessageHook(int, WPARAM, LPARAM);
extern DWORD WINAPI CpuLimit(LPVOID); 
extern DWORD WINAPI CpuSlow(LPVOID); 

typedef char *(*Geterrwarnmessage_Type)(unsigned long, unsigned long);
typedef int (*Preparedisasm_Type)(void);
typedef void (*Finishdisasm_Type)(void);
typedef unsigned long (*Disasm_Type)(const unsigned char *, unsigned long, unsigned long, t_disasm *, int, t_config *, int (*)(tchar *, unsigned long));

Geterrwarnmessage_Type pGeterrwarnmessage;
Preparedisasm_Type pPreparedisasm;
Finishdisasm_Type pFinishdisasm;
Disasm_Type pDisasm;

extern void InitScreenParameters(int);
extern void *HotPatch(void *, const char *, void *);
extern void *IATPatch(HMODULE, char *, void *, const char *, void *);
extern void *IATPatchEx(HMODULE, DWORD, char *, void *, const char *, void *);
void HookModule(HMODULE, int);
void RecoverScreenMode();
static void LockScreenMode(DWORD, DWORD, DWORD);

extern HANDLE hTraceMutex;

CRITICAL_SECTION TraceCS; 

static char *FlagNames[32]={
	"UNNOTIFY", "EMULATESURFACE", "CLIPCURSOR", "NEEDADMINCAPS",
	"HOOKDI", "MODIFYMOUSE", "HANDLEEXCEPTIONS", "SAVELOAD",
	"EMULATEBUFFER", "HOOKDI8", "BLITFROMBACKBUFFER", "SUPPRESSCLIPPING",
	"AUTOREFRESH", "FIXWINFRAME", "HIDEHWCURSOR", "SLOWDOWN",
	"ENABLECLIPPING", "LOCKWINSTYLE", "MAPGDITOPRIMARY", "FIXTEXTOUT",
	"KEEPCURSORWITHIN", "USERGB565", "SUPPRESSDXERRORS", "PREVENTMAXIMIZE",
	"LOCKEDSURFACE", "FIXPARENTWIN", "SWITCHVIDEOMEMORY", "CLIENTREMAPPING",
	"HANDLEALTF4", "LOCKWINPOS", "HOOKCHILDWIN", "MESSAGEPROC"
};

static char *Flag2Names[32]={
	"RECOVERSCREENMODE", "REFRESHONRESIZE", "BACKBUFATTACH", "MODALSTYLE",
	"KEEPASPECTRATIO", "INIT8BPP", "FORCEWINRESIZE", "INIT16BPP",
	"KEEPCURSORFIXED", "DISABLEGAMMARAMP", "**", "FIXNCHITTEST",
	"LIMITFPS", "SKIPFPS", "SHOWFPS", "HIDEMULTIMONITOR",
	"TIMESTRETCH", "HOOKOPENGL", "WALLPAPERMODE", "SHOWHWCURSOR",
	"GDISTRETCHED", "SHOWFPSOVERLAY", "FAKEVERSION", "FULLRECTBLT",
	"NOPALETTEUPDATE", "SUPPRESSIME", "NOBANNER", "WINDOWIZE",
	"LIMITRESOURCES", "STARTDEBUG", "SETCOMPATIBILITY", "WIREFRAME",
};

static char *Flag3Names[32]={
	"FORCEHOOKOPENGL", "MARKBLIT", "HOOKDLLS", "SUPPRESSD3DEXT",
	"HOOKENABLED", "FIXD3DFRAME", "FORCE16BPP", "BLACKWHITE",
	"MARKLOCK", "SINGLEPROCAFFINITY", "EMULATEREGISTRY", "CDROMDRIVETYPE",
	"NOWINDOWMOVE", "FORCECLIPPER", "LOCKSYSCOLORS", "GDIEMULATEDC",
	"FULLSCREENONLY", "FONTBYPASS", "MINIMALCAPS", "DEFAULTMESSAGES",
	"BUFFEREDIOFIX", "FILTERMESSAGES", "PEEKALLMESSAGES", "SURFACEWARN",
	"ANALYTICMODE", "FORCESHEL", "CAPMASK", "COLORFIX",
	"NODDRAWBLT", "NODDRAWFLIP", "NOGDIBLT", "NOPIXELFORMAT",
};

static char *Flag4Names[32]={
	"NOALPHACHANNEL", "SUPPRESSCHILD", "FIXREFCOUNTER", "SHOWTIMESTRETCH",
	"ZBUFFERCLEAN", "ZBUFFER0CLEAN", "ZBUFFERALWAYS", "DISABLEFOGGING",
	"NOPOWER2FIX", "NOPERFCOUNTER", "BILINEAR2XFILTER", "INTERCEPTRDTSC",
	"LIMITSCREENRES", "NOFILLRECT", "HOOKGLIDE", "HIDEDESKTOP",
	"STRETCHTIMERS", "NOFLIPEMULATION", "NOTEXTURES", "RETURNNULLREF",
	"FINETIMING", "NATIVERES", "SUPPORTSVGA", "SUPPORTHDTV",
	"RELEASEMOUSE", "ENABLETIMEFREEZE", "HOTPATCH", "ENABLEHOTKEYS",
	"HOTPATCHALWAYS", "NOD3DRESET", "OVERRIDEREGISTRY", "HIDECDROMEMPTY",
};

static char *Flag5Names[32]={
	"DIABLOTWEAK", "CLEARTARGET", "NOWINPOSCHANGES", "ANSIWIDE",
	"NOBLT", "USELASTCORE", "DOFASTBLT", "AEROBOOST",
	"QUARTERBLT", "NOIMAGEHLP", "BILINEARFILTER", "REPLACEPRIVOPS",
	"REMAPMCI", "TEXTUREHIGHLIGHT", "TEXTUREDUMP", "TEXTUREHACK",
	"TEXTURETRANSP", "NORMALIZEPERFCOUNT", "HYBRIDMODE", "GDICOLORCONV",
	"INJECTSON", "ENABLESONHOOK", "FREEZEINJECTEDSON", "GDIMODE",
	"CENTERTOWIN", "STRESSRESOURCES", "MESSAGEPUMP", "TEXTUREFORMAT", 
	"DEINTERLACE", "LOCKRESERVEDPALETTE", "UNLOCKZORDER", "EASPORTSHACK",
};

static char *Flag6Names[32]={
	"FORCESWAPEFFECT", "LEGACYALLOC", "NODESTROYWINDOW", "NOMOVIES",
	"SUPPRESSRELEASE", "FIXMOVIESCOLOR", "WOW64REGISTRY", "DISABLEMAXWINMODE",
	"FIXPITCH", "POWER2WIDTH", "HIDETASKBAR", "ACTIVATEAPP",
	"NOSYSMEMPRIMARY", "NOSYSMEMBACKBUF", "CONFIRMONCLOSE", "TERMINATEONCLOSE",
	"FLIPEMULATION", "SETZBUFFERBITDEPTHS", "SHAREDDC", "WOW32REGISTRY",
	"STRETCHMOVIES", "BYPASSMCI", "FIXPIXELZOOM", "REUSEEMULATEDDC",
	"CREATEDESKTOP", "NOWINDOWHOOKS", "SYNCPALETTE", "VIRTUALJOYSTICK",
	"UNACQUIRE", "HOOKGOGLIBS", "BYPASSGOGLIBS", "EMULATERELMOUSE",
};

static char *Flag7Names[32]={
	"LIMITDDRAW", "DISABLEDISABLEALTTAB", "FIXCLIPPERAREA", "HOOKDIRECTSOUND",
	"HOOKSMACKW32", "BLOCKPRIORITYCLASS", "CPUSLOWDOWN", "CPUMAXUSAGE",
	"NOWINERRORS", "SUPPRESSOVERLAY", "INIT24BPP", "INIT32BPP",
	"", "", "", "",
	"", "", "", "",
	"", "", "", "",
	"", "", "", "",
	"", "", "", "",
};

static char *Flag8Names[32]={
	"", "", "", "",
	"", "", "", "",
	"", "", "", "",
	"", "", "", "",
	"", "", "", "",
	"", "", "", "",
	"", "", "", "",
	"", "", "", "",
};

static char *TFlagNames[32]={
	"OUTTRACE", "OUTDDRAWTRACE", "OUTWINMESSAGES", "OUTCURSORTRACE",
	"**", "**", "ASSERTDIALOG", "OUTIMPORTTABLE",
	"OUTDEBUG", "OUTREGISTRY", "TRACEHOOKS", "OUTD3DTRACE",
	"OUTDXWINTRACE", "ADDTIMESTAMP", "OUTDEBUGSTRING", "ERASELOGFILE",
	"", "", "", "",
	"", "", "", "",
	"", "", "", "",
	"", "", "", "",
};

char *GetDxWndPath()
{
	static BOOL DoOnce = TRUE;
	static char sFolderPath[MAX_PATH];

	if(DoOnce){
		DWORD dwAttrib;		
		dwAttrib = GetFileAttributes("dxwnd.dll");
		//if (dwAttrib != INVALID_FILE_ATTRIBUTES && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY)) {
		//	MessageBox(0, "DXWND: ERROR can't locate itself", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		//	exit(0);
		//}
		GetModuleFileName(GetModuleHandle("dxwnd"), sFolderPath, MAX_PATH);
		sFolderPath[strlen(sFolderPath)-strlen("dxwnd.dll")] = 0; // terminate the string just before "dxwnd.dll"
		DoOnce = FALSE;
	}

	return sFolderPath;
}

static void OutTraceHeader(FILE *fp)
{
	SYSTEMTIME Time;
	char Version[20+1];
	int i;
	DWORD dword;
	GetLocalTime(&Time);
	GetDllVersion(Version);
	fprintf(fp, "*** DxWnd %s log BEGIN: %02d-%02d-%04d %02d:%02d:%02d ***\n",
		Version, Time.wDay, Time.wMonth, Time.wYear, Time.wHour, Time.wMinute, Time.wSecond);
	fprintf(fp, "*** Flags= ");
	for(i=0, dword=dxw.dwFlags1;  i<32; i++, dword>>=1) if(dword & 0x1) fprintf(fp, "%s ", FlagNames[i]);
	for(i=0, dword=dxw.dwFlags2; i<32; i++, dword>>=1) if(dword & 0x1) fprintf(fp, "%s ", Flag2Names[i]);
	for(i=0, dword=dxw.dwFlags3; i<32; i++, dword>>=1) if(dword & 0x1) fprintf(fp, "%s ", Flag3Names[i]);
	for(i=0, dword=dxw.dwFlags4; i<32; i++, dword>>=1) if(dword & 0x1) fprintf(fp, "%s ", Flag4Names[i]);
	for(i=0, dword=dxw.dwFlags5; i<32; i++, dword>>=1) if(dword & 0x1) fprintf(fp, "%s ", Flag5Names[i]);
	for(i=0, dword=dxw.dwFlags6; i<32; i++, dword>>=1) if(dword & 0x1) fprintf(fp, "%s ", Flag6Names[i]);
	for(i=0, dword=dxw.dwFlags7; i<32; i++, dword>>=1) if(dword & 0x1) fprintf(fp, "%s ", Flag7Names[i]);
	for(i=0, dword=dxw.dwFlags8; i<32; i++, dword>>=1) if(dword & 0x1) fprintf(fp, "%s ", Flag8Names[i]);
	for(i=0, dword=dxw.dwTFlags; i<32; i++, dword>>=1) if(dword & 0x1) fprintf(fp, "%s ", TFlagNames[i]);
	fprintf(fp, "***\n");
}

#define DXWMAXLOGSIZE 4096

void OutTrace(const char *format, ...)
{
	va_list al;
	static char path[MAX_PATH];
	static FILE *fp=NULL; // GHO: thread safe???
	char sBuf[DXWMAXLOGSIZE+1];
	DWORD tFlags;
	static GetTickCount_Type pGetTick;

	// check global log flag
	if(!(dxw.dwTFlags & (OUTTRACE|OUTDEBUGSTRING))) return;
	tFlags = dxw.dwTFlags;
	dxw.dwTFlags = 0x0; // to avoid possible log recursion while loading C runtime libraries!!!

	WaitForSingleObject(hTraceMutex, INFINITE);
	if (fp == NULL){
		char *OpenMode = (tFlags & ERASELOGFILE) ? "w+" : "a+";
		pGetTick = GetTickCount; // save function pointer
		GetCurrentDirectory(MAX_PATH, path);
		strcat(path, "\\dxwnd.log");
		fp = fopen(path, OpenMode);
		if (fp==NULL){ // in case of error (e.g. current dir on unwritable CD unit)... 
			strcpy(path, GetDxWndPath());
			strcat(path, "\\dxwnd.log");
			fp = fopen(path, OpenMode);
		}
		if (fp==NULL)
			return; // last chance: do not log... 
		else 
			OutTraceHeader(fp);
	}
	va_start(al, format);
	//vfprintf(fp, format, al);
	vsprintf_s(sBuf, DXWMAXLOGSIZE, format, al);
	sBuf[DXWMAXLOGSIZE]=0; // just in case of log truncation
	va_end(al);
	if(tFlags & OUTTRACE) {
		if(tFlags & ADDTIMESTAMP) {
			DWORD tCount = (*pGetTick)();
			if (tFlags & ADDRELATIVETIME){
				static DWORD tLastCount = 0;
				DWORD tNow;
				tNow = tCount;
				tCount = tLastCount ? (tCount - tLastCount) : 0;
				tLastCount = tNow;
			}
			fprintf(fp, "%08.8d: ", tCount);
		}
		fputs(sBuf, fp);
		fflush(fp); 
	}
	if(tFlags & OUTDEBUGSTRING) OutputDebugString(sBuf);
	ReleaseMutex(hTraceMutex);

	dxw.dwTFlags = tFlags; // restore settings
}

#ifdef CHECKFORCOMPATIBILITYFLAGS
static BOOL CheckCompatibilityFlags()
{
	typedef DWORD (WINAPI *GetFileVersionInfoSizeA_Type)(LPCSTR, LPDWORD);
	typedef BOOL (WINAPI *GetFileVersionInfoA_Type)(LPCSTR, DWORD, DWORD, LPVOID);
	typedef BOOL (WINAPI *VerQueryValueA_Type)(LPCVOID, LPCSTR, LPVOID, PUINT);
	VerQueryValueA_Type pVerQueryValueA;
	GetFileVersionInfoA_Type pGetFileVersionInfoA;
	GetFileVersionInfoSizeA_Type pGetFileVersionInfoSizeA;

	HMODULE VersionLib;
	DWORD dwMajorVersion, dwMinorVersion;
	DWORD dwHandle = 0;
	int size;
	UINT len = 0;
    VS_FIXEDFILEINFO*   vsfi = NULL;
	OSVERSIONINFO vi;

	if(!(VersionLib=LoadLibrary("Version.dll"))) return FALSE;
	pGetFileVersionInfoA=(GetFileVersionInfoA_Type)GetProcAddress(VersionLib, "GetFileVersionInfoA");
	if(!pGetFileVersionInfoA) return FALSE;
	pGetFileVersionInfoSizeA=(GetFileVersionInfoSizeA_Type)GetProcAddress(VersionLib, "GetFileVersionInfoSizeA");
	if(!pGetFileVersionInfoSizeA) return FALSE;
	pVerQueryValueA=(VerQueryValueA_Type)GetProcAddress(VersionLib, "VerQueryValueA");
	if(!pVerQueryValueA) return FALSE;

	size = (*pGetFileVersionInfoSizeA)("kernel32.dll", &dwHandle);	
	BYTE* VersionInfo = new BYTE[size];
	(*pGetFileVersionInfoA)("kernel32.dll", dwHandle, size, VersionInfo);
    (*pVerQueryValueA)(VersionInfo, "\\", (void**)&vsfi, &len);
	dwMajorVersion=HIWORD(vsfi->dwProductVersionMS);
	dwMinorVersion=LOWORD(vsfi->dwProductVersionMS);
    delete[] VersionInfo;	
	vi.dwOSVersionInfoSize=sizeof(vi);
	GetVersionExA(&vi);
	if((vi.dwMajorVersion!=dwMajorVersion) || (vi.dwMinorVersion!=dwMinorVersion)) {
		MessageBox(NULL, "Compatibility settings detected!", "DxWnd", MB_OK);
		return TRUE;
	}
	return FALSE;
}
#endif

void OutTraceHex(BYTE *bBuf, int iLen)
{
	for(int i=0; i<iLen; i++) OutTrace("%02X ", *(bBuf++));
	OutTrace("\n");
}

void SuppressIMEWindow()
{
	OutTraceDW("WindowProc: SUPPRESS IME\n");
	typedef BOOL (WINAPI *ImmDisableIME_Type)(DWORD);
	ImmDisableIME_Type pImmDisableIME;
	HMODULE ImmLib;
	ImmLib=(*pLoadLibraryA)("Imm32");
	if(ImmLib){
		pImmDisableIME=(ImmDisableIME_Type)(*pGetProcAddress)(ImmLib,"ImmDisableIME");
		if(pImmDisableIME)(*pImmDisableIME)(-1);
		FreeLibrary(ImmLib);
	}
}

void HookDlls(HMODULE module)
{
	PIMAGE_NT_HEADERS pnth;
	PIMAGE_IMPORT_DESCRIPTOR pidesc;
	DWORD base, rva;
	PSTR impmodule;
	PIMAGE_THUNK_DATA ptname;
	extern char *SysNames[];

	base=(DWORD)module;
	OutTraceB("HookDlls: base=%x\n", base);
	__try{
		pnth = PIMAGE_NT_HEADERS(PBYTE(base) + PIMAGE_DOS_HEADER(base)->e_lfanew);
		if(!pnth) {
			OutTraceH("HookDlls: ERROR no pnth at %d\n", __LINE__);
			return;
		}
		rva = pnth->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
		if(!rva) {
			OutTraceH("HookDlls: ERROR no rva at %d\n", __LINE__);
			return;
		}

		for(pidesc = (PIMAGE_IMPORT_DESCRIPTOR)(base + rva); pidesc->FirstThunk; pidesc++){
			HMODULE DllBase;
			int idx;
			extern HMODULE SysLibs[];

			impmodule = (PSTR)(base + pidesc->Name);

			// skip dxwnd and system dll
			if(!lstrcmpi(impmodule, "DxWnd")) continue; 
			idx=dxw.GetDLLIndex(impmodule);
			if(!lstrcmpi(impmodule,dxw.CustomOpenGLLib))idx=SYSLIBIDX_OPENGL;
			if(idx != -1) {
				DllBase=GetModuleHandle(impmodule);
				SysLibs[idx]=DllBase;
				OutTraceH("HookDlls: system module %s at %x\n", impmodule, DllBase);
				continue;
			}

			OutTraceH("HookDlls: ENTRY timestamp=%x module=%s forwarderchain=%x\n", 
				pidesc->TimeDateStamp, impmodule, pidesc->ForwarderChain);
			if(pidesc->OriginalFirstThunk) {
				ptname = (PIMAGE_THUNK_DATA)(base + (DWORD)pidesc->OriginalFirstThunk);
			}
			else{
				ptname = 0;
				OutTraceH("HookDlls: no PE OFTs - stripped module=%s\n", impmodule);
			}

			DllBase=GetModuleHandle(impmodule);
			if(DllBase) HookModule(DllBase, 0);
		}
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{       
		OutTraceDW("HookDlls: EXCEPTION\n");
	}
	return;
}

void DumpImportTable(HMODULE module)
{
	PIMAGE_NT_HEADERS pnth;
	PIMAGE_IMPORT_DESCRIPTOR pidesc;
	DWORD base, rva;
	PSTR impmodule;
	PIMAGE_THUNK_DATA ptaddr;
	PIMAGE_THUNK_DATA ptname;
	PIMAGE_IMPORT_BY_NAME piname;

	base=(DWORD)module;
	OutTrace("DumpImportTable: base=%x\n", base);
	__try{
		pnth = PIMAGE_NT_HEADERS(PBYTE(base) + PIMAGE_DOS_HEADER(base)->e_lfanew);
		if(!pnth) {
			OutTrace("DumpImportTable: ERROR no pnth at %d\n", __LINE__);
			return;
		}
		rva = pnth->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
		if(!rva) {
			OutTrace("DumpImportTable: ERROR no rva at %d\n", __LINE__);
			return;
		}
		pidesc = (PIMAGE_IMPORT_DESCRIPTOR)(base + rva);

		while(pidesc->FirstThunk){
			impmodule = (PSTR)(base + pidesc->Name);
			OutTrace("DumpImportTable: ENTRY timestamp=%x module=%s forwarderchain=%x\n", 
				pidesc->TimeDateStamp, impmodule, pidesc->ForwarderChain);
			if(pidesc->OriginalFirstThunk) {
				ptname = (PIMAGE_THUNK_DATA)(base + (DWORD)pidesc->OriginalFirstThunk);
			}
			else{
				ptname = 0;
				OutTrace("DumpImportTable: no PE OFTs - stripped module=%s\n", impmodule);
			}
			ptaddr = (PIMAGE_THUNK_DATA)(base + (DWORD)pidesc->FirstThunk);
			while(ptaddr->u1.Function){
				OutTrace("addr=%x", ptaddr->u1.Function); 
				ptaddr ++;
				if(ptname){
					if(!IMAGE_SNAP_BY_ORDINAL(ptname->u1.Ordinal)){
						piname = (PIMAGE_IMPORT_BY_NAME)(base + (DWORD)ptname->u1.AddressOfData);
						OutTrace(" hint=%x name=%s", piname->Hint, piname->Name);
						ptname ++;
					}
				}
				OutTrace("\n");
			}
			OutTrace("*** EOT ***\n", ptaddr->u1.Function); 
			pidesc ++;
		}
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{       
		OutTraceDW("DumpImportTable: EXCEPTION\n");
	}
	return;
}

void SetHook(void *target, void *hookproc, void **hookedproc, char *hookname)
{
	void *tmp;
	char msg[201];
	DWORD dwTmp, oldprot;
	
	OutTraceH("SetHook: DEBUG target=%x, proc=%x name=%s\n", target, hookproc, hookname);
	// keep track of hooked call range to avoid re-hooking of hooked addresses !!!
	dwTmp = *(DWORD *)target;
	if(dwTmp == (DWORD)hookproc) {
		OutTraceH("target already hooked\n");
		return; // already hooked
	}
	if(*(DWORD *)hookedproc == (DWORD)hookproc) {
		OutTraceH("hook already hooked\n");
		return; // already hooked
	}
	if(dwTmp == 0){
		sprintf(msg,"SetHook ERROR: NULL target for %s\n", hookname);
		OutTraceDW(msg);
		if (IsAssertEnabled) MessageBox(0, msg, "SetHook", MB_OK | MB_ICONEXCLAMATION);
		return; // error condition
	}
	if(!VirtualProtect(target, 4, PAGE_READWRITE, &oldprot)) {
		sprintf(msg,"SetHook ERROR: target=%x err=%d\n", target, GetLastError());
		OutTraceDW(msg);
		if (IsAssertEnabled) MessageBox(0, msg, "SetHook", MB_OK | MB_ICONEXCLAMATION);
		return; // error condition
	}
	*(DWORD *)target = (DWORD)hookproc;
	if(!VirtualProtect(target, 4, oldprot, &oldprot)){
		OutTrace("SetHook: VirtualProtect ERROR target=%x, err=%x\n", target, GetLastError());
		return; // error condition
	}
	if(!FlushInstructionCache(GetCurrentProcess(), target, 4)){
		OutTrace("SetHook: FlushInstructionCache ERROR target=%x, err=%x\n", target, GetLastError());
		return; // error condition
	}
	tmp=(void *)dwTmp;

	if (*hookedproc && *hookedproc!=tmp) {
		sprintf(msg,"SetHook: proc=%s oldhook=%x->%x newhook=%x\n", hookname, hookedproc, *(DWORD *)hookedproc, tmp);
		OutTraceDW(msg);
		if (IsAssertEnabled) MessageBox(0, msg, "SetHook", MB_OK | MB_ICONEXCLAMATION);
		tmp = *hookedproc;
	}
	*hookedproc = tmp;
	OutTraceH("SetHook: DEBUG2 *hookedproc=%x, name=%s\n", tmp, hookname);
}

// v2.02.53: thorough scan - the IAT is scanned considering the possibility to have each dll module 
// replicated also many times. It may depend upon the compiling environment? 
// So far, it makes the difference for Dungeon Odissey

void *HookAPI(HMODULE module, char *dll, void *apiproc, const char *apiname, void *hookproc)
{
	if(dxw.dwTFlags & OUTIMPORTTABLE) OutTrace("HookAPI: module=%x dll=%s apiproc=%x apiname=%s hookproc=%x\n", 
		module, dll, apiproc, apiname, hookproc);

	if(!*apiname) { // check
		char *sMsg="HookAPI: NULL api name\n";
		OutTraceE(sMsg);
		if (IsAssertEnabled) MessageBox(0, sMsg, "HookAPI", MB_OK | MB_ICONEXCLAMATION);
		return 0;
	}

	if(dxw.dwFlags4 & HOTPATCHALWAYS) {
		void *orig;
		orig=HotPatch(apiproc, apiname, hookproc);
		if(orig) return orig;
	}

	return IATPatch(module, dll, apiproc, apiname, hookproc);
}

// v.2.1.80: unified positioning logic into CalculateWindowPos routine
// now taking in account for window menus (see "Alien Cabal")

void CalculateWindowPos(HWND hwnd, DWORD width, DWORD height, LPWINDOWPOS wp)
{
	RECT rect, desktop, workarea;
	DWORD dwStyle, dwExStyle;
	int MaxX, MaxY;
	HMENU hMenu;

	switch(dxw.Coordinates){
	case DXW_DESKTOP_CENTER:
		MaxX = dxw.iSizX;
		MaxY = dxw.iSizY;
		if (!MaxX) {
			MaxX = width;
			if(dxw.dwFlags4 & BILINEAR2XFILTER) MaxX <<= 1; // double
		}
		if (!MaxY) {
			MaxY = height;
			if(dxw.dwFlags4 & BILINEAR2XFILTER) MaxY <<= 1; // double
		}
		//GetClientRect(0, &desktop);
		(*pGetClientRect)((*pGetDesktopWindow)(), &desktop);
		rect.left =  (desktop.right - MaxX) / 2;
		rect.top = (desktop.bottom - MaxY) / 2;
		rect.right = rect.left + MaxX;
		rect.bottom = rect.top + MaxY; //v2.02.09
		break;
	case DXW_DESKTOP_WORKAREA:
		(*pSystemParametersInfoA)(SPI_GETWORKAREA, NULL, &workarea, 0);
		rect = workarea;
		break;
	case DXW_DESKTOP_FULL:
		(*pGetClientRect)((*pGetDesktopWindow)(), &workarea);
		rect = workarea;
		break;
	case DXW_SET_COORDINATES:
	default:
		rect.left =  dxw.iPosX;
		rect.top = dxw.iPosY; //v2.02.09
		MaxX = dxw.iSizX;
		MaxY = dxw.iSizY;
		if (!MaxX) MaxX = width;
		if (!MaxY) MaxY = height;
		rect.right = dxw.iPosX + MaxX;
		rect.bottom = dxw.iPosY + MaxY; //v2.02.09
		break;
	}

	RECT UnmappedRect;
	UnmappedRect=rect;
	dwStyle=(*pGetWindowLong)(hwnd, GWL_STYLE);
	dwExStyle=(*pGetWindowLong)(hwnd, GWL_EXSTYLE);
	// BEWARE: from MSDN -  If the window is a child window, the return value is undefined. 
	hMenu = (dwStyle & WS_CHILD) ? NULL : GetMenu(hwnd);	
	AdjustWindowRectEx(&rect, dwStyle, (hMenu!=NULL), dwExStyle);
	if (hMenu) __try {CloseHandle(hMenu);} __except(EXCEPTION_EXECUTE_HANDLER){};
	switch(dxw.Coordinates){
	case DXW_DESKTOP_WORKAREA:
	case DXW_DESKTOP_FULL:
		// if there's a menu, reduce height to fit area
		if(rect.top != UnmappedRect.top){
			rect.bottom = rect.bottom - UnmappedRect.top + rect.top;
		}
		break;
	default:
		break;
	}

	// shift down-right so that the border is visible
	if(rect.left < dxw.VirtualDesktop.left){
		rect.right = dxw.VirtualDesktop.left - rect.left + rect.right;
		rect.left = dxw.VirtualDesktop.left;
	}
	if(rect.top < dxw.VirtualDesktop.top){
		rect.bottom = dxw.VirtualDesktop.top - rect.top + rect.bottom;
		rect.top = dxw.VirtualDesktop.top;
	}

	// shift up-left so that the windows doesn't exceed on the other side
	if(rect.right > dxw.VirtualDesktop.right){
		rect.left = dxw.VirtualDesktop.right - rect.right + rect.left;
		rect.right = dxw.VirtualDesktop.right;
	}
	if(rect.bottom > dxw.VirtualDesktop.bottom){
		rect.top = dxw.VirtualDesktop.bottom - rect.bottom + rect.top;
		rect.bottom = dxw.VirtualDesktop.bottom;
	}

	// update the arguments for the window creation
	wp->x=rect.left;
	wp->y=rect.top;
	wp->cx=rect.right-rect.left;
	wp->cy=rect.bottom-rect.top;
}

void AdjustWindowPos(HWND hwnd, DWORD width, DWORD height)
{
	WINDOWPOS wp;
	OutTraceDW("AdjustWindowPos: hwnd=%x, size=(%d,%d)\n", hwnd, width, height);
	CalculateWindowPos(hwnd, width, height, &wp);
	OutTraceDW("AdjustWindowPos: fixed pos=(%d,%d) size=(%d,%d)\n", wp.x, wp.y, wp.cx, wp.cy);
	//if(!pSetWindowPos) pSetWindowPos=SetWindowPos;
	//OutTraceDW("pSetWindowPos=%x\n", pSetWindowPos);
	OutTraceDW("hwnd=%x pos=(%d,%d) size=(%d,%d)\n", pSetWindowPos, wp.x, wp.y, wp.cx, wp.cy);
	if(!(*pSetWindowPos)(hwnd, 0, wp.x, wp.y, wp.cx, wp.cy, 0)){
		OutTraceE("AdjustWindowPos: ERROR err=%d at %d\n", GetLastError(), __LINE__);
	}

	if(dxw.dwFlags2 & SUPPRESSIME) SuppressIMEWindow();
	dxw.ShowBanner(hwnd);
	if(dxw.dwFlags4 & HIDEDESKTOP) dxw.HideDesktop(dxw.GethWnd());
	return;
}

void HookWindowProc(HWND hwnd)
{
	WNDPROC pWindowProc;

	if(dxw.dwFlags6 & NOWINDOWHOOKS) return;

	pWindowProc = (WNDPROC)(*pGetWindowLong)(hwnd, GWL_WNDPROC);
	// don't hook twice ....	
	if ((pWindowProc == extWindowProc) ||
		(pWindowProc == extChildWindowProc) ||
		(pWindowProc == extDialogWindowProc)){
		// hooked already !!!
		OutTraceDW("GetWindowLong: hwnd=%x WindowProc HOOK already in place\n", hwnd);
		return;
	}

	// v2.03.22: don't remap WindowProc in case of special address 0xFFFFnnnn. 
	// This makes "The Hulk demo" work avoiding WindowProc recursion and stack overflow
	if (((DWORD)pWindowProc & 0xFFFF0000) == 0xFFFF0000){
		OutTraceDW("GetWindowLong: hwnd=%x WindowProc HOOK %x not updated\n", hwnd, pWindowProc);
		return;
	}

	long lres;
	dxwws.PutProc(hwnd, pWindowProc);
	lres=(*pSetWindowLongA)(hwnd, GWL_WNDPROC, (LONG)extWindowProc);
	OutTraceDW("SetWindowLong: HOOK hwnd=%x WindowProc=%x->%x\n", hwnd, lres, (LONG)extWindowProc);
}

void AdjustWindowFrame(HWND hwnd, DWORD width, DWORD height)
{
	HRESULT res=0;
	LONG style;

	OutTraceDW("AdjustWindowFrame hwnd=%x, size=(%d,%d) coord=%d\n", hwnd, width, height, dxw.Coordinates); 

	dxw.SetScreenSize(width, height);
	if (hwnd==NULL) return;

	switch(dxw.Coordinates){
		case DXW_SET_COORDINATES:
		case DXW_DESKTOP_CENTER:
			style = (dxw.dwFlags2 & MODALSTYLE) ?  0 : WS_OVERLAPPEDWINDOW;
			break;
		case DXW_DESKTOP_WORKAREA:
		case DXW_DESKTOP_FULL:
			style = 0;
			break;
	}

	(*pSetWindowLongA)(hwnd, GWL_STYLE, style);
	(*pSetWindowLongA)(hwnd, GWL_EXSTYLE, 0); 
	(*pShowWindow)(hwnd, SW_SHOWNORMAL);
	OutTraceDW("AdjustWindowFrame hwnd=%x, set style=%s extstyle=0\n", hwnd, (style == 0) ? "0" : "WS_OVERLAPPEDWINDOW"); 
	AdjustWindowPos(hwnd, width, height);

	// fixing windows message handling procedure
	HookWindowProc(hwnd);

	// fixing cursor view and clipping region

	if ((dxw.dwFlags1 & HIDEHWCURSOR) && dxw.IsFullScreen()) while ((*pShowCursor)(0) >= 0);
	if (dxw.dwFlags2 & SHOWHWCURSOR) while((*pShowCursor)(1) < 0);
	if (dxw.dwFlags1 & CLIPCURSOR) {
		OutTraceDW("AdjustWindowFrame: setting clip region\n");
		dxw.SetClipCursor();
	}

	(*pInvalidateRect)(hwnd, NULL, TRUE);
}

void HookSysLibsInit()
{
	static BOOL DoOnce = FALSE;
	if(DoOnce) return;
	DoOnce=TRUE;

	HookKernel32Init();
	HookUser32Init();
	HookGDI32Init();
	HookImagehlpInit();
}

DEVMODE InitDevMode;

static void SaveScreenMode()
{
	static BOOL DoOnce=FALSE;
	if(DoOnce) return;
	DoOnce=TRUE;
	(*pEnumDisplaySettings)(NULL, ENUM_CURRENT_SETTINGS, &InitDevMode);
	OutTraceDW("DXWND: Initial display mode WxH=(%dx%d) BitsPerPel=%d\n", 
		InitDevMode.dmPelsWidth, InitDevMode.dmPelsHeight, InitDevMode.dmBitsPerPel);
}

void RecoverScreenMode()
{
	DEVMODE CurrentDevMode;
	BOOL res;
	(*pEnumDisplaySettings)(NULL, ENUM_CURRENT_SETTINGS, &CurrentDevMode);
	OutTraceDW("ChangeDisplaySettings: recover CURRENT WxH=(%dx%d) BitsPerPel=%d TARGET WxH=(%dx%d) BitsPerPel=%d\n", 
		CurrentDevMode.dmPelsWidth, CurrentDevMode.dmPelsHeight, CurrentDevMode.dmBitsPerPel,
		InitDevMode.dmPelsWidth, InitDevMode.dmPelsHeight, InitDevMode.dmBitsPerPel);
	InitDevMode.dmFields = (DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT);
	res=(*pChangeDisplaySettingsExA)(NULL, &InitDevMode, NULL, 0, NULL);
	if(res) OutTraceE("ChangeDisplaySettings: ERROR err=%d at %d\n", GetLastError(), __LINE__);
}

void SwitchTo16BPP()
{
	DEVMODE CurrentDevMode;
	BOOL res;
	(*pEnumDisplaySettings)(NULL, ENUM_CURRENT_SETTINGS, &CurrentDevMode);
	OutTraceDW("ChangeDisplaySettings: CURRENT wxh=(%dx%d) BitsPerPel=%d -> 16\n", 
		CurrentDevMode.dmPelsWidth, CurrentDevMode.dmPelsHeight, CurrentDevMode.dmBitsPerPel);
	CurrentDevMode.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
	CurrentDevMode.dmBitsPerPel = 16;
	res=(*pChangeDisplaySettingsExA)(NULL, &CurrentDevMode, NULL, CDS_UPDATEREGISTRY, NULL);
	if(res) OutTraceE("ChangeDisplaySettings: ERROR err=%d at %d\n", GetLastError(), __LINE__);
}

static void LockScreenMode(DWORD dmPelsWidth, DWORD dmPelsHeight, DWORD dmBitsPerPel)
{
	DEVMODE InitDevMode;
	BOOL res;
	OutTraceDW("ChangeDisplaySettings: LOCK wxh=(%dx%d) BitsPerPel=%d -> wxh=(%dx%d) BitsPerPel=%d\n", 
		InitDevMode.dmPelsWidth, InitDevMode.dmPelsHeight, InitDevMode.dmBitsPerPel,
		dmPelsWidth, dmPelsHeight, dmBitsPerPel);
	if( (dmPelsWidth != InitDevMode.dmPelsWidth) ||
		(dmPelsHeight !=InitDevMode.dmPelsHeight) ||
		(dmBitsPerPel != InitDevMode.dmBitsPerPel)){
		res=(*pChangeDisplaySettingsExA)(NULL, &InitDevMode, NULL, 0, NULL);
		if(res) OutTraceE("ChangeDisplaySettings: ERROR err=%d at %d\n", GetLastError(), __LINE__);
	}
}

static HMODULE LoadDisasm()
{
	HMODULE disasmlib;
	
	disasmlib=(*pLoadLibraryA)("disasm.dll");
	if(!disasmlib) {
		OutTraceDW("DXWND: Load lib=\"%s\" failed err=%d\n", "disasm.dll", GetLastError());
		return NULL;
	}
	pGeterrwarnmessage=(Geterrwarnmessage_Type)(*pGetProcAddress)(disasmlib, "Geterrwarnmessage");
	pPreparedisasm=(Preparedisasm_Type)(*pGetProcAddress)(disasmlib, "Preparedisasm");
	pFinishdisasm=(Finishdisasm_Type)(*pGetProcAddress)(disasmlib, "Finishdisasm");
	pDisasm=(Disasm_Type)(*pGetProcAddress)(disasmlib, "Disasm");
	//OutTraceDW("DXWND: Load disasm.dll ptrs=%x,%x,%x,%x\n", pGeterrwarnmessage, pPreparedisasm, pFinishdisasm, pDisasm);

	return disasmlib;
}

LONG WINAPI myUnhandledExceptionFilter(LPEXCEPTION_POINTERS ExceptionInfo)
{
	OutTrace("UnhandledExceptionFilter: exception code=%x flags=%x addr=%x\n",
		ExceptionInfo->ExceptionRecord->ExceptionCode,
		ExceptionInfo->ExceptionRecord->ExceptionFlags,
		ExceptionInfo->ExceptionRecord->ExceptionAddress);
	DWORD oldprot;
	static HMODULE disasmlib = NULL;
	PVOID target = ExceptionInfo->ExceptionRecord->ExceptionAddress;
	switch(ExceptionInfo->ExceptionRecord->ExceptionCode){
	case 0xc0000094: // IDIV reg (Ultim@te Race Pro)
	case 0xc0000095: // DIV by 0 (divide overflow) exception (SonicR)
	case 0xc0000096: // CLI Priviliged instruction exception (Resident Evil), FB (Asterix & Obelix)
	case 0xc000001d: // FEMMS (eXpendable)
	case 0xc0000005: // Memory exception (Tie Fighter)
		int cmdlen;
		t_disasm da;
		if(!disasmlib){
			if (!(disasmlib=LoadDisasm())) return EXCEPTION_CONTINUE_SEARCH;
			(*pPreparedisasm)();
		}
		if(!VirtualProtect(target, 10, PAGE_READWRITE, &oldprot)) return EXCEPTION_CONTINUE_SEARCH; // error condition
		cmdlen=(*pDisasm)((BYTE *)target, 10, 0, &da, 0, NULL, NULL);
		OutTrace("UnhandledExceptionFilter: NOP opcode=%x len=%d\n", *(BYTE *)target, cmdlen);
		memset((BYTE *)target, 0x90, cmdlen); 
		VirtualProtect(target, 10, oldprot, &oldprot);
		if(!FlushInstructionCache(GetCurrentProcess(), target, cmdlen))
			OutTrace("UnhandledExceptionFilter: FlushInstructionCache ERROR target=%x, err=%x\n", target, GetLastError());
		// v2.03.10 skip replaced opcode
		ExceptionInfo->ContextRecord->Eip += cmdlen; // skip ahead op-code length
		return EXCEPTION_CONTINUE_EXECUTION;
		break;
	default:
		return EXCEPTION_CONTINUE_SEARCH;
	}
}

LPTOP_LEVEL_EXCEPTION_FILTER WINAPI extSetUnhandledExceptionFilter(LPTOP_LEVEL_EXCEPTION_FILTER lpTopLevelExceptionFilter)
{
	OutTraceDW("SetUnhandledExceptionFilter: lpExceptionFilter=%x\n", lpTopLevelExceptionFilter);
	extern LONG WINAPI myUnhandledExceptionFilter(LPEXCEPTION_POINTERS);
	return (*pSetUnhandledExceptionFilter)(myUnhandledExceptionFilter);
}

//#define USEPERFCOUNTERS

unsigned __int64 inline GetRDTSC() {
   __asm {
      ; Flush the pipeline
      XOR eax, eax
      CPUID
      ; Get RDTSC counter in edx:eax
      RDTSC
   }
}

LONG CALLBACK Int3Handler(PEXCEPTION_POINTERS ExceptionInfo)
{
	// Vectored Handler routine to intercept INT3 opcodes replacing RDTSC
 	if (ExceptionInfo->ExceptionRecord->ExceptionCode == 0x80000003){
		LARGE_INTEGER myPerfCount;
#ifdef USEPERFCOUNTERS
		if(!pQueryPerformanceCounter) pQueryPerformanceCounter=QueryPerformanceCounter;
		(*pQueryPerformanceCounter)(&myPerfCount);
#else
	   __asm {
		  ; Flush the pipeline
		  XOR eax, eax
		  CPUID
		  ; Get RDTSC counter in edx:eax
		  RDTSC 
		  mov myPerfCount.LowPart, eax
		  mov myPerfCount.HighPart, edx
	   }
#endif
		//myPerfCount = dxw.StretchCounter(myPerfCount);
		myPerfCount = dxw.StretchLargeCounter(myPerfCount);
		OutTraceDW("Int3Handler: INT3 at=%x tick=%x RDTSC=%x:%x\n", 
			ExceptionInfo->ExceptionRecord->ExceptionAddress, (*pGetTickCount)(), myPerfCount.HighPart, myPerfCount.LowPart);

		ExceptionInfo->ContextRecord->Edx = myPerfCount.HighPart;
		ExceptionInfo->ContextRecord->Eax = myPerfCount.LowPart;
		ExceptionInfo->ContextRecord->Eip++; // skip ahead one-byte ( jump over 0xCC op-code )
		ExceptionInfo->ContextRecord->Eip++; // skip ahead one-byte ( jump over 0x90 op-code )
		return EXCEPTION_CONTINUE_EXECUTION;
	}
	return EXCEPTION_CONTINUE_SEARCH;
}

void HookExceptionHandler(void)
{
	void *tmp;
	HMODULE base;

	OutTraceDW("Set exception handlers\n");
	base=GetModuleHandle(NULL);
	pSetUnhandledExceptionFilter = SetUnhandledExceptionFilter;
	//v2.1.75 override default exception handler, if any....
	LONG WINAPI myUnhandledExceptionFilter(LPEXCEPTION_POINTERS);
	tmp = HookAPI(base, "KERNEL32.dll", UnhandledExceptionFilter, "UnhandledExceptionFilter", myUnhandledExceptionFilter);
	// so far, no need to save the previous handler, but anyway...
	tmp = HookAPI(base, "KERNEL32.dll", SetUnhandledExceptionFilter, "SetUnhandledExceptionFilter", extSetUnhandledExceptionFilter);
	if(tmp) pSetUnhandledExceptionFilter = (SetUnhandledExceptionFilter_Type)tmp;

	SetErrorMode(SEM_NOGPFAULTERRORBOX|SEM_NOOPENFILEERRORBOX|SEM_FAILCRITICALERRORS);
	(*pSetUnhandledExceptionFilter)((LPTOP_LEVEL_EXCEPTION_FILTER)myUnhandledExceptionFilter);
}

void HookModule(HMODULE base, int dxversion)
{
	HookKernel32(base);
	HookUser32(base);
	HookOle32(base);
	HookWinMM(base, "winmm.dll");
	if(dxw.dwFlags6 & HOOKGOGLIBS) HookWinMM(base, "win32.dll");
	//if(dxw.dwFlags2 & SUPPRESSIME) HookImeLib(module);
	HookGDI32(base);
	if(dxw.dwFlags1 & HOOKDI) HookDirectInput(base);
	if(dxw.dwFlags1 & HOOKDI8) HookDirectInput8(base);
	if(dxw.dwTargetDDVersion != HOOKDDRAWNONE) HookDirectDraw(base, dxversion);
	HookDirect3D(base, dxversion);
	HookDirect3D7(base, dxversion);
	if(dxw.dwFlags2 & HOOKOPENGL) HookOpenGLLibs(base, dxw.CustomOpenGLLib); 
	if(dxw.dwFlags4 & HOOKGLIDE) HookGlideLibs(base); 
	if( (dxw.dwFlags3 & EMULATEREGISTRY) || 
		(dxw.dwFlags4 & OVERRIDEREGISTRY) || 
		(dxw.dwFlags6 & (WOW32REGISTRY|WOW64REGISTRY)) || 
		(dxw.dwTFlags & OUTREGISTRY)) HookAdvApi32(base);
	HookMSV4WLibs(base); // -- used by Aliens & Amazons demo: what for?
	HookAVIFil32(base);
	if(dxw.dwFlags7 & HOOKSMACKW32) HookSmackW32(base);
	if(dxw.dwFlags7 & HOOKDIRECTSOUND) HookDirectSound(base); 
	//HookComDlg32(base);
}

#define USEWINNLSENABLE

void DisableIME()
{
	BOOL res;
	HMODULE hm;
	hm=GetModuleHandle("User32");
	if(hm==NULL){
		OutTrace("DisableIME: GetModuleHandle(User32) ERROR err=%d\n", GetLastError());
		return;
	}
	// here, GetProcAddress may be not hooked yet!
	if(!pGetProcAddress) pGetProcAddress=GetProcAddress;
#ifdef USEWINNLSENABLE
	typedef BOOL (WINAPI *WINNLSEnableIME_Type)(HWND, BOOL);
	WINNLSEnableIME_Type pWINNLSEnableIME;
	pWINNLSEnableIME=(WINNLSEnableIME_Type)(*pGetProcAddress)(hm, "WINNLSEnableIME");
	OutTrace("DisableIME: GetProcAddress(WINNLSEnableIME)=%x\n", pWINNLSEnableIME);
	if(!pWINNLSEnableIME) return;
	SetLastError(0);
	res=(*pWINNLSEnableIME)(NULL, FALSE);
	OutTrace("IME previous state=%x error=%d\n", res, GetLastError());
#else
	typedef LRESULT (WINAPI *SendIMEMessage_Type)(HWND, LPARAM);
	SendIMEMessage_Type pSendIMEMessage;
	//pSendIMEMessage=(SendIMEMessage_Type)(*pGetProcAddress)(hm, "SendIMEMessage");
	pSendIMEMessage=(SendIMEMessage_Type)(*pGetProcAddress)(hm, "SendIMEMessageExA");
	OutTrace("DisableIME: GetProcAddress(SendIMEMessage)=%x\n", pSendIMEMessage);
	if(!pSendIMEMessage) return;
	HGLOBAL imeh;
	IMESTRUCT *imes;
	imeh=GlobalAlloc(GMEM_MOVEABLE|GMEM_SHARE, sizeof(IMESTRUCT));
	imes=(IMESTRUCT *)imeh;
	//imes->fnc=IME_SETLEVEL;
	imes->fnc=7;
	imes->wParam=1;
	SetLastError(0);
	res=(*pSendIMEMessage)(dxw.GethWnd(), (LPARAM)imeh);
	OutTrace("res=%x error=%d\n", res, GetLastError());
#endif
}

void SetSingleProcessAffinity(BOOL first)
{
	int i;
	DWORD ProcessAffinityMask, SystemAffinityMask;
	if(!GetProcessAffinityMask(GetCurrentProcess(), &ProcessAffinityMask, &SystemAffinityMask))
		OutTraceE("GetProcessAffinityMask: ERROR err=%d\n", GetLastError());
	OutTraceDW("Process affinity=%x\n", ProcessAffinityMask);
	if(first){
		for (i=0; i<(8 * sizeof(DWORD)); i++){
			if (ProcessAffinityMask & 0x1) break;
			ProcessAffinityMask >>= 1;
		}
		OutTraceDW("First process affinity bit=%d\n", i);
		ProcessAffinityMask = 0x1;
		for (; i; i--) ProcessAffinityMask <<= 1;
		OutTraceDW("Process affinity=%x\n", ProcessAffinityMask);
	}
	else {
		for (i=0; i<(8 * sizeof(DWORD)); i++){
			if (ProcessAffinityMask & 0x80000000) break;
			ProcessAffinityMask <<= 1;
		}
		i = 31 - i;
		OutTraceDW("Last process affinity bit=%d\n", i);
		ProcessAffinityMask = 0x1;
		for (; i; i--) ProcessAffinityMask <<= 1;
		OutTraceDW("Process affinity=%x\n", ProcessAffinityMask);
	}
	if (!SetProcessAffinityMask(GetCurrentProcess(), ProcessAffinityMask))
		OutTraceE("SetProcessAffinityMask: ERROR err=%d\n", GetLastError());
}

static void ReplaceRDTSC()
{
	typedef BOOL (WINAPI *GetModuleInformation_Type)(HANDLE, HMODULE, LPMODULEINFO, DWORD);
	HMODULE disasmlib;
	unsigned char *opcodes;
	t_disasm da;
	MODULEINFO mi;
	HMODULE psapilib;
	GetModuleInformation_Type pGetModuleInformation;
	DWORD dwSegSize;
	DWORD oldprot;

	if (!(disasmlib=LoadDisasm())) return;

	// getting segment size
	psapilib=(*pLoadLibraryA)("psapi.dll");
	if(!psapilib) {
		OutTraceDW("DXWND: Load lib=\"%s\" failed err=%d\n", "psapi.dll", GetLastError());
		return;
	}
	pGetModuleInformation=(GetModuleInformation_Type)(*pGetProcAddress)(psapilib, "GetModuleInformation");
	(*pGetModuleInformation)(GetCurrentProcess(), GetModuleHandle(NULL), &mi, sizeof(mi));
	dwSegSize = mi.SizeOfImage;
	FreeLibrary(psapilib);

	(*pPreparedisasm)();
	opcodes = (unsigned char *)mi.lpBaseOfDll;
	unsigned int offset = 0;
	BOOL cont = TRUE;
	OutTraceDW("DXWND: opcode starting at addr=%x size=%x\n", opcodes, dwSegSize);
	while (cont) {
		int cmdlen = 0;
		__try{
			cmdlen=(*pDisasm)(opcodes+offset,20,offset,&da,0,NULL,NULL);
			//OutTrace("offset=%x opcode=%x\n", offset, *(opcodes+offset));
		}
		__except (EXCEPTION_EXECUTE_HANDLER){
			OutTrace("exception at offset=%x\n", offset);
			if(opcodes+offset < mi.EntryPoint) {
				offset = (unsigned char *)mi.EntryPoint - (unsigned char *)mi.lpBaseOfDll;
				OutTraceDW("DXWND: opcode resuming at addr=%x\n", opcodes+offset);
				continue;
			}
			else
				cont=FALSE;
		}		
		if (cmdlen==0) break;
		// search for RDTSC opcode 0x0F31
		if((*(opcodes+offset) == 0x0F) && (*(opcodes+offset+1) == 0x31)){
			OutTraceDW("DXWND: RDTSC opcode found at addr=%x\n", (opcodes+offset));
			if(!VirtualProtect((LPVOID)(opcodes+offset), 4, PAGE_READWRITE, &oldprot)) {
				OutTrace("VirtualProtect ERROR: target=%x err=%d at %d\n", opcodes+offset, GetLastError(), __LINE__);
				return; // error condition
			}
			*(opcodes+offset) = 0xCC;	// __asm INT3
			*(opcodes+offset+1) = 0x90; // __asm NOP
			if(!VirtualProtect((LPVOID)(opcodes+offset), 4, oldprot, &oldprot)){
				OutTrace("VirtualProtect ERROR; target=%x, err=%d at %d\n", opcodes+offset, GetLastError(), __LINE__);
				return; // error condition
			}
		}
		// search for RDTSCP opcode 0x0F01F9
		if((*(opcodes+offset) == 0x0F) && (*(opcodes+offset+1) == 0x01) && (*(opcodes+offset+2) == 0xF9)){
			OutTraceDW("DXWND: RDTSC opcode found at addr=%x\n", (opcodes+offset));
			if(!VirtualProtect((LPVOID)(opcodes+offset), 4, PAGE_READWRITE, &oldprot)) {
				OutTrace("VirtualProtect ERROR: target=%x err=%d at %d\n", opcodes+offset, GetLastError(), __LINE__);
				return; // error condition
			}
			*(opcodes+offset) = 0xCC;	// __asm INT3
			*(opcodes+offset+1) = 0x90; // __asm NOP
			*(opcodes+offset+2) = 0x90; // __asm NOP
			if(!VirtualProtect((LPVOID)(opcodes+offset), 4, oldprot, &oldprot)){
				OutTrace("VirtualProtect ERROR; target=%x, err=%d at %d\n", opcodes+offset, GetLastError(), __LINE__);
				return; // error condition
			}
		}
		offset+=cmdlen; 
		if((offset+0x10) > (int)mi.lpBaseOfDll + dwSegSize) break; // skip last 16 bytes, just in case....
	}

	return;
	(*pFinishdisasm)();
	FreeLibrary(disasmlib);
}

static void ReplacePrivilegedOps()
{
	typedef BOOL (WINAPI *GetModuleInformation_Type)(HANDLE, HMODULE, LPMODULEINFO, DWORD);
	HMODULE disasmlib;
	unsigned char *opcodes;
	t_disasm da;
	MODULEINFO mi;
	HMODULE psapilib;
	GetModuleInformation_Type pGetModuleInformation;
	DWORD dwSegSize;
	DWORD oldprot;

	if (!(disasmlib=LoadDisasm())) return;

	// getting segment size
	psapilib=(*pLoadLibraryA)("psapi.dll");
	if(!psapilib) {
		OutTraceDW("DXWND: Load lib=\"%s\" failed err=%d\n", "psapi.dll", GetLastError());
		return;
	}
	pGetModuleInformation=(GetModuleInformation_Type)(*pGetProcAddress)(psapilib, "GetModuleInformation");
	(*pGetModuleInformation)(GetCurrentProcess(), GetModuleHandle(NULL), &mi, sizeof(mi));
	dwSegSize = mi.SizeOfImage;
	FreeLibrary(psapilib);

	(*pPreparedisasm)();
	opcodes = (unsigned char *)mi.lpBaseOfDll;
	unsigned int offset = 0;
	BOOL cont = TRUE;
	OutTraceDW("DXWND: opcode starting at addr=%x size=%x\n", opcodes, dwSegSize);
	while (cont) {
		int cmdlen = 0;
		__try{
			cmdlen=(*pDisasm)(opcodes+offset,20,offset,&da,0,NULL,NULL);
			//OutTrace("offset=%x opcode=%x\n", offset, *(opcodes+offset));
		}
		__except (EXCEPTION_EXECUTE_HANDLER){
			OutTrace("exception at offset=%x\n", offset);
			if(opcodes+offset < mi.EntryPoint) {
				offset = (unsigned char *)mi.EntryPoint - (unsigned char *)mi.lpBaseOfDll;
				OutTraceDW("DXWND: opcode resuming at addr=%x\n", opcodes+offset);
				continue;
			}
			else
				cont=FALSE;
		}		
		if (cmdlen==0) break;
		// search for IN opcode 0xEC (IN AL, DX)
		if(*(opcodes+offset) == 0xEC){
			OutTraceDW("DXWND: IN opcode found at addr=%x\n", (opcodes+offset));
			if(!VirtualProtect((LPVOID)(opcodes+offset), 8, PAGE_READWRITE, &oldprot)) {
				OutTrace("VirtualProtect ERROR: target=%x err=%d at %d\n", opcodes+offset, GetLastError(), __LINE__);
				return; // error condition
			}
			*(opcodes+offset) = 0x90;	// __asm NOP
			if((*(opcodes+offset+1) == 0xA8) && 
				((*(opcodes+offset+3) == 0x75) || (*(opcodes+offset+3) == 0x74))) { // both JNZ and JZ
				OutTraceDW("DXWND: IN loop found at addr=%x\n", (opcodes+offset));
				memset((opcodes+offset+1), 0x90, 4); // Ubik I/O loop
				offset+=4;
			}
			if(!VirtualProtect((LPVOID)(opcodes+offset), 8, oldprot, &oldprot)){
				OutTrace("VirtualProtect ERROR; target=%x, err=%d at %d\n", opcodes+offset, GetLastError(), __LINE__);
				return; // error condition
			}
		}
		offset+=cmdlen; 
		if((offset+0x10) > (int)mi.lpBaseOfDll + dwSegSize) break; // skip last 16 bytes, just in case....
	}

	return;
	(*pFinishdisasm)();
	FreeLibrary(disasmlib);
}

HWND hDesktopWindow = NULL;

// Message poller: its only purpose is to keep sending messages to the main window
// so that the message loop is kept running. It is a trick necessary to play 
// smack videos with smackw32.dll and AUTOREFRESH mode set
DWORD WINAPI MessagePoller(LPVOID lpParameter)
{
	#define DXWREFRESHINTERVAL 20
	while(TRUE){
		Sleep(DXWREFRESHINTERVAL);
		if(dxw.dwFlags2 & INDEPENDENTREFRESH)
			dxw.ScreenRefresh();
		else
			SendMessage(dxw.GethWnd(), WM_NCHITTEST, 0, 0);
	}
    return 0;
}

DWORD WINAPI TimeFreezePoller(LPVOID lpParameter)
{
	#define DXWREFRESHINTERVAL 20
	extern UINT VKeyConfig[];
	UINT FreezeToggleKey;
	FreezeToggleKey = VKeyConfig[DXVK_FREEZETIME];
	while(TRUE){
		Sleep(DXWREFRESHINTERVAL);
		if(GetAsyncKeyState(FreezeToggleKey) & 0xF000) dxw.ToggleFreezedTime();
	}
    return 0;
}

static void MemoryReserve()
{
	VirtualAlloc((LPVOID)0x4000000, 0x04000000, MEM_RESERVE, PAGE_READWRITE);
	VirtualAlloc((LPVOID)0x5000000, 0x00F00000, MEM_RESERVE, PAGE_READWRITE);
	VirtualAlloc((LPVOID)0x6000000, 0x00F00000, MEM_RESERVE, PAGE_READWRITE);
	VirtualAlloc((LPVOID)0x7000000, 0x00F00000, MEM_RESERVE, PAGE_READWRITE);
	VirtualAlloc((LPVOID)0x8000000, 0x00F00000, MEM_RESERVE, PAGE_READWRITE);
}

HWND CreateVirtualDesktop(LPRECT TargetPos)
{
	HWND hDesktopWindow;
	HINSTANCE hinst=NULL;

	HWND hParent = GetDesktopWindow(); // not hooked yet !
//	hDesktopWindow=CreateWindowEx(0, "dxwnd:desktop", "DxWnd Desktop", WS_OVERLAPPEDWINDOW, 0, 0, 0, 0, hParent, NULL, hinst, NULL);
	hDesktopWindow=CreateWindowEx(0, "Static", "DxWnd Desktop", WS_OVERLAPPEDWINDOW, 0, 0, 0, 0, hParent, NULL, hinst, NULL);
	if(hDesktopWindow){
		MoveWindow(hDesktopWindow, TargetPos->left, TargetPos->top, TargetPos->right-TargetPos->left, TargetPos->bottom-TargetPos->top, TRUE);
		SetWindowLong(hDesktopWindow, GWL_STYLE, WS_OVERLAPPEDWINDOW);
		ShowWindow(hDesktopWindow, SW_RESTORE);
		OutTraceDW("created desktop emulation: hwnd=%x\n", hDesktopWindow);
		return hDesktopWindow; 

	}
	else{
		OutTraceE("CreateWindowEx ERROR: err=%d at %d\n", GetLastError(), __LINE__);
		return NULL;
	}
}

extern HHOOK hMouseHook;

void HookInit(TARGETMAP *target, HWND hwnd)
{
	static BOOL DoOnce = TRUE;
	HMODULE base;
	char *sModule;
	char sModuleBuf[60+1];
	char sSourcePath[MAX_PATH+1];
	static char *dxversions[14]={
		"Automatic", "DirectX1~6", "", "", "", "", "", 
		"DirectX7", "DirectX8", "DirectX9", "DirectX10", "DirectX11", "None", ""
	};
	static char *Resolutions[]={
		"unlimited", "320x200", "400x300", "640x480", "800x600", "1024x768", "1280x960", "" // terminator
	};

	dxw.InitTarget(target);

	// reserve legacy memory segments
	if(dxw.dwFlags6 & LEGACYALLOC) MemoryReserve();

	// add the DxWnd install dir to the search path, to make all included dll linkable
	DWORD dwAttrib;		
	dwAttrib = GetFileAttributes("dxwnd.dll");
	//if (dwAttrib != INVALID_FILE_ATTRIBUTES && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY)) {
	//	MessageBox(0, "DXWND: ERROR can't locate itself", "ERROR", MB_OK | MB_ICONEXCLAMATION);
	//	exit(0);
	//}
	GetModuleFileName(GetModuleHandle("dxwnd"), sSourcePath, MAX_PATH);
	sSourcePath[strlen(sSourcePath)-strlen("dxwnd.dll")] = 0; // terminate the string just before "dxwnd.dll"
	SetDllDirectory(sSourcePath);

	if(dxw.dwFlags5 & HYBRIDMODE) {
		// special mode settings ....
		dxw.dwFlags1 |= EMULATESURFACE;
		dxw.dwFlags2 |= SETCOMPATIBILITY;
		dxw.dwFlags5 &= ~(BILINEARFILTER | AEROBOOST); 
	}
	if(dxw.dwFlags5 & GDIMODE) dxw.dwFlags1 |= EMULATESURFACE;
	if(dxw.dwFlags5 & STRESSRESOURCES) dxw.dwFlags5 |= LIMITRESOURCES;

	if(DoOnce){
		DoOnce = FALSE;
		dxw.VirtualDesktop.left		= GetSystemMetrics(SM_XVIRTUALSCREEN);
		dxw.VirtualDesktop.top		= GetSystemMetrics(SM_YVIRTUALSCREEN);
		dxw.VirtualDesktop.right	= dxw.VirtualDesktop.left + GetSystemMetrics(SM_CXVIRTUALSCREEN);
		dxw.VirtualDesktop.bottom	= dxw.VirtualDesktop.top + GetSystemMetrics(SM_CYVIRTUALSCREEN);
		OutTraceDW("Virtual Desktop: monitors=%d area=(%d,%d)-(%d,%d)\n", 
			GetSystemMetrics(SM_CMONITORS),
			dxw.VirtualDesktop.left, dxw.VirtualDesktop.top, dxw.VirtualDesktop.right, dxw.VirtualDesktop.bottom);
	}

	if(hwnd){ // v2.02.32: skip this when in code injection mode.
		// v2.1.75: is it correct to set hWnd here?
		//dxw.SethWnd(hwnd);
		dxw.hParentWnd=GetParent(hwnd);
		dxw.hChildWnd=hwnd;
		// v2.02.31: set main win either this one or the parent!
		dxw.SethWnd((dxw.dwFlags1 & FIXPARENTWIN) ? GetParent(hwnd) : hwnd);
		if(dxw.dwFlags4 & ENABLEHOTKEYS) dxw.MapKeysInit();
	}

	if(dxw.dwFlags6 & CREATEDESKTOP){
		RECT TargetPos;
		TargetPos.left = target->posx;
		TargetPos.right = target->posx+target->sizx;
		TargetPos.top = target->posy;
		TargetPos.bottom = target->posy+target->sizy;
		if (!hDesktopWindow) hDesktopWindow=CreateVirtualDesktop(&TargetPos);
	}

	if(IsTraceDW){
		char sInfo[1024];
		OSVERSIONINFO osinfo;
		strcpy(sInfo, "");
		if(hwnd) sprintf(sInfo, " hWnd=%x(hdc=%x) dxw.hParentWnd=%x(hdc=%x) desktop=%x(hdc=%x)", 
			hwnd, GetDC(hwnd), dxw.hParentWnd, GetDC(dxw.hParentWnd), GetDesktopWindow(), GetDC(GetDesktopWindow()));
		OutTrace("HookInit: path=\"%s\" module=\"%s\" dxversion=%s pos=(%d,%d) size=(%d,%d)%s\n", 
			target->path, target->module, dxversions[dxw.dwTargetDDVersion], 
			target->posx, target->posy, target->sizx, target->sizy, sInfo);
		osinfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
		if(GetVersionEx(&osinfo)){
			OutTrace("OS=(%d.%d) build=%d platform=%d service pack=%s\n", 
				osinfo.dwMajorVersion, osinfo.dwMinorVersion, osinfo.dwPlatformId, osinfo.dwPlatformId, osinfo.szCSDVersion);
		}
		if (dxw.dwFlags4 & LIMITSCREENRES) OutTrace("HookInit: max resolution=%s\n", (dxw.MaxScreenRes<6)?Resolutions[dxw.MaxScreenRes]:"unknown");
		if (dxw.dwFlags7 & LIMITDDRAW) OutTrace("HookInit: max supported IDidrectDrawInterface=%d\n", dxw.MaxDdrawInterface);
		if (dxw.dwFlags7 & CPUSLOWDOWN) OutTrace("HookInit: CPU slowdown ratio 1:%d\n", dxw.SlowRatio);
		if (dxw.dwFlags7 & CPUMAXUSAGE) OutTrace("HookInit: CPU maxusage ratio 1:%d\n", dxw.SlowRatio);
	}

	if (hwnd && IsDebug){
		DWORD dwStyle, dwExStyle;
		char ClassName[81];
		char WinText[81];
		dwStyle=GetWindowLong(dxw.GethWnd(), GWL_STYLE);
		dwExStyle=GetWindowLong(dxw.GethWnd(), GWL_EXSTYLE);
		GetClassName(dxw.GethWnd(), ClassName, sizeof(ClassName));
		GetWindowText(dxw.GethWnd(), WinText, sizeof(WinText));
		OutTrace("HookInit: dxw.hChildWnd=%x class=\"%s\" text=\"%s\" style=%x(%s) exstyle=%x(%s)\n", 
			dxw.hChildWnd, ClassName, WinText, dwStyle, ExplainStyle(dwStyle), dwExStyle, ExplainExStyle(dwExStyle));
		dwStyle=GetWindowLong(dxw.hParentWnd, GWL_STYLE);
		dwExStyle=GetWindowLong(dxw.hParentWnd, GWL_EXSTYLE);
		GetClassName(dxw.hParentWnd, ClassName, sizeof(ClassName));
		GetWindowText(dxw.hParentWnd, WinText, sizeof(WinText));
		OutTrace("HookInit: dxw.hParentWnd=%x class=\"%s\" text=\"%s\" style=%x(%s) exstyle=%x(%s)\n", 
			dxw.hParentWnd, ClassName, WinText, dwStyle, ExplainStyle(dwStyle), dwExStyle, ExplainExStyle(dwExStyle));
		OutTrace("HookInit: target window pos=(%d,%d) size=(%d,%d)\n", dxw.iPosX, dxw.iPosY, dxw.iSizX, dxw.iSizY);
		dxw.DumpDesktopStatus();
		typedef HRESULT (WINAPI *DwmIsCompositionEnabled_Type)(BOOL *);
		DwmIsCompositionEnabled_Type pDwmIsCompositionEnabled = NULL;
		HMODULE DwnApiHdl;
		DwnApiHdl = LoadLibrary("Dwmapi.dll");
		if (DwnApiHdl) pDwmIsCompositionEnabled = (DwmIsCompositionEnabled_Type)GetProcAddress(DwnApiHdl, "DwmIsCompositionEnabled");
		char *sRes;
		if(pDwmIsCompositionEnabled){
			HRESULT res;
			BOOL val;
			res = (*pDwmIsCompositionEnabled)(&val);
			if(res==S_OK) sRes = val ? "ENABLED" : "DISABLED";
			else sRes = "ERROR";
		}
		else sRes = "Unknown";
		OutTrace("HookInit: DWMComposition %s\n", sRes);
	}

	if (SKIPIMEWINDOW) {
		char ClassName[8+1];
		GetClassName(hwnd, ClassName, sizeof(ClassName));
		if(!strcmp(ClassName, "IME")){
			dxw.hChildWnd=GetParent(hwnd);
			dxw.hParentWnd=GetParent(dxw.hChildWnd);
			if (dxw.dwFlags2 & SUPPRESSIME) DestroyWindow(hwnd);
			// v2.02.31: set main win either this one or the parent!
			dxw.SethWnd((dxw.dwFlags1 & FIXPARENTWIN) ? dxw.hParentWnd : dxw.hChildWnd);
			hwnd = dxw.GethWnd();
			if(hwnd) OutTraceDW("HookInit: skipped IME window. current hWnd=%x(hdc=%x) dxw.hParentWnd=%x(hdc=%x)\n", 
				hwnd, GetDC(hwnd), dxw.hParentWnd, GetDC(dxw.hParentWnd));		
		}
	}

	if(dxw.dwFlags6 & CREATEDESKTOP){
		if (hDesktopWindow){
			OutTraceDW("HookInit: set new parent=%x to main win=%x\n", hDesktopWindow, dxw.hChildWnd);
			SetParent(dxw.hChildWnd, hDesktopWindow);
			dxw.hParentWnd = hDesktopWindow;
		}
	}

#ifdef CHECKFORCOMPATIBILITYFLAGS
	CheckCompatibilityFlags(); // v2.02.83 Check for change of OS release
#endif

	HookSysLibsInit(); // this just once...

	base=GetModuleHandle(NULL);
	if (dxw.dwFlags3 & SINGLEPROCAFFINITY) SetSingleProcessAffinity(TRUE);
	if (dxw.dwFlags5 & USELASTCORE) SetSingleProcessAffinity(FALSE);
	if (dxw.dwFlags4 & INTERCEPTRDTSC) AddVectoredExceptionHandler(1, Int3Handler); // 1 = first call, 0 = call last
	if (dxw.dwFlags1 & HANDLEEXCEPTIONS) HookExceptionHandler();
	if (dxw.dwTFlags & OUTIMPORTTABLE) DumpImportTable(base);
	if (dxw.dwFlags2 & SUPPRESSIME) DisableIME();

	if (dxw.dwFlags4 & INTERCEPTRDTSC) ReplaceRDTSC();
	if (dxw.dwFlags5 & REPLACEPRIVOPS) ReplacePrivilegedOps();

	// make InitPosition used for both DInput and DDraw
	if(dxw.Windowize) dxw.InitWindowPos(target->posx, target->posy, target->sizx, target->sizy);
	

	OutTraceB("HookInit: base hmodule=%x\n", base);
	HookModule(base, dxw.dwTargetDDVersion);
	if (dxw.dwFlags3 & HOOKDLLS) HookDlls(base);

	strncpy(sModuleBuf, dxw.gsModules, 60);
	sModule=strtok(sModuleBuf," ;");
	while (sModule) {
		base=(*pLoadLibraryA)(sModule);
		if(!base){
			OutTraceE("HookInit: LoadLibrary ERROR module=%s err=%d\n", sModule, GetLastError());
		}
		else {
			OutTraceDW("HookInit: hooking additional module=%s base=%x\n", sModule, base);
			if (dxw.dwTFlags & OUTIMPORTTABLE) DumpImportTable(base);
			HookModule(base, dxw.dwTargetDDVersion);
		}
		sModule=strtok(NULL," ;");
	}

	SaveScreenMode();
	if(dxw.dwFlags2 & RECOVERSCREENMODE) RecoverScreenMode();
	if(dxw.dwFlags3 & FORCE16BPP) SwitchTo16BPP();

	if (dxw.dwFlags1 & MESSAGEPROC){
		extern HINSTANCE hInst;
		typedef HHOOK (WINAPI *SetWindowsHookEx_Type)(int, HOOKPROC, HINSTANCE, DWORD);
		extern SetWindowsHookEx_Type pSetWindowsHookExA;
		hMouseHook=(*pSetWindowsHookExA)(WH_GETMESSAGE, MessageHook, hInst, GetCurrentThreadId());
		if(hMouseHook==NULL) OutTraceE("SetWindowsHookEx WH_GETMESSAGE failed: error=%d\n", GetLastError());
	}
 
	InitScreenParameters(0); // still unknown
	if(hwnd) HookWindowProc(hwnd);
	// in fullscreen mode, messages seem to reach and get processed by the parent window
	if((!dxw.Windowize) && hwnd) HookWindowProc(dxw.hParentWnd);

	// initialize window: if
	// 1) not in injection mode (hwnd != 0) and
	// 2) in Windowed mode and
	// 3) supposedly in fullscreen mode (dxw.IsFullScreen()) and
	// 4) configuration ask for a overlapped bordered window (dxw.dwFlags1 & FIXWINFRAME) then
	// update window styles: just this window or, when FIXPARENTWIN is set, the father one as well. 

	if (hwnd && dxw.Windowize && dxw.IsFullScreen() && (dxw.dwFlags1 & FIXWINFRAME)) {
		dxw.FixWindowFrame(dxw.hChildWnd);
		AdjustWindowPos(dxw.hChildWnd, target->sizx, target->sizy);
		if(dxw.dwFlags1 & FIXPARENTWIN) {
			dxw.FixWindowFrame(dxw.hParentWnd);
			AdjustWindowPos(dxw.hParentWnd, target->sizx, target->sizy);
		}
	}

	if (dxw.dwFlags1 & AUTOREFRESH) 
		CreateThread(NULL, 0, MessagePoller, NULL, 0, NULL);

	if (dxw.dwFlags4 & ENABLETIMEFREEZE)
		CreateThread(NULL, 0, TimeFreezePoller, NULL, 0, NULL);

	if(dxw.dwFlags7 & CPUSLOWDOWN)
		CreateThread(NULL, 0, CpuSlow, NULL, 0, NULL);
	else
	if(dxw.dwFlags7 & CPUMAXUSAGE)
		CreateThread(NULL, 0, CpuLimit, NULL, 0, NULL);

}

LPCSTR ProcToString(LPCSTR proc)
{
	static char sBuf[24+1];
	if(((DWORD)proc & 0xFFFF0000) == 0){
		sprintf_s(sBuf, 24, "#0x%x", proc);
		return sBuf;
	}
	else
		return proc;
}

FARPROC RemapLibraryEx(LPCSTR proc, HMODULE hModule, HookEntryEx_Type *Hooks)
{
	void *remapped_addr;
	for(; Hooks->APIName; Hooks++){
		if (!strcmp(proc,Hooks->APIName)){
			if  ((Hooks->HookStatus == HOOK_HOT_REQUIRED) ||
				((dxw.dwFlags4 & HOTPATCH) && (Hooks->HookStatus == HOOK_HOT_CANDIDATE)) ||  // hot patch candidate still to process - or
				((dxw.dwFlags4 & HOTPATCHALWAYS) && (Hooks->HookStatus != HOOK_HOT_LINKED))){ // force hot patch and not already hooked
											 
			if(!Hooks->OriginalAddress) {
					Hooks->OriginalAddress=(*pGetProcAddress)(hModule, Hooks->APIName);
					if(!Hooks->OriginalAddress) continue;
				}

				remapped_addr = HotPatch(Hooks->OriginalAddress, Hooks->APIName, Hooks->HookerAddress);
				if(remapped_addr == (void *)1) { // should never go here ...
					Hooks->HookStatus = HOOK_HOT_LINKED;
					continue; // already hooked
				}
				if(remapped_addr) {
					Hooks->HookStatus = HOOK_HOT_LINKED;
					if(Hooks->StoreAddress) *(Hooks->StoreAddress) = (FARPROC)remapped_addr;
				}			
			}
			if(Hooks->HookStatus == HOOK_HOT_LINKED) {
				OutTraceDW("GetProcAddress: hot patched proc=%s addr=%x\n", ProcToString(proc), Hooks->HookerAddress);
				return Hooks->HookerAddress;
			}
			if (Hooks->StoreAddress) *(Hooks->StoreAddress)=(*pGetProcAddress)(hModule, proc);
			OutTraceDW("GetProcAddress: hooking proc=%s addr=%x->%x\n", 
				ProcToString(proc), (Hooks->StoreAddress) ? *(Hooks->StoreAddress) : 0, Hooks->HookerAddress);
			return Hooks->HookerAddress;
		}
	}
	return NULL;
}

void HookLibraryEx(HMODULE hModule, HookEntryEx_Type *Hooks, char *DLLName)
{
	HMODULE hDLL = NULL;
	//OutTrace("HookLibrary: hModule=%x dll=%s\n", hModule, DLLName);
	for(; Hooks->APIName; Hooks++){
		void *remapped_addr;
		if(Hooks->HookStatus == HOOK_HOT_LINKED) continue; // skip any hot-linked entry
		if(((Hooks->HookStatus == HOOK_HOT_REQUIRED) ||
			((dxw.dwFlags4 & HOTPATCH) && (Hooks->HookStatus == HOOK_HOT_CANDIDATE)) ||  // hot patch candidate still to process - or
			((dxw.dwFlags4 & HOTPATCHALWAYS) && (Hooks->HookStatus != HOOK_HOT_LINKED))) // force hot patch and not already hooked
			&&
			Hooks->StoreAddress){							 // and save ptr available
			// Hot Patch - beware! This way yo're likely to hook unneeded libraries.
			if(!Hooks->OriginalAddress) {
				if(!hDLL) {
					hDLL = (*pLoadLibraryA)(DLLName);
					if(!hDLL) {
						OutTrace("HookLibrary: LoadLibrary failed on DLL=%s err=%x\n", DLLName, GetLastError());
						continue;
					}
				}
				Hooks->OriginalAddress=(*pGetProcAddress)(hDLL, Hooks->APIName);
				if(!Hooks->OriginalAddress) {
					OutTraceB("HookLibrary: GetProcAddress failed on API=%s err=%x\n", Hooks->APIName, GetLastError());
					continue;
				}
			}
			remapped_addr = HotPatch(Hooks->OriginalAddress, Hooks->APIName, Hooks->HookerAddress);
			if(remapped_addr == (void *)1) { // should never go here ...
				Hooks->HookStatus = HOOK_HOT_LINKED;
				continue; // already hooked
			}
			if(remapped_addr) {
				Hooks->HookStatus = HOOK_HOT_LINKED;
				*(Hooks->StoreAddress) = (FARPROC)remapped_addr;
				continue;
			}
		}

		remapped_addr = IATPatchEx(hModule, Hooks->ordinal, DLLName, Hooks->OriginalAddress, Hooks->APIName, Hooks->HookerAddress);
		if(remapped_addr)  {
			Hooks->HookStatus = HOOK_IAT_LINKED;
			if (Hooks->StoreAddress) *(Hooks->StoreAddress) = (FARPROC)remapped_addr;
		}
	}
}

void PinLibraryEx(HookEntryEx_Type *Hooks, char *DLLName)
{
	HMODULE hModule = NULL;
	hModule = (*pLoadLibraryA)(DLLName);
	if(!hModule) {
		OutTrace("PinLibrary: LoadLibrary failed on DLL=%s err=%x\n", DLLName, GetLastError());
		return;
	}
	for(; Hooks->APIName; Hooks++){
		if (Hooks->StoreAddress) *(Hooks->StoreAddress) = (*pGetProcAddress)(hModule, Hooks->APIName);
	}
}

BOOL IsHotPatchedEx(HookEntryEx_Type *Hooks, char *ApiName)
{
	for(; Hooks->APIName; Hooks++){
		if(!strcmp(Hooks->APIName, ApiName)) return (Hooks->HookStatus == HOOK_HOT_LINKED);
	}
	return FALSE;
}

void HookLibInitEx(HookEntryEx_Type *Hooks)
{
	for(; Hooks->APIName; Hooks++)
		if (Hooks->StoreAddress) *(Hooks->StoreAddress) = Hooks->OriginalAddress;
}
