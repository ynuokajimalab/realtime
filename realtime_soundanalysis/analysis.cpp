#include <windows.h>
#include <mmsystem.h>
#pragma comment (lib, "winmm.lib")
#include "resource.h"
#include "countjoint.h"

//メインウィンドウの設定
#define TITLE 	TEXT("analyze")
#define WIDTH	400
#define HEIGHT	400

//ボタンの設定
#define WIDTH_BT	180
#define HEIGHT_BT	100
#define MARGIN	10
//ラベルの設定
#define WIDTH_LB	200
#define HEIGHT_LB	20

#define SRATE 	11025
#define TIMEPERBUFFER 0.1


HWND hwnd, hwnd_btstart, hwnd_btend1, hwnd_btplay, hwnd_btend2, hwnd_lbcount, hwnd_lbdata, hwnd_lbjointtime;
RECT rc;
int iClientWidth, iClientHeight, iInterval;
joint* pJoint;

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp) {
	static WAVEFORMATEX wfe;
	static HWAVEOUT hWaveOut;
	static HWAVEIN hWaveIn;
	static BYTE *bWave1, *bWave2, *bSave, *bTmp;
	static WAVEHDR whdr1, whdr2;
	static DWORD dwLength = 0, dwCount, dwTempLength;
	static BOOL blReset = FALSE,blWaveinOpen = FALSE;

	switch (msg) {
	case WM_DESTROY:
		if (bSave) free(bSave);
		PostQuitMessage(0);
		return 0;
	case WM_CREATE:
		wfe.wFormatTag = WAVE_FORMAT_PCM;
		wfe.nChannels = 1;
		wfe.nSamplesPerSec = SRATE;
		wfe.wBitsPerSample = 8;
		wfe.nBlockAlign = wfe.nChannels * wfe.wBitsPerSample / 8;;
		wfe.nAvgBytesPerSec = SRATE*wfe.nBlockAlign;
		wfe.cbSize = 0;
		return 0;
	case WM_COMMAND:
		switch (LOWORD(wp)) {
		case IDB_STR:
			bWave1 = (BYTE*)malloc(SRATE);
			bWave2 = (BYTE*)malloc(SRATE);

			whdr1.lpData = (LPSTR)bWave1;
			whdr1.dwBufferLength = wfe.nAvgBytesPerSec*TIMEPERBUFFER;
			whdr1.dwBytesRecorded = 0;
			whdr1.dwFlags = 0;
			whdr1.dwLoops = 1;
			whdr1.lpNext = NULL;
			whdr1.dwUser = 0;
			whdr1.reserved = 0;

			whdr2.lpData = (LPSTR)bWave2;
			whdr2.dwBufferLength = wfe.nAvgBytesPerSec*TIMEPERBUFFER;
			whdr2.dwBytesRecorded = 0;
			whdr2.dwFlags = 0;
			whdr2.dwLoops = 1;
			whdr2.lpNext = NULL;
			whdr2.dwUser = 0;
			whdr2.reserved = 0;
			if (waveInOpen(&hWaveIn, WAVE_MAPPER, &wfe, (DWORD)hWnd, 0, CALLBACK_WINDOW) == MMSYSERR_NOERROR) {
				blWaveinOpen = TRUE;
			}
			waveInPrepareHeader(hWaveIn, &whdr1, sizeof(WAVEHDR));
			waveInPrepareHeader(hWaveIn, &whdr2, sizeof(WAVEHDR));
			break;
		case IDB_PLAY:
			whdr1.lpData = (LPSTR)bSave;
			whdr1.dwBufferLength = dwLength;
			whdr1.dwBytesRecorded = 0;
			whdr1.dwFlags = WHDR_BEGINLOOP | WHDR_ENDLOOP;
			whdr1.dwLoops = 1;
			whdr1.lpNext = NULL;
			whdr1.dwUser = 0;
			whdr1.reserved = 0;

			waveOutOpen(&hWaveOut, WAVE_MAPPER, &wfe,
				(DWORD)hWnd, 0, CALLBACK_WINDOW);
			waveOutPrepareHeader(
				hWaveOut, &whdr1, sizeof(WAVEHDR));
			waveOutWrite(hWaveOut, &whdr1, sizeof(WAVEHDR));
			break;
		case IDB_REC_END:
			blReset = TRUE;
			waveInReset(hWaveIn);
			break;
		case IDB_PLY_END:
			waveOutReset(hWaveOut);
			break;
		}
		return 0;
	case MM_WIM_OPEN:
		dwLength = 0;
		bSave = (BYTE*)realloc(bSave, 1);

		EnableWindow(GetDlgItem(hWnd, IDB_PLAY), FALSE);
		EnableWindow(GetDlgItem(hWnd, IDB_STR), FALSE);
		EnableWindow(GetDlgItem(hWnd, IDB_REC_END), TRUE);
		waveInAddBuffer(hWaveIn, &whdr1, sizeof(WAVEHDR));
		waveInAddBuffer(hWaveIn, &whdr2, sizeof(WAVEHDR));
		waveInStart(hWaveIn);
		return 0;
	case MM_WIM_DATA:
		if (((PWAVEHDR)lp)->dwBytesRecorded != ((PWAVEHDR)lp)->dwBufferLength) {
			if (blWaveinOpen == TRUE) {
				if (waveInClose(hWaveIn) == MMSYSERR_NOERROR) {
					blWaveinOpen = FALSE;
				}
			}
			blReset = FALSE;
			return 0;
		}
		dwTempLength = dwLength + ((PWAVEHDR)lp)->dwBytesRecorded;
		bTmp = (BYTE*)realloc(bSave, dwTempLength);
		if (blReset || !bTmp) {
			if (blWaveinOpen == TRUE) {
				if (waveInClose(hWaveIn) == MMSYSERR_NOERROR) {
					blWaveinOpen = FALSE;
				}
			}
			blReset = FALSE;
			return 0;
		}

		bSave = bTmp;

		for (dwCount = 0; dwCount < ((PWAVEHDR)lp)->dwBytesRecorded; dwCount++)
			*(bSave + dwLength + dwCount) = *(((PWAVEHDR)lp)->lpData + dwCount);
		dwLength += ((PWAVEHDR)lp)->dwBytesRecorded;
		waveInAddBuffer(hWaveIn, (PWAVEHDR)lp, sizeof(WAVEHDR));
		return 0;
	case MM_WIM_CLOSE:
		waveInUnprepareHeader(hWaveIn, &whdr1, sizeof(WAVEHDR));
		waveInUnprepareHeader(hWaveIn, &whdr2, sizeof(WAVEHDR));
		free(bWave1);
		free(bWave2);

		EnableWindow(GetDlgItem(hWnd, IDB_PLAY), TRUE);
		EnableWindow(GetDlgItem(hWnd, IDB_STR), TRUE);
		EnableWindow(GetDlgItem(hWnd, IDB_REC_END), FALSE);
		return 0;
	case MM_WOM_OPEN:
		EnableWindow(GetDlgItem(hWnd, IDB_PLAY), FALSE);
		EnableWindow(GetDlgItem(hWnd, IDB_STR), FALSE);
		EnableWindow(GetDlgItem(hWnd, IDB_PLY_END), TRUE);
		return 0;
	case MM_WOM_DONE:
		waveOutClose(hWaveOut);
		return 0;
	case MM_WOM_CLOSE:
		waveOutUnprepareHeader(hWaveOut, &whdr1, sizeof(WAVEHDR));
		EnableWindow(GetDlgItem(hWnd, IDB_PLAY), TRUE);
		EnableWindow(GetDlgItem(hWnd, IDB_STR), TRUE);
		EnableWindow(GetDlgItem(hWnd, IDB_PLY_END), FALSE);
		return 0;
	}
	return DefWindowProc(hWnd, msg, wp, lp);
}


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	PSTR lpCmdLine, int nCmdShow) {
	MSG msg;
	WNDCLASS winc;

	winc.style = CS_HREDRAW | CS_VREDRAW;
	winc.lpfnWndProc = WndProc;
	winc.cbClsExtra = 0;
	winc.cbWndExtra = DLGWINDOWEXTRA;
	winc.hInstance = hInstance;
	winc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	winc.hCursor = LoadCursor(NULL, IDC_ARROW);
	winc.hbrBackground = CreateSolidBrush(GetSysColor(COLOR_MENU));
	winc.lpszMenuName = NULL;
	winc.lpszClassName = TITLE;

	if (!RegisterClass(&winc)) return -1;

	hwnd = CreateWindow(TITLE, TEXT("ANALYZE"), WS_VISIBLE | WS_SYSMENU | WS_CAPTION,
		0, 0, WIDTH, HEIGHT, NULL, NULL, hInstance, NULL);

	GetClientRect(hwnd, &rc);
	iClientWidth = rc.right - rc.left;
	iInterval = iClientWidth - 2 * (MARGIN + WIDTH_BT);

	hwnd_btstart = CreateWindow(TEXT("button"), TEXT("start"), WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, MARGIN, MARGIN, WIDTH_BT, HEIGHT_BT, hwnd, (HMENU)IDB_STR, hInstance, NULL);
	hwnd_btend1 = CreateWindow(TEXT("button"), TEXT("end"), WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | WS_DISABLED, MARGIN + WIDTH_BT + iInterval, MARGIN, WIDTH_BT, HEIGHT_BT, hwnd, (HMENU)IDB_REC_END, hInstance, NULL);
	hwnd_btplay = CreateWindow(TEXT("button"), TEXT("play"), WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, MARGIN, MARGIN + HEIGHT_BT + iInterval, WIDTH_BT, HEIGHT_BT, hwnd, (HMENU)IDB_PLAY, hInstance, NULL);
	hwnd_btend2 = CreateWindow(TEXT("button"), TEXT("end"), WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | WS_DISABLED, MARGIN + WIDTH_BT + iInterval, MARGIN + HEIGHT_BT + iInterval, WIDTH_BT, HEIGHT_BT, hwnd, (HMENU)IDB_PLY_END, hInstance, NULL);
	hwnd_lbcount = CreateWindow(TEXT("STATIC"), TEXT("count:0"), WS_CHILD | WS_VISIBLE, MARGIN, MARGIN + 2 * HEIGHT_BT + 2 * iInterval, WIDTH_LB, HEIGHT_LB, hwnd, (HMENU)1, hInstance, NULL);
	hwnd_lbdata = CreateWindow(TEXT("STATIC"), TEXT("data :-"), WS_CHILD | WS_VISIBLE, MARGIN, MARGIN + 2 * HEIGHT_BT + HEIGHT_LB + 3 * iInterval, WIDTH_LB, HEIGHT_LB, hwnd, (HMENU)1, hInstance, NULL);
	hwnd_lbjointtime = CreateWindow(TEXT("STATIC"), TEXT("time :-"), WS_CHILD | WS_VISIBLE, MARGIN, MARGIN + 2 * HEIGHT_BT + 2 * HEIGHT_LB + 4 * iInterval, WIDTH_LB, HEIGHT_LB, hwnd, (HMENU)1, hInstance, NULL);

	if (hwnd == NULL) return -1;

	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return msg.wParam;
}