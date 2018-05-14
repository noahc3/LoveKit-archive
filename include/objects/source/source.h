#pragma once

class Source
{
	public:
		Source() {};
		Source(const char * path, bool stream);
		~Source();

		void Play();
		void SetLooping(bool loop);
		bool IsPlaying();
		void Stop();
		void Reset();
		void Update();
		bool IsStatic();
		void SetVolume(float volume);

		int GetAudioChannel();
		const char * GetPath();

	private:
		void Decode();
		void FillBuffer(void * audio);

		const char * path;
		ndspWaveBuf waveBuffer[2];

		FILE * fileHandle;
		OggVorbis_File vorbisFile;

		char * data;
		int audiochannel;

		u32 rate;
		u32 channels;
		u32 encoding;
		u32 nsamples;
		u32 size;
		u32 bitrate;

		bool loop;
		bool reset;

		int currentSection;

		float mix[12];
		ndspInterpType interp;
		
		bool stream; //if it should stream
		bool fillBuffer;
		bool playing;
};

extern std::map<int, Source *> streams;