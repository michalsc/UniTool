#include <graphics/gfxbase.h>
#include <graphics/displayinfo.h>
#include <libraries/mui.h>
#include <libraries/iffparse.h>
#include <datatypes/textclass.h>
#include <intuition/screens.h>

#include <proto/asl.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/unicam.h>
#include <proto/devicetree.h>
#include <proto/iffparse.h>

#include <clib/alib_protos.h>

#include "presets.h"
#include "messages.h"
#include "rga_common.h"
#include "rga_host.h"
#include "locale.h"

#define CATCOMP_NUMBERS

#include "strings.h"

struct GfxBase *        GfxBase         = NULL;
struct IntuitionBase *  IntuitionBase   = NULL;
struct Library *        MUIMasterBase   = NULL;
Object *                app             = NULL;
Object *                win             = NULL;
Object *                aboutWin        = NULL;
Object *                slCropX         = NULL;
Object *                slCropY         = NULL;
Object *                slCropW         = NULL;
Object *                slCropH         = NULL;
Object *                slAspect        = NULL;
Object *                chInteger       = NULL;
Object *                slScanlines     = NULL;
Object *                slScanlinesLaced= NULL;
Object *                chSmooth        = NULL;
Object *                slB             = NULL;
Object *                slC             = NULL;
Object *                slPhase         = NULL;
Object *                menuOpen        = NULL;
Object *                menuSaveAs      = NULL;
Object *                menuCopyConfig  = NULL;
Object *                menuAbout       = NULL;
Object *                menuQuit        = NULL;
Object *                menuDefaults    = NULL;
Object *                menuLastUsed    = NULL;
Object *                closeAbout      = NULL;
Object *                txEmuVer        = NULL;
Object *                txFTVer         = NULL;
Object *                txFTGit         = NULL;
Object *                vg              = NULL;

struct Hook kernelHook;
struct Hook cropSizeHook;
struct Hook cropOffsetHook;
struct Hook aspectHook;
struct Hook configHook;
struct Hook scanlineHook;
struct Hook defaultsHook;
struct Hook lastUsedHook;
struct Hook openHook;
struct Hook aboutHook;
struct Hook saveAsHook;
struct Hook copyConfigHook;
struct Window *backdrop;
struct Screen *screen;

ULONG def_x, def_y, def_w, def_h, def_aspect;
ULONG def_b, def_c, def_phase, def_scan, def_scanl;
ULONG def_integer, def_smooth;

void OpenNativeScreen(int ntsc)
{
    static UWORD pens[] = { -1 };
    ULONG id = 0;

    if (ntsc == 1) {
        id = NTSC_MONITOR_ID;
    }
    else if (ntsc == 0) {
        id = PAL_MONITOR_ID;
    }
    else {
        if (GfxBase->DisplayFlags & PAL) {
            id = PAL_MONITOR_ID;
        }
        else {
            id = NTSC_MONITOR_ID;
        }
    }

    screen = OpenScreenTags(NULL,
        SA_DisplayID,   id | HIRESLACE_KEY,
        SA_Depth,       2,
        SA_Title,       (ULONG)"UniTool GUI",
        SA_ShowTitle,   FALSE,
        SA_Quiet,       TRUE,
        SA_Type,        CUSTOMSCREEN,
        SA_Pens,        (ULONG)pens,
        SA_FullPalette, TRUE,
        TAG_DONE);

    backdrop = OpenWindowTags(NULL,
        WA_CustomScreen,  (ULONG)screen,
        WA_Left,          0,
        WA_Top,           0,
        WA_Width,         screen->Width,
        WA_Height,        screen->Height,
        WA_Borderless,    TRUE,
        WA_Backdrop,      TRUE,
        WA_Activate,      FALSE,   /* don't steal focus from MUI window */
        WA_NoCareRefresh, TRUE,    /* we handle our own drawing */
        WA_SmartRefresh,  TRUE,
        WA_RMBTrap,       TRUE,
        TAG_DONE);

    struct RastPort *rp = backdrop->RPort;
    LONG sw = backdrop->Width;   /* 640 */
    LONG sh = backdrop->Height;  /* 512 */
    LONG arm = sw / 32;

    LONG gridCols = 20;
    LONG gridRows = ntsc ? 12 : 16;

    LONG x0 = sw * 1 / gridCols + 1;
    LONG y0 = sh * 1 / gridRows + 1;
    LONG x1 = sw * 3 / gridCols - 1;
    LONG y1 = sh * 3 / gridRows - 1;

    static UWORD pattern[] = {
        0xAAAA,
        0x5555,
    };

    rp->AreaPtrn  = pattern;
    rp->AreaPtSz  = 1;        /* 2^1 = 2 rows */

    SetAPen(rp, 1);
    SetBPen(rp, 0);
    SetDrMd(rp, JAM2);

    RectFill(rp, x0, y0, x1, y1);

    x0 = sw * 17 / gridCols + 1;
    y0 = sh * 1 / gridRows + 1;
    x1 = sw * 19 / gridCols - 1;
    y1 = sh * 3 / gridRows - 1;

    SetAPen(rp, 2);

    RectFill(rp, x0, y0, x1, y1);

    x0 = sw * 17 / gridCols + 1;
    y0 = sh * 13 / gridRows + 1;
    x1 = sw * 19 / gridCols - 1;
    y1 = sh * 15 / gridRows - 1;

    SetAPen(rp, 3);

    RectFill(rp, x0, y0, x1, y1);

    x0 = sw * 1 / gridCols + 1;
    y0 = sh * 13 / gridRows + 1;
    x1 = sw * 3 / gridCols - 1;
    y1 = sh * 15 / gridRows - 1;

    SetAPen(rp, 1);
    SetBPen(rp, 2);

    RectFill(rp, x0, y0, x1, y1);

    rp->AreaPtrn = NULL;
    rp->AreaPtSz = 0;
    
    /* --- grid --- */
    SetAPen(rp, 1);

    for (LONG i = 1; i < gridCols; i++)
    {
        LONG x = sw * i / gridCols;
        Move(rp, x, 0);
        Draw(rp, x, sh - 1);
    }
    for (LONG i = 1; i < gridRows; i++)
    {
        LONG y = sh * i / gridRows;
        Move(rp, 0, y);
        Draw(rp, sw - 1, y);
    }

    DrawEllipse(rp, 320, 256, 250, 250);
    DrawEllipse(rp, 320, 256, 150, 150);

    DrawEllipse(rp, 64, 64, 48, 48);
    DrawEllipse(rp, 640 - 64, 64, 48, 48);
    DrawEllipse(rp, 64, 512 - 64, 48, 48);
    DrawEllipse(rp, 640 - 64, 512 - 64, 48, 48);

        /* --- corner markers at screen edges --- */
    SetAPen(rp, 2);

    /* top-left */
    Move(rp, 0, arm); Draw(rp, 0, 0); Draw(rp, arm, 0);
    /* top-right */
    Move(rp, sw - arm - 1, 0); Draw(rp, sw - 1, 0); Draw(rp, sw - 1, arm);
    /* bottom-left */
    Move(rp, arm, sh - 1); Draw(rp, 0, sh - 1); Draw(rp, 0, sh - arm - 1);
    /* bottom-right */
    Move(rp, sw - arm, sh - 1); Draw(rp, sw - 1, sh - 1); Draw(rp, sw - 1, sh - arm - 1);

    /* bottom-left */
    Move(rp, arm, 400 - 1); Draw(rp, 0, 400 - 1); Draw(rp, 0, 400 - arm - 1);
    /* bottom-right */
    Move(rp, sw - arm, 400 - 1); Draw(rp, sw - 1, 400 - 1); Draw(rp, sw - 1, 400 - arm - 1);
}

void UpdateDList()
{
    struct MsgPort * vc4Port;
    struct MsgPort * replyPort;

    vc4Port = FindPort("VideoCore");
    if (vc4Port)
    {
        struct VC4Msg cmd;
        replyPort = CreateMsgPort();

        cmd.msg.mn_ReplyPort = replyPort;
        cmd.msg.mn_Length = sizeof(struct VC4Msg);

        cmd.cmd = VCMD_UPDATE_UNICAM_DL;
        PutMsg(vc4Port, &cmd.msg);
        WaitPort(replyPort);
        GetMsg(replyPort);

        DeleteMsgPort(replyPort);
    }
}

ULONG CropOffsetHookFunc()
{
    extern APTR UnicamBase;
    ULONG dx,dy;

    get(slCropX, MUIA_Numeric_Value, &dx);
    get(slCropY, MUIA_Numeric_Value, &dy);

    UnicamSetCropOffset(dx, dy);

    UpdateDList();

    return 0;
}

ULONG CropSizeHookFunc()
{
    extern APTR UnicamBase;
    ULONG width,height;

    get(slCropW, MUIA_Numeric_Value, &width);
    get(slCropH, MUIA_Numeric_Value, &height);

    UnicamSetCropSize(width, height);

    set(slCropX, MUIA_Slider_Max, 720 - width);
    set(slCropY, MUIA_Slider_Max, 576 - height);

    UpdateDList();

    return 0;
}

ULONG AspectHookFunc()
{
    extern APTR UnicamBase;
    ULONG aspect;

    get(slAspect, MUIA_Numeric_Value, &aspect);

    UnicamSetAspect(aspect);

    UpdateDList();

    return 0;
}

ULONG KernelHookFunc()
{
    extern APTR UnicamBase;
    ULONG b,c;

    get(slB, MUIA_Numeric_Value, &b);
    get(slC, MUIA_Numeric_Value, &c);

    UnicamSetKernel(b, c);

    UpdateDList();

    return 0;
}

ULONG ConfigHookFunc()
{
    extern APTR UnicamBase;
    ULONG cfg;
    ULONG integer;
    ULONG phase;
    ULONG smooth;

    cfg = UnicamGetConfig();

    get(chInteger, MUIA_Selected, &integer);
    get(chSmooth, MUIA_Selected, &smooth);
    get(slPhase, MUIA_Numeric_Value, &phase);

    cfg = cfg & ~(UNICAMF_PHASE | UNICAMF_INTEGER | UNICAMF_SMOOTHING);
    
    cfg |= (phase << UNICAMB_PHASE) & UNICAMF_PHASE;
    
    if (integer)
        cfg |= UNICAMF_INTEGER;

    if (smooth)
        cfg |= UNICAMF_SMOOTHING;

    UnicamSetConfig(cfg);

    UpdateDList();

    return 0;
}

ULONG ScanlineHookFunc()
{
    ULONG sc, scl;

    get(slScanlines, MUIA_Numeric_Value, &sc);
    get(slScanlinesLaced, MUIA_Numeric_Value, &scl);

    rga_set_scanlines(sc, scl);

    return 0;
}

ULONG ResetToDefaultsHookFunc()
{
    ULONG scan, scanl, w, h, x, y, aspect, b, c, phase, integer, smooth;

    APTR DeviceTreeBase = OpenResource("devicetree.resource");
    APTR key = DT_OpenKey("/emu68/unicam");

    if (key)
    {
        const UWORD *u16;

        u16 = DT_GetPropValue(DT_FindProperty(key, "size"));
        w = u16[0];
        h = u16[1];

        u16 = DT_GetPropValue(DT_FindProperty(key, "offset"));
        x = u16[0];
        y = u16[1];

        aspect = *(ULONG*)DT_GetPropValue(DT_FindProperty(key, "aspect-ratio"));
        
        u16 = DT_GetPropValue(DT_FindProperty(key, "scaler"));
        phase = u16[1];

        u16 = DT_GetPropValue(DT_FindProperty(key, "kernel"));
        b = u16[0];
        c = u16[1];

        scan = *(ULONG*)DT_GetPropValue(DT_FindProperty(key, "scanlines"));
        scanl = *(ULONG*)DT_GetPropValue(DT_FindProperty(key, "laced-scanlines"));

        integer = !!DT_FindProperty(key, "integer-scaling");
        smooth = !!DT_FindProperty(key, "smoothing");

        DT_CloseKey(key);

        set(slScanlines, MUIA_Numeric_Value, scan);
        set(slScanlinesLaced, MUIA_Numeric_Value, scanl);
        set(slCropW, MUIA_Numeric_Value, w);
        set(slCropH, MUIA_Numeric_Value, h);
        set(slCropX, MUIA_Numeric_Value, x);
        set(slCropY, MUIA_Numeric_Value, y);
        set(slAspect, MUIA_Numeric_Value, aspect);
        set(slB, MUIA_Numeric_Value, b);
        set(slC, MUIA_Numeric_Value, c);
        set(slPhase, MUIA_Numeric_Value, phase);
        set(chInteger, MUIA_Selected, integer ? TRUE : FALSE);
        set(chSmooth, MUIA_Selected, smooth ? TRUE : FALSE);
    }

    return 0;
}

ULONG ResetToLastUsedHookFunc()
{
    set(slScanlines, MUIA_Numeric_Value, def_scan);
    set(slScanlinesLaced, MUIA_Numeric_Value, def_scanl);
    set(slCropW, MUIA_Numeric_Value, def_w);
    set(slCropH, MUIA_Numeric_Value, def_h);
    set(slCropX, MUIA_Numeric_Value, def_x);
    set(slCropY, MUIA_Numeric_Value, def_y);
    set(slAspect, MUIA_Numeric_Value, def_aspect);
    set(slB, MUIA_Numeric_Value, def_b);
    set(slC, MUIA_Numeric_Value, def_c);
    set(slPhase, MUIA_Numeric_Value, def_phase);
    set(chInteger, MUIA_Selected, def_integer ? TRUE : FALSE);
    set(chSmooth, MUIA_Selected, def_smooth ? TRUE : FALSE);

    return 0;
}

static LONG _strlen(const char *text)
{
    LONG res = 0;

    if (text != NULL)
    {
        while (*text++) res++;
    }

    return res;
}

void CopyToClipboard(char *text)
{
    extern const char iffparse_name[];
    struct Library *IFFParseBase = OpenLibrary(iffparse_name, 0);

    if (!IFFParseBase) return;

    struct IFFHandle *iff = AllocIFF();
    if (!iff) return;

    iff->iff_Stream = (ULONG)OpenClipboard(PRIMARY_CLIP);
    if (!iff->iff_Stream) { FreeIFF(iff); return; }

    InitIFFasClip(iff);

    if (!OpenIFF(iff, IFFF_WRITE))
    {
        PushChunk(iff, ID_FTXT, ID_FORM, IFFSIZE_UNKNOWN);
        PushChunk(iff, 0, ID_CHRS, IFFSIZE_UNKNOWN);
        WriteChunkBytes(iff, text, _strlen(text));
        PopChunk(iff);  /* CHRS */
        PopChunk(iff);  /* FORM */
        CloseIFF(iff);
    }

    CloseClipboard((struct ClipboardHandle *)iff->iff_Stream);
    FreeIFF(iff);
    CloseLibrary(IFFParseBase);
}

static void StuffChar(UBYTE c __asm__("d0"), UBYTE **ptr __asm__("a3"))
{
    *(*ptr)++ = c;
}

ULONG CopyConfigHookFunc()
{
    ULONG tmp;
    ULONG full_w, full_h;
    BYTE boot = 0;
    static char str[512];
    char *ptr = str;

    APTR DeviceTreeBase = OpenResource("devicetree.resource");
    APTR key = DT_OpenKey("/emu68/unicam");

    if (key)
    {
        const UWORD *u16;

        u16 = DT_GetPropValue(DT_FindProperty(key, "full-size"));
        full_w = u16[0];
        full_h = u16[1];

        const char *str = DT_GetPropValue(DT_FindProperty(key, "status"));
        if (str[0] == 'o' && str[1] == 'k' && str[2] == 'a' && str[3] == 'y' && str[4] == 0) {
            boot = 1;
        }

        DT_CloseKey(key);
    }

    RawDoFmt("dtoverlay=unicam", NULL, (void*)StuffChar, &ptr); ptr--;

    if (boot) {
        RawDoFmt(",boot", NULL, (void*)StuffChar, &ptr); ptr--;
    }

    get(chSmooth, MUIA_Selected, &tmp);
    if (tmp) {
        RawDoFmt(",smooth", NULL, (void*)StuffChar, &ptr); ptr--;
    }

    get(chInteger, MUIA_Selected, &tmp);
    if (tmp) {
        RawDoFmt(",int", NULL, (void*)StuffChar, &ptr); ptr--;
    }

    if (full_w != 720) {
        RawDoFmt(",full_width=%ld", &tmp, (void*)StuffChar, &ptr); ptr--;
    }

    if (full_h != 576) {
        RawDoFmt(",full_height=%ld", &tmp, (void*)StuffChar, &ptr); ptr--;
    }

    get(slCropW, MUIA_Numeric_Value, &tmp);
    if (tmp != full_w) {
        RawDoFmt(",w=%ld", &tmp, (void*)StuffChar, &ptr); ptr--;
    }
    
    get(slCropH, MUIA_Numeric_Value, &tmp);
    if (tmp != full_h) {
        RawDoFmt(",h=%ld", &tmp, (void*)StuffChar, &ptr); ptr--;
    }

    get(slCropX, MUIA_Numeric_Value, &tmp);
    if (tmp != 0) {
        RawDoFmt(",x=%ld", &tmp, (void*)StuffChar, &ptr); ptr--;
    }

    get(slCropY, MUIA_Numeric_Value, &tmp);
    if (tmp != 0) {
        RawDoFmt(",y=%ld", &tmp, (void*)StuffChar, &ptr); ptr--;
    }

    get(slAspect, MUIA_Numeric_Value, &tmp);
    if (tmp != 1000) {
        RawDoFmt(",asp=%ld", &tmp, (void*)StuffChar, &ptr); ptr--;
    }

    get(slB, MUIA_Numeric_Value, &tmp);
    if (tmp != 200) {
        RawDoFmt(",b=%ld", &tmp, (void*)StuffChar, &ptr); ptr--;
    }

    get(slC, MUIA_Numeric_Value, &tmp);
    if (tmp != 400) {
        RawDoFmt(",c=%ld", &tmp, (void*)StuffChar, &ptr); ptr--;
    }

    get(slPhase, MUIA_Numeric_Value, &tmp);
    if (tmp != 64) {
        RawDoFmt(",ph=%ld", &tmp, (void*)StuffChar, &ptr); ptr--;
    }

    get(slScanlines, MUIA_Numeric_Value, &tmp);
    if (tmp != 4) {
        RawDoFmt(",sc=%ld", &tmp, (void*)StuffChar, &ptr); ptr--;
    }

    get(slScanlinesLaced, MUIA_Numeric_Value, &tmp);
    if (tmp != 4) {
        RawDoFmt(",scl=%ld", &tmp, (void*)StuffChar, &ptr); ptr--;
    }

    CopyToClipboard(str);

    return 0;
}

ULONG OpenHookFunc()
{
    struct Library *AslBase = OpenLibrary("asl.library", 0);
    extern const char default_dir[];
    if (AslBase != NULL)
    {
        struct FileRequester *fr = AllocAslRequest(ASL_FileRequest, NULL);
        if (fr != NULL)
        {
            BOOL result = AslRequestTags(fr,
                ASLFR_TitleText, (ULONG)_(MSG_ASL_OPEN_PRESET),
                ASLFR_DoSaveMode, FALSE,
                ASLFR_RejectIcons, TRUE,
                ASLFR_InitialDrawer, (ULONG)default_dir,
                ASLFR_Screen, (ULONG)screen,
                TAG_DONE, 0UL
            );

            if (result)
            {
                struct Preset p;
                char *charptr;
                char *c;
                ULONG tmp;

                if (LoadPreset(&p, fr->fr_File, fr->fr_Drawer))
                {
                    set(slScanlines, MUIA_Numeric_Value, p.pr_Scanlines);
                    set(slScanlinesLaced, MUIA_Numeric_Value, p.pr_ScanlinesLaced);
                    set(slCropW, MUIA_Numeric_Value, p.pr_Width);
                    set(slCropH, MUIA_Numeric_Value, p.pr_Height);
                    set(slCropX, MUIA_Numeric_Value, p.pr_DX);
                    set(slCropY, MUIA_Numeric_Value, p.pr_DY);
                    set(slAspect, MUIA_Numeric_Value, p.pr_Aspect);
                    set(slB, MUIA_Numeric_Value, p.pr_B);
                    set(slC, MUIA_Numeric_Value, p.pr_C);
                    set(slPhase, MUIA_Numeric_Value, p.pr_Phase);
                    set(chInteger, MUIA_Selected, p.pr_Integer ? TRUE : FALSE);
                    set(chSmooth, MUIA_Selected, p.pr_Smooth ? TRUE : FALSE);
                }
            }

            FreeAslRequest(fr);
        }

        CloseLibrary(AslBase);
    }

    return 0;
}

ULONG SaveAsHookFunc()
{
    struct Library *AslBase = OpenLibrary("asl.library", 0);
    extern const char default_dir[];
    if (AslBase != NULL)
    {
        struct FileRequester *fr = AllocAslRequest(ASL_FileRequest, NULL);
        if (fr != NULL)
        {
            BOOL result = AslRequestTags(fr,
                ASLFR_TitleText, (ULONG)_(MSG_ASL_SAVE_PRESET),
                ASLFR_DoSaveMode, TRUE,
                ASLFR_RejectIcons, TRUE,
                ASLFR_InitialDrawer, (ULONG)default_dir,
                ASLFR_Screen, (ULONG)screen,
                TAG_DONE, 0UL
            );

            if (result)
            {
                struct Preset p;
                char *charptr;
                char *c;
                ULONG tmp;
                
                get(slCropW, MUIA_Numeric_Value, &tmp);
                p.pr_Width = tmp;

                get(slCropH, MUIA_Numeric_Value, &tmp);
                p.pr_Height = tmp;

                get(slCropX, MUIA_Numeric_Value, &tmp);
                p.pr_DX = tmp;

                get(slCropY, MUIA_Numeric_Value, &tmp);
                p.pr_DY = tmp;

                get(slAspect, MUIA_Numeric_Value, &tmp);
                p.pr_Aspect = tmp;

                get(slB, MUIA_Numeric_Value, &tmp);
                p.pr_B = tmp;

                get(slC, MUIA_Numeric_Value, &tmp);
                p.pr_C = tmp;

                get(slPhase, MUIA_Numeric_Value, &tmp);
                p.pr_Phase = tmp;

                get(slScanlines, MUIA_Numeric_Value, &tmp);
                p.pr_Scanlines = tmp;

                get(slScanlinesLaced, MUIA_Numeric_Value, &tmp);
                p.pr_ScanlinesLaced = tmp;

                get(chSmooth, MUIA_Selected, &tmp);
                p.pr_Smooth = tmp;

                get(chInteger, MUIA_Selected, &tmp);
                p.pr_Integer = tmp;

                SavePreset(&p, fr->fr_File, fr->fr_Drawer);
            }

            FreeAslRequest(fr);
        }

        CloseLibrary(AslBase);
    }

    return 0;
}

ULONG AboutHookFunc()
{
    static char ver[32] = {0};
    static char git[32] = {0};
    static char ever[32] = {0};

    APTR DeviceTreeBase = OpenResource("devicetree.resource");
    APTR key = DT_OpenKey("/emu68");

    if (key) {
        const ULONG *v = DT_GetPropValue(DT_FindProperty(key, "version"));
        char *ptr = ever;

        RawDoFmt("%ld.%ld.%ld", (APTR)v, (void*)StuffChar, &ptr);

        set(txEmuVer, MUIA_Text_Contents, (ULONG)ever);

        DT_CloseKey(key);
    }


    if (rga_get_string(FTCMD_GET_VERSION, ver, 32)) {
        set(txFTVer, MUIA_Text_Contents, (ULONG)ver);
    }
    else {
        set(txFTVer, MUIA_Text_Contents, (ULONG)_(MSG_NOT_AVAILABLE));
    }
    if (rga_get_string(FTCMD_GET_GIT, git, 32)) {
        set(txFTGit, MUIA_Text_Contents, (ULONG)git);
    } else {
        set(txFTGit, MUIA_Text_Contents, (ULONG)_(MSG_NOT_AVAILABLE));
    }
    
    set(aboutWin, MUIA_Window_Open, TRUE);

    return 0;
}

#define RLabel1(label) MUI_MakeObject(MUIO_Label,(unsigned long)label,MUIO_Label_SingleFrame)
#define CLabel1(label) MUI_MakeObject(MUIO_Label,(unsigned long)label,MUIO_Label_Centered|MUIO_Label_SingleFrame)

BOOL BuildGUI(struct Screen * myScreen)
{
    extern const char version[];
    extern APTR UnicamBase;

    app = ApplicationObject,
        MUIA_Application_Title,       "UniTool",
        MUIA_Application_Version,     version,
        MUIA_Application_Base,        "UNITOOL",

        MUIA_Application_Menustrip, MenustripObject,
            MUIA_Family_Child, MenuObject,
                MUIA_Menu_Title, (ULONG)_(MSG_MENU_PROJECT),
                MUIA_Family_Child, menuOpen = MenuitemObject,
                    MUIA_Menuitem_Title, (ULONG)_(MSG_MENU_OPEN),
                    MUIA_Menuitem_Shortcut, "O",
                End,
                MUIA_Family_Child, menuSaveAs = MenuitemObject,
                    MUIA_Menuitem_Title, (ULONG)_(MSG_MENU_SAVE_AS),
                    MUIA_Menuitem_Shortcut, "A",
                End,
                MUIA_Family_Child, MenuitemObject,
                    MUIA_Menuitem_Title, (APTR)-1,
                End,
                MUIA_Family_Child, menuAbout = MenuitemObject,
                    MUIA_Menuitem_Title, (ULONG)_(MSG_MENU_ABOUT),
                End,
                MUIA_Family_Child, MenuitemObject,
                    MUIA_Menuitem_Title, (APTR)-1,
                End,
                MUIA_Family_Child, menuQuit = MenuitemObject,
                    MUIA_Menuitem_Title, (ULONG)_(MSG_MENU_QUIT),
                    MUIA_Menuitem_Shortcut, "Q",
                End,
                
            End,
            MUIA_Family_Child, MenuObject,
                MUIA_Menu_Title, (ULONG)_(MSG_MENU_EDIT),
                MUIA_Family_Child, menuCopyConfig = MenuitemObject,
                    MUIA_Menuitem_Title, (ULONG)_(MSG_MENU_COPY_TO_CLIPBOARD),
                    MUIA_Menuitem_Shortcut, "C",
                End,
                MUIA_Family_Child, MenuitemObject,
                    MUIA_Menuitem_Title, (APTR)-1,
                End,
                MUIA_Family_Child, menuDefaults = MenuitemObject,
                    MUIA_Menuitem_Title, (ULONG)_(MSG_MENU_RESET_TO_DEFAULTS),
                    MUIA_Menuitem_Shortcut, "D",
                End,
                MUIA_Family_Child, menuLastUsed = MenuitemObject,
                    MUIA_Menuitem_Title, (ULONG)_(MSG_MENU_RESET_TO_LAST_USED),
                    MUIA_Menuitem_Shortcut, "L",
                End,
            End,
        End,

        SubWindow, aboutWin = WindowObject,
            MUIA_Window_Screen,       myScreen,
            MUIA_Window_LeftEdge,     MUIV_Window_LeftEdge_Centered,
            MUIA_Window_TopEdge,      MUIV_Window_TopEdge_Centered,
            MUIA_Window_Borderless,   TRUE,
            MUIA_Window_Title,        NULL,
            MUIA_Window_CloseGadget,  FALSE,
            MUIA_Window_DragBar,      FALSE,
            MUIA_Window_DepthGadget,  FALSE,
            MUIA_Window_SizeGadget,   FALSE,

            WindowContents, VGroup, GroupFrame,
                Child, VSpace(5),
                Child, TextObject,
                    MUIA_Text_PreParse, "\033b\033c",
                    MUIA_Text_Contents, "UniTool",
                End,
                Child, VSpace(5),
                Child, CLabel1(_(MSG_ABOUT_TEXT)),
                Child, VSpace(10),
                Child, ColGroup(2),
                    Child, RLabel1(_(MSG_ABOUT_VERSION)),
                    Child, LLabel1(VERSION_NUMBER),
                    Child, RLabel1(_(MSG_ABOUT_EMU68_VERSION)),
                    Child, txEmuVer = LLabel1(""),
                    Child, RLabel1(_(MSG_ABOUT_FT_VERSION)),
                    Child, txFTVer = LLabel1(""),
                    Child, RLabel1(_(MSG_ABOUT_FT_GIT)),
                    Child, txFTGit = LLabel1(""),
                End,
                Child, VSpace(12),
                Child, HGroup,
                    Child, HSpace(0),
                    Child, closeAbout = SimpleButton(_(MSG_ABOUT_DISMISS)),
                    Child, HSpace(0),
                End,
                Child, VSpace(5),
            End,
        End,

        SubWindow, win = WindowObject,
            MUIA_Window_Screen,       myScreen,
            MUIA_Window_LeftEdge,     MUIV_Window_LeftEdge_Centered,
            MUIA_Window_TopEdge,      MUIV_Window_TopEdge_Centered,
            MUIA_Window_Borderless,   TRUE,
            MUIA_Window_Title,        NULL,
            MUIA_Window_CloseGadget,  FALSE,
            MUIA_Window_DragBar,      FALSE,
            MUIA_Window_DepthGadget,  FALSE,
            MUIA_Window_SizeGadget,   FALSE,

            WindowContents, vg = VGroup,
                /* ---- Crop ---- */
                Child, VGroup, GroupFrameT(_(MSG_IMAGE_CROP_AND_SIZE)),
                    Child, ColGroup(2),
                        Child, LLabel1(_(MSG_WIDTH)),
                        Child, slCropW = SliderObject,
                            MUIA_Slider_Min, 1, MUIA_Slider_Max, 720,
                            MUIA_Slider_Level, 720,
                            MUIA_Window_ActiveObject, TRUE,
                            MUIA_MinWidth, 100,
                        End,

                        Child, LLabel1(_(MSG_HEIGHT)),
                        Child, slCropH = SliderObject,
                            MUIA_Slider_Min, 1, MUIA_Slider_Max, 576,
                            MUIA_Slider_Level, 576,
                        End,

                        Child, LLabel1(_(MSG_OFFSET_X)),
                        Child, slCropX = SliderObject,
                            MUIA_Slider_Min, 0, MUIA_Slider_Max, 719,
                            MUIA_Slider_Level, 0,
                        End,

                        Child, LLabel1(_(MSG_OFFSET_Y)),
                        Child, slCropY = SliderObject,
                            MUIA_Slider_Min, 0, MUIA_Slider_Max, 575,
                            MUIA_Slider_Level, 0,
                        End,

                        Child, LLabel1(_(MSG_ASPECT_RATIO)),
                        Child, slAspect = SliderObject,
                            MUIA_Slider_Min,   300,
                            MUIA_Slider_Max,   3000,
                            MUIA_Slider_Level, 1000,
                        End,
                    End,
                End,
                /* ---- Scale ---- */
                Child, VGroup, GroupFrameT(_(MSG_SCALING_AND_SMOOTHING)),
                    Child, ColGroup(2),
                        Child, LLabel1(_(MSG_INTEGER_ONLY)),
                        Child, HGroup,
                            Child, chInteger = CheckMark(FALSE),
                            Child, HSpace(0),
                            Child, LLabel1(_(MSG_SMOOTHING)),
                            Child, HGroup,
                                Child, chSmooth = CheckMark(FALSE),
                                Child, HSpace(0),
                            End,
                        End,

                        Child, LLabel1(_(MSG_KERNEL_B)),
                        Child, slB = SliderObject,
                            MUIA_Slider_Min,   0,
                            MUIA_Slider_Max,   1000,
                            MUIA_Slider_Level, 333,
                            MUIA_MinWidth,     100,
                        End,

                        Child, LLabel1(_(MSG_KERNEL_C)),
                        Child, slC = SliderObject,
                            MUIA_Slider_Min,   0,
                            MUIA_Slider_Max,   1000,
                            MUIA_Slider_Level, 333,
                        End,
                        
                        Child, LLabel1(_(MSG_PHASE)),
                        Child, slPhase = SliderObject,
                            MUIA_Slider_Min,   0,
                            MUIA_Slider_Max,   255,
                            MUIA_Slider_Level, 64,
                        End,
                    End,
                End,
                /* ---- Image ---- */
                Child, VGroup, GroupFrameT(_(MSG_SCANLINES)),
                    Child, ColGroup(2),
                        Child, LLabel1(_(MSG_SCANLINES_NORMAL)),
                        Child, slScanlines = SliderObject,
                            MUIA_Slider_Min,   0,
                            MUIA_Slider_Max,   4,
                            MUIA_Slider_Level, 0,
                            MUIA_MinWidth, 40,
                        End,

                        Child, LLabel1(_(MSG_SCANLINES_LACED)),
                        Child, slScanlinesLaced = SliderObject,
                            MUIA_Slider_Min,   0,
                            MUIA_Slider_Max,   4,
                            MUIA_Slider_Level, 0,
                        End,
                    End,
                End,
            End, // VGroup
        End, // WindowObject
    End; // ApplicationObject

    RGA_VideoStatus vstat;

    rga_flush_pipe();

    if (rga_get_video_status(&vstat)) {
        def_scan = vstat.scanline_level;
        def_scanl = vstat.scanline_level_laced;
        set(slScanlines, MUIA_Numeric_Value, def_scan);
        set(slScanlinesLaced, MUIA_Numeric_Value, def_scanl);
    }

    DoMethod(win, MUIM_Window_SetCycleChain,
        slCropW, slCropH, slCropX, slCropY, slAspect,
        chInteger, chSmooth, slB, slC, slPhase,
        slScanlines, slScanlinesLaced,
        NULL);

    ULONG val = UnicamGetCropOffset();
    def_x = val >> 16;
    def_y = val & 0xffff;
    
    val = UnicamGetCropSize();
    def_w = val >> 16;
    def_h = val & 0xffff;
    
    def_aspect = UnicamGetAspect();

    val = UnicamGetKernel();
    def_b = val >> 16;
    def_c = val & 0xffff;

    val = UnicamGetConfig();

    def_integer = !!(val & UNICAMF_INTEGER);
    def_smooth = !!(val & UNICAMF_SMOOTHING);
    def_phase = (val & UNICAMF_PHASE) >> UNICAMB_PHASE;

    set(slCropX, MUIA_Numeric_Value, def_x);
    set(slCropY, MUIA_Numeric_Value, def_y);
    set(slCropW, MUIA_Numeric_Value, def_w);
    set(slCropH, MUIA_Numeric_Value, def_h);
    set(slAspect, MUIA_Numeric_Value, def_aspect);
    set(slB, MUIA_Numeric_Value, def_b);
    set(slC, MUIA_Numeric_Value, def_c);
    set(slPhase, MUIA_Numeric_Value, def_phase);
    if (def_integer)
        set(chInteger, MUIA_Selected, TRUE);
    if (def_smooth)
        set(chSmooth, MUIA_Selected, TRUE);

    cropOffsetHook.h_Entry = (HOOKFUNC)CropOffsetHookFunc;
    cropSizeHook.h_Entry   = (HOOKFUNC)CropSizeHookFunc;
    aspectHook.h_Entry     = (HOOKFUNC)AspectHookFunc;
    kernelHook.h_Entry     = (HOOKFUNC)KernelHookFunc;
    configHook.h_Entry     = (HOOKFUNC)ConfigHookFunc;
    scanlineHook.h_Entry   = (HOOKFUNC)ScanlineHookFunc;
    defaultsHook.h_Entry   = (HOOKFUNC)ResetToDefaultsHookFunc;
    lastUsedHook.h_Entry   = (HOOKFUNC)ResetToLastUsedHookFunc;
    openHook.h_Entry       = (HOOKFUNC)OpenHookFunc;
    saveAsHook.h_Entry     = (HOOKFUNC)SaveAsHookFunc;
    copyConfigHook.h_Entry = (HOOKFUNC)CopyConfigHookFunc;
    aboutHook.h_Entry      = (HOOKFUNC)AboutHookFunc;

    DoMethod(slCropX, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime,
        app, 2, MUIM_CallHook, &cropOffsetHook);

    DoMethod(slCropY, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime,
        app, 2, MUIM_CallHook, &cropOffsetHook);

    DoMethod(slCropW, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime,
        app, 2, MUIM_CallHook, &cropSizeHook);

    DoMethod(slCropH, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime,
        app, 2, MUIM_CallHook, &cropSizeHook);

    DoMethod(slAspect, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime,
        app, 2, MUIM_CallHook, &aspectHook);

    DoMethod(slB, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime,
        app, 2, MUIM_CallHook, &kernelHook);

    DoMethod(slC, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime,
        app, 2, MUIM_CallHook, &kernelHook);

    DoMethod(slPhase, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime,
        app, 2, MUIM_CallHook, &configHook);

    DoMethod(chInteger, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
        app, 2, MUIM_CallHook, &configHook);

    DoMethod(chSmooth, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
        app, 2, MUIM_CallHook, &configHook);

    DoMethod(slScanlines, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime,
        app, 2, MUIM_CallHook, &scanlineHook);

    DoMethod(slScanlinesLaced, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime,
        app, 2, MUIM_CallHook, &scanlineHook);

    DoMethod(menuQuit, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
        (ULONG)app, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);

    DoMethod(menuOpen, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
        (ULONG)app, 2, MUIM_CallHook, &openHook);
    
    DoMethod(menuSaveAs, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
        (ULONG)app, 2, MUIM_CallHook, &saveAsHook);

    DoMethod(menuCopyConfig, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
        (ULONG)app, 2, MUIM_CallHook, &copyConfigHook);

    DoMethod(menuDefaults, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
        (ULONG)app, 2, MUIM_CallHook, &defaultsHook);

    DoMethod(menuLastUsed, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
        (ULONG)app, 2, MUIM_CallHook, &lastUsedHook);
    
    DoMethod(menuAbout, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
        (ULONG)app, 2, MUIM_CallHook, &aboutHook);

    DoMethod(menuAbout, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
        (ULONG)vg, 3, MUIM_Set, MUIA_Disabled, TRUE);

    DoMethod(win, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
        app, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);

    DoMethod(closeAbout, MUIM_Notify, MUIA_Pressed, FALSE,
        aboutWin, 3, MUIM_Set, MUIA_Window_Open, FALSE);

    DoMethod(closeAbout, MUIM_Notify, MUIA_Pressed, FALSE,
        vg, 3, MUIM_Set, MUIA_Disabled, FALSE);

    return app != NULL;
}

int StartGUI(LONG ntsc)
{
    UnicamBase = OpenResource("unicam.resource");
    GfxBase = (struct GfxBase *)OpenLibrary("graphics.library", 39);

    if (GfxBase)
    {
        IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 39);

        if (IntuitionBase)
        {
            MUIMasterBase = OpenLibrary(MUIMASTER_NAME, MUIMASTER_VMIN);

            if (MUIMasterBase)
            {
                OpenNativeScreen(ntsc);
                if (screen)
                {
                    //CloneWorkbenchPalette(screen);

                    if (BuildGUI(screen))
                    {
                        set(win, MUIA_Window_Open, TRUE);
                       
                        ULONG sigs = 0;
                        BOOL running = TRUE;
                        while (running)
                        {
                            ULONG ret = DoMethod(app, MUIM_Application_NewInput, &sigs);
                            if (ret == MUIV_Application_ReturnID_Quit)
                                running = FALSE;

                            if (running && sigs)
                                sigs = Wait(sigs | SIGBREAKF_CTRL_C);

                            if (sigs & SIGBREAKF_CTRL_C)
                                running = FALSE;
                        }

                        set(win, MUIA_Window_Open, FALSE);

                        MUI_DisposeObject(app);
                    }
                    
                    CloseWindow(backdrop);
                    CloseScreen(screen);
                }
            }
            else
            {
                Printf(_(MSG_FAILED_TO_OPEN_NAME_VERSION), (ULONG)MUIMASTER_NAME, MUIMASTER_VMIN);
            }

            CloseLibrary((struct Library *)IntuitionBase);
        }
        else
        {
            Printf(_(MSG_FAILED_TO_OPEN_NAME_VERSION), (ULONG)"intuition.library", 39);
        }

        CloseLibrary((struct Library *)GfxBase);
    }
    else
    {
        Printf(_(MSG_FAILED_TO_OPEN_NAME_VERSION), (ULONG)"graphics.library", 39);
    }
}
