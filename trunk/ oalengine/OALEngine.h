/*
	OALEngine.cpp - definition of a very simple sound engine/system using OpenAL.

	Copyright (c) 2013, Colthor - www.colthor.co.uk
	Google code repository: http://code.google.com/p/oalengine/

	Licenced under the Apache Licence version 2.0
	http://www.apache.org/licenses/LICENSE-2.0

*/

#pragma once

#include <AL/alut.h> 
#include <string>
#include <map>

typedef unsigned int SoundID;


class OALEngine
{
public:
	//Create engine with (up to) RequestedChannelCount -
	//check GetNumberOfChannels() to see how many were created.
	//if InitALUT is true then OpenAL will be initialised and shut down by the creation/destruction
	//of this object. If you create more than one, it's probably a good idea to only
	//have one instance where InitALUT is true, to create that one first
	//and destroy it last.
	OALEngine(unsigned int RequestedChannelCount, bool InitALUT);
	~OALEngine();

	//How many channels were created.
	//On Windows PC with bog-standard Realtek onboard audio
	//you'll probably have up to 256 to play with, but this can vary
	//wildly. Smaller numbers - 16 or 32, say - are probably
	//safer, and will definitely be faster.
	unsigned int GetNumberOfChannels() { return mChannelCount; };

	//Loads a sound from .wav file pointed to by Path and
	//stores it with name SoundName, which should be unique.
	bool LoadSound(std::string SoundName, std::string Path);

	//Unloads all sounds loaded with LoadSound.
	bool UnloadAllSounds();
	//Stops all sounds, resets all streams and clears their states.
	bool ClearAllChannels();

	//Play sound named SoundName. Pitch, gain and pan parameters are as functions below.
	//Higher priorities replace lower priorities;
	//MINIMUM_PRIORITY will be replaced by anything other than MINIMUM_PRIORITY if there're no empty slots
	//MAXIMUM_PRIORITY will NEVER be replaced.
	//Returns the unique ID of the new sound so it can be modified later.
	//(this isn't called PlaySound because that conflicts with something in windows.h and causes a link error.)
	SoundID PlayNewSound(std::string SoundName, bool Repeat, unsigned int Priority = 1024, float Panning = 0.0f, float Gain = 1.0f, float PitchMultiplier = 1.0f);

	//Stops all playing sounds, making them fair game for PlaySound()
	bool StopAllSounds();
	//Pauses all Playing sounds, ignoring stopped and paused.
	bool PauseAllSounds();
	//Resumes all Paused sounds, ignoring stopped and playing.
	bool ResumeAllSounds();

	// As *AllSounds, but for only the specified sound id.
	// Returning false means either it failed, or the sound isn't
	// playing any more.
	bool StopSound(SoundID id);
	bool PauseSound(SoundID id);
	bool ResumeSound(SoundID id);
	// restarts a sound from the beginning, regardless of state
	bool RestartSound(SoundID id);
	// Returns true if sound 'id' is currently assigned to a channel, false otherwise.
	bool SoundIsActive(SoundID id);
	
	//Sets sound panning between -1 (hard left), 0 (centre) and 1 (hard right)
	//with smooth (but more or less arbitrarily chosen) fading inbetween.
	//Numbers outside this range might have odd effects, or none; by default
	//attenuation is disabled.
	bool SetSoundPan(SoundID id, float Panning);
	//Sets a sound's pitch multiplier; -ve multipliers are set to 0.
	bool SetSoundPitchMultiplier(SoundID id, float PitchMultiplier);
	//Sets a sound's gain; negative numbers are set to 0.
	bool SetSoundGain(SoundID id, float Gain);

	//Helper consts for min/max priority (probably 0 and UINT_MAX, respectively)
	static const unsigned int PRIORITY_MINIMUM;
	static const unsigned int PRIORITY_MAXIMUM;
private:

	unsigned int mChannelCount;
	bool mInitAlut;

	//Array of OpenAL sources - some OpenAL functions can
	//operate on arrays which is why this isn't a member of SoundChannel.
	ALuint * mSources;

	//Channel info struct and array pointer
	class SoundChannel
	{
	public:
		SoundChannel() : id(0), PlayingSound(""), Priority(0) {};
		SoundID id;
		std::string PlayingSound;
		unsigned int Priority;
	} * mChannels;

	//map for storing (Sound Name, OpenAL Buffer ID) pairs.
	std::map<std::string, ALuint> mLoadedBuffers;

	// Gets buffer ID of sound SoundName
	bool GetBuffer(std::string SoundName, ALuint * BufferID);
	//Returns a unique ID for a playing sound.
	SoundID GetNextSoundID();
	// Gets the channel of sound 'id', returning false if it's not found.
	bool GetChannelFromSoundID(SoundID id, unsigned int * Channel);
};