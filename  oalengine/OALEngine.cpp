/*
	OALEngine.cpp - implementation of a very simple sound engine/system using OpenAL.

	Copyright (c) 2013, Colthor - www.colthor.co.uk
	Google code repository: http://code.google.com/p/oalengine/

	Licenced under the Apache Licence version 2.0
	http://www.apache.org/licenses/LICENSE-2.0

*/


#include <limits.h>
#include "OALEngine.h"


const unsigned int OALEngine::PRIORITY_MAXIMUM = UINT_MAX;
const unsigned int OALEngine::PRIORITY_MINIMUM = 0;

OALEngine::OALEngine(unsigned int RequestedChannelCount, bool InitALUT)
: mInitAlut(InitALUT), mSources(NULL), mChannels(NULL), mChannelCount(0)
{
	unsigned int CreatedChannels = RequestedChannelCount;
	bool SourcesGenerated = false;
	if(mInitAlut)
	{
		alutInit(NULL, NULL);
			
		alDistanceModel(AL_NONE);

		//If the listener is at (0,0,0) then panning sounds with position (x,0,0) gives with x:
		//-ve: hard left
		//  0: centred
		//+ve: hard right
		//with no fading between them. The value (0, 0.5, 0.5) was gleaned by experimentation to give
		//a result closer to what you'd expect (-1 = hard left, +1 = hard right, with smooth 
		//fading between).
		alListener3f(AL_POSITION, 0.0f, -0.5f,-0.5f);
	}

	mSources = new ALuint[RequestedChannelCount];

	alGetError();
	do
	{
		alGenSources(CreatedChannels, mSources);
		if (AL_NO_ERROR == alGetError())
		{
			SourcesGenerated = true;
		}
		else
		{
			CreatedChannels = (CreatedChannels >> 1);
		}
	} while(!SourcesGenerated && CreatedChannels > 0);

	if (SourcesGenerated)
	{
		mChannelCount = CreatedChannels;

		// Fewer sources than requested were created; resize source array to fit.
		if (CreatedChannels < RequestedChannelCount)
		{
			ALuint * tmp = new ALuint[mChannelCount];
			for(unsigned int i = 0; i < CreatedChannels; i++) tmp[i] = mSources[i];
			delete [] mSources;
			mSources = tmp;
		}
		mChannels = new SoundChannel[mChannelCount];
	}
	else
	{
		delete [] mSources;
		mSources = NULL;
		mChannelCount = 0;
	}
}

OALEngine::~OALEngine()
{
	UnloadAllSounds();

	if (mSources)
	{
		alDeleteSources(mChannelCount, mSources);
		delete[] mSources;
		mSources = NULL;
	}

	if (mChannels)
	{
		delete[] mChannels;
		mChannels = NULL;
	}

	if (mInitAlut)
	{
		alutExit();
	}
}

bool OALEngine::ClearAllChannels()
{
	if(!StopAllSounds()) return false;

	alGetError();

	//Unattach buffers from sources and reset them all to defaults.
	for(unsigned int i = 0; i < mChannelCount; i++)
	{
		alSourcei (mSources[i], AL_BUFFER, 0);
		if (AL_NO_ERROR != alGetError())
		{
			return false;
		}
		alSourcei(mSources[i], AL_LOOPING,  AL_FALSE);
		alSource3f(mSources[i], AL_POSITION, 0.0f, 0.0f, 0.0f);
		alSourcef(mSources[i], AL_PITCH, 1.0f);
		alSourcef(mSources[i], AL_GAIN, 1.0f);

		mChannels[i].id = 0;
		mChannels[i].PlayingSound.clear();
		mChannels[i].Priority = 0;
	}
	return true;
}

bool OALEngine::UnloadAllSounds()
{

	if(!ClearAllChannels()) return false;

	//unload all buffers in map
	// it++ will want to be changed to it = mLoadedBuffers.begin() if erase() invalidates iterator.
	// Which indeed it does, hence the change.
	std::map<std::string, ALuint>::iterator it;
	for(it = mLoadedBuffers.begin(); it != mLoadedBuffers.end(); /*it++*/ it = mLoadedBuffers.begin())
	{
		alDeleteBuffers( 1, &( (*it).second ) );
		if (AL_NO_ERROR != alGetError())
		{
			return false;
		}
		mLoadedBuffers.erase(it);
	}
	return true;
}

bool OALEngine::LoadSound(std::string SoundName, std::string Path)
{
	ALuint BufferID;

	BufferID = alutCreateBufferFromFile(Path.c_str());

	if( AL_NO_ERROR == alGetError())
	{
		std::pair<std::map<std::string, ALuint>::iterator, bool> p;
		p = mLoadedBuffers.insert( std::pair<std::string, ALuint>(SoundName, BufferID));
		return p.second;
	}
	else
	{
		return false;
	}
}

/*
	Play Sound
*/

SoundID OALEngine::PlayNewSound(std::string SoundName, bool Repeat, unsigned int Priority, float Panning, float Gain, float PitchMultiplier )
{
	ALuint buffer = 0;

	unsigned int Slot = UINT_MAX;

	unsigned int LowestPriority = PRIORITY_MAXIMUM;
	unsigned int PrioritySlot = UINT_MAX;

	//Find an unused slot, preferably one playing the same sound so we don't have to
	// find + match the buffer.
	for( unsigned int i = 0; i <  mChannelCount; i++)
	{
		if(mChannels[i].id == 0)
		{
			Slot = i;
			i = mChannelCount;
		}
		else
		{
			ALint state;
			alGetSourcei(mSources[i], AL_SOURCE_STATE, &state);

			if(AL_STOPPED == state)
			{
				Slot = i;
				i = mChannelCount;
			}
			else if (mChannels[i].Priority < LowestPriority)
			{
				LowestPriority = mChannels[i].Priority;
				PrioritySlot = i;
			}
		}
	}

	if(UINT_MAX == Slot && UINT_MAX != PrioritySlot && LowestPriority < Priority)
	{ //Displace lowest priority sound if it's lower than the one we want to play.
		Slot = PrioritySlot;
		alSourceStop(mSources[Slot]);
	}

	if(UINT_MAX == Slot) 
	{		
		return 0;
	}

	if(mChannels[Slot].PlayingSound != SoundName) //*/!SameSound)
	{
		//Get buffer containing sound SoundName
		if(!GetBuffer(SoundName, &buffer)) return 0;
		alSourcei(mSources[Slot], AL_BUFFER, buffer);
		mChannels[Slot].PlayingSound = SoundName;
	}
	mChannels[Slot].Priority = Priority;
	mChannels[Slot].id = GetNextSoundID();
	
	alSourcei(mSources[Slot], AL_LOOPING, Repeat ? AL_TRUE : AL_FALSE); //Yes, that would *almost* certainly cast correctly.

	alSource3f(mSources[Slot], AL_POSITION, Panning, 0.0f, 0.0f);
	if(PitchMultiplier < 0.0f) PitchMultiplier = 0.0f;
	alSourcef(mSources[Slot], AL_PITCH, PitchMultiplier);
	if(Gain < 0.0f) Gain = 0.0f;
	alSourcef(mSources[Slot], AL_GAIN, Gain);


	alSourcePlay(mSources[Slot]);
	
	return mChannels[Slot].id;

}

/*
	Individual sound parameter functions
*/


bool OALEngine::SetSoundPan(SoundID id, float Panning)
{
	unsigned int i;
	if(!GetChannelFromSoundID(id, &i)) return false;
	alGetError();
	alSource3f(mSources[i], AL_POSITION, Panning, 0.0f, 0.0f);

	return (AL_NO_ERROR == alGetError());
}

bool OALEngine::SetSoundPitchMultiplier(SoundID id, float PitchMultiplier)
{
	unsigned int i;
	if(!GetChannelFromSoundID(id, &i)) return false;
	alGetError();
	if(PitchMultiplier < 0.0f) PitchMultiplier = 0.0f;
	alSourcef(mSources[i], AL_PITCH, PitchMultiplier);

	return (AL_NO_ERROR == alGetError());
}

bool OALEngine::SetSoundGain(SoundID id, float Gain)
{
	unsigned int i;
	if(!GetChannelFromSoundID(id, &i)) return false;
	alGetError();
	if(Gain < 0.0f) Gain = 0.0f;
	alSourcef(mSources[i], AL_GAIN, Gain);

	return (AL_NO_ERROR == alGetError());
}



bool OALEngine::StopAllSounds()
{
	alSourceStopv(mChannelCount, mSources);

	return (AL_NO_ERROR == alGetError());
}

bool OALEngine::PauseAllSounds()
{
	ALint state;
	for(unsigned int i = 0; i < mChannelCount; i++)
	{
		alGetSourcei(mSources[i], AL_SOURCE_STATE, &state);

		if(AL_PLAYING == state) alSourcePause(mSources[i]);

	}
	return (AL_NO_ERROR == alGetError());
}

bool OALEngine::ResumeAllSounds()
{
	ALint state;
	for(unsigned int i = 0; i < mChannelCount; i++)
	{
		alGetSourcei(mSources[i], AL_SOURCE_STATE, &state);

		if(AL_PAUSED == state) alSourcePlay(mSources[i]);

	}
	return (AL_NO_ERROR == alGetError());
}


bool OALEngine::StopSound(SoundID id)
{
	unsigned int i;
	if(!GetChannelFromSoundID(id, &i)) return false;

	alSourceStop(mSources[i]);
	return (AL_NO_ERROR == alGetError());
}

bool OALEngine::PauseSound(SoundID id)
{
	ALint state;
	unsigned int i;
	if(!GetChannelFromSoundID(id, &i)) return false;
	alGetError();
	alGetSourcei(mSources[i], AL_SOURCE_STATE, &state);

	if(AL_NO_ERROR != alGetError()) return false;

	if(AL_PLAYING == state)
	{
		alSourcePause(mSources[i]);
	}
	else
	{
		return false;
	}
	
	return (AL_NO_ERROR == alGetError());
}

bool OALEngine::ResumeSound(SoundID id)
{
	ALint state;
	unsigned int i;
	if(!GetChannelFromSoundID(id, &i)) return false;
	alGetError();
	alGetSourcei(mSources[i], AL_SOURCE_STATE, &state);

	if(AL_NO_ERROR != alGetError()) return false;

	if(AL_PAUSED == state)
	{
		alSourcePlay(mSources[i]);
	}
	else
	{
		return false;
	}
	
	return (AL_NO_ERROR == alGetError());
}

bool OALEngine::RestartSound(SoundID id)
{
	unsigned int i;
	if(!GetChannelFromSoundID(id, &i)) return false;

	alSourceRewind(mSources[i]);
	if(AL_NO_ERROR != alGetError()) return false;

	alSourcePlay(mSources[i]);
	return (AL_NO_ERROR == alGetError());
}

bool OALEngine::SoundIsActive(SoundID id)
{
	unsigned int i;
	return GetChannelFromSoundID(id, &i);
}

bool OALEngine::GetBuffer(std::string SoundName, ALuint * BufferID)
{
	std::map<std::string, ALuint>::iterator it = mLoadedBuffers.find(SoundName);

	if( mLoadedBuffers.end() == it ) return false;

	*BufferID = (*it).second;
	return true;

}

bool OALEngine::GetChannelFromSoundID(SoundID id, unsigned int * Channel)
{
	for(unsigned int i = 0; i < mChannelCount; i++)
	{
		if(id == mChannels[i].id)
		{
			*Channel = i;
			return true;
		}
	}
	return false;
}

SoundID OALEngine::GetNextSoundID()
{
	static SoundID ID = 0;
	return ++ID;
}