# Using OALEngine #

**Sample:**
this code will play test.wav, and exit once return is pressed.
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

**Explanation**

`OALEngine(16, true)`: The constructor parameters request 16 channels (ie. concurrently playing sounds - "sources" in OpenAL-speak), and that the OALEngine object initialises and shuts down OpenAL when it is created and destroyed.

The number of channels supported is OpenAL implementation dependent; I get 256 using my laptop's onboard SigmaTel audio, but unfortunately the only way I know of to find out is to try to create them and see if it works. If this fails, OALEngine will automatically try again with half as many channels, and so on until it succeeds or hits 1 and gives up. A smaller number of channels will be more likely to be granted, and will also be faster.

If you have multiple instances of OALEngine it's a good idea to have the first one created initialise OpenAL, and make sure that's the last one destroyed too. Pass false to other instances so they don't re-initialise/shutdown OpenAL.

`LoadSound("test", "test.wav")`: This loads the sound test.wav and stores it with the name "test".

`PlayNewSound("test", false)`: This plays the sound named "test" if there is an available channel, without repeating it. This member function has several optional parameters, and returns a SoundID so a playing sound's parameters can be changed later.

Most member functions return a boolean value; true if they worked, and false if they didn't. I appreciate this isn't the most helpful, but simple, remember?

For more information see OALEngine.h which contains explanatory comments for each member.

# Multiple Instances #

Multiple instances are perfectly fine. They do share the same pool of OpenAL sources (so if you can have a maximum of 256 channels you could create two OALEngines with 128 channels each, but not two using all 256), but each OALEngine has its own, completely separate set of channels and loaded sounds.

Why use more than one? If you have two unrelated systems it'll be faster if they each have their own OALEngine than if they share one with twice as many channels. As the number of loaded sounds and the number of channels increases, performance will decrease; specifically, loaded sounds are stored in a std::map, so with an increasing number time to add and acces sounds will increase as with any other std::map. Everything else is very simple, stored in linear arrays, so should be O(N) where N is the number of channels unless I'm doing anything particularly stupid.