#include <list>
#include <iostream>
#include <algorithm>
#include "Core.h"
using namespace std;

//#include "Noise.h"

vector<synth::note> vecNotes;
mutex muxNotes;
synth::instrument_bell instBell;
synth::instrument_harmonica instHarm;
synth::instrument_drumkick instKick;
synth::instrument_drumsnare instSnare;
synth::instrument_drumhihat instHiHat;

typedef bool(*lambda)(synth::note const& item);
template<class T>
void safe_remove(T &v, lambda f)
{
	auto n = v.begin();
	while (n != v.end())
		if (!f(*n))
			n = v.erase(n);
		else
			++n;
}

// Returns amplitude (-1.0 to +1.0) as a function of time
FTYPE MakeNoise(int nChannel, FTYPE dTime)
{
	unique_lock<mutex> lm(muxNotes);
	FTYPE dMixedOutput = 0.0;

	// Iterate through all active notes, and mix together
	for (auto &n : vecNotes)
	{
		bool bNoteFinished = false;
		FTYPE dSound = 0;

		// Get sample for this note by using the correct instrument and envelope
		if (n.channel != nullptr)
			dSound = n.channel->sound(dTime, n, bNoteFinished);

		// Mix into output
		dMixedOutput += dSound;

		if (bNoteFinished) // Flag note to be removed
			n.active = false;
	}
	safe_remove<vector<synth::note>>(vecNotes, [](synth::note const& item) { return item.active; });
	return dMixedOutput * 0.2;
}

int main()
{

	// Get all sound hardware
	vector<wstring> devices = NoiseMaker<short>::Enumerate();

	// Create sound machine
	NoiseMaker<short> sound(devices[0], 44100, 1, 8, 256);

	// Link noise function with sound machine
	sound.SetUserFunction(MakeNoise);

	//intial clock stuff 
	auto clock_old_time = chrono::high_resolution_clock::now();
	auto clock_real_time = chrono::high_resolution_clock::now();
	double dElapsedTime = 0.0;
	double dWallTime = 0.0;


	//SET THE DRUM STUFF HERE
	//Sequencer
	synth::sequencer seq(90.0);
	seq.AddInstrument(&instKick);
	seq.AddInstrument(&instSnare);
	seq.AddInstrument(&instHiHat);

	seq.vecChannel.at(0).sBeat = L"X...X...X..X.X..";
	seq.vecChannel.at(1).sBeat = L"..X...X...X...X.";
	seq.vecChannel.at(2).sBeat = L"X.X.X.X.X.X.X.XX";

	wcout << "Welcome To My Sound Synthesizer" << endl;
	// Display a keyboard
	wcout << endl <<
		"|   |   |   |   |   | |   |   |   |   | |   | |   |   |   |" << endl <<
		"|   | S |   |   | F | | G |   |   | J | | K | | L |   |   |" << endl <<
		"|   |___|   |   |___| |___|   |   |___| |___| |___|   |   |__" << endl <<
		"|     |     |     |     |     |     |     |     |     |     |" << endl <<
		"|  Z  |  X  |  C  |  V  |  B  |  N  |  M  |  ,  |  .  |  /  |" << endl <<
		"|_____|_____|_____|_____|_____|_____|_____|_____|_____|_____|" << endl << endl;


	while (1)
	{
		// --- SOUND STUFF ---

		// Update Timings =======================================================================================
		clock_real_time = chrono::high_resolution_clock::now();
		auto time_last_loop = clock_real_time - clock_old_time;
		clock_old_time = clock_real_time;
		dElapsedTime = chrono::duration<FTYPE>(time_last_loop).count();
		dWallTime += dElapsedTime;
		FTYPE dTimeNow = sound.GetTime();

		// Sequencer
		int newNotes = seq.Update(dElapsedTime);
		muxNotes.lock();
		for (int a = 0; a < newNotes; a++)
		{
			seq.vecNotes[a].on = dTimeNow;
			vecNotes.emplace_back(seq.vecNotes[a]);
		}
		muxNotes.unlock();

		// Keyboard 
		for (int k = 0; k < 16; k++)
		{
			short nKeyState = GetAsyncKeyState((unsigned char)("ZSXCFVGBNJMK\xbcL\xbe\xbf"[k]));

			// Check if note already exists in currently playing notes
			muxNotes.lock();
			auto noteFound = find_if(vecNotes.begin(), vecNotes.end(), [&k](synth::note const& item) { return item.id == k + 64 && item.channel == &instHarm; });
			if (noteFound == vecNotes.end())
			{
				// Note not found in vector
				if (nKeyState & 0x8000)
				{
					// Key has been pressed so create a new note
					synth::note n;
					n.id = k + 64;
					n.on = dTimeNow;
					n.active = true;
					//set the instrument u want to play here 
					n.channel = &instHarm;

					// Add note to vector
					vecNotes.emplace_back(n);
				}
			}
			else
			{
				// Note exists in vector
				if (nKeyState & 0x8000)
				{
					// Key is still held, so do nothing
					if (noteFound->off > noteFound->on)
					{
						// Key has been pressed again during release phase
						noteFound->on = dTimeNow;
						noteFound->active = true;
					}
				}
				else
				{
					// Key has been released, so switch off
					if (noteFound->off < noteFound->on)
						noteFound->off = dTimeNow;
				}
			}
			muxNotes.unlock();
		}

		
	}


	return 0;
}
