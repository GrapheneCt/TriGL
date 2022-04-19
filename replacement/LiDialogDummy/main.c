#include <kernel.h>

int module_start(SceSize args, const void * argp)
{
	return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize args, const void * argp)
{
	return SCE_KERNEL_STOP_SUCCESS;
}

void showDialog(char *text)
{
	sceClibPrintf("--- fwDialog ---\n%s\n----------------\n", text);

	while (1) {
		sceKernelDelayThread(10000);
	}
}

int fwDialog_0575EE1C() { return 0; }
int fwDialog_14B4F898() { return 0; }
int fwDialog_15DBC805() { return 0; }
int fwDialog_3FB39954() { return 0; }
int fwDialog_40D0C601() { return 0; }
int fwDialog_4EAD278F() { return 0; }
int fwDialog_6366F73D() { return 0; }
int fwDialog_63F835F0(char *text, int param_2, int param_3, int param_4)
{
	showDialog(text);

	return 0; 
}
int fwDialog_90810E0A() { return 0; }
int fwDialog_9AF23640(char *text, int param_2, int param_3)
{
	showDialog(text);

	return 0;
}
int fwDialog_A20A364C() { return 0; }
int fwDialog_D04D173B()
{
	// init
	return 0;
}
int fwDialog_D6F9F508()
{
	showDialog("fwDialog_D6F9F508");

	return 0;
}
int fwDialog_F37FFF30(int param_1, int param_2)
{
	showDialog("fwDialog_F37FFF30");

	return 0;
}
int fwDialog_F917A161(char *text)
{
	showDialog(text);

	return 0;
}