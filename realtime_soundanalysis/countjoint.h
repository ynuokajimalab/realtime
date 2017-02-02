#pragma once
#include <stdio.h>
#include <windows.h>
#include <math.h>

#define ALPHA 4.4
#define BETA 0.04
#define GANMA 2.0/3.0
#define THRESHOLD 10


typedef struct{
	DWORD count;
	BYTE data;
	double jointtime;
	double sleeptime;
}joint;

typedef struct {
	BYTE data;
	DWORD index;
}EightBitData;

void set8BitData(EightBitData* pmindata, EightBitData* pmaxdata, EightBitData* prepresentivedata);
EightBitData bigger8BitData(EightBitData data1, EightBitData data2);
EightBitData smaller8BitData(EightBitData data1, EightBitData data2);
EightBitData representive8BitData(EightBitData data1, EightBitData data2);
DWORD CheckWake(joint* pjoint,int jointnum, DWORD dwDataLength, PWAVEFORMATEX pWaveFormat);
DWORD CheckSoundOnGauss(DWORD dwSaveLength, PWAVEHDR pSound, PWAVEFORMATEX pwf, joint* pcjoint, int jointnum);
DWORD CheckSound(DWORD dwSaveLength, PWAVEHDR pSound, PWAVEFORMATEX pwf, joint* pjoint,int jointnum);
double gettime(DWORD dwSaveLength, PWAVEFORMATEX pWaveFormat);

DWORD CheckSound(DWORD dwSaveLength, PWAVEHDR pSound, PWAVEFORMATEX pwf, joint* pjoint, int jointnum) {
	int judge = 0;
	if(CheckWake(pjoint,jointnum,dwSaveLength,pwf))
		judge = CheckSoundOnGauss(dwSaveLength, pSound, pwf, pjoint,jointnum);

	return judge;
}

DWORD CheckSoundOnGauss(DWORD dwSaveLength, PWAVEHDR pSound, PWAVEFORMATEX pwf, joint* pjoint,int jointnum) {
	DWORD judge = 0,BufferNum;
	double jtime,stime,interval;
	EightBitData tempdata,maxdata, mindata, representivedata;
	BufferNum = (pSound->dwBufferLength) / (pwf->nBlockAlign);

	set8BitData(&mindata,&maxdata,&representivedata);
	
		for (DWORD i = 0; i < BufferNum; i++) {
			tempdata.data = (*(pSound->lpData+i));
			tempdata.index = i;
			maxdata = bigger8BitData(maxdata,tempdata);
			mindata = smaller8BitData(mindata, tempdata);
		}
		representivedata = representive8BitData(maxdata,mindata);
		if ((abs(representivedata.data-128)) > THRESHOLD) {
			judge = 1;
			jtime = gettime(dwSaveLength,pwf);
			if (pjoint->count == 0)
			{
				stime = 1.0;
				interval = jtime;
			}else {
				stime = pjoint[jointnum-1].sleeptime;
				interval = jtime - pjoint[jointnum-1].jointtime;
			}
			if (stime > interval*GANMA ) {
				stime -= interval * BETA;
			}
			else{
				stime += interval * BETA;
			}
			(pjoint + jointnum)->count = jointnum+1;
			(pjoint + jointnum)->data = representivedata.data;
			(pjoint + jointnum)->jointtime = jtime;
			(pjoint + jointnum)->sleeptime = stime;
		}
	return judge;
}

double gettime(DWORD dwDataLength, PWAVEFORMATEX pWaveFormat){
	double time;
	time = (double)dwDataLength / pWaveFormat->nAvgBytesPerSec;
	return time;
}

DWORD CheckWake(joint* pjoint,int jointnum, DWORD dwDataLength, PWAVEFORMATEX pWaveFormat) {
	int judge = 1;
	double jointinterval;
	if (jointnum > 0) {
		jointinterval = gettime(dwDataLength,pWaveFormat) - pjoint[jointnum-1].jointtime;
		if (pjoint[jointnum-1].sleeptime >= jointinterval)
		{
			judge = 0;
		}	
	}
	return judge;
}

void initJointData(joint* pJoint) {
	pJoint->count = 0;
	pJoint->data = 128;
	pJoint->jointtime = 0.0;
	pJoint->sleeptime = 0.0;
}


void set8BitData(EightBitData* pmindata,EightBitData* pmaxdata, EightBitData* prepresentivedata) {
	prepresentivedata->data = 128;
	prepresentivedata->index = 0;
	pmaxdata->data = 0;
	pmaxdata->index = 0;
	pmindata->data = 255;
	pmindata->index = 0;
	pmindata->data = 255;
	pmindata->index = 0;
}

EightBitData bigger8BitData(EightBitData data1, EightBitData data2) {
	EightBitData biggerdata;
	if ((data1.data-128) < (data2.data-128)) {
		biggerdata.data = data2.data;
		biggerdata.index = data2.index;
	}
	else {
		biggerdata.data = data1.data;
		biggerdata.index = data1.index;
	}
	return biggerdata;
}
EightBitData smaller8BitData(EightBitData data1, EightBitData data2) {
	EightBitData smallerdata;
	if ((data1.data-128) > (data2.data-128)) {
		smallerdata.data = data2.data;
		smallerdata.index = data2.index;
	}
	else {
		smallerdata.data = data1.data;
		smallerdata.index = data1.index;
	}
	return smallerdata;
}

EightBitData representive8BitData(EightBitData data1, EightBitData data2) {
	EightBitData representivedata;
	if (abs(data1.data - 128) < abs(data2.data - 128)) {
		representivedata.data = data2.data;
		representivedata.index = data2.index;
	}
	else {
		representivedata.data = data1.data;
		representivedata.index = data1.index;
	}
	return representivedata;
}

void UpdateJointList(HWND hListView, joint* pJoint, DWORD dwCount) {
	if (dwCount != 0) {
		static LVITEM item = { 0 };
		static TCHAR tCount[8], tData[8], tJointTime[16], tSleepTime[16];
		static char cJointTime[16], cSleepTime[16];
		item.mask = LVIF_TEXT;

		wsprintf(tCount, TEXT("%d"), (pJoint + dwCount - 1)->count);
		wsprintf(tData, TEXT("%d"), (pJoint + dwCount - 1)->data);

		sprintf_s(cJointTime,16,"%f", (pJoint + dwCount - 1)->jointtime);
		sprintf_s(cSleepTime, 16, "%f", (pJoint + dwCount - 1)->sleeptime);

		wsprintf(tJointTime, TEXT("%hs"), cJointTime);
		wsprintf(tSleepTime, TEXT("%hs"), cSleepTime);


		item.pszText = (LPWSTR)tCount;
		item.iItem = dwCount - 1;
		item.iSubItem = 0;
		ListView_InsertItem(hListView, &item);

		item.pszText = (LPWSTR)tData;
		item.iSubItem = 1;
		ListView_SetItem(hListView, &item);

		item.pszText = (LPWSTR)tJointTime;
		item.iSubItem = 2;
		ListView_SetItem(hListView, &item);


		item.pszText = (LPWSTR)tSleepTime;
		item.iSubItem = 3;
		ListView_SetItem(hListView, &item);
	}
	else if (dwCount == 0) {
		if(hListView != NULL)
		ListView_DeleteAllItems(hListView);
	}

}
