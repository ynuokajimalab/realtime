#pragma once
#include <windows.h>
#include <math.h>

#define S_RATE 0.06
#define T_RATE 4.0/3.0
#define THRESHOLD 120


typedef struct{
	int count;
	SHORT data;
	double jointtime;
	double sleeptime;
}joint;

int CheckWake(joint* pjoint,int jointnum, DWORD dwDataLength, PWAVEFORMATEX pWaveFormat);
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
	DWORD judge = 0, maxindex, minindex, representive_index;
	double jtime,stime,interval;
	SHORT temp, max, min, representive_value;

		max = -32768;
		min = 32767;
		for (DWORD i = 0; i < pSound->dwBufferLength; i++) {
			temp = (SHORT)(*(pSound->lpData+i));
			if (max < temp) {
				max = temp;
				maxindex = i;
			}
			if (min > temp) {
				min = temp;
				minindex = i;
			}
		}
		if (max > -min) {
			representive_value = max;
			representive_index = maxindex;
		}
		else {
			representive_value = min;
			representive_index = minindex;
		}
		if (fabs(representive_value) > THRESHOLD) {
			judge = 1;
			jtime = gettime(dwSaveLength,pwf);
			if (pjoint == NULL)
			{
				stime = 1.0;
				interval = jtime;
			}else {
				stime = pjoint[jointnum-1].sleeptime;
				interval = jtime - pjoint[jointnum-1].jointtime;
			}
			if (stime > interval*T_RATE ) {
				stime -= interval * S_RATE;
			}
			else{
				stime += interval * S_RATE;
			}
			(pjoint + jointnum)->count = jointnum+1;
			(pjoint + jointnum)->data = representive_value;
			(pjoint + jointnum)->jointtime = jtime;
			(pjoint + jointnum)->sleeptime = stime;
		}
	return judge;
}

double gettime(DWORD dwDataLength, PWAVEFORMATEX pWaveFormat){
	double time;
	time = (double)dwDataLength / pWaveFormat->nSamplesPerSec;
	return time;
}

int CheckWake(joint* pjoint,int jointnum, DWORD dwDataLength, PWAVEFORMATEX pWaveFormat) {
	int judge = 1;
	double jointinterval;
	if (jointnum != 0) {
		jointinterval = gettime(dwDataLength,pWaveFormat) - pjoint[jointnum-1].jointtime;
		if (pjoint[jointnum-1].sleeptime >= jointinterval)
		{
			judge = 0;
		}	
	}
	return judge;
}


void ShowCountJoint(joint* pJoint) {

}