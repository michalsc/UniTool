#include <exec/types.h>
#include <exec/execbase.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/unicam.h>

#include "messages.h"

int main(int);

/* Startup code including workbench message support */
int _start()
{
    struct ExecBase *SysBase = *(struct ExecBase **)4;
    struct Process *p = NULL;
    struct WBStartup *wbmsg = NULL;
    int ret = 0;

    p = (struct Process *)SysBase->ThisTask;

    if (p->pr_CLI == 0)
    {
        WaitPort(&p->pr_MsgPort);
        wbmsg = (struct WBStartup *)GetMsg(&p->pr_MsgPort);
    }

    ret = main(wbmsg ? 1 : 0);

    if (wbmsg)
    {
        Forbid();
        ReplyMsg((struct Message *)wbmsg);
    }

    return ret;
}

struct ExecBase *       SysBase;
struct DosLibrary *     DOSBase;
APTR                    UnicamBase;

#define APPNAME "EmuControl"

static const char version[] __attribute__((used)) = "$VER: " VERSION_STRING;

#define RDA_TEMPLATE "WIDTH/K/N,HEIGHT/K/N,X/K/N,Y/K/N,B/K/N,C/K/N,ASPECT/K/N,PHASE/K/N,SCALER/K/N,SMOOTH/S,INTEGER/S,GUI/S,S=Silent/S"

enum {
    OPT_WIDTH,
    OPT_HEIGHT,
    OPT_DX,
    OPT_DY,
    OPT_B,
    OPT_C,
    OPT_ASPECT,
    OPT_PHASE,
    OPT_SCALER,
    OPT_SMOOTH,
    OPT_INTEGER,
    OPT_GUI,
    OPT_SILENT,
    OPT_COUNT
};

LONG result[OPT_COUNT];

int main(int wantGUI)
{
    struct RDArgs *args;
    SysBase = *(struct ExecBase **)4;

    UnicamBase = OpenResource("unicam.resource");
    if (UnicamBase == NULL)
        return -1;

    /* Unicam resource must be of version 1.2 or higher, otherwise it does not expose functions we need */
    if (((struct Library *)UnicamBase)->lib_Version < 1 ||
        (((struct Library *)UnicamBase)->lib_Version == 1 && ((struct Library *)UnicamBase)->lib_Revision < 3))
        return -1;

    DOSBase = (struct DosLibrary *)OpenLibrary("dos.library", 37);
    if (DOSBase == NULL)
        return -1;

    if (!wantGUI)
    {
        const ULONG max_height = UnicamGetSize() & 0xffff;
        const ULONG max_width = UnicamGetSize() >> 16;
        const ULONG current_height = UnicamGetCropSize() & 0xffff;
        const ULONG current_width = UnicamGetCropSize() >> 16;
        const ULONG current_dy = UnicamGetCropOffset() & 0xffff;
        const ULONG current_dx = UnicamGetCropOffset() >> 16;
        const ULONG kernel_b = UnicamGetKernel() >> 16;
        const ULONG kernel_c = UnicamGetKernel() & 0xffff;

        ULONG silent = 0;
        ULONG smooth = 0;
        ULONG integer = 0;
        LONG width = current_width;
        LONG height = current_height;
        LONG dx = current_dx;
        LONG dy = current_dy;
        LONG b = kernel_b;
        LONG c = kernel_c;
        LONG phase = -1;
        LONG scaler = -1;
        LONG aspect = -1;

        args = ReadArgs(RDA_TEMPLATE, result, NULL);

        if (args)
        {
            wantGUI = result[OPT_GUI];
            silent = result[OPT_SILENT];
            smooth = result[OPT_SMOOTH];
            integer = result[OPT_INTEGER];

            if (!silent)
                Printf("%s\n", (ULONG)&VERSION_STRING[6]);

            if (result[OPT_WIDTH]) {
                width = *(LONG*)(result[OPT_WIDTH]);
                
                if (width < 0 || width > max_width) {
                    width = current_width;
                }
            }

            if (result[OPT_HEIGHT]) {
                height = *(LONG*)(result[OPT_HEIGHT]);
                
                if (height < 0 || height > max_height) {
                    height = current_height;
                }
            }

            if (result[OPT_DX]) {
                dx = *(LONG*)(result[OPT_DX]);
                
                if (dx < 0 || dx + width > max_width) {
                    dx = current_dx;
                }
            }

            if (result[OPT_DY]) {
                dy = *(LONG*)(result[OPT_DY]);
                
                if (dy < 0 || dy + height > max_height) {
                    dy = current_dy;
                }
            }

            if (result[OPT_B]) {
                b = *(LONG*)(result[OPT_B]);
                
                if (b < 0 || b > 1000) {
                    b = -1;
                }
            }

            if (result[OPT_C]) {
                c = *(LONG*)(result[OPT_C]);
                
                if (c < 0 || c > 1000) {
                    c = -1;
                }
            }

            if (result[OPT_ASPECT]) {
                aspect = *(LONG*)(result[OPT_ASPECT]);
                
                if (aspect < 333 || aspect > 3000) {
                    aspect = -1;
                }
            }

            if (result[OPT_PHASE]) {
                phase = *(LONG*)(result[OPT_PHASE]);
                
                if (phase < 0 || phase > 255) {
                    phase = -1;
                }
            }

            if (result[OPT_SCALER]) {
                scaler = *(LONG*)(result[OPT_SCALER]);
                
                if (scaler < 0 || scaler > 3) {
                    scaler = -1;
                }
            }

            UnicamSetCropSize(width, height);
            UnicamSetCropOffset(dx, dy);

            if (aspect > 0)
                UnicamSetAspect(aspect);

            UnicamSetKernel(b, c);

            ULONG cfg = UnicamGetConfig();

            if (smooth) {
                cfg |= UNICAMF_SMOOTHING;
            }
            else {
                cfg &= ~UNICAMF_SMOOTHING;
            }

            if (integer) {
                cfg |= UNICAMF_INTEGER;
            }
            else {
                cfg &= ~UNICAMF_INTEGER;
            }

            if (phase >= 0) {
                cfg &= ~UNICAMF_PHASE;
                cfg |= (phase << UNICAMB_PHASE) & UNICAMF_PHASE;
            }

            if (scaler >= 0) {
                cfg &= ~UNICAMF_SCALER;
                cfg |= (scaler << UNICAMB_SCALER) & UNICAMF_SCALER;
            }
            
            UnicamSetConfig(cfg);

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

            FreeArgs(args);
        }
    }

    return 0;
}
