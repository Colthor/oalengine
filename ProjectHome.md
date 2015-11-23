OALEngine is a very simple sound system for playing .wav files in multiple channels, as might be required for a game. It's written in C++, using OpenAL and ALUT.

It should be cross-platform, as long as OpenAL and ALUT are available, although it's currently only been tested on Windows.

It is **not** thread-safe.

The OpenAL SDK and libraries, as well as FreeALUT, can be downloaded from Creative Labs at: http://connect.creativelabs.com/openal/Downloads/Forms/AllItems.aspx

A very simple example of use:
```
#include "OALEngine.h"
#include <stdio.h>

int main(int argc, char *argv[])
{
	OALEngine * engine = new OALEngine(16, true);

	engine->LoadSound("test", "test.wav");
	engine->PlayNewSound("test", false);

	getchar();
	delete engine;

	return 0;
}
```