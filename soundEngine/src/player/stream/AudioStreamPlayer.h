#ifndef URCHINENGINE_AUDIOSTREAMPLAYER_H
#define URCHINENGINE_AUDIOSTREAMPLAYER_H

#include <string>
#include <iostream>
#include <thread>

#include "player/AudioPlayer.h"
#include "player/stream/StreamUpdateWorker.h"
#include "sound/Sound.h"

namespace urchin
{

	class AudioStreamPlayer : public AudioPlayer
	{
		public:
			explicit AudioStreamPlayer(const Sound *);

			static void initializeStreamWorkerThread();
			static void destroyStreamWorkerThread();

			void play();
			void playLoop();
			void pause();
			void stop();

		private:
			void play(bool);

			//stream chunk updater thread
			static StreamUpdateWorker *streamUpdateWorker;
			static std::thread *streamUpdateWorkerThread;
	};

}

#endif
