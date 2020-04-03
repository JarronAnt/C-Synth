#include <iostream>
#include "Noise.h"
using namespace std;

//globals
atomic<double> freqOutput = 0.0;
double octaveFreq = 110.0; //A2
double d12thRootOf2 = pow(2.0, 1.0 / 12.0);

enum type {
	SINE_WAVE,
	SQUARE_WAVE,
	TRIANGLE_WAVE,
	SAW_WAVE1,
	SAW_WAVE2,
	RAND_NOISE
};


double w(double hz) {
	return hz * 2 * PI;
}

double osc(double hz, double dtime, type t) {
	switch (t) {
	case SINE_WAVE:
		return sin(w(hz) * dtime);
		break;
	case SQUARE_WAVE:
		return sin(w(freqOutput) * dtime) > 0.0 ? 1.0 : -1.0;
		break;
	case TRIANGLE_WAVE:
		return  asin(sin(w(hz) * dtime)) *2.0 / PI;
		break;
	case SAW_WAVE1:{
		double out = 0.0;
		for (double n = 1.0; n < 100.0; n++) {
			out += sin(n*w(hz)*dtime) / n;
		}
		return out * (2.0 / PI); 
	}
		break;
	case SAW_WAVE2:
		return (2.0 / PI)*(hz*PI*fmod(dtime, 1.0 / hz) - (PI / 2.0));
		break;
	case RAND_NOISE:
		return 2.0*((double)rand() / (double)RAND_MAX) - 1.0;
		break;
	default:
		return 0.0;
	}
}


struct envelopeADSR {
	double at, dt, sustain, rt,startAmp,triggerOn,triggerOFF;
	bool isNoteOn;

	envelopeADSR() {
		at = 0.500;
		dt = 0.01;
		startAmp = 1.0;
		sustain = 0.8;
		rt = 0.700;
		triggerOn = 0.0;
		triggerOFF = 0.0;
		isNoteOn = false;
	}

	double getAmp(double dtime) {
		double ampl = 0.0;
		double lifeTime = dtime - triggerOn;

		if (isNoteOn) {
			//ADS

			//attack
			if (lifeTime <= at) {
				ampl = (lifeTime / at) * startAmp;
			}

			//decay
			if (lifeTime > at && lifeTime <= (at+dt)) {
				ampl = ((lifeTime - at) / dt) * (sustain-startAmp)+startAmp;

			}

			if (lifeTime > (at + dt)) {
				ampl = sustain;
			}
		}
		else {
			//R
			ampl = ((dtime - triggerOFF) / rt) * (0.0 - sustain) + sustain;
		}
		if (ampl <= 0.0001)
			ampl = 0.0;

		return ampl;
	}

	void noteOn(double timeON) {
		triggerOn = timeON;
		isNoteOn = true;
	}

	void noteOff(double timeOFF) {
		triggerOFF = timeOFF;
		isNoteOn = false;
	}
};


envelopeADSR e;

double makeNoise(double dtime) {
	return e.getAmp(dtime)*osc(freqOutput, dtime, TRIANGLE_WAVE) * 0.4;
}


int main() {

	//get all sound hadware
	vector<wstring> devices = NoiseMaker<short>::Enumerate();
	//show the finidings 
	for (auto d : devices) wcout << "Output Devices:" << d << endl;

	//create the machine
	NoiseMaker<short> sound(devices[0], 44100, 1, 8, 512);

	//send the function to the sound maker 
	sound.SetUserFunction(makeNoise);

	int currKey = -1;
	bool keyPressed = false;
	//main loop & keyboard functions

	while (1) {
		 keyPressed = false;
		//map the keyboard to keys on a piano
		for (int k = 0; k < 16; k++) {
			if (GetAsyncKeyState((unsigned char)("ZSXCFVGBNJMK\xbc\xbe\xbf"[k])) & 0x8000){
				if (currKey != k) {
					freqOutput = octaveFreq * pow(d12thRootOf2, k);
					e.noteOn(sound.GetTime());
					wcout << "\rNote On : " << sound.GetTime() << "s " << freqOutput << "Hz";
					currKey = k;
				}
				keyPressed = true;

			}
		}

		if (!keyPressed) {
			if (currKey != -1)
			{
				wcout << "\rNote Off: " << sound.GetTime() << "s                        ";
				e.noteOff(sound.GetTime());
				currKey = -1;
			}

		}
	}

	return 0;
}