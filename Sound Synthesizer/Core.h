#pragma once
#define FTYPE double

#include "Noise.h"

namespace synth
{
	//////////////////////////////////////////////////////////////////////////////
	// Utilities

	// Converts frequency (Hz) to angular velocity
	FTYPE w(const FTYPE dHertz)
	{
		return dHertz * 2.0 * PI;
	}

	struct instrument_base;

	// A basic note
	struct note
	{
		int id;		// Position in scale
		FTYPE on;	// Time note was activated
		FTYPE off;	// Time note was deactivated
		bool active;
		instrument_base *channel;

		note()
		{
			id = 0;
			on = 0.0;
			off = 0.0;
			active = false;
			channel = nullptr;
		}

		//bool operator==(const note& n1, const note& n2) { return n1.id == n2.id; }
	};

	//////////////////////////////////////////////////////////////////////////////
	// Oscillator
	enum TYPE {
		OSC_SINE,
		OSC_SQUARE,
		OSC_TRIANGLE,
		OSC_SAW_ANA,
		OSC_SAW_DIG,
		OSC_NOISE,
	};


	FTYPE osc(const FTYPE dTime, const FTYPE dHertz, TYPE t = OSC_SINE,
		const FTYPE dLFOHertz = 0.0, const FTYPE dLFOAmplitude = 0.0, FTYPE dCustom = 50.0)
	{

		FTYPE dFreq = w(dHertz) * dTime + dLFOAmplitude * dHertz * (sin(w(dLFOHertz) * dTime));

		switch (t)
		{
		case OSC_SINE: // Sine wave bewteen -1 and +1
			return sin(dFreq);

		case OSC_SQUARE: // Square wave between -1 and +1
			return sin(dFreq) > 0 ? 1.0 : -1.0;

		case OSC_TRIANGLE: // Triangle wave between -1 and +1
			return asin(sin(dFreq)) * (2.0 / PI);

		case OSC_SAW_ANA: // Saw wave (analogue / warm / slow)
		{
			FTYPE dOutput = 0.0;
			for (FTYPE n = 1.0; n < dCustom; n++)
				dOutput += (sin(n*dFreq)) / n;
			return dOutput * (2.0 / PI);
		}

		case OSC_SAW_DIG:
			return (2.0 / PI) * (dHertz * PI * fmod(dTime, 1.0 / dHertz) - (PI / 2.0));

		case OSC_NOISE:
			return 2.0 * ((FTYPE)rand() / (FTYPE)RAND_MAX) - 1.0;

		default:
			return 0.0;
		}
	}

	//////////////////////////////////////////////////////////////////////////////
	// Scale to Frequency conversion

	const int SCALE_DEFAULT = 0;

	FTYPE scale(const int nNoteID, const int nScaleID = SCALE_DEFAULT)
	{
		switch (nScaleID)
		{
		case SCALE_DEFAULT: default:
			return 8 * pow(1.0594630943592952645618252949463, nNoteID);
		}
	}


	//////////////////////////////////////////////////////////////////////////////
	// Envelopes

	struct envelope
	{
		virtual FTYPE amplitude(const FTYPE dTime, const FTYPE dTimeOn, const FTYPE dTimeOff) = 0;
	};

	struct envelope_adsr : public envelope
	{
		FTYPE dAttackTime;
		FTYPE dDecayTime;
		FTYPE dSustainAmplitude;
		FTYPE dReleaseTime;
		FTYPE dStartAmplitude;

		envelope_adsr()
		{
			dAttackTime = 0.1;
			dDecayTime = 0.1;
			dSustainAmplitude = 1.0;
			dReleaseTime = 0.2;
			dStartAmplitude = 1.0;
		}

		virtual FTYPE amplitude(const FTYPE dTime, const FTYPE dTimeOn, const FTYPE dTimeOff)
		{
			FTYPE dAmplitude = 0.0;
			FTYPE dReleaseAmplitude = 0.0;

			if (dTimeOn > dTimeOff) // Note is on
			{
				FTYPE dLifeTime = dTime - dTimeOn;

				if (dLifeTime <= dAttackTime)
					dAmplitude = (dLifeTime / dAttackTime) * dStartAmplitude;

				if (dLifeTime > dAttackTime && dLifeTime <= (dAttackTime + dDecayTime))
					dAmplitude = ((dLifeTime - dAttackTime) / dDecayTime) * (dSustainAmplitude - dStartAmplitude) + dStartAmplitude;

				if (dLifeTime > (dAttackTime + dDecayTime))
					dAmplitude = dSustainAmplitude;
			}
			else // Note is off
			{
				FTYPE dLifeTime = dTimeOff - dTimeOn;

				if (dLifeTime <= dAttackTime)
					dReleaseAmplitude = (dLifeTime / dAttackTime) * dStartAmplitude;

				if (dLifeTime > dAttackTime && dLifeTime <= (dAttackTime + dDecayTime))
					dReleaseAmplitude = ((dLifeTime - dAttackTime) / dDecayTime) * (dSustainAmplitude - dStartAmplitude) + dStartAmplitude;

				if (dLifeTime > (dAttackTime + dDecayTime))
					dReleaseAmplitude = dSustainAmplitude;

				dAmplitude = ((dTime - dTimeOff) / dReleaseTime) * (0.0 - dReleaseAmplitude) + dReleaseAmplitude;
			}

			// Amplitude should not be negative
			if (dAmplitude <= 0.01)
				dAmplitude = 0.0;

			return dAmplitude;
		}
	};

	FTYPE env(const FTYPE dTime, envelope &env, const FTYPE dTimeOn, const FTYPE dTimeOff)
	{
		return env.amplitude(dTime, dTimeOn, dTimeOff);
	}


	struct instrument_base
	{
		FTYPE dVolume;
		synth::envelope_adsr env;
		FTYPE fMaxLifeTime;
		wstring name;
		virtual FTYPE sound(const FTYPE dTime, synth::note n, bool &bNoteFinished) = 0;
	};

	struct instrument_bell : public instrument_base
	{
		instrument_bell()
		{
			env.dAttackTime = 0.01;
			env.dDecayTime = 1.0;
			env.dSustainAmplitude = 0.0;
			env.dReleaseTime = 1.0;
			fMaxLifeTime = 3.0;
			dVolume = 1.0;
			name = L"Bell";
		}

		virtual FTYPE sound(const FTYPE dTime, synth::note n, bool &bNoteFinished)
		{
			FTYPE dAmplitude = synth::env(dTime, env, n.on, n.off);
			if (dAmplitude <= 0.0) bNoteFinished = true;

			FTYPE dSound =
				+1.00 * synth::osc(dTime - n.on, synth::scale(n.id + 12), synth::OSC_SINE, 5.0, 0.001)
				+ 0.50 * synth::osc(dTime - n.on, synth::scale(n.id + 24))
				+ 0.25 * synth::osc(dTime - n.on, synth::scale(n.id + 36));

			return dAmplitude * dSound * dVolume;
		}

	};

	struct instrument_bell8 : public instrument_base
	{
		instrument_bell8()
		{
			env.dAttackTime = 0.01;
			env.dDecayTime = 0.5;
			env.dSustainAmplitude = 0.8;
			env.dReleaseTime = 1.0;
			fMaxLifeTime = 3.0;
			dVolume = 1.0;
			name = L"8-Bit Bell";
		}

		virtual FTYPE sound(const FTYPE dTime, synth::note n, bool &bNoteFinished)
		{
			FTYPE dAmplitude = synth::env(dTime, env, n.on, n.off);
			if (dAmplitude <= 0.0) bNoteFinished = true;

			FTYPE dSound =
				+1.00 * synth::osc(dTime - n.on, synth::scale(n.id), synth::OSC_SQUARE, 5.0, 0.001)
				+ 0.50 * synth::osc(dTime - n.on, synth::scale(n.id + 12))
				+ 0.25 * synth::osc(dTime - n.on, synth::scale(n.id + 24));

			return dAmplitude * dSound * dVolume;
		}

	};

	struct instrument_harmonica : public instrument_base
	{
		instrument_harmonica()
		{
			env.dAttackTime = 0.00;
			env.dDecayTime = 1.0;
			env.dSustainAmplitude = 0.95;
			env.dReleaseTime = 0.1;
			fMaxLifeTime = -1.0;
			name = L"Harmonica";
			dVolume = 0.3;
		}

		virtual FTYPE sound(const FTYPE dTime, synth::note n, bool &bNoteFinished)
		{
			FTYPE dAmplitude = synth::env(dTime, env, n.on, n.off);
			if (dAmplitude <= 0.0) bNoteFinished = true;

			FTYPE dSound =
				+1.0  * synth::osc(n.on - dTime, synth::scale(n.id - 12), synth::OSC_SAW_ANA, 5.0, 0.001, 100)
				+ 1.00 * synth::osc(dTime - n.on, synth::scale(n.id), synth::OSC_SQUARE, 5.0, 0.001)
				+ 0.50 * synth::osc(dTime - n.on, synth::scale(n.id + 12), synth::OSC_SQUARE)
				+ 0.05  * synth::osc(dTime - n.on, synth::scale(n.id + 24), synth::OSC_NOISE);

			return dAmplitude * dSound * dVolume;
		}

	};


	struct instrument_drumkick : public instrument_base
	{
		instrument_drumkick()
		{
			env.dAttackTime = 0.01;
			env.dDecayTime = 0.15;
			env.dSustainAmplitude = 0.0;
			env.dReleaseTime = 0.0;
			fMaxLifeTime = 1.5;
			name = L"Drum Kick";
			dVolume = 1.0;
		}

		virtual FTYPE sound(const FTYPE dTime, synth::note n, bool &bNoteFinished)
		{
			FTYPE dAmplitude = synth::env(dTime, env, n.on, n.off);
			if (fMaxLifeTime > 0.0 && dTime - n.on >= fMaxLifeTime)	bNoteFinished = true;

			FTYPE dSound =
				+0.99 * synth::osc(dTime - n.on, synth::scale(n.id - 36), synth::OSC_SINE, 1.0, 1.0)
				+ 0.01 * synth::osc(dTime - n.on, 0, synth::OSC_NOISE);

			return dAmplitude * dSound * dVolume;
		}

	};

	struct instrument_drumsnare : public instrument_base
	{
		instrument_drumsnare()
		{
			env.dAttackTime = 0.0;
			env.dDecayTime = 0.2;
			env.dSustainAmplitude = 0.0;
			env.dReleaseTime = 0.0;
			fMaxLifeTime = 1.0;
			name = L"Drum Snare";
			dVolume = 1.0;
		}

		virtual FTYPE sound(const FTYPE dTime, synth::note n, bool &bNoteFinished)
		{
			FTYPE dAmplitude = synth::env(dTime, env, n.on, n.off);
			if (fMaxLifeTime > 0.0 && dTime - n.on >= fMaxLifeTime)	bNoteFinished = true;

			FTYPE dSound =
				+0.5 * synth::osc(dTime - n.on, synth::scale(n.id - 24), synth::OSC_SINE, 0.5, 1.0)
				+ 0.5 * synth::osc(dTime - n.on, 0, synth::OSC_NOISE);

			return dAmplitude * dSound * dVolume;
		}

	};


	struct instrument_drumhihat : public instrument_base
	{
		instrument_drumhihat()
		{
			env.dAttackTime = 0.01;
			env.dDecayTime = 0.05;
			env.dSustainAmplitude = 0.0;
			env.dReleaseTime = 0.0;
			fMaxLifeTime = 1.0;
			name = L"Drum HiHat";
			dVolume = 0.5;
		}

		virtual FTYPE sound(const FTYPE dTime, synth::note n, bool &bNoteFinished)
		{
			FTYPE dAmplitude = synth::env(dTime, env, n.on, n.off);
			if (fMaxLifeTime > 0.0 && dTime - n.on >= fMaxLifeTime)	bNoteFinished = true;

			FTYPE dSound =
				+0.1 * synth::osc(dTime - n.on, synth::scale(n.id - 12), synth::OSC_SQUARE, 1.5, 1)
				+ 0.9 * synth::osc(dTime - n.on, 0, synth::OSC_NOISE);

			return dAmplitude * dSound * dVolume;
		}

	};


	struct sequencer
	{
	public:
		struct channel
		{
			instrument_base* instrument;
			wstring sBeat;
		};

	public:
		sequencer(float tempo = 120.0f, int beats = 4, int subbeats = 4)
		{
			nBeats = beats;
			nSubBeats = subbeats;
			fTempo = tempo;
			fBeatTime = (60.0f / fTempo) / (float)nSubBeats;
			nCurrentBeat = 0;
			nTotalBeats = nSubBeats * nBeats;
			fAccumulate = 0;
		}


		int Update(FTYPE fElapsedTime)
		{
			vecNotes.clear();

			fAccumulate += fElapsedTime;
			while (fAccumulate >= fBeatTime)
			{
				fAccumulate -= fBeatTime;
				nCurrentBeat++;

				if (nCurrentBeat >= nTotalBeats)
					nCurrentBeat = 0;

				int c = 0;
				for (auto v : vecChannel)
				{
					if (v.sBeat[nCurrentBeat] == L'X')
					{
						note n;
						n.channel = vecChannel[c].instrument;
						n.active = true;
						n.id = 64;
						vecNotes.push_back(n);
					}
					c++;
				}
			}



			return vecNotes.size();
		}

		void AddInstrument(instrument_base *inst)
		{
			channel c;
			c.instrument = inst;
			vecChannel.push_back(c);
		}

	public:
		int nBeats;
		int nSubBeats;
		FTYPE fTempo;
		FTYPE fBeatTime;
		FTYPE fAccumulate;
		int nCurrentBeat;
		int nTotalBeats;

	public:
		vector<channel> vecChannel;
		vector<note> vecNotes;


	private:

	};

}