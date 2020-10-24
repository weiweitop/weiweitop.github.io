// IOS reload test program (C) 2009 bushing

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <ogcsys.h>
#include <gccore.h>
#include <stdarg.h>
#include <ctype.h>

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

typedef void (*Loader_Entry)(void);

Loader_Entry loader = (Loader_Entry)0x80001800;
void __IPC_Reinitialize(void);
void __UnmaskIrq(u32);
void __MaskIrq(u32);

void udelay(int us);
static raw_irq_handler_t prolog_ios_irq = NULL;

void sync_before_read(void *p, u32 len)
{
        u32 a, b;

        a = (u32)p & ~0x1f;
        b = ((u32)p + len + 0x1f) & ~0x1f;

        for ( ; a < b; a += 32)
                asm("dcbi 0,%0" : : "b"(a));

        asm("sync ; isync");
}

void sync_after_write(const void *p, u32 len)
{
        u32 a, b;

        a = (u32)p & ~0x1f;
        b = ((u32)p + len + 0x1f) & ~0x1f;

        for ( ; a < b; a += 32)
                asm("dcbst 0,%0" : : "b"(a));

        asm("sync ; isync");
}

//-----------------------------------------------------------------------------------

void checkAndReload(void) {
	PAD_ScanPads();
	int buttonsDown = PAD_ButtonsHeld(0);
	if( (buttonsDown & PAD_TRIGGER_Z) && (buttonsDown & PAD_BUTTON_START)) {
		loader();
	}
}

void waita(void) {
	while(1) {
		VIDEO_WaitVSync();
		PAD_ScanPads();
		int buttonsDown = PAD_ButtonsDown(0);
		if(buttonsDown & PAD_BUTTON_A)
			return;
		if( (buttonsDown & PAD_TRIGGER_Z) && (buttonsDown & PAD_BUTTON_START)) {
			loader();
		}
	}
}

void printvers(void) {
	printf("IOS Version: IOS%d %d.%d\n",IOS_GetVersion(),IOS_GetRevisionMajor(),IOS_GetRevisionMinor());
}

int doreload=0, dooff=0;

void reload(void) {
	doreload=1;
}

void shutdown(void) {
	dooff=1;
}


s32 test_LaunchNewIOS(int version)
{
        u32 numviews;
		u32 boot2_version = 0;
        s32 res;
        u64 titleID = 0x100000000LL;
        STACK_ALIGN(tikview,views,4,32);
        s32 newversion;

        if(version < 3 || version > 0xFF) {
                return IOS_EBADVERSION;
        }
		res = ES_GetBoot2Version(&boot2_version);
		
		printf("IOS Version: IOS%d %d.%d / boot2v%u\n",IOS_GetVersion(), IOS_GetRevisionMajor(),
			IOS_GetRevisionMinor(), boot2_version);
		
        titleID |= version;
        printf("Launching IOS TitleID: %016llx\n",titleID);

        res = ES_GetNumTicketViews(titleID, &numviews);
        if(res < 0) {
                printf(" GetNumTicketViews failed: %d\n",res);
                return res;
        }
        if(numviews > 4) {
                printf(" GetNumTicketViews too many views: %u\n",numviews);
                return IOS_ETOOMANYVIEWS;
        }

        res = ES_GetTicketViews(titleID, views, numviews);
        if(res < 0) {
                printf(" GetTicketViews failed: %d\n",res);
               return res;
        }

        res = ES_LaunchTitleBackground(titleID, &views[0]);

        if(res < 0) {
                printf(" LaunchTitle failed: %d\n",res);
                return res;
		}

		*((u32*)0x80003140) = 0;  // clear old version number
		sync_after_write((u32 *)0x80003140, 4);

		printf("ES_LaunchTitleBackground done, masking IRQ\n");
	    __MaskIrq(IRQ_PI_ACR);
        prolog_ios_irq = IRQ_Free(IRQ_PI_ACR);

		printf("waiting for IOS version number to be set\n");
		
		u32 ios_counter;
//     wait for IOS version number to be set
		for (ios_counter = 0; (*((u32*)0x80003140) >> 16) == 0; ios_counter++ ) {
			sync_before_read((u32*)0x80003140, 4);
            udelay(1000);
		}			

     	version = *((u32*)0x80003140);
        printf("IOS started after %u ticks: IOS%d v%d.%d\n", ios_counter, version >> 16, 
			(version >> 8) & 0xff, version & 0xff);

		u32 ipc_counter;
//     wait for IPC to start on the newly-loaded IOS
		for (ipc_counter = 0; !(read32(0x0d000004) & 2); ipc_counter++)
			udelay(1000);

		printf("IPC started after %u ticks\n", ipc_counter);

		IRQ_Request(IRQ_PI_ACR, prolog_ios_irq, NULL);
        prolog_ios_irq = NULL;
        __UnmaskIrq(IRQ_PI_ACR);

		printf ("reinitializing IPC\n");
       __IPC_Reinitialize();   
		printf ("resetting ES\n");
       __ES_Reset();   
		printf ("reinitializing IOS subsystems\n");
       __IOS_InitializeSubsystems();   
		printf ("reinit done\n");

        newversion = IOS_GetVersion();
        printf("IOS Version: IOS%d %d.%d\n",newversion,IOS_GetRevisionMajor(),IOS_GetRevisionMinor());
		u32 device_id = 0;
		
		ES_GetDeviceID(&device_id);
		printf("\n\n\nstats: id=0x%08x boot2v%u IOSticks=%u IPCticks=%u\n", device_id, boot2_version, ios_counter, ipc_counter);
        return version;
}

int main(int argc, char **argv) {
	VIDEO_Init();
	PAD_Init();

	rmode = VIDEO_GetPreferredMode(NULL);
	
	xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
	
	VIDEO_Configure(rmode);
	VIDEO_ClearFrameBuffer(rmode,xfb,COLOR_BLACK);
	VIDEO_SetNextFramebuffer(xfb);
	VIDEO_SetBlack(FALSE);
	VIDEO_Flush();
	VIDEO_WaitVSync();
	if(rmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();
	CON_InitEx(rmode,20,30,rmode->fbWidth-40,rmode->xfbHeight-60);

	SYS_SetResetCallback(reload);
	SYS_SetPowerCallback(shutdown);
	
	printf("IOS reload test program (C) 2009 bushing\n");
	test_LaunchNewIOS(34);
	
	while(1) {
		VIDEO_WaitVSync();
		PAD_ScanPads();
		int buttonsDown = PAD_ButtonsDown(0);
		if( (buttonsDown & PAD_TRIGGER_Z) || doreload) {
			exit(0);
		}
	}

	return 0;
}
