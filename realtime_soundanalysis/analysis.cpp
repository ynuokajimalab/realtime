#include <windows.h>
#include <mmsystem.h>
#include <commctrl.h>
#pragma comment(lib, "ComCtl32.lib ")
#pragma comment (lib, "winmm.lib")
#include "resource.h"
#include "countjoint.h"

//メインウィンドウの設定
#define MAINWINDOW 	TEXT("analyze")
#define WIDTH	400
#define HEIGHT	400

#define SUBWINDOW TEXT("analyze support")
#define WIDTH_SAVEWINDOW 200
#define HEIGHT_SAVEWINDOW 160

//ボタンの設定
#define WIDTH_BT	180
#define HEIGHT_BT	100
#define MARGIN	10
//ラベルの設定
#define WIDTH_LB	200
#define HEIGHT_LB	25
//エディットの設定
#define WIDTH_ED	100
#define HEIGHT_ED	25
//OKボタンの設定
#define WIDTH_OKBT	60
#define HEIGHT_OKBT	20

#define SRATE 	11025
#define TIMEPERBUFFER 0.5

#define SAVEFILENAME TEXT("test")

void saveSound(BYTE* pSound, PWAVEFORMATEX pWaveFormat, DWORD* pDataSize, PTSTR file_name);
double getSD(BYTE* data, DWORD num);

BYTE *bWave1, *bWave2, *bSave, *bTmp;
WAVEFORMATEX wfe;
DWORD dwLength = 0, dwCount, dwTempLength,dwJoint;
HWND hwnd,hwnd2,hwnd3, hwnd_btstart, hwnd_btend1, hwnd_btplay, hwnd_btend2,hwnd_btshow, hwnd_btsave, hwnd_lbcount, hwnd_lbdata, hwnd_lbjointtime,
	hwnd2_lvJoints, hwnd3_lbexplain,hwnd3_edfilename,hwnd3_lbtype,hwnd3_btok;
RECT rc,rc2,rc3;
int iClientWidth, iClientHeight, iClientWidth2,iClientHeight2, iClientWidth3, iClientHeight3,iInterval,iCount;
DOUBLE dbThreshold;
joint* pJoint;
LVCOLUMN col;
LPWSTR strOrgFile,strWaveFile,strJointFile;


LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp) {
	static HWAVEOUT hWaveOut;
	static HWAVEIN hWaveIn;
	static WAVEHDR whdr1, whdr2;
	static BOOL blReset = FALSE,blWaveinOpen = FALSE;
	static TCHAR tCount[8], tData[8], tTime[8];

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
		case IDB_SHOW:
			ShowWindow(hwnd2, TRUE);
			break;
		case IDB_SAVE:
			if (dwLength != 0) {
				ShowWindow(hwnd3, SW_SHOW);
				EnableWindow(hwnd3, TRUE);
			}
			break;
		}
		return 0;
	case MM_WIM_OPEN:
		dwLength = 0;
		dwJoint = 0;
		UpdateJointList(hwnd2_lvJoints,pJoint,0);
		bSave = (BYTE*)realloc(bSave, 1);
		pJoint = (joint*)realloc(pJoint, sizeof(joint));
		initJointData(pJoint);

		EnableWindow(GetDlgItem(hWnd, IDB_PLAY), FALSE);
		EnableWindow(GetDlgItem(hWnd, IDB_STR), FALSE);
		EnableWindow(GetDlgItem(hWnd, IDB_REC_END), TRUE);
		EnableWindow(GetDlgItem(hWnd, IDB_SAVE), FALSE);
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
		dbThreshold =ALPHA* getSD(bSave,dwTempLength/wfe.nBlockAlign);

		//ジョイント音の解析、表示
		if (CheckSound(dwTempLength, (PWAVEHDR)lp, &wfe, pJoint, dwJoint,dbThreshold))
		{
			dwJoint++;
			pJoint = (joint*)realloc(pJoint, (dwJoint + 1) * sizeof(joint));
			wsprintf(tCount, TEXT("%d"), (pJoint + dwJoint - 1)->count);
			wsprintf(tData, TEXT("%d"), (pJoint + dwJoint - 1)->data);
			SetWindowText(hwnd_lbcount, (LPCTSTR)tCount);
			SetWindowText(hwnd_lbdata, (LPCTSTR)tData);
			UpdateJointList(hwnd2_lvJoints,pJoint,dwJoint);
			(pJoint + dwJoint)->count = -1;
		}
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
		EnableWindow(GetDlgItem(hWnd, IDB_SAVE), TRUE);
		return 0;
	case MM_WOM_OPEN:
		EnableWindow(GetDlgItem(hWnd, IDB_PLAY), FALSE);
		EnableWindow(GetDlgItem(hWnd, IDB_STR), FALSE);
		EnableWindow(GetDlgItem(hWnd, IDB_PLY_END), TRUE);
		EnableWindow(GetDlgItem(hWnd, IDB_SAVE), FALSE);
		return 0;
	case MM_WOM_DONE:
		waveOutClose(hWaveOut);
		return 0;
	case MM_WOM_CLOSE:
		waveOutUnprepareHeader(hWaveOut, &whdr1, sizeof(WAVEHDR));
		EnableWindow(GetDlgItem(hWnd, IDB_PLAY), TRUE);
		EnableWindow(GetDlgItem(hWnd, IDB_STR), TRUE);
		EnableWindow(GetDlgItem(hWnd, IDB_PLY_END), FALSE);
		EnableWindow(GetDlgItem(hWnd, IDB_SAVE), TRUE);
		return 0;
	}
	return DefWindowProc(hWnd, msg, wp, lp);
}

LRESULT CALLBACK WndProc2(HWND hSubWnd, UINT msg, WPARAM wp, LPARAM lp) {
	switch (msg){
	case WM_COMMAND:
		switch (LOWORD(wp)){
		case IDB_OK:
			strOrgFile = (LPWSTR)malloc(64*sizeof(TCHAR));
			strWaveFile = (LPWSTR)malloc(64 * sizeof(TCHAR));
			strJointFile = (LPWSTR)malloc(64 * sizeof(TCHAR));
			GetWindowText(hwnd3_edfilename,strOrgFile, 64 * sizeof(TCHAR));
			lstrcpy(strWaveFile,strOrgFile);
			lstrcpy(strJointFile, strOrgFile);
			lstrcat(strWaveFile,TEXT(".wav"));
			lstrcat(strJointFile, TEXT(".txt"));
			saveSound(bSave, &wfe, &dwLength, strWaveFile);
			saveJoint(strJointFile,pJoint,dwJoint);
			EnableWindow(hwnd3, FALSE);
			ShowWindow(hwnd3, SW_HIDE);
			break;
		}
	case WM_SYSCOMMAND:
		if (LOWORD(wp) == SC_CLOSE) {
			ShowWindow(hSubWnd, SW_HIDE);
			return 0;
		}
	default:
		break;
	}
	return DefWindowProc(hSubWnd, msg, wp, lp);
}



int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	PSTR lpCmdLine, int nCmdShow) {
	MSG msg;
	WNDCLASS winc,winc2;

	winc.style = CS_HREDRAW | CS_VREDRAW;
	winc.lpfnWndProc = WndProc;
	winc.cbClsExtra = 0;
	winc.cbWndExtra = DLGWINDOWEXTRA;
	winc.hInstance = hInstance;
	winc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	winc.hCursor = LoadCursor(NULL, IDC_ARROW);
	winc.hbrBackground = CreateSolidBrush(GetSysColor(COLOR_MENU));
	winc.lpszMenuName = NULL;
	winc.lpszClassName = MAINWINDOW;

	winc2 = winc;
	winc2.lpfnWndProc = WndProc2;
	winc2.lpszClassName = SUBWINDOW;

	if (!RegisterClass(&winc)) return -1;
	if (!RegisterClass(&winc2)) return -1;

	hwnd = CreateWindow(MAINWINDOW, TEXT("ANALYZE"), WS_VISIBLE | WS_SYSMENU | WS_CAPTION,
		0, 0, WIDTH, HEIGHT, NULL, NULL, hInstance, NULL);
	hwnd2 = CreateWindow(SUBWINDOW, TEXT("JOINT_LIST"), WS_SYSMENU | WS_CAPTION,
		WIDTH, 0, WIDTH, HEIGHT, NULL, NULL, hInstance, NULL);
	hwnd3 = CreateWindow(SUBWINDOW, TEXT("decide wavefile's name"), WS_SYSMENU | WS_CAPTION | WS_DISABLED,
		WIDTH, HEIGHT, WIDTH_SAVEWINDOW, HEIGHT_SAVEWINDOW, hwnd, NULL, hInstance, NULL);

	GetClientRect(hwnd, &rc);
	iClientWidth = rc.right - rc.left;
	iInterval = iClientWidth - 2 * (MARGIN + WIDTH_BT);
	hwnd_btstart = CreateWindow(TEXT("button"), TEXT("start"), WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, MARGIN, MARGIN, WIDTH_BT, HEIGHT_BT, hwnd, (HMENU)IDB_STR, hInstance, NULL);
	hwnd_btend1 = CreateWindow(TEXT("button"), TEXT("end"), WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | WS_DISABLED, MARGIN + WIDTH_BT + iInterval, MARGIN, WIDTH_BT, HEIGHT_BT, hwnd, (HMENU)IDB_REC_END, hInstance, NULL);
	hwnd_btplay = CreateWindow(TEXT("button"), TEXT("play"), WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, MARGIN, MARGIN + HEIGHT_BT + iInterval, WIDTH_BT, HEIGHT_BT, hwnd, (HMENU)IDB_PLAY, hInstance, NULL);
	hwnd_btend2 = CreateWindow(TEXT("button"), TEXT("end"), WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | WS_DISABLED, MARGIN + WIDTH_BT + iInterval, MARGIN + HEIGHT_BT + iInterval, WIDTH_BT, HEIGHT_BT, hwnd, (HMENU)IDB_PLY_END, hInstance, NULL);
	hwnd_btsave = CreateWindow(TEXT("button"), TEXT("save"), WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | WS_DISABLED, MARGIN, MARGIN + 2 * HEIGHT_BT + 2 * iInterval, WIDTH_BT, HEIGHT_BT, hwnd, (HMENU)IDB_SAVE, hInstance, NULL);
	hwnd_btshow = CreateWindow(TEXT("button"), TEXT("show"), WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, MARGIN + WIDTH_BT + iInterval, MARGIN +2* HEIGHT_BT + 2*iInterval, WIDTH_BT, HEIGHT_BT, hwnd, (HMENU)IDB_SHOW, hInstance, NULL);

	InitCommonControls();
	GetClientRect(hwnd2, &rc2);
	iClientWidth2 = rc2.right - rc2.left;
	iClientHeight2 = rc2.bottom - rc2.top;
	hwnd2_lvJoints = CreateWindowEx(0,WC_LISTVIEW, NULL,WS_CHILD | WS_VISIBLE | LVS_REPORT,
		0, 0, iClientWidth2, iClientHeight2, hwnd2, (HMENU)1,hInstance, NULL);

	InitCommonControls();
	GetClientRect(hwnd3, &rc3);
	iClientWidth3= rc3.right - rc3.left;
	iClientHeight3 = rc3.bottom - rc3.top;
	hwnd3_lbexplain = CreateWindow(TEXT("STATIC"), TEXT("保存するファイル名"), WS_CHILD | WS_VISIBLE | WS_BORDER | SS_CENTER, MARGIN, MARGIN, iClientWidth3 - 2 * MARGIN, HEIGHT_LB, hwnd3, (HMENU)1, hInstance, NULL);
	hwnd3_edfilename = CreateWindow(TEXT("EDIT"), SAVEFILENAME, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_RIGHT,MARGIN, MARGIN+HEIGHT_LB, WIDTH_ED, HEIGHT_ED, hwnd3, (HMENU)1, hInstance, NULL);
	hwnd3_lbtype = CreateWindow(TEXT("STATIC"), TEXT(".wav"), WS_CHILD | WS_VISIBLE , MARGIN+WIDTH_ED, MARGIN + HEIGHT_LB, WIDTH_ED/2, HEIGHT_ED, hwnd3, (HMENU)1, hInstance, NULL);
	hwnd3_btok = CreateWindow(TEXT("button"), TEXT("決定"), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, (iClientWidth3-WIDTH_OKBT)/2, MARGIN + HEIGHT_LB+HEIGHT_ED+MARGIN, WIDTH_OKBT, HEIGHT_OKBT, hwnd3, (HMENU)IDB_OK, hInstance, NULL);

	col.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
	col.fmt = LVCFMT_LEFT;
	col.cx = iClientWidth2/4;
	col.pszText = TEXT("count");
	col.iSubItem = 0;
	ListView_InsertColumn(hwnd2_lvJoints, 0, &col);
	col.iSubItem = 1;
	col.pszText = TEXT("data");
	ListView_InsertColumn(hwnd2_lvJoints, 1, &col);
	col.iSubItem = 2;
	col.pszText = TEXT("jointtime");
	ListView_InsertColumn(hwnd2_lvJoints, 2, &col);
	col.iSubItem = 3;
	col.pszText = TEXT("sleeptime");
	ListView_InsertColumn(hwnd2_lvJoints, 3, &col);

	if (hwnd == NULL || hwnd2 == NULL) return -1;

	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return msg.wParam;
}

double getSD(BYTE* data,DWORD num) {
	DWORD dw;
	double ans = 0;
	for (dw = 0; dw < num;dw++) {
		ans += (data[dw]-128)* (data[dw] - 128);
	}
	ans /= num;
	ans = sqrt(ans);
	return ans;
}

void saveSound(BYTE* pSound, PWAVEFORMATEX pWaveFormat,DWORD* pDataSize, PTSTR file_name)
{
		DWORD dwCount, dwWriteSize;
		const DWORD dwFileSize = *pDataSize + 36,dwFmtSize = 16;
		HANDLE hFile;

		hFile = CreateFile(file_name, GENERIC_WRITE, 0, NULL,
			CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFile == INVALID_HANDLE_VALUE) {
			MessageBox(NULL, TEXT("ファイルが開けません"), NULL, MB_OK);
			return ;
		}

		WriteFile(hFile, "RIFF", 4, &dwWriteSize, NULL);
		WriteFile(hFile, &dwFileSize, 4, &dwWriteSize, NULL);
		WriteFile(hFile, "WAVE", 4, &dwWriteSize, NULL);

		WriteFile(hFile, "fmt ", 4, &dwWriteSize, NULL);
		WriteFile(hFile, &dwFmtSize, 4, &dwWriteSize, NULL);
		WriteFile(hFile, pWaveFormat, 16, &dwWriteSize, NULL);
		WriteFile(hFile, "data", 4, &dwWriteSize, NULL);
		WriteFile(hFile, pDataSize, 4, &dwWriteSize, NULL);
		WriteFile(hFile, pSound, *pDataSize, &dwWriteSize, NULL);

		MessageBox(NULL, TEXT("wavファイルを保存しました"),TEXT("成功"), MB_OK);


		CloseHandle(hFile);

}