#include <3ds.h>
#include <malloc.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#define SOC_ALIGN 0x1000
#define SOC_BUFFERSIZE 0x100000
#define REMOTE_HOST_PORT 1337
int main(int argc, char **argv) {
	gfxInitDefault();
	aptSetSleepAllowed(false);
	PrintConsole top, bottom;
	consoleInit(GFX_TOP, &top);
	consoleInit(GFX_BOTTOM, &bottom);
	consoleSelect(&bottom);
	printf("\x1b[30;0HPress start to exit");
	int ret = 0;
	u32* soc_buffer = (u32*) memalign(SOC_ALIGN, SOC_BUFFERSIZE);
	if (soc_buffer == NULL) {
		consoleSelect(&bottom);
		printf("\x1b[1;0Hmemalign() failed to allocate");
		while (aptMainLoop()) {
			hidScanInput();
			u32 kDown = hidKeysDown();
			if (kDown & KEY_START) break;
			gfxFlushBuffers();
			gfxSwapBuffers();
			gspWaitForVBlank();
		}
		gfxExit();
		return -1;
	}
	if ((ret = socInit(soc_buffer, SOC_BUFFERSIZE)) != 0) {
		consoleSelect(&bottom);
		printf("\x1b[1;0HsocInit() error 0x%08x", (unsigned int) ret);
	}
	int socket_desc;
	struct sockaddr_in server;
	socket_desc = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_desc == -1) {
		consoleSelect(&bottom);
		printf("\x1b[1;0HSocket creation failed");
		while (aptMainLoop()) {
			hidScanInput();
			u32 kDown = hidKeysDown();
			if (kDown & KEY_START) break;
			gfxFlushBuffers();
			gfxSwapBuffers();
			gspWaitForVBlank();
		}
		gfxExit();
		return -1;
	}
	unsigned char remote_host_addr[4] = {192, 168, 0, 0};
	char selected = 1;
	consoleSelect(&top);
	while (aptMainLoop()) {
		hidScanInput();
		u32 down = hidKeysDown();
		if (down & KEY_START) goto prgmend;
		if (down & KEY_A) break;
		if (down & KEY_DLEFT) {
			selected--;
			if (selected < 1) selected = 15;
			if (selected == 4 || selected == 8 || selected == 12) selected--;
		}
		if (down & KEY_DRIGHT) {
			selected++;
			if (selected > 15) selected = 1;
			if (selected == 4 || selected == 8 || selected == 12) selected++;
		}
		if (down & KEY_DUP) {
			if (selected < 4) {
				remote_host_addr[0]++;
			} else if (selected < 8) {
				remote_host_addr[1]++;
			} else if (selected < 12) {
				remote_host_addr[2]++;
			} else {
				remote_host_addr[3]++;
			}
		}
		if (down & KEY_DDOWN) {
			if (selected < 4) {
				remote_host_addr[0]--;
			} else if (selected < 8) {
				remote_host_addr[1]--;
			} else if (selected < 12) {
				remote_host_addr[2]--;
			} else {
				remote_host_addr[3]--;
			}
		}
		if (down & KEY_L) {
			if (selected < 4) {
				remote_host_addr[0] -= 10;
			} else if (selected < 8) {
				remote_host_addr[1] -= 10;
			} else if (selected < 12) {
				remote_host_addr[2] -= 10;
			} else {
				remote_host_addr[3] -= 10;
			}
		}
		if (down & KEY_R) {
			if (selected < 4) {
				remote_host_addr[0] += 10;
			} else if (selected < 8) {
				remote_host_addr[1] += 10;
			} else if (selected < 12) {
				remote_host_addr[2] += 10;
			} else {
				remote_host_addr[3] += 10;
			}
		}
		printf("\x1b[1;0H%03i.%03i.%03i.%03i", remote_host_addr[0], remote_host_addr[1], remote_host_addr[2], remote_host_addr[3]);
		printf("\x1b[2;0H               ");
		printf("\x1b[2;%iH^", selected);
		printf("\x1b[4;0HD-Pad to edit, shoulder buttons to add/sub 10");
		gfxFlushBuffers();
		gfxSwapBuffers();
		gspWaitForVBlank();
	}
	char blank[50];
	for (int i = 0; i < sizeof(blank)/sizeof(blank[0]); i++) {
		blank[i] = ' ';
	}
	consoleSelect(&top);
	for (int i = 0; i < 31; i++) {
		printf("\x1b[%i;0H%s", i, blank);
	}
	consoleSelect(&bottom);
	char remote_host_addr_cstr[16];
	snprintf(remote_host_addr_cstr, sizeof(remote_host_addr_cstr)/sizeof(remote_host_addr_cstr[0]), "%i.%i.%i.%i", remote_host_addr[0], remote_host_addr[1], remote_host_addr[2], remote_host_addr[3]);
	printf("\x1b[1;0HConnecting to %s:%i", remote_host_addr_cstr, REMOTE_HOST_PORT);
	printf("\x1b[2;0HConsole may freeze if address is wrong");
	gfxFlushBuffers();
	gfxSwapBuffers();
	gspWaitForVBlank();
	server.sin_addr.s_addr = inet_addr(remote_host_addr_cstr);
	server.sin_family = AF_INET;
	server.sin_port = htons(REMOTE_HOST_PORT);
	if (connect(socket_desc, (struct sockaddr *) &server, sizeof(server)) < 0) {
		consoleSelect(&bottom);
		printf("\x1b[3;0HCouldn't connect to remote server");
		while (aptMainLoop()) {
			hidScanInput();
			u32 kDown = hidKeysDown();
			if (kDown & KEY_START) break;
			gfxFlushBuffers();
			gfxSwapBuffers();
			gspWaitForVBlank();
		}
		gfxExit();
		return -1;
	}
	fcntl(socket_desc, F_SETFL, fcntl(socket_desc, F_GETFL, 0) | O_NONBLOCK);
	consoleSelect(&bottom);
	printf("\x1b[3;0HConnected to remote server");
	char reply[1550];
	int line = 1;
	char replyComp[sizeof(reply)/sizeof(reply[0])];
	while (aptMainLoop()) {
		hidScanInput();
		u32 down = hidKeysDown();
		if (down & KEY_START) break;
		if (recv(socket_desc, reply, sizeof(reply)/sizeof(reply[0]), 0) > -1) {
			if (reply[0] == 'e' && reply[1] == 'x' && reply[2] == 'i' && reply[3] == 't') {
				char blank[50];
				for (int i = 0; i < sizeof(blank)/sizeof(blank[0]); i++) {
					blank[i] = ' ';
				}
				consoleSelect(&top);
				for (int i = 0; i < 31; i++) {
					printf("\x1b[%i;0H%s", i, blank);
				}
				consoleSelect(&bottom);
				printf("\x1b[3;0HConnection terminated by remote host");
				while (aptMainLoop()) {
					hidScanInput();
					u32 down = hidKeysDown();
					if (down & KEY_START) goto prgmend;
					gfxFlushBuffers();
					gfxSwapBuffers();
					gspWaitForVBlank();
				}
			}
			if (strcmp(reply, replyComp) != 0) {
				consoleSelect(&top);
				char *token = strtok(reply, "\n");
				while (token != NULL) {
					printf("\x1b[%i;0H%s", line, token);
					token = strtok(NULL, "\n");
					line++;
					if (line > 30) line = 1;
				}
			}
			strcpy(replyComp, reply);
			for (int i = 0; i < sizeof(reply)/sizeof(reply[0]); i++) {
				reply[i] = '\0';
			}
		}
		gfxFlushBuffers();
		gfxSwapBuffers();
		gspWaitForVBlank();
	}
	prgmend:
	close(socket_desc);
	aptSetSleepAllowed(true);
	gfxExit();
	return 0;
}