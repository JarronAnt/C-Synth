#include <iostream>
#include "Noise.h"
using namespace std;

//globals
atomic<double> freqOutput = 0.0;
double octaveFreq = 110.0; //A2
double d12thRootOf2 = pow(2.0, 1.0 / 12.0);

double makeNoise(double dtime) {
	//square wave
	double x = 1.0 * sin(freqOutput * 2 * PI * dtime) > 0.0 ? 0.2 : -0.2;
	//sin wave 
	double s = 1.0 * (sin(freqOutput * 2 * PI * dtime) + sin((freqOutput + 20.0) * 2 * PI * dtime));

	double t = 1.0 * (sin(freqOutput * 2 * PI * dtime));
	return t * 0.4;
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
			if (GetAsyncKeyState((unsigned char)("ZSXCFVGBNJMK\xbc\xbe"[k])) & 0x8000){
				if (currKey != k) {
					freqOutput = octaveFreq * pow(d12thRootOf2, k);
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
				currKey = -1;
			}

			freqOutput = 0.0;
		}
	}

	return 0;
}