#ifndef LIBSTD_H
#define LIBSTD_H

#ifdef LIBSTD_IMPLEMENTATION
#  define AUDIO_IMPLEMENTATION
#  define DECODER_IMPLEMENTATION
#  define IO_IMPLEMENTATION
#  define MF_DECODER_IMPLEMENTATION
#  define TYPES_IMPLEMENTATION
#  define WAV_IMPLEMENTATION
#  define WINDOW_IMPLEMENTATION
#  define STB_IMAGE_IMPLEMENTATION
#endif // LIBSTD_IMPLEMENTATION

#ifdef AUDIO_DISABLE
#  define AUDIO_H
#endif // AUDIO_DISABLE

#ifdef DECODER_DISABLE
#  define DECODER_H
#endif // DECODER_DISABLE

#ifdef IO_DISABLE
#  define IO_H
#endif // IO_DISABLE

#ifdef MF_DECODER_DISABLE
#  define MF_DECODER_H
#endif // MF_DECODER_DISABLE

#ifdef TYPES_DISABLE
#  define TYPES_H
#endif // TYPES_DISABLE

#ifdef WAV_DISABLE
#  define WAV_H
#endif // WAV_DISABLE

#ifdef WINDOW_DISABLE
#  define WINDOW_H
#endif // WINDOW_DISABLE

#ifdef STB_IMAGE_DISABLE
#  define STB_IMAGE_H
#endif // STB_IMAGE_DISABLE

#ifdef LIBSTD_DEF
#  define AUDIO_DEF LIBSTD_DEF
#  define DECODER_DEF LIBSTD_DEF
#  define IO_DEF LIBSTD_DEF
#  define MF_DECODER_DEF LIBSTD_DEF
#  define TYPES_DEF LIBSTD_DEF
#  define WAV_DEF LIBSTD_DEF
#  define WINDOW_DEF LIBSTD_DEF
#  define STB_IMAGE_DEF LIBSTD_DEF
#endif // LIBSTD_DEF

#ifndef AUDIO_H
#define AUDIO_H

// win32
//   mingw: -lole32 -lxaudio2_8
//   msvc : ole32.lib

// linux
//   gcc  : -lasound

#include <stdbool.h>
#ifdef _WIN32
#  include <xaudio2.h>
#elif linux
#  include <alsa/asoundlib.h>
#endif //_WIN32

#ifndef AUDIO_DEF
#  define AUDIO_DEF static inline
#endif //AUDIO_DEF

typedef enum{
    AUDIO_FMT_S16,
    AUDIO_FMT_FLT,
}Audio_Fmt;

typedef struct{
    int channels;
    int sample_rate;
    int sample_size;

#ifdef _WIN32
    IXAudio2SourceVoice *xaudio2_source_voice;
    HANDLE semaphore;
#elif linux
    snd_pcm_t *alsa_snd_pcm;
#endif
}Audio;

// Public
AUDIO_DEF bool audio_init(Audio *audio, Audio_Fmt fmt, int channels, int sample_rate);
AUDIO_DEF void audio_play(Audio *audio, unsigned char *data, int samples);
//AUDIO_DEF void audio_play_async(Audio *audio, unsigned char *data, int samples);
AUDIO_DEF void audio_block(Audio *audio);
AUDIO_DEF void audio_free(Audio *audio);

AUDIO_DEF bool audio_fmt_bits_per_sample(int *bits, Audio_Fmt fmt);

// Private

#ifdef AUDIO_IMPLEMENTATION

#ifdef _WIN32

static bool audio_co_initialized = false;
static IXAudio2* audio_xaudio2 = NULL;
static IXAudio2MasteringVoice *audio_xaudio2_mastering_voice = NULL;

AUDIO_DEF void audio_xaudio2_OnBufferEnd(IXAudio2VoiceCallback* This, void* pBufferContext);
AUDIO_DEF void audio_xaudio2_OnStreamEnd(IXAudio2VoiceCallback* This);
AUDIO_DEF void audio_xaudio2_OnVoiceProcessingPassEnd(IXAudio2VoiceCallback* This);
AUDIO_DEF void audio_xaudio2_OnVoiceProcessingPassStart(IXAudio2VoiceCallback* This, UINT32 SamplesRequired);
AUDIO_DEF void audio_xaudio2_OnBufferStart(IXAudio2VoiceCallback* This, void* pBufferContext);
AUDIO_DEF void audio_xaudio2_OnLoopEnd(IXAudio2VoiceCallback* This, void* pBufferContext);
AUDIO_DEF void audio_xaudio2_OnVoiceError(IXAudio2VoiceCallback* This, void* pBufferContext, HRESULT Error);

static IXAudio2VoiceCallback audio_xaudio2_callbacks = {
    .lpVtbl = &(IXAudio2VoiceCallbackVtbl) {
	.OnStreamEnd = audio_xaudio2_OnStreamEnd,
	.OnVoiceProcessingPassEnd = audio_xaudio2_OnVoiceProcessingPassEnd,
	.OnVoiceProcessingPassStart = audio_xaudio2_OnVoiceProcessingPassStart,
	.OnBufferEnd = audio_xaudio2_OnBufferEnd,
	.OnBufferStart = audio_xaudio2_OnBufferStart,
	.OnLoopEnd = audio_xaudio2_OnLoopEnd,
	.OnVoiceError = audio_xaudio2_OnVoiceError,
    }
};

AUDIO_DEF bool audio_fmt_format_tag(int *tag, Audio_Fmt fmt) {
    switch(fmt) {
    case AUDIO_FMT_S16: {
	*tag = WAVE_FORMAT_PCM;
	return true;
    } break;
    case AUDIO_FMT_FLT: {
	*tag = WAVE_FORMAT_IEEE_FLOAT;
	return true;
    } break;
    default: {
	return false;	
    } break;
    }
}

AUDIO_DEF bool audio_init(Audio *audio, Audio_Fmt fmt, int channels, int sample_rate) {    
    audio->sample_rate = sample_rate;
    audio->channels    = channels;

    int bits_per_sample;
    if(!audio_fmt_bits_per_sample(&bits_per_sample, fmt)) {
	return false;
    }
    int format_tag;
    if(!audio_fmt_format_tag(&format_tag, fmt)) {
	return false;
    }
    audio->sample_size = bits_per_sample * channels / 8;

    if( !audio_co_initialized ) {
	if( FAILED(CoInitializeEx(NULL, COINIT_MULTITHREADED)) ) {
	    return false;
	}
	audio_co_initialized = true;
    } 

    if( !audio_xaudio2 &&
	FAILED(XAudio2Create(&audio_xaudio2, 0, XAUDIO2_DEFAULT_PROCESSOR)) ) {
	return false;
    }

    if(!audio_xaudio2_mastering_voice &&
       FAILED(audio_xaudio2->lpVtbl->
	      CreateMasteringVoice(audio_xaudio2,
				   &audio_xaudio2_mastering_voice,
				   channels,
				   sample_rate,
				   0,
				   0,
				   NULL,
				   AudioCategory_GameEffects)) ) {
	return false;
    }

    WAVEFORMATEX wave_format;
    wave_format.wFormatTag = (WORD) format_tag;
    wave_format.nChannels = (WORD) channels;
    wave_format.nSamplesPerSec = sample_rate;
    wave_format.wBitsPerSample = (WORD) bits_per_sample;
    wave_format.nBlockAlign = (wave_format.nChannels * wave_format.wBitsPerSample) / 8;
    wave_format.nAvgBytesPerSec = wave_format.nSamplesPerSec * wave_format.nBlockAlign;
    wave_format.cbSize = 0;
    audio->xaudio2_source_voice = NULL;
    if( FAILED(audio_xaudio2->lpVtbl->
	       CreateSourceVoice(audio_xaudio2,
				 &audio->xaudio2_source_voice,
				 &wave_format,
				 0,
				 1.f,
				 &audio_xaudio2_callbacks,
				 NULL,
				 NULL)) ) {
	return false;
    }

    audio->xaudio2_source_voice->lpVtbl->Start(audio->xaudio2_source_voice, 0, XAUDIO2_COMMIT_NOW);    
    audio->semaphore = CreateSemaphore(NULL, 0, 1, NULL);
    ReleaseSemaphore(audio->semaphore, 1, NULL);
    
    if(GetLastError() == ERROR_ALREADY_EXISTS) {
	return false;
    }
    
    return true;
}

AUDIO_DEF void audio_play(Audio *audio, unsigned char *data, int samples) {
    XAUDIO2_BUFFER xaudioBuffer = {0};
  
    xaudioBuffer.AudioBytes = samples * audio->sample_size;
    xaudioBuffer.pAudioData = data;
    xaudioBuffer.pContext = audio;
    
    audio->xaudio2_source_voice->lpVtbl->SubmitSourceBuffer(audio->xaudio2_source_voice, &xaudioBuffer, NULL);

    WaitForSingleObject((audio)->semaphore, INFINITE);
}

AUDIO_DEF void audio_play_async(Audio *audio, unsigned char *data, int samples) {
    XAUDIO2_BUFFER xaudioBuffer = {0};
  
    xaudioBuffer.AudioBytes = samples * audio->sample_size;
    xaudioBuffer.pAudioData = data;
    xaudioBuffer.pContext = audio;
    
    audio->xaudio2_source_voice->lpVtbl->SubmitSourceBuffer(audio->xaudio2_source_voice, &xaudioBuffer, NULL);
}

AUDIO_DEF void audio_block(Audio *audio) {
    while(1) {
	Sleep(10);
	DWORD result = WaitForSingleObject(audio->semaphore, 0);
	if(result == WAIT_OBJECT_0) {
	    return;
	}
    }
}

AUDIO_DEF void audio_free(Audio *audio) {
    audio->xaudio2_source_voice->lpVtbl->Stop(audio->xaudio2_source_voice, 0, 0);
    audio->xaudio2_source_voice->lpVtbl->DestroyVoice(audio->xaudio2_source_voice);
    CloseHandle(audio->semaphore);
}

AUDIO_DEF void audio_xaudio2_OnBufferEnd(IXAudio2VoiceCallback* This, void* pBufferContext) {
    (void) This;
    Audio *audio = (Audio *) pBufferContext;
    ReleaseSemaphore(audio->semaphore, 1, NULL);
}
AUDIO_DEF void audio_xaudio2_OnBufferStart(IXAudio2VoiceCallback* This, void* pBufferContext) { (void) This; (void) pBufferContext; }
AUDIO_DEF void audio_xaudio2_OnStreamEnd(IXAudio2VoiceCallback* This) { (void) This; }
AUDIO_DEF void audio_xaudio2_OnVoiceProcessingPassEnd(IXAudio2VoiceCallback* This) { (void) This; }
AUDIO_DEF void audio_xaudio2_OnVoiceProcessingPassStart(IXAudio2VoiceCallback* This, UINT32 SamplesRequired) { (void) This; (void) SamplesRequired; }
AUDIO_DEF void audio_xaudio2_OnLoopEnd(IXAudio2VoiceCallback* This, void* pBufferContext) { (void) This; (void) pBufferContext; }
AUDIO_DEF void audio_xaudio2_OnVoiceError(IXAudio2VoiceCallback* This, void* pBufferContext, HRESULT Error) { (void) This; (void) pBufferContext, (void) Error; }

#elif linux

AUDIO_DEF bool audio_fmt_pcm_format(snd_pcm_format_t *format, Audio_Fmt fmt) {
  switch(fmt) {
  case AUDIO_FMT_S16: {
    *format = SND_PCM_FORMAT_S16_LE;
    return true;
  } break;
  default: {
    return false;
  } break;
  }
}

AUDIO_DEF bool audio_init(Audio *audio, Audio_Fmt fmt, int channels, int sample_rate) {
  if(snd_pcm_open(&audio->alsa_snd_pcm, "default", SND_PCM_STREAM_PLAYBACK, 0) < 0) {
    return false;
  }

  int bits_per_sample;
  if(!audio_fmt_bits_per_sample(&bits_per_sample, fmt)) {
    return false;
  }
  audio->sample_size = bits_per_sample * channels / 8;

  snd_pcm_format_t snd_pcm_format;
  if(!audio_fmt_pcm_format(&snd_pcm_format, fmt)) {
    return false;
  }
  
  if(snd_pcm_set_params(audio->alsa_snd_pcm,
		        snd_pcm_format,
			SND_PCM_ACCESS_RW_INTERLEAVED,
			channels,
			sample_rate,
			0,
			sample_rate / 4) < 0) {
    return false;
  }
  snd_pcm_uframes_t buffer_size = 0;
  snd_pcm_uframes_t period_size = 0;
  if(snd_pcm_get_params(audio->alsa_snd_pcm, &buffer_size, &period_size) < 0) {
    return false;
  }
  snd_pcm_prepare(audio->alsa_snd_pcm);

  return true;  
}

AUDIO_DEF void audio_play(Audio *audio, unsigned char *data, int samples) {

  while(samples > 0) {
    int ret = snd_pcm_writei(audio->alsa_snd_pcm, data, samples);
    if(ret <= 0) {
      snd_pcm_recover(audio->alsa_snd_pcm, ret, 1);
      //snd_pcm_prepare(audio->device);
    } else {
      samples -= ret;
    }
  }

}

AUDIO_DEF void audio_free(Audio *audio) {
  snd_pcm_drain(audio->alsa_snd_pcm);
  snd_pcm_close(audio->alsa_snd_pcm);
  audio->alsa_snd_pcm = NULL;
}

AUDIO_DEF void audio_block(Audio *audio) {

  (void) audio;
  // alsa by default blocks the execution, unlike xaudio2

}

#endif //_WIN32

AUDIO_DEF bool audio_fmt_bits_per_sample(int *bits, Audio_Fmt fmt) {
    switch(fmt) {
    case AUDIO_FMT_S16: {
	*bits = 16;
	return true;
    } break;
    case AUDIO_FMT_FLT: {
	*bits = 32;
	return true;
    } break;
    default: {
	return false;	
    } break;
    }
}

#endif //AUDIO_IMPLEMENTATION

#endif //AUDIO_H
#ifndef DECODER_H
#define DECODER_H

// TODO: implement seeking / seeking-api
// TODO: Maybe add decoder_url_read / decoder_url_seek

// win32
//   mingw: -lavformat -lavcodec -lavutil -lswresample
//   msvc : avformat.lib avcodec.lib avutil.lib swresample.lib

// linux
//   gcc  : -lavformat -lavcodec -lavutil -lswresample

#include <stdbool.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>

#ifndef DECODER_DEF
#  define DECODER_DEF static inline
#endif //DECODER_DEF

typedef enum {
  DECODER_FMT_NONE = 0,
  DECODER_FMT_U8,
  DECODER_FMT_S16,
  DECODER_FMT_S32,
  DECODER_FMT_FLT,
  DECODER_FMT_DBL,
  DECODER_FMT_U8P,
  DECODER_FMT_S16P,
  DECODER_FMT_S32P,
  DECODER_FMT_FLTP,
  DECODER_FMT_DBLP,
}Decoder_Fmt;

typedef struct Decoder Decoder;

typedef int (*Decoder_Read)(void *opaque, uint8_t* buffer, int buffer_size);
typedef int64_t (*Decoder_Seek)(void *opaque, int64_t offset, int whence);

typedef struct{
  const unsigned char *data;
  uint64_t size;
  uint64_t pos;
}Decoder_Memory;

struct Decoder{
  AVIOContext *av_io_context;    
  AVFormatContext *av_format_context;
  AVCodecContext *av_codec_context;
  SwrContext *swr_context;
  int stream_index;

  AVPacket *packet;
  AVFrame *frame;
  int64_t pts;

  float volume;
  float target_volume;

  int samples;
  int sample_size;

  bool continue_receive;
  bool continue_convert;
};

// Public
DECODER_DEF bool decoder_slurp(Decoder_Read read,
			       Decoder_Seek seek,
			       void *opaque,
			       Decoder_Fmt fmt,
			       float volume,
			       int *channels,
			       int *sample_rate,
			       unsigned char **samples,
			       unsigned int *samples_count);

DECODER_DEF bool decoder_slurp_file(const char *filepath,
				    Decoder_Fmt fmt,
				    float volume,
				    int *channels,
				    int *sample_rate,
				    unsigned char **samples,
				    unsigned int *samples_count);

DECODER_DEF bool decoder_slurp_memory(const char *memory,
				      size_t memory_len,
				      Decoder_Fmt fmt,
				      float volume,
				      int *channels,
				      int *sample_rate,
				      unsigned char **samples,
				      unsigned int *samples_count);

DECODER_DEF bool decoder_init(Decoder *decoder,
			      Decoder_Read read,
			      Decoder_Seek seek,
			      void *opaque,
			      Decoder_Fmt fmt,
			      float volume,
			      int samples,
			      int *channels,
			      int *sample_rate);
DECODER_DEF bool decoder_decode(Decoder *decoder, int *out_samples, unsigned char *out_buf);
DECODER_DEF void decoder_free(Decoder *decoder);
DECODER_DEF bool decoder_fmt_to_bits_per_sample(int *bits, Decoder_Fmt fmt);
DECODER_DEF bool decoder_fmt_to_libav_fmt(enum AVSampleFormat *av_fmt, Decoder_Fmt fmt);

// Protected
DECODER_DEF int64_t decoder_file_seek(void *opaque, int64_t offset, int whence);
DECODER_DEF int decoder_file_read(void *opaque, uint8_t *buf, int _buf_size);
  
DECODER_DEF int64_t decoder_memory_seek(void *opaque, int64_t offset, int whence);
DECODER_DEF int decoder_memory_read(void *opaque, uint8_t *buf, int _buf_size);

#ifdef DECODER_IMPLEMENTATION

DECODER_DEF bool decoder_slurp_memory(const char *memory,
				      size_t memory_len,
				      Decoder_Fmt fmt,
				      float volume,
				      int *channels,
				      int *sample_rate,
				      unsigned char **out_samples,
				      unsigned int *out_samples_count) {
  Decoder_Memory mem = {
    .data = (const unsigned char *) memory,
    .pos = 0,
    .size = memory_len,
  };

  return decoder_slurp(decoder_memory_read,
		       decoder_memory_seek,
		       &mem,
		       fmt,
		       volume,
		       channels,
		       sample_rate,
		       out_samples,
		       out_samples_count);

}

DECODER_DEF bool decoder_slurp_file(const char *filepath,
				    Decoder_Fmt fmt,
				    float volume,
				    int *channels,
				    int *sample_rate,
				    unsigned char **out_samples,
				    unsigned int *out_samples_count) {
  FILE *f = fopen(filepath, "rb");
  if(!f) {
    return false;
  }
  
  if(!decoder_slurp(decoder_file_read,
		    decoder_file_seek,
		    f,
		    fmt,
		    volume,
		    channels,
		    sample_rate,
		    out_samples,
		    out_samples_count)) {
    fclose(f);
    return false;
  }

  fclose(f);
  return true;
}

DECODER_DEF bool decoder_slurp(Decoder_Read read,
			       Decoder_Seek seek,
			       void *opaque,
			       Decoder_Fmt fmt,
			       float volume,
			       int *channels,
			       int *sample_rate,
			       unsigned char **out_samples,
			       unsigned int *out_samples_count) {
  Decoder decoder;
  if(!decoder_init(&decoder, read, seek, opaque,
		   fmt, 1152, volume, channels, sample_rate)) {
    return false;
  }

  unsigned int samples_count = 0;
  unsigned int samples_cap = 5 * (*sample_rate) * (*channels);
  unsigned char *samples = malloc(samples_cap * decoder.sample_size);
  if(!samples) {
    decoder_free(&decoder);
    return false;
  }

  unsigned char decoded_samples[1152 * 4];
  int decoded_samples_count;
  while(decoder_decode(&decoder, &decoded_samples_count, decoded_samples)) {

    unsigned int new_samples_cap = samples_cap;
    while(samples_count + decoded_samples_count > new_samples_cap) {
      new_samples_cap *= 2;
    }
    if(new_samples_cap != samples_cap) {
      samples_cap = new_samples_cap;
      samples = realloc(samples, samples_cap * decoder.sample_size);
      if(!samples) {
	decoder_free(&decoder);
	return false;
      }
    }
    
    memcpy(samples + samples_count * decoder.sample_size,
	   decoded_samples,
	   decoded_samples_count * decoder.sample_size);

    samples_count += (unsigned int) decoded_samples_count;
  }

  *out_samples = samples;
  *out_samples_count = samples_count;

  decoder_free(&decoder);  
  return true;
}

DECODER_DEF bool decoder_init(Decoder *decoder,
			      Decoder_Read read,
			      Decoder_Seek seek,
			      void *opaque,
			      Decoder_Fmt fmt,
			      float volume,
			      int samples,
			      int *channels,
			      int *sample_rate) {

  decoder->av_io_context = NULL;
  decoder->av_format_context = NULL;
  decoder->av_codec_context = NULL;
  decoder->swr_context = NULL;
  decoder->packet = NULL;
  decoder->frame = NULL;
  decoder->target_volume = -1.f;
  decoder->volume = volume;

  decoder->samples = samples;
  enum AVSampleFormat av_sample_format;
  if(!decoder_fmt_to_libav_fmt(&av_sample_format, fmt)) {
      return false;
  }

  decoder->av_io_context = avio_alloc_context(NULL, 0, 0, opaque, read, NULL, seek);
  if(!decoder->av_io_context) {
    decoder_free(decoder);
    return false;
  }

  decoder->av_format_context = avformat_alloc_context();
  if(!decoder->av_format_context) {
    decoder_free(decoder);
    return false;
  }

  decoder->av_format_context->pb = decoder->av_io_context;
  decoder->av_format_context->flags = AVFMT_FLAG_CUSTOM_IO;
  if (avformat_open_input(&decoder->av_format_context, "", NULL, NULL) != 0) {
    decoder_free(decoder);
    return false;
  }

  if(avformat_find_stream_info(decoder->av_format_context, NULL) < 0) {
    decoder_free(decoder);
    return false;
  }

  decoder->stream_index = -1;

  const AVCodec *av_codec = NULL;
  AVCodecParameters *av_codec_parameters = NULL;
  for(size_t i=0;i<decoder->av_format_context->nb_streams;i++) {
    av_codec_parameters = decoder->av_format_context->streams[i]->codecpar;
    if(av_codec_parameters->codec_type == AVMEDIA_TYPE_AUDIO) {
      decoder->stream_index = (int) i;
      av_codec = avcodec_find_decoder(av_codec_parameters->codec_id);
      if(!av_codec) {
	decoder_free(decoder);
	return false;
      }
      break;
    }
  }
  if(av_codec == NULL) {
    decoder_free(decoder);
    return false;
  }
  
  decoder->av_codec_context = avcodec_alloc_context3(av_codec);
  if(!decoder) {
    decoder_free(decoder);
    return false;
  }

  if(avcodec_parameters_to_context(decoder->av_codec_context, av_codec_parameters) < 0) {
    decoder_free(decoder);
    return false;
  }

  *sample_rate = (int) decoder->av_codec_context->sample_rate;

  if(avcodec_open2(decoder->av_codec_context, av_codec, NULL) < 0) {
    decoder_free(decoder);
    return false;
  }
  
  decoder->swr_context = swr_alloc();
  if(!decoder->swr_context) {
    decoder_free(decoder);
    return false;
  }
  
  char chLayoutDescription[128];

  // I have received different compile errors/warnings, due to 
  // different libav-versions. Use one of the following implementations:

  /*
  // BEGIN FIRST
  int sts = av_channel_layout_describe(&av_codec_parameters->ch_layout, chLayoutDescription, sizeof(chLayoutDescription));
  if(sts < 0) {
    return false;
  }

  if(strcmp(chLayoutDescription, "stereo") == 0) {
    *channels = 2;
  } else if(strcmp(chLayoutDescription, "mono") == 0) {
    *channels = 1;    
  } else if(strcmp(chLayoutDescription, "2 channels") == 0) {
    memcpy(chLayoutDescription, "stereo", 7);
    *channels = 2;
  } else if(strcmp(chLayoutDescription, "1 channel")) {
  memcpy(chLayoutDescription, "mono", 5);
    *channels = 1;
  } else {
    return false;
  }
  // END FIRST
  */

  // BEGIN SECOND
  uint64_t ch_layout = av_codec_parameters->channel_layout;
  if(ch_layout & AV_CH_LAYOUT_MONO) {
    memcpy(chLayoutDescription, "mono", 5);
    *channels = 2;
  } else if (ch_layout & AV_CH_LAYOUT_STEREO) {
    memcpy(chLayoutDescription, "stereo", 7);
    *channels = 2;
  } else {
    return false;
  }
  // END SECOND

  av_opt_set(decoder->swr_context, "in_channel_layout", chLayoutDescription, 0);
  av_opt_set(decoder->swr_context, "out_channel_layout", chLayoutDescription, 0);
  av_opt_set_int(decoder->swr_context, "in_sample_fmt", decoder->av_codec_context->sample_fmt, 0);
  av_opt_set_int(decoder->swr_context, "in_sample_rate", decoder->av_codec_context->sample_rate, 0);
  av_opt_set_int(decoder->swr_context, "out_sample_fmt", av_sample_format, 0);
  av_opt_set_int(decoder->swr_context, "out_sample_rate", decoder->av_codec_context->sample_rate, 0);
  av_opt_set_double(decoder->swr_context, "rmvol", volume, 0);
  
  decoder->target_volume = volume;
  decoder->volume = volume;
    
  if(swr_init(decoder->swr_context) < 0) {
    decoder_free(decoder);
    return false;
  }
  //swr_set_quality(decoder->swr_context, 7);
  //swr_set_resample_mode(decoder->swr_context, SWR_FILTER_TYPE_CUBIC);

  int bits_per_sample;
  if(!decoder_fmt_to_bits_per_sample(&bits_per_sample, fmt)) {
    return false;
  }
  decoder->sample_size = *channels * bits_per_sample / 8;
    
  decoder->packet = av_packet_alloc();
  if(!decoder->packet) {
    decoder_free(decoder);
    return false;
  }
  
  decoder->frame = av_frame_alloc();
  if(!decoder->frame) {
    decoder_free(decoder);
    return false;
  }
  
  return true;    
}

DECODER_DEF void decoder_free(Decoder *decoder) {
  
  decoder->continue_receive = false;
  decoder->continue_convert = false;

  if(decoder->frame) {
    av_frame_free(&decoder->frame);
    decoder->frame = NULL;    
  }
  
  if(decoder->packet) {
    av_packet_free(&decoder->packet);
    decoder->packet = NULL;    
  }

  if(decoder->swr_context) {
    swr_free(&decoder->swr_context);
    decoder->swr_context = NULL;    
  }

  if(decoder->av_codec_context) {
    avcodec_close(decoder->av_codec_context);
    avcodec_free_context(&decoder->av_codec_context);
    decoder->av_codec_context = NULL;    
  }

  if(decoder->av_format_context) {
    avformat_close_input(&decoder->av_format_context);
    decoder->av_format_context = NULL;    
  }

  if(decoder->av_io_context) {
    avio_context_free(&decoder->av_io_context);
    decoder->av_io_context = NULL;
  }
}

DECODER_DEF bool decoder_decode(Decoder *decoder, int *out_samples, unsigned char *buffer) {
  *out_samples = 0;
  if(!decoder->continue_convert) {
    
    if(!decoder->continue_receive) {
      if(av_read_frame(decoder->av_format_context, decoder->packet) < 0) {
	decoder->continue_receive = false;
	decoder->continue_convert = false;
	return false;
      }
      if(decoder->packet->stream_index != decoder->stream_index) {
	decoder->continue_receive = false;
	decoder->continue_convert = false;

	av_packet_unref(decoder->packet);
	return true;
      }
    
      decoder->continue_receive = true;

      if(avcodec_send_packet(decoder->av_codec_context, decoder->packet) < 0) {
	//fprintf(stderr, "ERROR: fatal error in avcodec_send_packet\n");
	//exit(1);
	return false;
      }
    }  

    if(avcodec_receive_frame(decoder->av_codec_context, decoder->frame) >= 0) {

      if(decoder->target_volume != decoder->volume) {
	av_opt_set_double(decoder->swr_context, "rmvol", decoder->target_volume, 0);
	double volume;
	swr_init(decoder->swr_context);
	av_opt_get_double(decoder->swr_context, "rmvol", 0, &volume);
	decoder->volume = (float) volume;
      }

      decoder->pts = decoder->frame->pts;
      
      *out_samples = swr_convert(decoder->swr_context, &buffer, decoder->samples,
				 (const unsigned char **) (decoder->frame->data),
				 decoder->frame->nb_samples);
      
      if(*out_samples > 0) {
	decoder->continue_convert = true;
      } else {
	decoder->continue_convert = false;

	av_frame_unref(decoder->frame);
      }
    } else {
      *out_samples = 0;
      
      decoder->continue_convert = false;
      decoder->continue_receive = false;

      av_packet_unref(decoder->packet);
    }

    return true;
  }
      
  *out_samples = swr_convert(decoder->swr_context, &buffer, decoder->samples, NULL, 0);

  if(*out_samples > 0) {
    decoder->continue_convert = true;
  } else {
    decoder->continue_convert = false;    
    av_packet_unref(decoder->packet);
  }

  return true;

}


DECODER_DEF bool decoder_fmt_to_libav_fmt(enum AVSampleFormat *av_fmt, Decoder_Fmt fmt) {
  switch(fmt) {
  case DECODER_FMT_S16: {
    *av_fmt = AV_SAMPLE_FMT_S16;
    return true;
  } break;
  case DECODER_FMT_S32: {
    *av_fmt = AV_SAMPLE_FMT_S32;
    return true;
  } break;
  case DECODER_FMT_FLT: {
    *av_fmt = AV_SAMPLE_FMT_FLT;
    return true;
  } break;
  default: {
    return false;
  } 
  }
}

DECODER_DEF bool decoder_fmt_to_bits_per_sample(int *bits, Decoder_Fmt fmt) {
  switch(fmt) {
  case DECODER_FMT_S16: {
    *bits = 16;
    return true;
  } break;
  case DECODER_FMT_S32: {
    *bits = 32;
    return true;
  } break;
  case DECODER_FMT_FLT: {
    *bits = 32;
    return true;
  } break;
  default: {
    return false;
  } 
  }  
}

DECODER_DEF int decoder_memory_read(void *opaque, uint8_t *buf, int _buf_size) {
  Decoder_Memory *memory = (Decoder_Memory *) opaque;

  size_t buf_size = (size_t) _buf_size;

  if (buf_size > memory->size - memory->pos) {
    buf_size = memory->size - memory->pos;
  }

  if (buf_size <= 0) {
    return AVERROR_EOF;
  }

  memcpy(buf, memory->data + memory->pos, buf_size);
  memory->pos += buf_size;

  return (int )buf_size;
}

DECODER_DEF int64_t decoder_memory_seek(void *opaque, int64_t offset, int whence) {

  Decoder_Memory *memory = (Decoder_Memory *) opaque;

  switch (whence) {
  case SEEK_SET:
    memory->pos = offset;
    break;
  case SEEK_CUR:
    memory->pos += offset;
    break;
  case SEEK_END:
    memory->pos = memory->size + offset;
    break;
  case AVSEEK_SIZE:
    return (int64_t) memory->size;
  default:
    return AVERROR_INVALIDDATA;
  }

  if (memory->pos > memory->size) {
    return AVERROR(EIO);
  }
    
  return memory->pos;
}

DECODER_DEF int decoder_file_read(void *opaque, uint8_t *buf, int buf_size) {
  FILE *f = (FILE *)opaque;

  size_t bytes_read = fread(buf, 1, buf_size, f);

  if (bytes_read == 0) {
    if(feof(f)) return AVERROR_EOF;
    else return AVERROR(errno);
  }
  
  return (int) bytes_read;
}

DECODER_DEF int64_t decoder_file_seek(void *opaque, int64_t offset, int whence) {
  
  FILE *f = (FILE *)opaque;
  
  if (whence != SEEK_SET && whence != SEEK_CUR && whence != SEEK_END) {
    return AVERROR_INVALIDDATA;
  }

  if(fseek(f, (long) offset, whence)) {
    return AVERROR(errno);
  }

  return ftell(f);
}

#endif //DECODER_IMPLEMENTATION

#endif //DECODER_H
#ifndef IO_H
#define IO_H

#include <stdio.h>
#include <stdbool.h>

#ifdef _WIN32
#  include <windows.h>
#endif //_WIN32

#ifndef IO_DEF
#  define IO_DEF
#endif //IO_DEF

#ifndef IO_LOG
#  ifndef IO_QUIET
#    define IO_LOG(fmt, ...) fprintf(stderr, "IO_LOG: "fmt"\n", __VA_ARGS__)
#  else 
#    define IO_LOG(...)
#  endif // IO_VERBOSE
#endif //IO_LOG

////////////////////////////////////////////////////////////////////////////////////////

// Io_Util

IO_DEF bool io_slurp_file(const char *filepath, unsigned char **data, size_t *data_size);
IO_DEF bool io_exists(const char *file_path, bool *is_file);

////////////////////////////////////////////////////////////////////////////////////////

// Io_Dir

typedef struct{
#ifdef _WIN32
    WIN32_FIND_DATAW file_data;
    HANDLE handle;
    bool stop;
#else
    struct dirent *ent;
    DIR *handle;
#endif //_WIN32

    const char *name;
}Io_Dir;

typedef struct{
#ifdef _WIN32
  char abs_name[MAX_PATH];
#else
  char abs_name[PATH_MAX];
#endif //_WIN32

  char *name;
  bool is_dir;  
}Io_Dir_Entry;

IO_DEF bool io_dir_open(Io_Dir *dir, const char *dir_path);
IO_DEF bool io_dir_next(Io_Dir *dir, Io_Dir_Entry *entry);
IO_DEF void io_dir_close(Io_Dir *dir);

////////////////////////////////////////////////////////////////////////////////////////

// Io_File

typedef enum{
  IO_MODE_READ = 0,
  IO_MODE_WRITE,
  COUNT_IO_MODE,
}Io_Mode;

#ifdef _WIN32
typedef struct{ HANDLE handle; DWORD size; DWORD pos; }Io_File;
#else
typedef struct{ FILE *f; }Io_File;
#endif //_WIN32

IO_DEF bool io_file_size(Io_File *f, size_t *size);

IO_DEF bool io_file_open(Io_File *f, const char *filepath, Io_Mode mode);
IO_DEF int io_file_seek(Io_File *f, long int offset, int whence);
IO_DEF long int io_file_tell(Io_File *f);
IO_DEF size_t io_file_read(Io_File *f, void *ptr, size_t size, size_t count);
IO_DEF size_t io_file_write(Io_File *f, void *ptr, size_t size, size_t nmemb);
IO_DEF void io_file_close(Io_File *f);

////////////////////////////////////////////////////////////////////////////////////////

IO_DEF int io_last_error();
IO_DEF const char *io_last_error_cstr();

#ifdef IO_IMPLEMENTATION

#define io_file_write_cstr(f, cstr) io_file_write((f), (cstr), 1, (strlen(cstr)))

IO_DEF bool io_slurp_file(const char *filepath, unsigned char **data, size_t *data_size) {
  Io_File f;
  if(!io_file_open(&f, filepath, IO_MODE_READ)) {
    IO_LOG("Failed to open '%s': (%d) %s",
	   filepath, io_last_error(), io_last_error_cstr());
    return false;
  }

  if(!io_file_size(&f, data_size)) {
    io_file_close(&f);
    return false;
  }

  unsigned char *result = malloc(*data_size);
  if(!result) {
    io_file_close(&f);
    IO_LOG("Failed to allocate enough memory. Tried to allocate: %zu bytes", *data_size);
    return false;
  }

  if(*data_size != io_file_read(&f, result, 1, *data_size)) {
    io_file_close(&f);
    IO_LOG("Failed to read: '%s': (%d) %s",
	   filepath, io_last_error(), io_last_error_cstr());
    return false;
  }

  *data = result;

  return true;
}

IO_DEF bool io_exists(const char *file_path, bool *is_file) {
  DWORD attribs = GetFileAttributes(file_path);
  if(is_file) *is_file = !(attribs & FILE_ATTRIBUTE_DIRECTORY);
  return attribs != INVALID_FILE_ATTRIBUTES;
}

////////////////////////////////////////////////////////////////////////////////////////

IO_DEF bool io_dir_open(Io_Dir *dir, const char *dir_path) {
#ifdef _WIN32
  int num_wchars = MultiByteToWideChar(CP_UTF8, 0, dir_path, -1, NULL, 0); 
  wchar_t *my_wstring = (wchar_t *)malloc((num_wchars+1) * sizeof(wchar_t));
  MultiByteToWideChar(CP_UTF8, 0, dir_path, -1, my_wstring, num_wchars);
  my_wstring[num_wchars-1] = '*';
  my_wstring[num_wchars] = 0;

  // Use my_wstring as a const wchar_t *
  dir->handle = FindFirstFileExW(my_wstring, FindExInfoStandard, &dir->file_data, FindExSearchNameMatch, NULL, 0);
  if(dir->handle == INVALID_HANDLE_VALUE) {
    free(my_wstring);
    return false;
  }

  bool is_dir = (dir->file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) > 0;
  if(!is_dir) {
    free(my_wstring);
    return false;
  }

  dir->name = dir_path;
  dir->stop = false;

  free(my_wstring);
  return true;
#else
  return false;
#endif //_WIN32
}

IO_DEF bool io_dir_next(Io_Dir *dir, Io_Dir_Entry *entry) {
#ifdef _WIN32
  if(dir->stop) {
    return false;
  }

  entry->is_dir = (dir->file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) > 0;

  size_t len = strlen(dir->name);
  memcpy(entry->abs_name, dir->name, len);
  int len2 = WideCharToMultiByte(CP_ACP, 0, dir->file_data.cFileName, -1, NULL, 0, NULL, NULL);
  WideCharToMultiByte(CP_ACP, 0, dir->file_data.cFileName, -1, entry->abs_name + len, len2, NULL, NULL);

  //WHAT IS THIS
  if(entry->is_dir) {
    entry->abs_name[len + len2-1] = '/';
    entry->abs_name[len + len2] = 0;       
  } else {
    entry->abs_name[len + len2-1] = 0;
  }

  entry->name = (char *) &entry->abs_name[len];

  if(FindNextFileW(dir->handle, &dir->file_data) == 0) {
    dir->stop = true;
  }

  return true;
#else
  return false;
#endif //_WIN32
}

IO_DEF void io_dir_close(Io_Dir *dir) {
#ifdef _WIN32
  FindClose(dir->handle);
#else
  
#endif //_WIN32
}

////////////////////////////////////////////////////////////////////////////////////////

IO_DEF bool io_file_size(Io_File *f, size_t *size) {
#ifdef _WIN32
  *size = f->size;
  return true;
#else
  return false;
#endif //_WIN32
}

IO_DEF bool io_file_open(Io_File *f, const char *filepath, Io_Mode mode) {
#ifdef _WIN32
  if(mode < 0 || COUNT_IO_MODE <= mode)
    return false;

  f->handle = INVALID_HANDLE_VALUE;

  if(mode == IO_MODE_READ) {
    f->handle = CreateFile(filepath, GENERIC_READ,
			   FILE_SHARE_READ,
			   NULL,
			   OPEN_EXISTING,
			   FILE_ATTRIBUTE_NORMAL,
			   NULL);
    if(f->handle == INVALID_HANDLE_VALUE)
      goto error;

    f->size = GetFileSize(f->handle, NULL);
    if(f->size == INVALID_FILE_SIZE)
      goto error;

    f->pos = 0;
  } else {
    f->handle = CreateFile(filepath,
			   GENERIC_WRITE, 0, NULL,
			   CREATE_ALWAYS,
			   FILE_ATTRIBUTE_NORMAL,
			   NULL);
    if(f->handle == INVALID_HANDLE_VALUE)
      goto error;
    
    f->pos = 0;
    f->size = INVALID_FILE_SIZE;
  }
  
  return true;
 error:

  if(f->handle != INVALID_HANDLE_VALUE)
    CloseHandle(f->handle);
  
  return false;
#else
  return false;
#endif //_WIN32  
}

IO_DEF int io_file_seek(Io_File *f, long int offset, int whence) {
#ifdef _WIN32
  DWORD moveMethod;

  switch (whence) {
  case SEEK_SET: {
    moveMethod = FILE_BEGIN;    
  } break;
  case SEEK_CUR: {
    moveMethod = FILE_CURRENT;    
  } break;
  case SEEK_END: {
    moveMethod = FILE_END;    
  } break;
  default: {
    return -1;  // Invalid whence
  } break;
  }

  f->pos = SetFilePointer(f->handle, offset, NULL, moveMethod);
  if(f->pos == INVALID_SET_FILE_POINTER)
    return -1;

  return 0;
#else
  return -1;
#endif //_WIN32
}

IO_DEF long int io_file_tell(Io_File *f) {
#ifdef _WIN32
  return (long int) f->pos;
#else
  return -1;
#endif //_WIN32
}

IO_DEF void io_file_close(Io_File *f) {
#ifdef _WIN32
  CloseHandle(f->handle);
#else
#endif //_WIN32
}


IO_DEF size_t io_file_read(Io_File *f, void *ptr, size_t size, size_t count) {
#ifdef _WIN32
  DWORD bytes_read;
  DWORD bytes_to_read = (DWORD) (size * count);

  if(!ReadFile(f->handle, ptr, bytes_to_read, &bytes_read, NULL))
    return 0;
  f->pos += bytes_read;

  return (size_t) (bytes_read / size);
#else
  return 0;
#endif //_WIN32
}

IO_DEF size_t io_file_write(Io_File *f, void *ptr, size_t size, size_t nmemb) {
#ifdef _WIN32
  DWORD bytes_written;
  DWORD bytes_to_write = (DWORD) (size * nmemb);
  
  if(!WriteFile(f->handle, ptr, bytes_to_write, &bytes_written, NULL))
    return 0;

  return (size_t) (bytes_written / size);
#else
  return 0;
#endif //_WIN32
}

////////////////////////////////////////////////////////////////////////////////////////

IO_DEF int io_last_error() {
#ifdef _WIN32
  return GetLastError();
#else
  return 0;
#endif //_WIN32
}

IO_DEF const char *io_last_error_cstr() {
#ifdef _WIN32
  DWORD error = GetLastError();
  static char buffer[1024];

  FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM,
		 NULL,
		 error,
		 MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		 (LPSTR) &buffer,
		 sizeof(buffer),
		 NULL);
  
  return buffer;
#else
  return NULL;
#endif //_WIN32
}

#endif //IO_IMPLEMENTATION

#endif //IO_H
#ifndef MF_DECODER_H
#define MF_DECODER_H

//CREDITS
// - https://github.com/sipsorcery/mediafoundationsamples
// - https://www.gamedev.net/articles/programming/general-and-gameplay-programming/decoding-audio-for-xaudio2-with-microsoft-media-foundation-r4280/

// win32
//   mingw: -lmf -lmfplat -lmfuuid -lmfreadwrite
//   msvc : mf.lib mfplat.lib mfuuid.lib mfreadwrite.lib

#include <stdbool.h>
#include <mfapi.h>
#include <mfplay.h>
#include <mfreadwrite.h>

#ifndef MF_DECODER_DEF
#  define MF_DECODER_DEF static inline
#endif //MF_DECODER_DEF

typedef enum{
    MF_DECODER_FMT_S16,
    MF_DECODER_FMT_FLT,
}MF_Decoder_Fmt;

MF_DECODER_DEF bool mf_decoder_fmt_format_guid(const GUID **guid, MF_Decoder_Fmt fmt);
MF_DECODER_DEF bool mf_decoder_fmt_bits_per_sample(int *bits, MF_Decoder_Fmt fmt);

typedef struct{
    IMFSourceReader *source_reader;
    IMFMediaType *media_type;
    IMFMediaType *output_media_type;
    IMFMediaBuffer *media_buffer;
    IMFByteStream *byte_stream;

    IMFSample *audio_sample;
    IMFSample *prev_audio_sample;

    bool locked;
    int sample_size;
}MF_Decoder;

MF_DECODER_DEF bool mf_decoder_slurp(const char *path, MF_Decoder_Fmt fmt, int *channels, int *sample_rate, unsigned char **samples, unsigned int *samples_count);
MF_DECODER_DEF bool mf_decoder_slurp_memory(const unsigned char *memory, size_t memory_len, MF_Decoder_Fmt fmt, int *channels, int *sample_rate, unsigned char **out_samples, unsigned int *out_samples_count);
MF_DECODER_DEF bool mf_decoder_slurp_impl(MF_Decoder *decoder, int *channels, int *sample_rate, unsigned char **out_samples, unsigned int *out_samples_count);

MF_DECODER_DEF bool mf_decoder_init(MF_Decoder *decoder, const char *path, MF_Decoder_Fmt fmt, int *channels, int *sample_rate);
MF_DECODER_DEF bool mf_decoder_init_memory(MF_Decoder *decoder, const unsigned char *memory, size_t memory_len, MF_Decoder_Fmt fmt, int *channels, int *sample_rate);
MF_DECODER_DEF bool mf_decoder_init_impl(MF_Decoder *decoder, MF_Decoder_Fmt fmt, int *channels, int *sample_rate);

MF_DECODER_DEF bool mf_decoder_decode(MF_Decoder *decoder, unsigned char **samples, unsigned int *out_samples);
MF_DECODER_DEF void mf_decoder_free(MF_Decoder *decoder);

#ifdef MF_DECODER_IMPLEMENTATION

static bool mf_decoder_mf_startup = false;

MF_DECODER_DEF bool mf_decoder_slurp_impl(MF_Decoder *decoder, int *channels, int *sample_rate, unsigned char **out_samples, unsigned int *out_samples_count) {
  
  unsigned int samples_count = 0;
  unsigned int samples_cap = 5 * (*sample_rate) * (*channels);
  unsigned char *samples = malloc(samples_cap * decoder->sample_size);
  if(!samples) {
    mf_decoder_free(decoder);
    return false;
  }

  unsigned char *decoded_samples;
  unsigned int decoded_samples_count;
  while(mf_decoder_decode(decoder, &decoded_samples, &decoded_samples_count)) {

    unsigned int new_samples_cap = samples_cap;
    while(samples_count + decoded_samples_count > new_samples_cap) {
      new_samples_cap *= 2;
    }
    if(new_samples_cap != samples_cap) {
      samples_cap = new_samples_cap;
      samples = realloc(samples, samples_cap * decoder->sample_size);
      if(!samples) {
	mf_decoder_free(decoder);
	return false;
      }
    }
    
    memcpy(samples + samples_count * decoder->sample_size,
	   decoded_samples,
	   decoded_samples_count * decoder->sample_size);

    samples_count += decoded_samples_count;
  }

  *out_samples = samples;
  *out_samples_count = samples_count;

  return true;
}

MF_DECODER_DEF bool mf_decoder_slurp(const char *path, MF_Decoder_Fmt fmt, int *channels, int *sample_rate, unsigned char **out_samples, unsigned int *out_samples_count) {
  MF_Decoder decoder;
  if(!mf_decoder_init(&decoder, path, fmt, channels, sample_rate)) {
    return false;
  }

  if(!mf_decoder_slurp_impl(&decoder, channels, sample_rate, out_samples, out_samples_count)) {
    return false;
  }

  mf_decoder_free(&decoder);
  return true;
}

MF_DECODER_DEF bool mf_decoder_slurp_memory(const unsigned char *memory, size_t memory_len, MF_Decoder_Fmt fmt, int *channels, int *sample_rate, unsigned char **out_samples, unsigned int *out_samples_count) {
  MF_Decoder decoder;
  if(!mf_decoder_init_memory(&decoder, memory, memory_len, fmt, channels, sample_rate)) {
    return false;
  }

  if(!mf_decoder_slurp_impl(&decoder, channels, sample_rate, out_samples, out_samples_count)) {
    return false;
  }

  mf_decoder_free(&decoder);
  return true;
}

MF_DECODER_DEF bool mf_decoder_init(MF_Decoder *decoder, const char *path, MF_Decoder_Fmt fmt, int *channels, int *sample_rate) {

  if(!mf_decoder_mf_startup) {
    if(MFStartup(MF_VERSION, 0) != S_OK) {
      return false;
    }

    mf_decoder_mf_startup = true;
  }

  decoder->source_reader = NULL;
  decoder->byte_stream = NULL;
  
  int num_wchars = MultiByteToWideChar(CP_UTF8, 0, path, -1, NULL, 0); 
  wchar_t *my_wstring = (wchar_t *)malloc((num_wchars+1) * sizeof(wchar_t));
  if(!my_wstring) {
    return false;
  }
  MultiByteToWideChar(CP_UTF8, 0, path, -1, my_wstring, num_wchars);
  my_wstring[num_wchars] = 0;

  // SourceReader
  HRESULT result = MFCreateSourceReaderFromURL(my_wstring,
					       NULL,
					       &decoder->source_reader);
  free(my_wstring);
  if(result != S_OK) {
    return false;
  }

  return mf_decoder_init_impl(decoder, fmt, channels, sample_rate);
}

MF_DECODER_DEF bool mf_decoder_init_impl(MF_Decoder *decoder, MF_Decoder_Fmt fmt, int *channels, int *sample_rate) {

  decoder->media_type = NULL;
  decoder->output_media_type = NULL;
  
  // MediaType
  if(decoder->source_reader->lpVtbl->GetCurrentMediaType(decoder->source_reader, (DWORD) MF_SOURCE_READER_FIRST_AUDIO_STREAM, &decoder->media_type) != S_OK) {
    goto error;
  }

  if(decoder->source_reader->lpVtbl->SetStreamSelection(decoder->source_reader, (DWORD) MF_SOURCE_READER_FIRST_AUDIO_STREAM, TRUE) != S_OK) {
    goto error;
  }

  // MediaType - Attributes
  GUID major_type;
  if(decoder->media_type->lpVtbl->GetMajorType(decoder->media_type, &major_type) != S_OK) {
    goto error;
  }
    
  if( memcmp(&major_type, &MFMediaType_Audio, sizeof(major_type)) != 0) {
    goto error;
  }
    
  UINT32 attr_count;
  if(decoder->media_type->lpVtbl->GetCount(decoder->media_type, &attr_count) != S_OK) {
    goto error;
  }

  bool got_channels = false;
  bool got_sample_rate = false;
  for(UINT32 i=0;i<attr_count;i++) {
    GUID guid_id;
    if(decoder->media_type->lpVtbl->GetItemByIndex(decoder->media_type, i, &guid_id, NULL) != S_OK) {
      goto error;
    }

    MF_ATTRIBUTE_TYPE attr_type;
    if(decoder->media_type->lpVtbl->GetItemType(decoder->media_type, &guid_id, &attr_type) != S_OK) {
      goto error;
    }

    if( memcmp(&guid_id, &MF_MT_AUDIO_NUM_CHANNELS, sizeof(guid_id)) == 0 ) {
	    
      if(attr_type != MF_ATTRIBUTE_UINT32) {
	goto error;
      }

      UINT32 value;
      if(decoder->media_type->lpVtbl->GetUINT32(decoder->media_type, &guid_id, &value) != S_OK) {
	goto error;
      }

      *channels = (int) value;
      got_channels = true;
    } else if(memcmp(&guid_id, &MF_MT_AUDIO_SAMPLES_PER_SECOND, sizeof(guid_id)) == 0 ) {

      if(attr_type != MF_ATTRIBUTE_UINT32) {
	goto error;
      }

      UINT32 value;
      if(decoder->media_type->lpVtbl->GetUINT32(decoder->media_type, &guid_id, &value) != S_OK) {
	goto error;
      }

      *sample_rate = (int) value;
      got_sample_rate = true;
    }
  }

  if(!got_channels || !got_sample_rate) {
    return false;
  }

  // Output-MediaType
  if( MFCreateMediaType(&decoder->output_media_type) != S_OK) {
    goto error;
  }    

  if( decoder->output_media_type->lpVtbl->SetGUID(decoder->output_media_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Audio) != S_OK) {
    goto error;
  }

  int bits_per_sample;
  if( !mf_decoder_fmt_bits_per_sample(&bits_per_sample, fmt) ) {
    return false;
  }
  decoder->sample_size = bits_per_sample * (*channels) / 8;

  const GUID *guid;
  if( !mf_decoder_fmt_format_guid(&guid, fmt) ) {
    goto error;
  }

  if( decoder->output_media_type->lpVtbl->SetGUID(decoder->output_media_type, &MF_MT_SUBTYPE, guid) != S_OK) {
    goto error;
  }    

  if( decoder->source_reader->lpVtbl->SetCurrentMediaType(decoder->source_reader, MF_SOURCE_READER_FIRST_AUDIO_STREAM, NULL, decoder->output_media_type)) {
    goto error;
  }

  decoder->locked = false;
  decoder->prev_audio_sample = NULL;
  return true;
    
 error:
  if(decoder->source_reader) decoder->source_reader->lpVtbl->Release(decoder->source_reader);
  if(decoder->media_type) decoder->media_type->lpVtbl->Release(decoder->media_type);
  if(decoder->output_media_type) decoder->output_media_type->lpVtbl->Release(decoder->output_media_type);
  return false;
}

MF_DECODER_DEF bool mf_decoder_init_memory(MF_Decoder *decoder, const unsigned char *memory, size_t memory_len, MF_Decoder_Fmt fmt, int *channels, int *sample_rate) {

  if(!mf_decoder_mf_startup) {
    if(MFStartup(MF_VERSION, 0) != S_OK) {
      return false;
    }

    mf_decoder_mf_startup = true;
  }

  decoder->source_reader = NULL;
  decoder->byte_stream = NULL;
  
  if( MFCreateTempFile(
		       MF_ACCESSMODE_READWRITE,
		       MF_OPENMODE_DELETE_IF_EXIST,
		       MF_FILEFLAGS_NONE,
		       &decoder->byte_stream) != S_OK) {
    goto error;
  }
  ULONG wrote_bytes;
  if(decoder->byte_stream->lpVtbl->Write(decoder->byte_stream, memory, memory_len, &wrote_bytes) != S_OK) {
    goto error;
  }
  if(decoder->byte_stream->lpVtbl->SetCurrentPosition(decoder->byte_stream, 0) != S_OK) {
    goto error;
  } 

  // SourceReader
  if(MFCreateSourceReaderFromByteStream(decoder->byte_stream,
					NULL,
					&decoder->source_reader) != S_OK) {
    goto error;
  }


  return mf_decoder_init_impl(decoder, fmt, channels, sample_rate);
 error:
  if(decoder->byte_stream) decoder->byte_stream->lpVtbl->Release(decoder->byte_stream);
  return false;
}

MF_DECODER_DEF bool mf_decoder_decode(MF_Decoder *decoder, unsigned char **samples, unsigned int *out_samples) {

    if(decoder->locked) {
	if(decoder->media_buffer->lpVtbl->Unlock(decoder->media_buffer) != S_OK) {
	    return false;
	}

	decoder->media_buffer->lpVtbl->Release(decoder->media_buffer);
	if(decoder->prev_audio_sample) decoder->prev_audio_sample->lpVtbl->Release(decoder->prev_audio_sample);
	decoder->prev_audio_sample = decoder->audio_sample;

	decoder->locked = false;
    }
    
    decoder->audio_sample = NULL;
    DWORD stream_index, flags;
    LONGLONG ll_audio_time_stamp;
    if(decoder->source_reader->lpVtbl->ReadSample(decoder->source_reader,
						  MF_SOURCE_READER_FIRST_AUDIO_STREAM,
						  0,
						  &stream_index,
						  &flags,
						  &ll_audio_time_stamp,
						  &decoder->audio_sample) != S_OK) {
	return false;
    }


    if( flags & (MF_SOURCE_READERF_ENDOFSTREAM
		 | MF_SOURCE_READERF_NEWSTREAM
		 | MF_SOURCE_READERF_NATIVEMEDIATYPECHANGED
		 | MF_SOURCE_READERF_CURRENTMEDIATYPECHANGED)) {
	return false;
    }

    if(!decoder->audio_sample) {
	return false;
    }

    decoder->media_buffer = NULL;
    if(decoder->audio_sample->lpVtbl->
       ConvertToContiguousBuffer(decoder->audio_sample, &decoder->media_buffer) != S_OK) {
	return false;
    }

    DWORD samples_len_bytes;
    if(decoder->media_buffer->lpVtbl->Lock(decoder->media_buffer, samples, NULL, &samples_len_bytes) != S_OK) {
	return false;
    }
    *out_samples = samples_len_bytes / decoder->sample_size;

    decoder->locked = true;
    
    return true;
}

MF_DECODER_DEF void mf_decoder_free(MF_Decoder *decoder) {

    if(decoder->locked) {
	decoder->media_buffer->lpVtbl->Unlock(decoder->media_buffer);
	decoder->locked = false;
    }
    
    decoder->source_reader->lpVtbl->Release(decoder->source_reader);
    decoder->output_media_type->lpVtbl->Release(decoder->output_media_type);
    decoder->media_type->lpVtbl->Release(decoder->media_type);
    if(decoder->byte_stream) decoder->byte_stream->lpVtbl->Release(decoder->byte_stream);
}

MF_DECODER_DEF bool mf_decoder_fmt_format_guid(const GUID **guid, MF_Decoder_Fmt fmt) {
    switch(fmt) {
    case MF_DECODER_FMT_S16: {
	*guid = &MFAudioFormat_PCM;
	return true;
    } break;
    case MF_DECODER_FMT_FLT: {
	//*guid = &MFAudioFormat_FLT;
	//return true;
	return false;
    } break;
    default: {
	return false;
    } break;
    }
}

MF_DECODER_DEF bool mf_decoder_fmt_bits_per_sample(int *bits, MF_Decoder_Fmt fmt) {
    switch(fmt) {
    case MF_DECODER_FMT_S16: {
	*bits = 16;
	return true;
    } break;
    case MF_DECODER_FMT_FLT: {
	*bits = 32;
	return true;
    } break;
    default: {
	return false;	
    } break;
    }

}

#endif //MF_DECODER_IMPLEMENTATION

#endif //MF_DECODER_H
#ifndef TYPES_H
#define TYPES_H

#include <stdio.h>
#include <stdint.h>

typedef uint8_t u8;
typedef char s8; // because of mingw warning not 'int8_t'
typedef uint16_t u16;
typedef int16_t s16;
typedef uint32_t u32;
typedef int32_t s32;
typedef uint64_t u64;
typedef int64_t s64;

typedef float f32;
typedef double f64;

#define return_defer(n) do{			\
    result = (n);				\
    goto defer;					\
  }while(0)

#define errorf(...) do{						\
    fflush(stdout);						\
    fprintf(stderr, "%s:%d:ERROR: ", __FILE__, __LINE__);	\
    fprintf(stderr,  __VA_ARGS__ );				\
    fprintf(stderr, "\n");					\
    fflush(stderr);						\
  }while(0)

#define panicf(...) do{						\
    errorf(__VA_ARGS__);					\
    exit(1);							\
  }while(0)

#endif // TYPES_H
#ifndef WAV_H
#define WAV_H

//https://de.wikipedia.org/wiki/RIFF_WAVE

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#ifndef WAV_DEF
#  define WAV_DEF static inline
#endif //WAV_DEF

typedef struct {
  int8_t chunkID[4];  // 'RIFF'
  uint32_t chunkSize;
  int8_t riffType[4]; // 'WAVE'
  int8_t fmtID[4];    // 'fmt '
  uint32_t fmtChunkSize;
  uint16_t wFormatTag;
  uint16_t channels; // wChannels
  uint32_t sample_rate; // dwSamplesPerSec
  uint32_t dwAvgBytesPerSec;
  uint16_t wBlockAlign;
  uint16_t wBitsPerSample;
  uint8_t data[4];  // 'data'
  uint32_t dataSize;
} Wav_Header;

typedef Wav_Header Wav;

// Public:
WAV_DEF bool wav_slurp(Wav *wav_header, const char *filepath, unsigned char **data, uint32_t *size);
WAV_DEF bool wav_read(Wav *wav_header, unsigned char *memory, size_t memory_len, unsigned char **data, uint32_t *size);

#ifdef WAV_IMPLEMENTATION

WAV_DEF bool wav_slurp(Wav *wav_header, const char *filepath, unsigned char **data, uint32_t *size) {
  FILE* f = fopen(filepath, "rb");
  if(!f) {
    return false;
  }

  size_t n = fread(wav_header, sizeof(*wav_header), 1, f);
  if(n != 1) {
    fclose(f);
    return false;
  }

  *size = (uint32_t) wav_header->chunkSize - sizeof(*wav_header);
  *data = (unsigned char *) malloc(*size);
  if(!(*data)) {
    fclose(f);
    return false;
  }
  size_t m = fread(*data, 1, *size, f);
  if(m != *size) {
    fclose(f);
    return false;
  }

  fclose(f);
  return true;
}

WAV_DEF bool wav_read(Wav *wav_header, unsigned char *memory, size_t memory_len, unsigned char **data, uint32_t *size) {

  size_t header_size = sizeof(*wav_header);

  if(memory_len < header_size) {
    return false;
  }
  
  memcpy(wav_header, memory, header_size);
  
  memory += header_size;
  memory_len -= header_size;

  *size = (uint32_t) wav_header->chunkSize - header_size;
  *data = memory;  

  return true;
}

#endif //WAV_IMPLEMENTATION

#endif //WAV_H
#ifndef WINDOW_H
#define WINDOW_H

// win32
//   msvc : user32.lib gdi32.lib opengl32.lib (shell32.lib)
//   mingw:

#ifdef WINDOW_VERBOSE
#  include <stdio.h>
#  define WINDOW_LOG(...) fprintf(stderr, __VA_ARGS__);
#else
#  define WINDOW_LOG(...)
#endif //WINDOW_VERBOSE

#include <stdbool.h>
#include <math.h>

#include <windows.h>
#include <GL/GL.h>

#ifndef WINDOW_DEF
#  define WINDOW_DEF static inline
#endif //WINDOW_DEF

#ifndef PI
#  define PI 3.141592653589793f
#endif //PI

#define WINDOW_DOUBLE_CLICK_TIME_MS 500

#define WINDOW_BACKSPACE 8
#define WINDOW_ESCAPE 27
#define WINDOW_SPACE 32

typedef enum{
  WINDOW_EVENT_NONE = 0,
  WINDOW_EVENT_KEYPRESS,
  WINDOW_EVENT_KEYRELEASE,
  WINDOW_EVENT_MOUSEPRESS,
  WINDOW_EVENT_MOUSERELEASE,
  WINDOW_EVENT_FILEDROP,
}Window_Event_Type;

typedef struct{
  MSG msg;
  Window_Event_Type type;
  union{
    char key;
    long long value;
  }as;
}Window_Event;

typedef struct{
  HDROP h_drop;
  char path[MAX_PATH];

  int count;
  int index;
}Window_Dragged_Files;

typedef struct{
  HWND hwnd;
  HDC dc;
  RECT rect;
  POINT point;
  LARGE_INTEGER performance_frequency;
  LARGE_INTEGER time;
  bool is_shift_down;
  
  double dt;
  int running;
  int width, height;
}Window;

#define WINDOW_RUNNING       0x1
#define WINDOW_NOT_RESIZABLE 0x2
#define WINDOW_DRAG_N_DROP   0x4
#define WINDOW_FULLSCREEN    0x8


WINDOW_DEF bool window_init(Window *w, int width, int height, const char *title, int flags);
WINDOW_DEF bool window_set_vsync(Window *w, bool use_vsync);
WINDOW_DEF bool window_peek(Window *w, Window_Event *event);
WINDOW_DEF bool window_get_mouse_position(Window *w, int *width, int *height);
WINDOW_DEF void window_swap_buffers(Window *w);
WINDOW_DEF bool window_toggle_fullscreen(Window *w);
WINDOW_DEF void window_free(Window *w);
WINDOW_DEF bool window_show_cursor(Window *w, bool show);

WINDOW_DEF bool window_dragged_files_init(Window_Dragged_Files *files, Window_Event *event);
WINDOW_DEF bool window_dragged_files_next(Window_Dragged_Files *files, char **path);
WINDOW_DEF void window_dragged_files_free(Window_Dragged_Files *files);

WINDOW_DEF bool window_compile_shader(GLuint *shader, GLenum shader_type, const char *shader_source);
WINDOW_DEF bool window_link_program(GLuint *program, GLuint vertex_shader, GLuint fragment_shader);

#ifndef WINDOW_NO_RENDERER

typedef struct{
  float x, y;
}Window_Renderer_Vec2f;

WINDOW_DEF Window_Renderer_Vec2f window_renderer_vec2f(float x, float y);

typedef struct{
  float x, y, z, w;
}Window_Renderer_Vec4f;

WINDOW_DEF Window_Renderer_Vec4f window_renderer_vec4f(float x, float y, float z, float w);

typedef struct{
  Window_Renderer_Vec2f position;
  Window_Renderer_Vec4f color;
  Window_Renderer_Vec2f uv;
}Window_Renderer_Vertex;

#define WINDOW_RENDERER_VERTEX_ATTR_POSITION 0
#define WINDOW_RENDERER_VERTEX_ATTR_COLOR 1
#define WINDOW_RENDERER_VERTEX_ATTR_UV 2

#define WINDOW_RENDERER_CAP 1024

typedef struct{
  GLuint vao, vbo;
  GLuint vertex_shader, fragment_shader;
  GLuint program;
  
  GLuint textures;
  unsigned int images_count;

#ifdef __STB_INCLUDE_STB_TRUETYPE_H__
  stbtt_bakedchar font_cdata[96]; // ASCII 32..126 is 95 glyphs
#endif //__STB_INCLUDE_STB_TRUETYPE_H__
    
  int font_index;    
  int tex_index;

  float width, height;
  Window_Renderer_Vec4f background;

  Window_Renderer_Vertex verticies[WINDOW_RENDERER_CAP];
  int verticies_count;
}Window_Renderer;

static Window_Renderer_Vec4f WHITE = {1, 1, 1, 1};
static Window_Renderer_Vec4f RED   = {1, 0, 0, 1};
static Window_Renderer_Vec4f BLUE  = {0, 0, 1, 1};
static Window_Renderer_Vec4f GREEN = {0, 1, 0, 1};
static Window_Renderer_Vec4f BLACK = {0, 0, 0, 1};

#define Vec2f Window_Renderer_Vec2f
#define vec2f(x, y) window_renderer_vec2f((x), (y))
#define Vec4f Window_Renderer_Vec4f
#define vec4f(x, y, z, w) window_renderer_vec4f((x), (y), (z), (w))
#define draw_triangle window_renderer_triangle
#define draw_solid_triangle window_renderer_solid_triangle
#define draw_solid_rect window_renderer_solid_rect
#define draw_solid_rounded_rect window_renderer_solid_rounded_rect
#define draw_solid_rounded_shaded_rect window_renderer_solid_rounded_shaded_rect
#define draw_solid_rect_angle window_renderer_solid_rect_angle
#define push_texture window_renderer_push_texture
#define draw_texture window_renderer_texture
#define draw_texture_colored window_renderer_texture_colored
#define draw_solid_circle window_renderer_solid_circle

#ifdef __STB_INCLUDE_STB_TRUETYPE_H__
#  define push_font window_renderer_push_font
#  define draw_text(cstr, pos, factor) window_renderer_text((cstr), strlen((cstr)), (pos), (factor), (WHITE))
#  define draw_text_colored(cstr, pos, factor, color) window_renderer_text((cstr), strlen((cstr)), (pos), (factor), (color))
#  define draw_text_len(cstr, cstr_len, pos, factor) window_renderer_text((cstr), (cstr_len), (pos), (factor), (WHITE))
#  define draw_text_len_colored(cstr, cstr_len, pos, factor, color) window_renderer_text((cstr), (cstr_len), (pos), (factor), (color))

#  define measure_text(cstr, factor, size) window_renderer_measure_text((cstr), strlen((cstr)), (factor), (size));
#  define measure_text_len(cstr, cstr_len, factor, size) window_renderer_measure_text((cstr), (cstr_len), (factor), (size));
#endif //__STB_INCLUDE_STB_TRUETYPE_H__

WINDOW_DEF bool window_renderer_init(Window_Renderer *r);
WINDOW_DEF void window_renderer_begin(int width, int height);
WINDOW_DEF void window_renderer_set_color(Window_Renderer_Vec4f color);
WINDOW_DEF void window_renderer_vertex(Window_Renderer_Vec2f p, Window_Renderer_Vec4f c, Window_Renderer_Vec2f uv);
WINDOW_DEF void window_renderer_triangle(Window_Renderer_Vec2f p1, Window_Renderer_Vec2f p2, Window_Renderer_Vec2f p3, Window_Renderer_Vec4f c1, Window_Renderer_Vec4f c2, Window_Renderer_Vec4f c3, Window_Renderer_Vec2f uv1, Window_Renderer_Vec2f uv2, Window_Renderer_Vec2f uv3);
WINDOW_DEF void window_renderer_solid_triangle(Window_Renderer_Vec2f p1, Window_Renderer_Vec2f p2, Window_Renderer_Vec2f p3, Window_Renderer_Vec4f c);
WINDOW_DEF void window_renderer_quad(Window_Renderer_Vec2f p1, Window_Renderer_Vec2f p2, Window_Renderer_Vec2f p3, Window_Renderer_Vec2f p4,Window_Renderer_Vec4f c1,Window_Renderer_Vec4f c2,Window_Renderer_Vec4f c3,Window_Renderer_Vec4f c4, Window_Renderer_Vec2f uv1, Window_Renderer_Vec2f uv2, Window_Renderer_Vec2f uv3, Window_Renderer_Vec2f uv4);
WINDOW_DEF void window_renderer_solid_rect(Window_Renderer_Vec2f pos, Window_Renderer_Vec2f size,Window_Renderer_Vec4f color);
WINDOW_DEF void window_renderer_solid_rounded_rect(Vec2f pos, Vec2f size, float radius, int parts, Vec4f color);
WINDOW_DEF void window_renderer_solid_rounded_shaded_rect(Window_Renderer_Vec2f pos, Window_Renderer_Vec2f size, float radius, int parts, float shade_px, Window_Renderer_Vec4f color);
WINDOW_DEF void window_renderer_solid_rect_angle(Window_Renderer_Vec2f pos, Window_Renderer_Vec2f size, float angle, Window_Renderer_Vec4f color);
WINDOW_DEF bool window_renderer_push_texture(int width, int height, const void *data, bool grey, unsigned int *index);
WINDOW_DEF void window_renderer_texture(unsigned int texture, Window_Renderer_Vec2f p, Window_Renderer_Vec2f s, Window_Renderer_Vec2f uvp, Window_Renderer_Vec2f uvs);
WINDOW_DEF void window_renderer_texture_colored(unsigned int texture, Window_Renderer_Vec2f p, Window_Renderer_Vec2f s, Window_Renderer_Vec2f uvp, Window_Renderer_Vec2f uvs, Window_Renderer_Vec4f c);
WINDOW_DEF void window_renderer_solid_circle(Window_Renderer_Vec2f pos, float start_angle, float end_angle, float radius, int parts, Window_Renderer_Vec4f color);
WINDOW_DEF void window_renderer_end();
WINDOW_DEF void window_renderer_free();

#ifdef __STB_INCLUDE_STB_TRUETYPE_H__
WINDOW_DEF bool window_renderer_push_font(const char *filepath, float pixel_height);
WINDOW_DEF void window_renderer_measure_text(const char *cstr, size_t cstr_len, float scale, Vec2f *size);
WINDOW_DEF void window_renderer_text(const char *cstr, size_t cstr_len, Window_Renderer_Vec2f pos, float scale, Window_Renderer_Vec4f color);
#endif //__STB_INCLUDE_STB_TRUETYPE_H__

#endif //WINDOW_NO_RENDERER


// opengl - functions / types / definitions
#define GL_TEXTURE0 0x84C0
#define GL_TEXTURE1 0x84C1
#define GL_TEXTURE2 0x84C2
#define GL_TEXTURE3 0x84C3
#define GL_TEXTURE4 0x84C4
#define GL_TEXTURE5 0x84C5

#define GL_ARRAY_BUFFER 0x8892
#define GL_DYNAMIC_DRAW 0x88E8
#define GL_ACTIVE_UNIFORMS 0x8B86

#define GL_FRAGMENT_SHADER 0x8B30
#define GL_VERTEX_SHADER 0x8B31
#define GL_COMPILE_STATUS 0x8B81
#define GL_LINK_STATUS 0x8B82
#define GL_CLAMP_TO_EDGE 0x812F
#define GL_RGB_INTEGER 0x8D98
#define GL_UNSIGNED_INT_8_8_8_8_REV 0x8367
#define GL_MULTISAMPLE  0x809D
#define GL_SAMPLES 0x80A9

#define GL_BGRA 0x80E1
#define GL_RGB 0x1907
#define GL_BGR 0x80E0

#define GL_MULTISAMPLE 0x809D
#define GL_MULTISAMPLE_ARB 0x809D
#define GL_MULTISAMPLE_BIT 0x20000000
#define GL_MULTISAMPLE_BIT_ARB 0x20000000
#define GL_MULTISAMPLE_FILTER_HINT_NV 0x8534
#define GL_SAMPLE_ALPHA_TO_COVERAGE 0x809E
#define GL_SAMPLE_BUFFERS 0x80A8
#define GL_SAMPLES 0x80A9

typedef ptrdiff_t GLsizeiptr;
typedef ptrdiff_t GLintptr;
typedef char GLchar;

void glActiveTexture(GLenum texture);
void glGenVertexArrays(GLsizei n, GLuint *arrays);
void glBindVertexArray(GLuint array);
void glGenBuffers(GLsizei n, GLuint *buffers);
void glBindBuffer(GLenum target, GLuint buffer);
void glBufferData(GLenum target, GLsizeiptr size, const void * data, GLenum usage);
void glEnableVertexAttribArray(GLuint index);
void glVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void * pointer);
GLuint glCreateShader(GLenum shaderType);
void glShaderSource(GLuint shader, GLsizei count, const GLchar **_string, const GLint *length);
void glCompileShader(GLuint shader);
void glGetShaderiv(GLuint shader, GLenum pname, GLint *params);
void glGetShaderInfoLog(GLuint shader, GLsizei maxLength, GLsizei *length, GLchar *infoLog);
GLuint glCreateProgram(void);
void glAttachShader(GLuint program, GLuint shader);
void glLinkProgram(GLuint program);
void glGetProgramInfoLog(GLuint program, GLsizei maxLength, GLsizei *length, GLchar *infoLog);
void glGetProgramiv(GLuint program, GLenum pname, GLint *params);
void glUseProgram(GLuint program);
void glBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const void * data);
void glUniform2f(GLint location, GLfloat v0, GLfloat v1);
void glUniform1f(GLint location, GLfloat v0);
void glUniform1fv(GLint location, GLsizei count, const GLfloat *value);
void glUniform2fv(GLint location, GLsizei count, const GLfloat *value);
void glUniform1i(GLint location, GLint v0);
GLint glGetUniformLocation(GLuint program, const GLchar *name);
void glGetActiveUniform(GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name);
void glGetUniformfv(GLuint program, GLint location, GLfloat *params);
void glGetUniformiv(GLuint program, GLint location, GLsizei bufSize, GLint *params);
void glSampleCoverage(GLfloat value, GLboolean invert);
int wglSwapIntervalEXT(GLint interval);

#ifdef WINDOW_IMPLEMENTATION

#ifndef WINDOW_NO_RENDERER
static Window_Renderer window_renderer;
static bool window_renderer_inited = false;
#endif //WINDOW_NO_RENDERER

LRESULT CALLBACK Window_Implementation_WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {

  if(message == WM_CLOSE ||
     message == WM_DESTROY) {
    Window *w = (Window *) GetWindowLongPtr(hWnd, 0);
    if(w != NULL) {
      w->running = 0;
    }
    PostQuitMessage(0);
    return 0;
  } else {
    return DefWindowProc(hWnd, message, wParam, lParam);
  }
  
}

WINDOW_DEF void window_win32_opengl_init();

WINDOW_DEF bool window_init(Window *w, int width, int height, const char *title, int flags) {

  STARTUPINFO startupInfo;
  GetStartupInfo(&startupInfo);
  HMODULE hInstance = GetModuleHandle(NULL);
  DWORD nCmdShow = startupInfo.wShowWindow;

#if 1
  HINSTANCE user32Lib = LoadLibrary("user32.dll");
  if(!user32Lib) {
    return false;
  }

  PROC setProcessDPIAware = GetProcAddress(user32Lib, "SetProcessDPIAware");
  if(!setProcessDPIAware) {
    return false;
  }
  setProcessDPIAware();

  FreeLibrary(user32Lib);
#else
  SetProcessDPIAware();
#endif

  WNDCLASSEX wc = {0};
  wc.cbSize = sizeof(WNDCLASSEX);
  wc.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc = Window_Implementation_WndProc;
  wc.hInstance = hInstance;
  wc.lpszClassName = title;
  wc.cbWndExtra = sizeof(LONG_PTR);
  wc.cbClsExtra = 0;
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);
  
  HICON icon = LoadIcon(hInstance, MAKEINTRESOURCE(1));
  wc.hIcon = icon; // ICON when tabbing
  wc.hIconSm = icon; //ICON default
  
  if(!RegisterClassEx(&wc)) {
    return false;
  }

  int screen_width = GetSystemMetrics(SM_CXSCREEN);
  int screen_height = GetSystemMetrics(SM_CYSCREEN);

  // WS_THICKFRAME :: resizable
  // WS_MAXIMIZEBOX :: maximizable

#define WINDOW_STYLE (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX)
#define WINDOW_RESIZABLE_STYLE (WS_THICKFRAME | WS_MAXIMIZEBOX)

  DWORD style = WINDOW_STYLE;
  if(!(flags & WINDOW_NOT_RESIZABLE)) {
    style |= WINDOW_RESIZABLE_STYLE;
  } else {
    width -= 10;
    height -= 10;
  }

  //add space  
  width += 16;
  height += 39;

  DWORD window_flags = 0;
  if((flags & WINDOW_DRAG_N_DROP)) {
    window_flags |= WS_EX_ACCEPTFILES;
  }

  w->hwnd = CreateWindowEx(window_flags,
			   wc.lpszClassName,
			   wc.lpszClassName,
			   style,
			   screen_width / 2 - width/2,
			   screen_height / 2 - height/2,
			   width,
			   height,
			   NULL,
			   NULL,
			   hInstance,
			   NULL);
  if(!w->hwnd) {
    return false;
  }
  w->dc = GetDC(w->hwnd);

  //BEGIN opengl
  HDC w_dc = GetDC(w->hwnd);

  PIXELFORMATDESCRIPTOR desired_format = {0};
  desired_format.nSize = sizeof(desired_format);
  desired_format.nVersion = 1;
  desired_format.dwFlags = PFD_SUPPORT_OPENGL|PFD_DRAW_TO_WINDOW|PFD_DOUBLEBUFFER;
  desired_format.cColorBits = 32;
  desired_format.cAlphaBits = 8;

  int suggested_format_index = ChoosePixelFormat(w_dc, &desired_format);
  PIXELFORMATDESCRIPTOR suggested_format;
  DescribePixelFormat(w_dc, suggested_format_index, sizeof(suggested_format), &suggested_format);
  SetPixelFormat(w_dc, suggested_format_index, &suggested_format);
  
  HGLRC opengl_rc = wglCreateContext(w_dc);
  if(!wglMakeCurrent(w_dc, opengl_rc)) {
    return false;
  }
  ReleaseDC(w->hwnd, w_dc);
  //END opengl

  LONG_PTR lptr = {0};
  memcpy(&lptr, &w, sizeof(w));  
  SetWindowLongPtr(w->hwnd, 0, lptr);  

  ShowWindow(w->hwnd, nCmdShow);
  UpdateWindow(w->hwnd);

  w->running = WINDOW_RUNNING;
  w->width = width;
  w->height = height;
  w->is_shift_down = false;

  // load non-default-opengl-functions
  window_win32_opengl_init();

#ifndef WINDOW_NO_RENDERER
  if(!window_renderer_inited) {
    if(!window_renderer_init(&window_renderer)) {
      return false;	    
    }
	
    window_renderer_inited = true;
  }
#endif //WINDOW_NO_RENDERER

  // use vsync as default
  if(!window_set_vsync(w, true)) {
    return false;
  }

  if((flags & WINDOW_FULLSCREEN) && !window_toggle_fullscreen(w)) {
    return false;
  }

  QueryPerformanceFrequency(&w->performance_frequency);
  QueryPerformanceCounter(&w->time);
  w->dt = 0;
    
  return true;
}

WINDOW_DEF bool window_set_vsync(Window *w, bool use_vsync) {
  (void) w;
  return wglSwapIntervalEXT(use_vsync ? 1 : 0);
}

WINDOW_DEF bool window_peek(Window *w, Window_Event *e) {
    
  MSG *msg = &e->msg;

  while(true) {
    if(!PeekMessage(msg, w->hwnd, 0, 0, PM_REMOVE)) {
      break;
    }

    TranslateMessage(msg);
    DispatchMessage(msg);

    e->type = WINDOW_EVENT_NONE;

    switch(msg->message) {
    case WM_DROPFILES: {
      e->type = WINDOW_EVENT_FILEDROP;
      e->as.value = msg->wParam;
    } break;
    case WM_RBUTTONUP:  {
      e->type = WINDOW_EVENT_MOUSERELEASE;
      e->as.key = 'R';
      ReleaseCapture();
    } break;
    case WM_RBUTTONDOWN: {
      e->type = WINDOW_EVENT_MOUSEPRESS;
      e->as.key = 'R';
      SetCapture(w->hwnd);
    } break;
    case WM_LBUTTONUP: {
      e->type = WINDOW_EVENT_MOUSERELEASE;
      e->as.key = 'L';
      ReleaseCapture();
    } break;
    case WM_LBUTTONDOWN: {
      e->type = WINDOW_EVENT_MOUSEPRESS;
      e->as.key = 'L';
      SetCapture(w->hwnd);
    } break;
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    case WM_KEYDOWN:
    case WM_KEYUP: {
      bool was_down = ((msg->lParam & (1 << 30)) != 0);
      bool is_down = ((msg->lParam & (1 << 31)) == 0);

      if(was_down != is_down) {

	if(msg->wParam == VK_SHIFT) {
	  if(was_down) {
	    w->is_shift_down = false;
	  } else {
	    w->is_shift_down = true;
	  }
	  continue;
	}


	char c = (char) msg->wParam;
	if(!w->is_shift_down && 'A' <= c && c <= 'Z') {
	  c += 32;
	}	
	e->as.key = c;
	if(was_down) {
	  e->type = WINDOW_EVENT_KEYRELEASE;
	} else {
	  e->type = WINDOW_EVENT_KEYPRESS;
	}

	
      }
      
    } break;
    default: {
    } break;
    }

    if(e->type != WINDOW_EVENT_NONE) {
      return true;
    }
  }

  // width, height
  if(!(w->running & WINDOW_FULLSCREEN)) {
    if(GetClientRect(w->hwnd, &w->rect)) {
      w->width = (w->rect.right - w->rect.left);
      w->height = (w->rect.bottom - w->rect.top);
    }
  }

  //dt
  LARGE_INTEGER time;
  QueryPerformanceCounter(&time);
  w->dt = ((double) time.QuadPart - (double) w->time.QuadPart)
    * 1000
    / (double) w->performance_frequency.QuadPart;
  w->time = time;


  return false;
}

WINDOW_DEF bool window_get_mouse_position(Window *w, int *width, int *height) {
  if(GetCursorPos(&w->point) && ScreenToClient(w->hwnd, &w->point)) {
    *width = w->point.x;
    *height = w->point.y;
    return true;
  }

  return false;
}
  
WINDOW_DEF void window_swap_buffers(Window *w) {   
  SwapBuffers(w->dc);
}

WINDOW_DEF bool window_toggle_fullscreen(Window *w) {

  DWORD style = (DWORD) GetWindowLongPtr(w->hwnd, GWL_STYLE);

  if(w->running & WINDOW_FULLSCREEN) {

    style &= ~WS_POPUP;
    style |= WINDOW_STYLE; // fix this
    if(!(w->running & WINDOW_NOT_RESIZABLE)) {
      style |= WINDOW_RESIZABLE_STYLE;
    }
    w->running &= ~WINDOW_FULLSCREEN;
    
    SetWindowPos(w->hwnd, NULL,
		 w->rect.left,
		 w->rect.top,
		 w->rect.right - w->rect.left,
		 w->rect.bottom - w->rect.top,
		 SWP_FRAMECHANGED);    
  } else {    
    if(!GetWindowRect(w->hwnd, &w->rect)) {
      return false;
    }

    int width = GetSystemMetrics(SM_CXSCREEN);
    int height = GetSystemMetrics(SM_CYSCREEN);

    style &= ~WINDOW_STYLE;
    if(!(w->running & WINDOW_NOT_RESIZABLE)) {
      style &= ~WINDOW_RESIZABLE_STYLE;
    }
    style |= WS_POPUP;
    w->running |= WINDOW_FULLSCREEN;
    w->width = width;
    w->height = height;

    SetWindowPos(w->hwnd, HWND_TOP, 0, 0, width, height, SWP_FRAMECHANGED);
  }

  SetWindowLongPtr(w->hwnd, GWL_STYLE, style);

  return true;
}

WINDOW_DEF void window_free(Window *w) {	
  ReleaseDC(w->hwnd, w->dc);
  DestroyWindow(w->hwnd);
}

WINDOW_DEF bool window_show_cursor(Window *w, bool show) {
  (void) w;

  CURSORINFO info;
  info.cbSize = sizeof(CURSORINFO);
  if(!GetCursorInfo(&info)) {
    return false;
  }

  bool visible = info.flags;
  if(show && !visible) {
    ShowCursor(true);
  } else if(!show && visible){
    ShowCursor(false);
  }

  return true;
}

WINDOW_DEF bool window_dragged_files_init(Window_Dragged_Files *files, Window_Event *event) {
  files->h_drop = (HDROP) event->as.value;
  files->count = DragQueryFile(files->h_drop, 0xffffffff, files->path, MAX_PATH);
  if(files->count <= 0) {
    return false;
  }
  files->index = 0;

  return true;
}

WINDOW_DEF bool window_dragged_files_next(Window_Dragged_Files *files, char **path) {
  if(files->index >= files->count) {
    return false;
  }
  DragQueryFile(files->h_drop, files->index++, files->path, MAX_PATH);
  *path = files->path;

  return true;
}

WINDOW_DEF void window_dragged_files_free(Window_Dragged_Files *files) {
  DragFinish(files->h_drop);
}

WINDOW_DEF const char *window_shader_type_name(GLenum shader) {
  switch (shader) {
  case GL_VERTEX_SHADER:
    return "GL_VERTEX_SHADER";
  case GL_FRAGMENT_SHADER:
    return "GL_FRAGMENT_SHADER";
  default:
    return "(Unknown)";
  }
}

WINDOW_DEF bool window_compile_shader(GLuint *shader, GLenum shader_type, const char *shader_source) {
  *shader = glCreateShader(shader_type);
  glShaderSource(*shader, 1, &shader_source, NULL);
  glCompileShader(*shader);

  GLint compiled = 0;
  glGetShaderiv(*shader, GL_COMPILE_STATUS, &compiled);

  if (!compiled) {
    GLchar message[1024];
    GLsizei message_size = 0;
    glGetShaderInfoLog(*shader, sizeof(message), &message_size, message);
    WINDOW_LOG("ERROR: could not compile %s\n", window_shader_type_name(shader_type));
    WINDOW_LOG("%.*s\n", message_size, message);
    return false;
  }

  return true;
}

WINDOW_DEF bool window_link_program(GLuint *program, GLuint vertex_shader, GLuint fragment_shader) {
  *program = glCreateProgram();
  glAttachShader(*program, vertex_shader);
  glAttachShader(*program, fragment_shader);

  glLinkProgram(*program);
  
  GLint linked = 0;
  glGetProgramiv(*program, GL_LINK_STATUS, &linked);
  if(!linked) {
    GLsizei message_size = 0;
    GLchar message[1024];

    glGetProgramInfoLog(*program, sizeof(message), &message_size, message);
    WINDOW_LOG("ERROR: Program Linking: %.*s\n", message_size, message);
    return false;
  }
  
  return true;
    
}

////////////////////////////////////////////////////////////////////////
// renderer - definitions
////////////////////////////////////////////////////////////////////////

#ifndef WINDOW_NO_RENDERER

static const char* window_renderer_vertex_shader_source =
  "#version 330 core\n"
  "\n"
  "layout(location = 0) in vec2 position;\n"
  "layout(location = 1) in vec4 color;\n"
  "layout(location = 2) in vec2 uv;\n"
  "\n"
  "uniform float resolution_x;\n"
  "uniform float resolution_y;\n"
  "\n"
  "out vec4 out_color;\n"
  "out vec2 out_uv;\n"
  "\n"
  "vec2 resolution_project(vec2 point) {\n"
  "    return 2 * point / vec2(resolution_x, resolution_y) - 1;\n"
  "}\n"
  "\n"
  "void main() {\n"
  "  out_color = color;\n"
  "  out_uv = uv;\n"
  "  gl_Position = vec4(resolution_project(position), 0, 1);\n"
  "}";

static const char *window_renderer_fragment_shader_source=
  "#version 330 core\n"
  "\n"
  "uniform sampler2D font_tex;\n"
  "uniform sampler2D tex;\n"
  "\n"
  "in vec4 out_color;\n"
  "in vec2 out_uv;\n"
  "\n"
  "out vec4 fragColor;\n"
  "\n"
  "void main() {\n"
  "    if(out_uv.x < 0 && out_uv.y < 0) {\n"
  "        fragColor = out_color;\n"
  "    } else if(out_color.w < 0) {\n"
  "        vec4 color = texture(font_tex, vec2(out_uv.x, 1-out_uv.y));\n"
  "        float a = color.w * -out_color.w;\n"
  "        color = (color + vec4(1, 1, 1, 0)) * out_color;\n"
  "        color.w = a;\n"
  "        fragColor = color;\n"
  "    } else {\n"
  "        vec4 color = texture(tex, vec2(out_uv.x, 1-out_uv.y));\n"
  "        color = color * out_color;\n"
  "        fragColor = color;\n"
  "    }\n"
  "}\n";

WINDOW_DEF Window_Renderer_Vec2f window_renderer_vec2f(float x, float y) {
  return (Window_Renderer_Vec2f) { x, y};
}

WINDOW_DEF Window_Renderer_Vec4f window_renderer_vec4f(float x, float y, float z, float w) {
  return (Window_Renderer_Vec4f) { x, y, z, w};
}

WINDOW_DEF bool window_renderer_init(Window_Renderer *r) {
  (void) WHITE;
  (void) RED;
  (void) BLUE;
  (void) GREEN;
  (void) BLACK;
    
  // introduce 'verticies' to opengl
  glGenVertexArrays(1, &r->vao);
  glBindVertexArray(r->vao);

  glGenBuffers(1, &r->vbo);
  glBindBuffer(GL_ARRAY_BUFFER, r->vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(r->verticies), r->verticies, GL_DYNAMIC_DRAW);

  // introduce 'Vertex' to opengl
  glEnableVertexAttribArray(WINDOW_RENDERER_VERTEX_ATTR_POSITION);
  glVertexAttribPointer(WINDOW_RENDERER_VERTEX_ATTR_POSITION,
			sizeof(Window_Renderer_Vec2f)/sizeof(float),
			GL_FLOAT,
			GL_FALSE,
			sizeof(Window_Renderer_Vertex),
			(GLvoid *) offsetof(Window_Renderer_Vertex, position));

  glEnableVertexAttribArray(WINDOW_RENDERER_VERTEX_ATTR_COLOR);
  glVertexAttribPointer(WINDOW_RENDERER_VERTEX_ATTR_COLOR,
			sizeof(Window_Renderer_Vec4f)/sizeof(float),
			GL_FLOAT,
			GL_FALSE,
			sizeof(Window_Renderer_Vertex),
			(GLvoid *) offsetof(Window_Renderer_Vertex, color));

  glEnableVertexAttribArray(WINDOW_RENDERER_VERTEX_ATTR_UV);
  glVertexAttribPointer(WINDOW_RENDERER_VERTEX_ATTR_UV,
			sizeof(Window_Renderer_Vec2f)/sizeof(float),
			GL_FLOAT,
			GL_FALSE,
			sizeof(Window_Renderer_Vertex),
			(GLvoid *) offsetof(Window_Renderer_Vertex, uv));
  
  // compile shaders
  if(!window_compile_shader(&r->vertex_shader, GL_VERTEX_SHADER, window_renderer_vertex_shader_source)) {
    return false;
  }
    
  if(!window_compile_shader(&r->fragment_shader, GL_FRAGMENT_SHADER, window_renderer_fragment_shader_source)) {
    return false;
  }

  // link program
  if(!window_link_program(&r->program, r->vertex_shader, r->fragment_shader)) {
    return false;
  }
  glUseProgram(r->program);


  r->images_count = 0;
  r->verticies_count = 0;
  r->font_index = -1;
  
  return true;
}

WINDOW_DEF void window_renderer_begin(int width, int height) {

  Window_Renderer *r = &window_renderer;

  if(width > 0 && height > 0) {

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    
    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT);
    Window_Renderer_Vec4f *c = &r->background;
    glClearColor(c->x, c->y, c->z, c->w);

    // tell vertex shader what is the resolution is
    glUniform1fv(glGetUniformLocation(r->program, "resolution_x"), 1, &r->width);
    glUniform1fv(glGetUniformLocation(r->program, "resolution_y"), 1, &r->height);

    r->width = (float) width;
    r->height = (float) height;
  }


  if(r->font_index > 0) {
    GLint uniformLocation1 = glGetUniformLocation(r->program, "font_tex");
    glUniform1i(uniformLocation1, r->font_index);	
  }

  r->tex_index = -1;
}

WINDOW_DEF void window_renderer_set_color(Window_Renderer_Vec4f color) {
  Window_Renderer *r = &window_renderer;
  r->background = color;
}

WINDOW_DEF void window_renderer_vertex(Window_Renderer_Vec2f p, Window_Renderer_Vec4f c, Window_Renderer_Vec2f uv) {

  Window_Renderer *r = &window_renderer;

  if(r->verticies_count < WINDOW_RENDERER_CAP) {
    Window_Renderer_Vertex *last = &r->verticies[r->verticies_count];
    last->position = p;
    last->color = c;
    last->uv = uv;
    r->verticies_count++;	
  } 
}

WINDOW_DEF void window_renderer_triangle(Window_Renderer_Vec2f p1, Window_Renderer_Vec2f p2, Window_Renderer_Vec2f p3, Window_Renderer_Vec4f c1, Window_Renderer_Vec4f c2, Window_Renderer_Vec4f c3, Window_Renderer_Vec2f uv1, Window_Renderer_Vec2f uv2, Window_Renderer_Vec2f uv3) {

  Window_Renderer *r = &window_renderer;

  if(r->verticies_count + 3 >= WINDOW_RENDERER_CAP) {
    window_renderer_end();
  }
    
  window_renderer_vertex(p1, c1, uv1);
  window_renderer_vertex(p2, c2, uv2);
  window_renderer_vertex(p3, c3, uv3);	
}

WINDOW_DEF void window_renderer_solid_triangle(Window_Renderer_Vec2f p1, Window_Renderer_Vec2f p2, Window_Renderer_Vec2f p3, Window_Renderer_Vec4f c) {

  Window_Renderer *r = &window_renderer;

  if(r->verticies_count + 3 >= WINDOW_RENDERER_CAP) {
    window_renderer_end();
  }
	
  Window_Renderer_Vec2f uv = window_renderer_vec2f(-1, -1);
  window_renderer_vertex(p1, c, uv);
  window_renderer_vertex(p2, c, uv);
  window_renderer_vertex(p3, c, uv);
}

WINDOW_DEF void window_renderer_quad(Window_Renderer_Vec2f p1, Window_Renderer_Vec2f p2, Window_Renderer_Vec2f p3, Window_Renderer_Vec2f p4, Window_Renderer_Vec4f c1, Window_Renderer_Vec4f c2, Window_Renderer_Vec4f c3, Window_Renderer_Vec4f c4, Window_Renderer_Vec2f uv1, Window_Renderer_Vec2f uv2, Window_Renderer_Vec2f uv3, Window_Renderer_Vec2f uv4) {
  window_renderer_triangle(p1, p2, p4, c1, c2, c4, uv1, uv2, uv4);
  window_renderer_triangle(p1, p3, p4, c1, c3, c4, uv1, uv3, uv4);
}

WINDOW_DEF void window_renderer_solid_rect(Window_Renderer_Vec2f pos, Window_Renderer_Vec2f size, Window_Renderer_Vec4f color) {
  Vec2f uv = vec2f(-1, -1);
  window_renderer_quad(pos,
		       window_renderer_vec2f(pos.x + size.x, pos.y),
		       window_renderer_vec2f(pos.x, pos.y + size.y),
		       window_renderer_vec2f(pos.x + size.x, pos.y + size.y),
		       color, color, color, color, uv, uv, uv, uv);
}

WINDOW_DEF void window_renderer_solid_rounded_rect(Vec2f pos, Vec2f size, float radius, int parts, Vec4f color) {
  if(size.y < 4 * radius) radius = size.y / 4; 

  window_renderer_solid_rect(vec2f(pos.x, pos.y + radius),
		  vec2f(size.x, size.y - 2 * radius),
		  color);

  window_renderer_solid_rect(vec2f(pos.x + radius, pos.y),
		  vec2f(size.x - 2 * radius, size.y),
		  color);

  pos = vec2f(pos.x + radius, pos.y + radius);
  size = vec2f(size.x - 2 * radius, size.y - 2 * radius);

  //bottom left
  window_renderer_solid_circle(pos,
		    PI, PI * 3 / 2,
		    radius,
		    parts,
		    color);

  //bottom right
  window_renderer_solid_circle(vec2f(pos.x + size.x, pos.y),
		    PI * 3 / 2, 2 * PI,
		    radius,
		    parts,
		    color);

  //top left
  window_renderer_solid_circle(vec2f(pos.x, pos.y + size.y),
		    PI / 2, PI,
		    radius,
		    parts,
		    color);

  //top right
  window_renderer_solid_circle(vec2f(pos.x + size.x, pos.y + size.y),
		    0, PI /2,
		    radius,
		    parts,
		    color);  
}

WINDOW_DEF void window_renderer_solid_rounded_shaded_rect(Window_Renderer_Vec2f pos,
							  Window_Renderer_Vec2f size,
							  float radius,
							  int parts,
							  float shade_px,
							  Window_Renderer_Vec4f color) {
  window_renderer_solid_rounded_rect(vec2f(pos.x - shade_px, pos.y - shade_px),
				     vec2f(size.x + 2 *shade_px, size.y + 2 *shade_px),
				     radius, parts, vec4f(0, 0, 0, color.w * .5));
  window_renderer_solid_rounded_rect(pos, size, radius, parts, color);
}

WINDOW_DEF void window_renderer_solid_rect_angle(Window_Renderer_Vec2f pos, Window_Renderer_Vec2f size,
						 float angle,
						 Window_Renderer_Vec4f color) {
  Vec2f uv = vec2f(-1, -1);

  float s = sinf(angle);
  float c = cosf(angle);

  float dy1 = s * size.x;
  float dx1 = c * size.x;
  
  float dy2 = c * size.y;
  float dx2 = s * size.y;

  window_renderer_triangle(pos,
			   window_renderer_vec2f(pos.x + dx1, pos.y + dy1),
			   window_renderer_vec2f(pos.x - dx2, pos.y + dy2),
			   color, color, color, uv, uv, uv);

  window_renderer_triangle(window_renderer_vec2f(pos.x + dx1, pos.y + dy1),
			   window_renderer_vec2f(pos.x - dx2, pos.y + dy2),
			   window_renderer_vec2f(pos.x + dx1 - dx2, pos.y + dy1 + dy2),
			   color, color, color, uv, uv, uv);
}

WINDOW_DEF void window_renderer_texture(unsigned int texture,
					Window_Renderer_Vec2f p, Window_Renderer_Vec2f s,
					Window_Renderer_Vec2f uvp, Window_Renderer_Vec2f uvs) {

  Window_Renderer *r = &window_renderer;

  if(r->tex_index != -1) {
    window_renderer_end();
  }

  r->tex_index = (int) texture;
  GLint uniformLocation1 = glGetUniformLocation(r->program, "tex");
  glUniform1i(uniformLocation1, r->tex_index);
    
  Vec4f c = vec4f(1, 1, 1, 1);
  window_renderer_quad(p,
		       window_renderer_vec2f(p.x + s.x, p.y),
		       window_renderer_vec2f(p.x, p.y + s.y),
		       window_renderer_vec2f(p.x + s.x, p.y + s.y),
		       c, c, c, c,
		       uvp,
		       window_renderer_vec2f(uvp.x + uvs.x, uvp.y),
		       window_renderer_vec2f(uvp.x, uvp.y + uvs.y),
		       window_renderer_vec2f(uvp.x + uvs.x, uvp.y + uvs.y));
}

WINDOW_DEF void window_renderer_texture_colored(unsigned int texture,
						Window_Renderer_Vec2f p, Window_Renderer_Vec2f s,
					        Window_Renderer_Vec2f uvp, Window_Renderer_Vec2f uvs,
						Window_Renderer_Vec4f c) {
  Window_Renderer *r = &window_renderer;
  
  if(r->tex_index != -1) {
    window_renderer_end();
  }

  r->tex_index = (int) texture;
  GLint uniformLocation1 = glGetUniformLocation(r->program, "tex");
  glUniform1i(uniformLocation1, r->tex_index);
  
  window_renderer_quad(
		       p,
		       window_renderer_vec2f(p.x + s.x, p.y),
		       window_renderer_vec2f(p.x, p.y + s.y),
		       window_renderer_vec2f(p.x + s.x, p.y + s.y),
		       c, c, c, c,
		       uvp,
		       window_renderer_vec2f(uvp.x + uvs.x, uvp.y),
		       window_renderer_vec2f(uvp.x, uvp.y + uvs.y),
		       window_renderer_vec2f(uvp.x + uvs.x, uvp.y + uvs.y));
}


WINDOW_DEF void window_renderer_solid_circle(Window_Renderer_Vec2f pos,
					     float start_angle, float end_angle,
					     float radius,
					     int parts,
					     Window_Renderer_Vec4f color) {
  float P = end_angle - start_angle;
  float A = start_angle;

  Window_Renderer_Vec2f old = {radius * cosf(A),
			       radius * sinf(A)};

  float t = 0.0f;
  float dt = 1.f / (float) parts;

  for(int j=1;j<=parts;j++) {
    t += dt;
    Window_Renderer_Vec2f new = {radius * cosf(A + P * t),
				 radius * sinf(A + P * t)};
    window_renderer_solid_triangle(pos,
				   window_renderer_vec2f(pos.x + new.x, pos.y + new.y),
				   window_renderer_vec2f(pos.x + old.x, pos.y + old.y),
				   color);
    old = new;
  }

  
}

WINDOW_DEF bool window_renderer_push_texture(int width, int height, const void *data, bool grey, unsigned int *index) {

  Window_Renderer *r = &window_renderer;

  GLenum current_texture;
  switch(r->images_count) {
  case 0:
    current_texture = GL_TEXTURE0;
    break;
  case 1:
    current_texture = GL_TEXTURE1;
    break;
  case 2:
    current_texture = GL_TEXTURE2;
    break;
  case 3:
    current_texture = GL_TEXTURE3;
    break;
  default:
    return false;
  }
  
  glActiveTexture(current_texture);
  
  glGenTextures(1, &r->textures);
  glBindTexture(GL_TEXTURE_2D, r->textures);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  if(grey) {
    glTexImage2D(GL_TEXTURE_2D,
		 0,
		 GL_ALPHA,
		 width,
		 height,
		 0,
		 GL_ALPHA,
		 GL_UNSIGNED_BYTE,
		 data);	
  } else {
    glTexImage2D(GL_TEXTURE_2D,
		 0,
		 GL_RGBA,
		 width,
		 height,
		 0,
		 GL_RGBA,
		 GL_UNSIGNED_INT_8_8_8_8_REV,
		 data);
  }

  *index = r->images_count++;

  return true;
}

WINDOW_DEF void window_renderer_end() {

  Window_Renderer *r = &window_renderer;
  
  glBufferSubData(GL_ARRAY_BUFFER, 0, r->verticies_count * sizeof(Window_Renderer_Vertex), r->verticies);
  glDrawArrays(GL_TRIANGLES, 0, r->verticies_count);
  r->verticies_count = 0;
}

WINDOW_DEF void window_renderer_free(Window_Renderer *r) {
  (void) r;
}

#ifdef __STB_INCLUDE_STB_TRUETYPE_H__
#include <stdio.h>

#define WINDOW_RENDERER_STB_TEMP_BITMAP_SIZE 1024

WINDOW_DEF bool window_renderer_push_font(const char *filepath, float pixel_height) {


  FILE *f = fopen(filepath, "rb");
  if(!f) {
    WINDOW_LOG("ERROR: Can not open file: %s\n", filepath);
    return false;
  }

  if(fseek(f, 0, SEEK_END) < 0) {
    WINDOW_LOG("ERROR: Can not seek in file: %s\n", filepath);
    fclose(f);
    return false;
  }

  long m = ftell(f);
  if(m < 0) {
    WINDOW_LOG("ERROR: Can not read file(ftell): %s\n", filepath);
    fclose(f);
    return false;
  }  

  if(fseek(f, 0, SEEK_SET) < 0) {
    WINDOW_LOG("ERROR: Can not seek in file: %s\n", filepath);
    fclose(f);
    return false;
  }

  unsigned char *buffer = (unsigned char *) malloc((size_t) m);
  if(!buffer) {
    WINDOW_LOG("ERROR: Can not allocate enough memory\n");
    fclose(f);
    return false;
  }

  size_t _m = (size_t) m;
  size_t n = fread(buffer, 1, _m, f);
  if(n != _m) {
    WINDOW_LOG("ERROR: Failed to read file(fread): %s\n", filepath);
    free(buffer);
    fclose(f);
    return false;
  }
    
  fclose(f);

  unsigned char *temp_bitmap = malloc(WINDOW_RENDERER_STB_TEMP_BITMAP_SIZE *
				      WINDOW_RENDERER_STB_TEMP_BITMAP_SIZE);
  
  Window_Renderer *r = &window_renderer;
  stbtt_BakeFontBitmap(buffer,0, pixel_height,
		       temp_bitmap,
		       WINDOW_RENDERER_STB_TEMP_BITMAP_SIZE,
		       WINDOW_RENDERER_STB_TEMP_BITMAP_SIZE,
		       32, 96, r->font_cdata);

  unsigned int tex;
  bool result = push_texture(1024, 1024, temp_bitmap, true, &tex);

  r->font_index = (int) tex;

  free(buffer);
  free(temp_bitmap);
    
  return result;
}

WINDOW_DEF void window_renderer_text(const char *cstr, size_t cstr_len, Window_Renderer_Vec2f pos, float factor, Window_Renderer_Vec4f color) {

  Window_Renderer *r = &window_renderer;

  float x = 0;
  float y = 0;
  color.w *= -1;
    
  for(size_t i=0;i<cstr_len;i++) {
    unsigned char c = cstr[i];
    if (c < 32 && c >= 128) {
      continue;
    }

    float _y = y;

    stbtt_aligned_quad q;
    stbtt_GetBakedQuad(r->font_cdata,
		       WINDOW_RENDERER_STB_TEMP_BITMAP_SIZE,
		       WINDOW_RENDERER_STB_TEMP_BITMAP_SIZE, c-32, &x, &y, &q,1);
    //1=opengl & d3d10+,0=d3d9

    Window_Renderer_Vec2f p = vec2f(pos.x + q.x0 * factor,pos.y + y + _y + factor * (y - q.y1) );
    Window_Renderer_Vec2f s = vec2f((q.x1 - q.x0) * factor, (q.y1 - q.y0) * factor);
    Window_Renderer_Vec2f uvp = vec2f(q.s0, 1 - q.t1);
    Window_Renderer_Vec2f uvs = vec2f(q.s1 - q.s0, q.t1 - q.t0);	

	
    window_renderer_quad(
			 p,
			 window_renderer_vec2f(p.x + s.x, p.y),
			 window_renderer_vec2f(p.x, p.y + s.y),
			 window_renderer_vec2f(p.x + s.x, p.y + s.y),
			 color, color, color, color,
			 uvp,
			 window_renderer_vec2f(uvp.x + uvs.x, uvp.y),
			 window_renderer_vec2f(uvp.x, uvp.y + uvs.y),
			 window_renderer_vec2f(uvp.x + uvs.x, uvp.y + uvs.y));       
  }

}

WINDOW_DEF void window_renderer_measure_text(const char *cstr, size_t cstr_len, float factor, Vec2f *size) {
  Window_Renderer *r = &window_renderer;

  size->y = 0;
  size->x = 0;

  float x = 0;
  float y = 0;
    
  for(size_t i=0;i<cstr_len;i++) {
    unsigned char c = cstr[i];
    if (c < 32 && c >= 128) {
      continue;
    }

    float _y = y;

    stbtt_aligned_quad q;
    stbtt_GetBakedQuad(r->font_cdata,
		       WINDOW_RENDERER_STB_TEMP_BITMAP_SIZE,
		       WINDOW_RENDERER_STB_TEMP_BITMAP_SIZE,
		       c-32, &x, &y, &q, 1);

    size->x = q.y1 - q.y0;
    float height = q.x1 - q.x0;
    if(height > size->y) size->y = height;
  }
  size->x = x * factor;
  size->y *= factor;
}

#endif //__STB_INCLUDE_STB_TRUETYPE_H__

#endif //WINDOW_NO_RENDERER

////////////////////////////////////////////////////////////////////////
// opengl - definitions
////////////////////////////////////////////////////////////////////////

PROC _glActiveTexture = NULL;
void glActiveTexture(GLenum texture) { _glActiveTexture(texture); }

PROC _glGenVertexArrays = NULL;
void glGenVertexArrays(GLsizei n, GLuint *arrays) { _glGenVertexArrays(n, arrays); }

PROC _glBindVertexArray = NULL;
void glBindVertexArray(GLuint array) { _glBindVertexArray(array); }

PROC _glGenBuffers = NULL;
void glGenBuffers(GLsizei n, GLuint *buffers) { _glGenBuffers(n, buffers); }

PROC _glBindBuffer = NULL;
void glBindBuffer(GLenum target, GLuint buffer) { _glBindBuffer(target, buffer); }

PROC _glBufferData = NULL;
void glBufferData(GLenum target, GLsizeiptr size, const void * data, GLenum usage) { _glBufferData(target, size, data, usage); }

PROC _glEnableVertexAttribArray = NULL;
void glEnableVertexAttribArray(GLuint index) { _glEnableVertexAttribArray(index); }

PROC _glVertexAttribPointer = NULL;
void glVertexAttribPointer(GLuint index,
			   GLint size,
			   GLenum type,
			   GLboolean normalized,
			   GLsizei stride,
			   const void * pointer) {
  _glVertexAttribPointer(index, size, type, normalized, stride, pointer);
}

PROC _glCreateShader = NULL;
GLuint glCreateShader(GLenum shaderType) { return (GLuint) _glCreateShader(shaderType); }

PROC _glShaderSource = NULL;
void glShaderSource(GLuint shader,
		    GLsizei count,
		    const GLchar **_string,
		    const GLint *length) {
  _glShaderSource(shader, count, _string, length);
}

PROC _glCompileShader = NULL;
void glCompileShader(GLuint shader) { _glCompileShader(shader); }

PROC _glGetShaderiv = NULL;
void glGetShaderiv(GLuint shader, GLenum pname, GLint *params) {
  _glGetShaderiv(shader, pname, params);
}

PROC _glGetShaderInfoLog = NULL;
void glGetShaderInfoLog(GLuint shader,
			GLsizei maxLength,
			GLsizei *length,
			GLchar *infoLog) {
  _glGetShaderInfoLog(shader, maxLength, length, infoLog);
}

PROC _glCreateProgram = NULL;
GLuint glCreateProgram(void) { return (GLuint) _glCreateProgram(); }

PROC _glAttachShader = NULL;
void glAttachShader(GLuint program, GLuint shader) { _glAttachShader(program, shader); }

PROC _glLinkProgram = NULL;
void glLinkProgram(GLuint program) { _glLinkProgram(program); }

PROC _glGetProgramInfoLog = NULL;
void glGetProgramInfoLog(GLuint program,
			 GLsizei maxLength,
			 GLsizei *length,
			 GLchar *infoLog) {
  _glGetProgramInfoLog(program, maxLength, length, infoLog);
}

PROC _glGetProgramiv = NULL;
void glGetProgramiv(GLuint program,
		    GLenum pname,
		    GLint *params) {
  _glGetProgramiv(program, pname, params);
}

PROC _glUseProgram = NULL;
void glUseProgram(GLuint program) { _glUseProgram(program); }

PROC _glBufferSubData = NULL;
void glBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const void * data) {
  _glBufferSubData(target, offset, size, data);
}

PROC _glUniform2f = NULL;
void glUniform2f(GLint location, GLfloat v0, GLfloat v1) {
  _glUniform2f(location, v0, v1);
}

PROC _glUniform1f = NULL;
void glUniform1f(GLint location, GLfloat v0) {
  _glUniform1f(location, v0);
}

PROC _glUniform1fv = NULL;
void glUniform1fv(GLint location, GLsizei count, const GLfloat *value) {
  _glUniform1fv(location, count, value);
}

PROC _glUniform2fv = NULL;
void glUniform2fv(GLint location, GLsizei count, const GLfloat *value) {
  _glUniform2fv(location, count, value);
}

PROC _glUniform1i = NULL;
void glUniform1i(GLint location, GLint v0) {
  _glUniform1i(location, v0);
}

PROC _glGetUniformLocation = NULL;
GLint glGetUniformLocation(GLuint program, const GLchar *name) { return (GLint) _glGetUniformLocation(program, name); }

PROC _glGetActiveUniform = NULL;
void glGetActiveUniform(GLuint program,
			GLuint index,
			GLsizei bufSize,
			GLsizei *length,
			GLint *size,
			GLenum *type,
			GLchar *name) {
  _glGetActiveUniform(program, index, bufSize, length, size, type, name);
}

PROC _glGetUniformfv = NULL;
void glGetUniformfv(GLuint program, GLint location, GLfloat *params) {
  _glGetUniformfv(program, location, params);
}

PROC _glGetUniformiv = NULL;
void glGetUniformiv(GLuint program, GLint location, GLsizei bufSize, GLint *params) {
  _glGetUniformiv(program, location, bufSize, params);
}

PROC _wglSwapIntervalEXT = NULL;
int wglSwapIntervalEXT(GLint interval) {
  return (int) _wglSwapIntervalEXT(interval);
}

PROC _glSampleCoverage = NULL;
void glSampleCoverage(GLfloat value, GLboolean invert) {
  _glSampleCoverage(value, invert);
}


WINDOW_DEF void window_win32_opengl_init() {
  if(_glActiveTexture != NULL) {
    return;
  }
  
  _glActiveTexture = wglGetProcAddress("glActiveTexture");
  _glGenVertexArrays = wglGetProcAddress("glGenVertexArrays");
  _glBindVertexArray= wglGetProcAddress("glBindVertexArray");
  _glGenBuffers= wglGetProcAddress("glGenBuffers");
  _glBindBuffer= wglGetProcAddress("glBindBuffer");
  _glBufferData= wglGetProcAddress("glBufferData");
  _glEnableVertexAttribArray= wglGetProcAddress("glEnableVertexAttribArray");
  _glVertexAttribPointer= wglGetProcAddress("glVertexAttribPointer");
  _glCreateShader= wglGetProcAddress("glCreateShader");
  _glShaderSource= wglGetProcAddress("glShaderSource");
  _glCompileShader= wglGetProcAddress("glCompileShader");
  _glGetShaderiv= wglGetProcAddress("glGetShaderiv");
  _glGetShaderInfoLog= wglGetProcAddress("glGetShaderInfoLog");
  _glCreateProgram= wglGetProcAddress("glCreateProgram");
  _glAttachShader= wglGetProcAddress("glAttachShader");
  _glLinkProgram= wglGetProcAddress("glLinkProgram");
  _glGetProgramInfoLog= wglGetProcAddress("glGetProgramInfoLog");
  _glGetProgramiv= wglGetProcAddress("glGetProgramiv");
  _glUseProgram= wglGetProcAddress("glUseProgram");
  _glBufferSubData= wglGetProcAddress("glBufferSubData");
  _glUniform2f= wglGetProcAddress("glUniform2f");
  _glUniform1f= wglGetProcAddress("glUniform1f");
  _glUniform1i= wglGetProcAddress("glUniform1i");
  _glGetUniformLocation= wglGetProcAddress("glGetUniformLocation");
  _glGetActiveUniform= wglGetProcAddress("glGetActiveUniform");
  _glGetUniformfv= wglGetProcAddress("glGetUniformfv");
  _glUniform1fv= wglGetProcAddress("glUniform1fv");
  _glUniform2fv= wglGetProcAddress("glUniform2fv");
  _glGetUniformiv= wglGetProcAddress("glGetUniformiv");
  _wglSwapIntervalEXT = wglGetProcAddress("wglSwapIntervalEXT");
}

#endif //WINDOW_IMPLEMENTATION

#endif //WINDOW_H
/* stb_image - v2.27 - public domain image loader - http://nothings.org/stb
                                  no warranty implied; use at your own risk

   Do this:
      #define STB_IMAGE_IMPLEMENTATION
   before you include this file in *one* C or C++ file to create the implementation.

   // i.e. it should look like this:
   #include ...
   #include ...
   #include ...
   #define STB_IMAGE_IMPLEMENTATION
   #include "stb_image.h"

   You can #define STBI_ASSERT(x) before the #include to avoid using assert.h.
   And #define STBI_MALLOC, STBI_REALLOC, and STBI_FREE to avoid using malloc,realloc,free


   QUICK NOTES:
      Primarily of interest to game developers and other people who can
          avoid problematic images and only need the trivial interface

      JPEG baseline & progressive (12 bpc/arithmetic not supported, same as stock IJG lib)
      PNG 1/2/4/8/16-bit-per-channel

      TGA (not sure what subset, if a subset)
      BMP non-1bpp, non-RLE
      PSD (composited view only, no extra channels, 8/16 bit-per-channel)

      GIF (*comp always reports as 4-channel)
      HDR (radiance rgbE format)
      PIC (Softimage PIC)
      PNM (PPM and PGM binary only)

      Animated GIF still needs a proper API, but here's one way to do it:
          http://gist.github.com/urraka/685d9a6340b26b830d49

      - decode from memory or through FILE (define STBI_NO_STDIO to remove code)
      - decode from arbitrary I/O callbacks
      - SIMD acceleration on x86/x64 (SSE2) and ARM (NEON)

   Full documentation under "DOCUMENTATION" below.


LICENSE

  See end of file for license information.

RECENT REVISION HISTORY:

      2.27  (2021-07-11) document stbi_info better, 16-bit PNM support, bug fixes
      2.26  (2020-07-13) many minor fixes
      2.25  (2020-02-02) fix warnings
      2.24  (2020-02-02) fix warnings; thread-local failure_reason and flip_vertically
      2.23  (2019-08-11) fix clang static analysis warning
      2.22  (2019-03-04) gif fixes, fix warnings
      2.21  (2019-02-25) fix typo in comment
      2.20  (2019-02-07) support utf8 filenames in Windows; fix warnings and platform ifdefs
      2.19  (2018-02-11) fix warning
      2.18  (2018-01-30) fix warnings
      2.17  (2018-01-29) bugfix, 1-bit BMP, 16-bitness query, fix warnings
      2.16  (2017-07-23) all functions have 16-bit variants; optimizations; bugfixes
      2.15  (2017-03-18) fix png-1,2,4; all Imagenet JPGs; no runtime SSE detection on GCC
      2.14  (2017-03-03) remove deprecated STBI_JPEG_OLD; fixes for Imagenet JPGs
      2.13  (2016-12-04) experimental 16-bit API, only for PNG so far; fixes
      2.12  (2016-04-02) fix typo in 2.11 PSD fix that caused crashes
      2.11  (2016-04-02) 16-bit PNGS; enable SSE2 in non-gcc x64
                         RGB-format JPEG; remove white matting in PSD;
                         allocate large structures on the stack;
                         correct channel count for PNG & BMP
      2.10  (2016-01-22) avoid warning introduced in 2.09
      2.09  (2016-01-16) 16-bit TGA; comments in PNM files; STBI_REALLOC_SIZED

   See end of file for full revision history.


 ============================    Contributors    =========================

 Image formats                          Extensions, features
    Sean Barrett (jpeg, png, bmp)          Jetro Lauha (stbi_info)
    Nicolas Schulz (hdr, psd)              Martin "SpartanJ" Golini (stbi_info)
    Jonathan Dummer (tga)                  James "moose2000" Brown (iPhone PNG)
    Jean-Marc Lienher (gif)                Ben "Disch" Wenger (io callbacks)
    Tom Seddon (pic)                       Omar Cornut (1/2/4-bit PNG)
    Thatcher Ulrich (psd)                  Nicolas Guillemot (vertical flip)
    Ken Miller (pgm, ppm)                  Richard Mitton (16-bit PSD)
    github:urraka (animated gif)           Junggon Kim (PNM comments)
    Christopher Forseth (animated gif)     Daniel Gibson (16-bit TGA)
                                           socks-the-fox (16-bit PNG)
                                           Jeremy Sawicki (handle all ImageNet JPGs)
 Optimizations & bugfixes                  Mikhail Morozov (1-bit BMP)
    Fabian "ryg" Giesen                    Anael Seghezzi (is-16-bit query)
    Arseny Kapoulkine                      Simon Breuss (16-bit PNM)
    John-Mark Allen
    Carmelo J Fdez-Aguera

 Bug & warning fixes
    Marc LeBlanc            David Woo          Guillaume George     Martins Mozeiko
    Christpher Lloyd        Jerry Jansson      Joseph Thomson       Blazej Dariusz Roszkowski
    Phil Jordan                                Dave Moore           Roy Eltham
    Hayaki Saito            Nathan Reed        Won Chun
    Luke Graham             Johan Duparc       Nick Verigakis       the Horde3D community
    Thomas Ruf              Ronny Chevalier                         github:rlyeh
    Janez Zemva             John Bartholomew   Michal Cichon        github:romigrou
    Jonathan Blow           Ken Hamada         Tero Hanninen        github:svdijk
    Eugene Golushkov        Laurent Gomila     Cort Stratton        github:snagar
    Aruelien Pocheville     Sergio Gonzalez    Thibault Reuille     github:Zelex
    Cass Everitt            Ryamond Barbiero                        github:grim210
    Paul Du Bois            Engin Manap        Aldo Culquicondor    github:sammyhw
    Philipp Wiesemann       Dale Weiler        Oriol Ferrer Mesia   github:phprus
    Josh Tobin                                 Matthew Gregan       github:poppolopoppo
    Julian Raschke          Gregory Mullen     Christian Floisand   github:darealshinji
    Baldur Karlsson         Kevin Schmidt      JR Smith             github:Michaelangel007
                            Brad Weinberger    Matvey Cherevko      github:mosra
    Luca Sas                Alexander Veselov  Zack Middleton       [reserved]
    Ryan C. Gordon          [reserved]                              [reserved]
                     DO NOT ADD YOUR NAME HERE

                     Jacko Dirks

  To add your name to the credits, pick a random blank space in the middle and fill it.
  80% of merge conflicts on stb PRs are due to people adding their name at the end
  of the credits.
*/

#ifndef STBI_INCLUDE_STB_IMAGE_H
#define STBI_INCLUDE_STB_IMAGE_H

// DOCUMENTATION
//
// Limitations:
//    - no 12-bit-per-channel JPEG
//    - no JPEGs with arithmetic coding
//    - GIF always returns *comp=4
//
// Basic usage (see HDR discussion below for HDR usage):
//    int x,y,n;
//    unsigned char *data = stbi_load(filename, &x, &y, &n, 0);
//    // ... process data if not NULL ...
//    // ... x = width, y = height, n = # 8-bit components per pixel ...
//    // ... replace '0' with '1'..'4' to force that many components per pixel
//    // ... but 'n' will always be the number that it would have been if you said 0
//    stbi_image_free(data)
//
// Standard parameters:
//    int *x                 -- outputs image width in pixels
//    int *y                 -- outputs image height in pixels
//    int *channels_in_file  -- outputs # of image components in image file
//    int desired_channels   -- if non-zero, # of image components requested in result
//
// The return value from an image loader is an 'unsigned char *' which points
// to the pixel data, or NULL on an allocation failure or if the image is
// corrupt or invalid. The pixel data consists of *y scanlines of *x pixels,
// with each pixel consisting of N interleaved 8-bit components; the first
// pixel pointed to is top-left-most in the image. There is no padding between
// image scanlines or between pixels, regardless of format. The number of
// components N is 'desired_channels' if desired_channels is non-zero, or
// *channels_in_file otherwise. If desired_channels is non-zero,
// *channels_in_file has the number of components that _would_ have been
// output otherwise. E.g. if you set desired_channels to 4, you will always
// get RGBA output, but you can check *channels_in_file to see if it's trivially
// opaque because e.g. there were only 3 channels in the source image.
//
// An output image with N components has the following components interleaved
// in this order in each pixel:
//
//     N=#comp     components
//       1           grey
//       2           grey, alpha
//       3           red, green, blue
//       4           red, green, blue, alpha
//
// If image loading fails for any reason, the return value will be NULL,
// and *x, *y, *channels_in_file will be unchanged. The function
// stbi_failure_reason() can be queried for an extremely brief, end-user
// unfriendly explanation of why the load failed. Define STBI_NO_FAILURE_STRINGS
// to avoid compiling these strings at all, and STBI_FAILURE_USERMSG to get slightly
// more user-friendly ones.
//
// Paletted PNG, BMP, GIF, and PIC images are automatically depalettized.
//
// To query the width, height and component count of an image without having to
// decode the full file, you can use the stbi_info family of functions:
//
//   int x,y,n,ok;
//   ok = stbi_info(filename, &x, &y, &n);
//   // returns ok=1 and sets x, y, n if image is a supported format,
//   // 0 otherwise.
//
// Note that stb_image pervasively uses ints in its public API for sizes,
// including sizes of memory buffers. This is now part of the API and thus
// hard to change without causing breakage. As a result, the various image
// loaders all have certain limits on image size; these differ somewhat
// by format but generally boil down to either just under 2GB or just under
// 1GB. When the decoded image would be larger than this, stb_image decoding
// will fail.
//
// Additionally, stb_image will reject image files that have any of their
// dimensions set to a larger value than the configurable STBI_MAX_DIMENSIONS,
// which defaults to 2**24 = 16777216 pixels. Due to the above memory limit,
// the only way to have an image with such dimensions load correctly
// is for it to have a rather extreme aspect ratio. Either way, the
// assumption here is that such larger images are likely to be malformed
// or malicious. If you do need to load an image with individual dimensions
// larger than that, and it still fits in the overall size limit, you can
// #define STBI_MAX_DIMENSIONS on your own to be something larger.
//
// ===========================================================================
//
// UNICODE:
//
//   If compiling for Windows and you wish to use Unicode filenames, compile
//   with
//       #define STBI_WINDOWS_UTF8
//   and pass utf8-encoded filenames. Call stbi_convert_wchar_to_utf8 to convert
//   Windows wchar_t filenames to utf8.
//
// ===========================================================================
//
// Philosophy
//
// stb libraries are designed with the following priorities:
//
//    1. easy to use
//    2. easy to maintain
//    3. good performance
//
// Sometimes I let "good performance" creep up in priority over "easy to maintain",
// and for best performance I may provide less-easy-to-use APIs that give higher
// performance, in addition to the easy-to-use ones. Nevertheless, it's important
// to keep in mind that from the standpoint of you, a client of this library,
// all you care about is #1 and #3, and stb libraries DO NOT emphasize #3 above all.
//
// Some secondary priorities arise directly from the first two, some of which
// provide more explicit reasons why performance can't be emphasized.
//
//    - Portable ("ease of use")
//    - Small source code footprint ("easy to maintain")
//    - No dependencies ("ease of use")
//
// ===========================================================================
//
// I/O callbacks
//
// I/O callbacks allow you to read from arbitrary sources, like packaged
// files or some other source. Data read from callbacks are processed
// through a small internal buffer (currently 128 bytes) to try to reduce
// overhead.
//
// The three functions you must define are "read" (reads some bytes of data),
// "skip" (skips some bytes of data), "eof" (reports if the stream is at the end).
//
// ===========================================================================
//
// SIMD support
//
// The JPEG decoder will try to automatically use SIMD kernels on x86 when
// supported by the compiler. For ARM Neon support, you must explicitly
// request it.
//
// (The old do-it-yourself SIMD API is no longer supported in the current
// code.)
//
// On x86, SSE2 will automatically be used when available based on a run-time
// test; if not, the generic C versions are used as a fall-back. On ARM targets,
// the typical path is to have separate builds for NEON and non-NEON devices
// (at least this is true for iOS and Android). Therefore, the NEON support is
// toggled by a build flag: define STBI_NEON to get NEON loops.
//
// If for some reason you do not want to use any of SIMD code, or if
// you have issues compiling it, you can disable it entirely by
// defining STBI_NO_SIMD.
//
// ===========================================================================
//
// HDR image support   (disable by defining STBI_NO_HDR)
//
// stb_image supports loading HDR images in general, and currently the Radiance
// .HDR file format specifically. You can still load any file through the existing
// interface; if you attempt to load an HDR file, it will be automatically remapped
// to LDR, assuming gamma 2.2 and an arbitrary scale factor defaulting to 1;
// both of these constants can be reconfigured through this interface:
//
//     stbi_hdr_to_ldr_gamma(2.2f);
//     stbi_hdr_to_ldr_scale(1.0f);
//
// (note, do not use _inverse_ constants; stbi_image will invert them
// appropriately).
//
// Additionally, there is a new, parallel interface for loading files as
// (linear) floats to preserve the full dynamic range:
//
//    float *data = stbi_loadf(filename, &x, &y, &n, 0);
//
// If you load LDR images through this interface, those images will
// be promoted to floating point values, run through the inverse of
// constants corresponding to the above:
//
//     stbi_ldr_to_hdr_scale(1.0f);
//     stbi_ldr_to_hdr_gamma(2.2f);
//
// Finally, given a filename (or an open file or memory block--see header
// file for details) containing image data, you can query for the "most
// appropriate" interface to use (that is, whether the image is HDR or
// not), using:
//
//     stbi_is_hdr(char *filename);
//
// ===========================================================================
//
// iPhone PNG support:
//
// We optionally support converting iPhone-formatted PNGs (which store
// premultiplied BGRA) back to RGB, even though they're internally encoded
// differently. To enable this conversion, call
// stbi_convert_iphone_png_to_rgb(1).
//
// Call stbi_set_unpremultiply_on_load(1) as well to force a divide per
// pixel to remove any premultiplied alpha *only* if the image file explicitly
// says there's premultiplied data (currently only happens in iPhone images,
// and only if iPhone convert-to-rgb processing is on).
//
// ===========================================================================
//
// ADDITIONAL CONFIGURATION
//
//  - You can suppress implementation of any of the decoders to reduce
//    your code footprint by #defining one or more of the following
//    symbols before creating the implementation.
//
//        STBI_NO_JPEG
//        STBI_NO_PNG
//        STBI_NO_BMP
//        STBI_NO_PSD
//        STBI_NO_TGA
//        STBI_NO_GIF
//        STBI_NO_HDR
//        STBI_NO_PIC
//        STBI_NO_PNM   (.ppm and .pgm)
//
//  - You can request *only* certain decoders and suppress all other ones
//    (this will be more forward-compatible, as addition of new decoders
//    doesn't require you to disable them explicitly):
//
//        STBI_ONLY_JPEG
//        STBI_ONLY_PNG
//        STBI_ONLY_BMP
//        STBI_ONLY_PSD
//        STBI_ONLY_TGA
//        STBI_ONLY_GIF
//        STBI_ONLY_HDR
//        STBI_ONLY_PIC
//        STBI_ONLY_PNM   (.ppm and .pgm)
//
//   - If you use STBI_NO_PNG (or _ONLY_ without PNG), and you still
//     want the zlib decoder to be available, #define STBI_SUPPORT_ZLIB
//
//  - If you define STBI_MAX_DIMENSIONS, stb_image will reject images greater
//    than that size (in either width or height) without further processing.
//    This is to let programs in the wild set an upper bound to prevent
//    denial-of-service attacks on untrusted data, as one could generate a
//    valid image of gigantic dimensions and force stb_image to allocate a
//    huge block of memory and spend disproportionate time decoding it. By
//    default this is set to (1 << 24), which is 16777216, but that's still
//    very big.

#ifndef STBI_NO_STDIO
#include <stdio.h>
#endif // STBI_NO_STDIO

#define STBI_VERSION 1

enum
{
   STBI_default = 0, // only used for desired_channels

   STBI_grey       = 1,
   STBI_grey_alpha = 2,
   STBI_rgb        = 3,
   STBI_rgb_alpha  = 4
};

#include <stdlib.h>
typedef unsigned char stbi_uc;
typedef unsigned short stbi_us;

#ifdef __cplusplus
extern "C" {
#endif

#ifndef STBIDEF
#ifdef STB_IMAGE_STATIC
#define STBIDEF static
#else
#define STBIDEF extern
#endif
#endif

//////////////////////////////////////////////////////////////////////////////
//
// PRIMARY API - works on images of any type
//

//
// load image by filename, open file, or memory buffer
//

typedef struct
{
   int      (*read)  (void *user,char *data,int size);   // fill 'data' with 'size' bytes.  return number of bytes actually read
   void     (*skip)  (void *user,int n);                 // skip the next 'n' bytes, or 'unget' the last -n bytes if negative
   int      (*eof)   (void *user);                       // returns nonzero if we are at end of file/data
} stbi_io_callbacks;

////////////////////////////////////
//
// 8-bits-per-channel interface
//

STBIDEF stbi_uc *stbi_load_from_memory   (stbi_uc           const *buffer, int len   , int *x, int *y, int *channels_in_file, int desired_channels);
STBIDEF stbi_uc *stbi_load_from_callbacks(stbi_io_callbacks const *clbk  , void *user, int *x, int *y, int *channels_in_file, int desired_channels);

#ifndef STBI_NO_STDIO
STBIDEF stbi_uc *stbi_load            (char const *filename, int *x, int *y, int *channels_in_file, int desired_channels);
STBIDEF stbi_uc *stbi_load_from_file  (FILE *f, int *x, int *y, int *channels_in_file, int desired_channels);
// for stbi_load_from_file, file pointer is left pointing immediately after image
#endif

#ifndef STBI_NO_GIF
STBIDEF stbi_uc *stbi_load_gif_from_memory(stbi_uc const *buffer, int len, int **delays, int *x, int *y, int *z, int *comp, int req_comp);
#endif

#ifdef STBI_WINDOWS_UTF8
STBIDEF int stbi_convert_wchar_to_utf8(char *buffer, size_t bufferlen, const wchar_t* input);
#endif

////////////////////////////////////
//
// 16-bits-per-channel interface
//

STBIDEF stbi_us *stbi_load_16_from_memory   (stbi_uc const *buffer, int len, int *x, int *y, int *channels_in_file, int desired_channels);
STBIDEF stbi_us *stbi_load_16_from_callbacks(stbi_io_callbacks const *clbk, void *user, int *x, int *y, int *channels_in_file, int desired_channels);

#ifndef STBI_NO_STDIO
STBIDEF stbi_us *stbi_load_16          (char const *filename, int *x, int *y, int *channels_in_file, int desired_channels);
STBIDEF stbi_us *stbi_load_from_file_16(FILE *f, int *x, int *y, int *channels_in_file, int desired_channels);
#endif

////////////////////////////////////
//
// float-per-channel interface
//
#ifndef STBI_NO_LINEAR
   STBIDEF float *stbi_loadf_from_memory     (stbi_uc const *buffer, int len, int *x, int *y, int *channels_in_file, int desired_channels);
   STBIDEF float *stbi_loadf_from_callbacks  (stbi_io_callbacks const *clbk, void *user, int *x, int *y,  int *channels_in_file, int desired_channels);

   #ifndef STBI_NO_STDIO
   STBIDEF float *stbi_loadf            (char const *filename, int *x, int *y, int *channels_in_file, int desired_channels);
   STBIDEF float *stbi_loadf_from_file  (FILE *f, int *x, int *y, int *channels_in_file, int desired_channels);
   #endif
#endif

#ifndef STBI_NO_HDR
   STBIDEF void   stbi_hdr_to_ldr_gamma(float gamma);
   STBIDEF void   stbi_hdr_to_ldr_scale(float scale);
#endif // STBI_NO_HDR

#ifndef STBI_NO_LINEAR
   STBIDEF void   stbi_ldr_to_hdr_gamma(float gamma);
   STBIDEF void   stbi_ldr_to_hdr_scale(float scale);
#endif // STBI_NO_LINEAR

// stbi_is_hdr is always defined, but always returns false if STBI_NO_HDR
STBIDEF int    stbi_is_hdr_from_callbacks(stbi_io_callbacks const *clbk, void *user);
STBIDEF int    stbi_is_hdr_from_memory(stbi_uc const *buffer, int len);
#ifndef STBI_NO_STDIO
STBIDEF int      stbi_is_hdr          (char const *filename);
STBIDEF int      stbi_is_hdr_from_file(FILE *f);
#endif // STBI_NO_STDIO


// get a VERY brief reason for failure
// on most compilers (and ALL modern mainstream compilers) this is threadsafe
STBIDEF const char *stbi_failure_reason  (void);

// free the loaded image -- this is just free()
STBIDEF void     stbi_image_free      (void *retval_from_stbi_load);

// get image dimensions & components without fully decoding
STBIDEF int      stbi_info_from_memory(stbi_uc const *buffer, int len, int *x, int *y, int *comp);
STBIDEF int      stbi_info_from_callbacks(stbi_io_callbacks const *clbk, void *user, int *x, int *y, int *comp);
STBIDEF int      stbi_is_16_bit_from_memory(stbi_uc const *buffer, int len);
STBIDEF int      stbi_is_16_bit_from_callbacks(stbi_io_callbacks const *clbk, void *user);

#ifndef STBI_NO_STDIO
STBIDEF int      stbi_info               (char const *filename,     int *x, int *y, int *comp);
STBIDEF int      stbi_info_from_file     (FILE *f,                  int *x, int *y, int *comp);
STBIDEF int      stbi_is_16_bit          (char const *filename);
STBIDEF int      stbi_is_16_bit_from_file(FILE *f);
#endif



// for image formats that explicitly notate that they have premultiplied alpha,
// we just return the colors as stored in the file. set this flag to force
// unpremultiplication. results are undefined if the unpremultiply overflow.
STBIDEF void stbi_set_unpremultiply_on_load(int flag_true_if_should_unpremultiply);

// indicate whether we should process iphone images back to canonical format,
// or just pass them through "as-is"
STBIDEF void stbi_convert_iphone_png_to_rgb(int flag_true_if_should_convert);

// flip the image vertically, so the first pixel in the output array is the bottom left
STBIDEF void stbi_set_flip_vertically_on_load(int flag_true_if_should_flip);

// as above, but only applies to images loaded on the thread that calls the function
// this function is only available if your compiler supports thread-local variables;
// calling it will fail to link if your compiler doesn't
STBIDEF void stbi_set_unpremultiply_on_load_thread(int flag_true_if_should_unpremultiply);
STBIDEF void stbi_convert_iphone_png_to_rgb_thread(int flag_true_if_should_convert);
STBIDEF void stbi_set_flip_vertically_on_load_thread(int flag_true_if_should_flip);

// ZLIB client - used by PNG, available for other purposes

STBIDEF char *stbi_zlib_decode_malloc_guesssize(const char *buffer, int len, int initial_size, int *outlen);
STBIDEF char *stbi_zlib_decode_malloc_guesssize_headerflag(const char *buffer, int len, int initial_size, int *outlen, int parse_header);
STBIDEF char *stbi_zlib_decode_malloc(const char *buffer, int len, int *outlen);
STBIDEF int   stbi_zlib_decode_buffer(char *obuffer, int olen, const char *ibuffer, int ilen);

STBIDEF char *stbi_zlib_decode_noheader_malloc(const char *buffer, int len, int *outlen);
STBIDEF int   stbi_zlib_decode_noheader_buffer(char *obuffer, int olen, const char *ibuffer, int ilen);


#ifdef __cplusplus
}
#endif

//
//
////   end header file   /////////////////////////////////////////////////////
#endif // STBI_INCLUDE_STB_IMAGE_H

#ifdef STB_IMAGE_IMPLEMENTATION

#if defined(STBI_ONLY_JPEG) || defined(STBI_ONLY_PNG) || defined(STBI_ONLY_BMP) \
  || defined(STBI_ONLY_TGA) || defined(STBI_ONLY_GIF) || defined(STBI_ONLY_PSD) \
  || defined(STBI_ONLY_HDR) || defined(STBI_ONLY_PIC) || defined(STBI_ONLY_PNM) \
  || defined(STBI_ONLY_ZLIB)
   #ifndef STBI_ONLY_JPEG
   #define STBI_NO_JPEG
   #endif
   #ifndef STBI_ONLY_PNG
   #define STBI_NO_PNG
   #endif
   #ifndef STBI_ONLY_BMP
   #define STBI_NO_BMP
   #endif
   #ifndef STBI_ONLY_PSD
   #define STBI_NO_PSD
   #endif
   #ifndef STBI_ONLY_TGA
   #define STBI_NO_TGA
   #endif
   #ifndef STBI_ONLY_GIF
   #define STBI_NO_GIF
   #endif
   #ifndef STBI_ONLY_HDR
   #define STBI_NO_HDR
   #endif
   #ifndef STBI_ONLY_PIC
   #define STBI_NO_PIC
   #endif
   #ifndef STBI_ONLY_PNM
   #define STBI_NO_PNM
   #endif
#endif

#if defined(STBI_NO_PNG) && !defined(STBI_SUPPORT_ZLIB) && !defined(STBI_NO_ZLIB)
#define STBI_NO_ZLIB
#endif


#include <stdarg.h>
#include <stddef.h> // ptrdiff_t on osx
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#if !defined(STBI_NO_LINEAR) || !defined(STBI_NO_HDR)
#include <math.h>  // ldexp, pow
#endif

#ifndef STBI_NO_STDIO
#include <stdio.h>
#endif

#ifndef STBI_ASSERT
#include <assert.h>
#define STBI_ASSERT(x) assert(x)
#endif

#ifdef __cplusplus
#define STBI_EXTERN extern "C"
#else
#define STBI_EXTERN extern
#endif


#ifndef _MSC_VER
   #ifdef __cplusplus
   #define stbi_inline inline
   #else
   #define stbi_inline
   #endif
#else
   #define stbi_inline __forceinline
#endif

#ifndef STBI_NO_THREAD_LOCALS
   #if defined(__cplusplus) &&  __cplusplus >= 201103L
      #define STBI_THREAD_LOCAL       thread_local
   #elif defined(__GNUC__) && __GNUC__ < 5
      #define STBI_THREAD_LOCAL       __thread
   #elif defined(_MSC_VER)
      #define STBI_THREAD_LOCAL       __declspec(thread)
   #elif defined (__STDC_VERSION__) && __STDC_VERSION__ >= 201112L && !defined(__STDC_NO_THREADS__)
      #define STBI_THREAD_LOCAL       _Thread_local
   #endif

   #ifndef STBI_THREAD_LOCAL
      #if defined(__GNUC__)
        #define STBI_THREAD_LOCAL       __thread
      #endif
   #endif
#endif

#ifdef _MSC_VER
typedef unsigned short stbi__uint16;
typedef   signed short stbi__int16;
typedef unsigned int   stbi__uint32;
typedef   signed int   stbi__int32;
#else
#include <stdint.h>
typedef uint16_t stbi__uint16;
typedef int16_t  stbi__int16;
typedef uint32_t stbi__uint32;
typedef int32_t  stbi__int32;
#endif

// should produce compiler error if size is wrong
typedef unsigned char validate_uint32[sizeof(stbi__uint32)==4 ? 1 : -1];

#ifdef _MSC_VER
#define STBI_NOTUSED(v)  (void)(v)
#else
#define STBI_NOTUSED(v)  (void)sizeof(v)
#endif

#ifdef _MSC_VER
#define STBI_HAS_LROTL
#endif

#ifdef STBI_HAS_LROTL
   #define stbi_lrot(x,y)  _lrotl(x,y)
#else
   #define stbi_lrot(x,y)  (((x) << (y)) | ((x) >> (-(y) & 31)))
#endif

#if defined(STBI_MALLOC) && defined(STBI_FREE) && (defined(STBI_REALLOC) || defined(STBI_REALLOC_SIZED))
// ok
#elif !defined(STBI_MALLOC) && !defined(STBI_FREE) && !defined(STBI_REALLOC) && !defined(STBI_REALLOC_SIZED)
// ok
#else
#error "Must define all or none of STBI_MALLOC, STBI_FREE, and STBI_REALLOC (or STBI_REALLOC_SIZED)."
#endif

#ifndef STBI_MALLOC
#define STBI_MALLOC(sz)           malloc(sz)
#define STBI_REALLOC(p,newsz)     realloc(p,newsz)
#define STBI_FREE(p)              free(p)
#endif

#ifndef STBI_REALLOC_SIZED
#define STBI_REALLOC_SIZED(p,oldsz,newsz) STBI_REALLOC(p,newsz)
#endif

// x86/x64 detection
#if defined(__x86_64__) || defined(_M_X64)
#define STBI__X64_TARGET
#elif defined(__i386) || defined(_M_IX86)
#define STBI__X86_TARGET
#endif

#if defined(__GNUC__) && defined(STBI__X86_TARGET) && !defined(__SSE2__) && !defined(STBI_NO_SIMD)
// gcc doesn't support sse2 intrinsics unless you compile with -msse2,
// which in turn means it gets to use SSE2 everywhere. This is unfortunate,
// but previous attempts to provide the SSE2 functions with runtime
// detection caused numerous issues. The way architecture extensions are
// exposed in GCC/Clang is, sadly, not really suited for one-file libs.
// New behavior: if compiled with -msse2, we use SSE2 without any
// detection; if not, we don't use it at all.
#define STBI_NO_SIMD
#endif

#if defined(__MINGW32__) && defined(STBI__X86_TARGET) && !defined(STBI_MINGW_ENABLE_SSE2) && !defined(STBI_NO_SIMD)
// Note that __MINGW32__ doesn't actually mean 32-bit, so we have to avoid STBI__X64_TARGET
//
// 32-bit MinGW wants ESP to be 16-byte aligned, but this is not in the
// Windows ABI and VC++ as well as Windows DLLs don't maintain that invariant.
// As a result, enabling SSE2 on 32-bit MinGW is dangerous when not
// simultaneously enabling "-mstackrealign".
//
// See https://github.com/nothings/stb/issues/81 for more information.
//
// So default to no SSE2 on 32-bit MinGW. If you've read this far and added
// -mstackrealign to your build settings, feel free to #define STBI_MINGW_ENABLE_SSE2.
#define STBI_NO_SIMD
#endif

#if !defined(STBI_NO_SIMD) && (defined(STBI__X86_TARGET) || defined(STBI__X64_TARGET))
#define STBI_SSE2
#include <emmintrin.h>

#ifdef _MSC_VER

#if _MSC_VER >= 1400  // not VC6
#include <intrin.h> // __cpuid
static int stbi__cpuid3(void)
{
   int info[4];
   __cpuid(info,1);
   return info[3];
}
#else
static int stbi__cpuid3(void)
{
   int res;
   __asm {
      mov  eax,1
      cpuid
      mov  res,edx
   }
   return res;
}
#endif

#define STBI_SIMD_ALIGN(type, name) __declspec(align(16)) type name

#if !defined(STBI_NO_JPEG) && defined(STBI_SSE2)
static int stbi__sse2_available(void)
{
   int info3 = stbi__cpuid3();
   return ((info3 >> 26) & 1) != 0;
}
#endif

#else // assume GCC-style if not VC++
#define STBI_SIMD_ALIGN(type, name) type name __attribute__((aligned(16)))

#if !defined(STBI_NO_JPEG) && defined(STBI_SSE2)
static int stbi__sse2_available(void)
{
   // If we're even attempting to compile this on GCC/Clang, that means
   // -msse2 is on, which means the compiler is allowed to use SSE2
   // instructions at will, and so are we.
   return 1;
}
#endif

#endif
#endif

// ARM NEON
#if defined(STBI_NO_SIMD) && defined(STBI_NEON)
#undef STBI_NEON
#endif

#ifdef STBI_NEON
#include <arm_neon.h>
#ifdef _MSC_VER
#define STBI_SIMD_ALIGN(type, name) __declspec(align(16)) type name
#else
#define STBI_SIMD_ALIGN(type, name) type name __attribute__((aligned(16)))
#endif
#endif

#ifndef STBI_SIMD_ALIGN
#define STBI_SIMD_ALIGN(type, name) type name
#endif

#ifndef STBI_MAX_DIMENSIONS
#define STBI_MAX_DIMENSIONS (1 << 24)
#endif

///////////////////////////////////////////////
//
//  stbi__context struct and start_xxx functions

// stbi__context structure is our basic context used by all images, so it
// contains all the IO context, plus some basic image information
typedef struct
{
   stbi__uint32 img_x, img_y;
   int img_n, img_out_n;

   stbi_io_callbacks io;
   void *io_user_data;

   int read_from_callbacks;
   int buflen;
   stbi_uc buffer_start[128];
   int callback_already_read;

   stbi_uc *img_buffer, *img_buffer_end;
   stbi_uc *img_buffer_original, *img_buffer_original_end;
} stbi__context;


static void stbi__refill_buffer(stbi__context *s);

// initialize a memory-decode context
static void stbi__start_mem(stbi__context *s, stbi_uc const *buffer, int len)
{
   s->io.read = NULL;
   s->read_from_callbacks = 0;
   s->callback_already_read = 0;
   s->img_buffer = s->img_buffer_original = (stbi_uc *) buffer;
   s->img_buffer_end = s->img_buffer_original_end = (stbi_uc *) buffer+len;
}

// initialize a callback-based context
static void stbi__start_callbacks(stbi__context *s, stbi_io_callbacks *c, void *user)
{
   s->io = *c;
   s->io_user_data = user;
   s->buflen = sizeof(s->buffer_start);
   s->read_from_callbacks = 1;
   s->callback_already_read = 0;
   s->img_buffer = s->img_buffer_original = s->buffer_start;
   stbi__refill_buffer(s);
   s->img_buffer_original_end = s->img_buffer_end;
}

#ifndef STBI_NO_STDIO

static int stbi__stdio_read(void *user, char *data, int size)
{
   return (int) fread(data,1,size,(FILE*) user);
}

static void stbi__stdio_skip(void *user, int n)
{
   int ch;
   fseek((FILE*) user, n, SEEK_CUR);
   ch = fgetc((FILE*) user);  /* have to read a byte to reset feof()'s flag */
   if (ch != EOF) {
      ungetc(ch, (FILE *) user);  /* push byte back onto stream if valid. */
   }
}

static int stbi__stdio_eof(void *user)
{
   return feof((FILE*) user) || ferror((FILE *) user);
}

static stbi_io_callbacks stbi__stdio_callbacks =
{
   stbi__stdio_read,
   stbi__stdio_skip,
   stbi__stdio_eof,
};

static void stbi__start_file(stbi__context *s, FILE *f)
{
   stbi__start_callbacks(s, &stbi__stdio_callbacks, (void *) f);
}

//static void stop_file(stbi__context *s) { }

#endif // !STBI_NO_STDIO

static void stbi__rewind(stbi__context *s)
{
   // conceptually rewind SHOULD rewind to the beginning of the stream,
   // but we just rewind to the beginning of the initial buffer, because
   // we only use it after doing 'test', which only ever looks at at most 92 bytes
   s->img_buffer = s->img_buffer_original;
   s->img_buffer_end = s->img_buffer_original_end;
}

enum
{
   STBI_ORDER_RGB,
   STBI_ORDER_BGR
};

typedef struct
{
   int bits_per_channel;
   int num_channels;
   int channel_order;
} stbi__result_info;

#ifndef STBI_NO_ICO
static void *stbi__ico_load(stbi__context *s, int *x, int *y, int *comp, int req_comp, stbi__result_info *ri);
static int stbi__ico_test(stbi__context *s);
static int stbi__ico_info(stbi__context *s, int *x, int *y, int *comp);
static int stbi__ico_test_raw(stbi__context *s);
#endif //STBI_NO_ICO

#ifndef STBI_NO_JPEG
static int      stbi__jpeg_test(stbi__context *s);
static void    *stbi__jpeg_load(stbi__context *s, int *x, int *y, int *comp, int req_comp, stbi__result_info *ri);
static int      stbi__jpeg_info(stbi__context *s, int *x, int *y, int *comp);
#endif

#ifndef STBI_NO_PNG
static int      stbi__png_test(stbi__context *s);
static void    *stbi__png_load(stbi__context *s, int *x, int *y, int *comp, int req_comp, stbi__result_info *ri);
static int      stbi__png_info(stbi__context *s, int *x, int *y, int *comp);
static int      stbi__png_is16(stbi__context *s);
#endif

#ifndef STBI_NO_BMP
static int      stbi__bmp_test(stbi__context *s);
static void    *stbi__bmp_load(stbi__context *s, int *x, int *y, int *comp, int req_comp, stbi__result_info *ri);
static int      stbi__bmp_info(stbi__context *s, int *x, int *y, int *comp);
#endif

#ifndef STBI_NO_TGA
static int      stbi__tga_test(stbi__context *s);
static void    *stbi__tga_load(stbi__context *s, int *x, int *y, int *comp, int req_comp, stbi__result_info *ri);
static int      stbi__tga_info(stbi__context *s, int *x, int *y, int *comp);
#endif

#ifndef STBI_NO_PSD
static int      stbi__psd_test(stbi__context *s);
static void    *stbi__psd_load(stbi__context *s, int *x, int *y, int *comp, int req_comp, stbi__result_info *ri, int bpc);
static int      stbi__psd_info(stbi__context *s, int *x, int *y, int *comp);
static int      stbi__psd_is16(stbi__context *s);
#endif

#ifndef STBI_NO_HDR
static int      stbi__hdr_test(stbi__context *s);
static float   *stbi__hdr_load(stbi__context *s, int *x, int *y, int *comp, int req_comp, stbi__result_info *ri);
static int      stbi__hdr_info(stbi__context *s, int *x, int *y, int *comp);
#endif

#ifndef STBI_NO_PIC
static int      stbi__pic_test(stbi__context *s);
static void    *stbi__pic_load(stbi__context *s, int *x, int *y, int *comp, int req_comp, stbi__result_info *ri);
static int      stbi__pic_info(stbi__context *s, int *x, int *y, int *comp);
#endif

#ifndef STBI_NO_GIF
static int      stbi__gif_test(stbi__context *s);
static void    *stbi__gif_load(stbi__context *s, int *x, int *y, int *comp, int req_comp, stbi__result_info *ri);
static void    *stbi__load_gif_main(stbi__context *s, int **delays, int *x, int *y, int *z, int *comp, int req_comp);
static int      stbi__gif_info(stbi__context *s, int *x, int *y, int *comp);
#endif

#ifndef STBI_NO_PNM
static int      stbi__pnm_test(stbi__context *s);
static void    *stbi__pnm_load(stbi__context *s, int *x, int *y, int *comp, int req_comp, stbi__result_info *ri);
static int      stbi__pnm_info(stbi__context *s, int *x, int *y, int *comp);
static int      stbi__pnm_is16(stbi__context *s);
#endif

static
#ifdef STBI_THREAD_LOCAL
STBI_THREAD_LOCAL
#endif
const char *stbi__g_failure_reason;

STBIDEF const char *stbi_failure_reason(void)
{
   return stbi__g_failure_reason;
}

#ifndef STBI_NO_FAILURE_STRINGS
static int stbi__err(const char *str)
{
   stbi__g_failure_reason = str;
   return 0;
}
#endif

static void *stbi__malloc(size_t size)
{
    return STBI_MALLOC(size);
}

// stb_image uses ints pervasively, including for offset calculations.
// therefore the largest decoded image size we can support with the
// current code, even on 64-bit targets, is INT_MAX. this is not a
// significant limitation for the intended use case.
//
// we do, however, need to make sure our size calculations don't
// overflow. hence a few helper functions for size calculations that
// multiply integers together, making sure that they're non-negative
// and no overflow occurs.

// return 1 if the sum is valid, 0 on overflow.
// negative terms are considered invalid.
static int stbi__addsizes_valid(int a, int b)
{
   if (b < 0) return 0;
   // now 0 <= b <= INT_MAX, hence also
   // 0 <= INT_MAX - b <= INTMAX.
   // And "a + b <= INT_MAX" (which might overflow) is the
   // same as a <= INT_MAX - b (no overflow)
   return a <= INT_MAX - b;
}

// returns 1 if the product is valid, 0 on overflow.
// negative factors are considered invalid.
static int stbi__mul2sizes_valid(int a, int b)
{
   if (a < 0 || b < 0) return 0;
   if (b == 0) return 1; // mul-by-0 is always safe
   // portable way to check for no overflows in a*b
   return a <= INT_MAX/b;
}

#if !defined(STBI_NO_JPEG) || !defined(STBI_NO_PNG) || !defined(STBI_NO_TGA) || !defined(STBI_NO_HDR)
// returns 1 if "a*b + add" has no negative terms/factors and doesn't overflow
static int stbi__mad2sizes_valid(int a, int b, int add)
{
   return stbi__mul2sizes_valid(a, b) && stbi__addsizes_valid(a*b, add);
}
#endif

// returns 1 if "a*b*c + add" has no negative terms/factors and doesn't overflow
static int stbi__mad3sizes_valid(int a, int b, int c, int add)
{
   return stbi__mul2sizes_valid(a, b) && stbi__mul2sizes_valid(a*b, c) &&
      stbi__addsizes_valid(a*b*c, add);
}

// returns 1 if "a*b*c*d + add" has no negative terms/factors and doesn't overflow
#if !defined(STBI_NO_LINEAR) || !defined(STBI_NO_HDR) || !defined(STBI_NO_PNM)
static int stbi__mad4sizes_valid(int a, int b, int c, int d, int add)
{
   return stbi__mul2sizes_valid(a, b) && stbi__mul2sizes_valid(a*b, c) &&
      stbi__mul2sizes_valid(a*b*c, d) && stbi__addsizes_valid(a*b*c*d, add);
}
#endif

#if !defined(STBI_NO_JPEG) || !defined(STBI_NO_PNG) || !defined(STBI_NO_TGA) || !defined(STBI_NO_HDR)
// mallocs with size overflow checking
static void *stbi__malloc_mad2(int a, int b, int add)
{
   if (!stbi__mad2sizes_valid(a, b, add)) return NULL;
   return stbi__malloc(a*b + add);
}
#endif

static void *stbi__malloc_mad3(int a, int b, int c, int add)
{
   if (!stbi__mad3sizes_valid(a, b, c, add)) return NULL;
   return stbi__malloc(a*b*c + add);
}

#if !defined(STBI_NO_LINEAR) || !defined(STBI_NO_HDR) || !defined(STBI_NO_PNM)
static void *stbi__malloc_mad4(int a, int b, int c, int d, int add)
{
   if (!stbi__mad4sizes_valid(a, b, c, d, add)) return NULL;
   return stbi__malloc(a*b*c*d + add);
}
#endif

// stbi__err - error
// stbi__errpf - error returning pointer to float
// stbi__errpuc - error returning pointer to unsigned char

#ifdef STBI_NO_FAILURE_STRINGS
   #define stbi__err(x,y)  0
#elif defined(STBI_FAILURE_USERMSG)
   #define stbi__err(x,y)  stbi__err(y)
#else
   #define stbi__err(x,y)  stbi__err(x)
#endif

#define stbi__errpf(x,y)   ((float *)(size_t) (stbi__err(x,y)?NULL:NULL))
#define stbi__errpuc(x,y)  ((unsigned char *)(size_t) (stbi__err(x,y)?NULL:NULL))

STBIDEF void stbi_image_free(void *retval_from_stbi_load)
{
   STBI_FREE(retval_from_stbi_load);
}

#ifndef STBI_NO_LINEAR
static float   *stbi__ldr_to_hdr(stbi_uc *data, int x, int y, int comp);
#endif

#ifndef STBI_NO_HDR
static stbi_uc *stbi__hdr_to_ldr(float   *data, int x, int y, int comp);
#endif

static int stbi__vertically_flip_on_load_global = 0;

STBIDEF void stbi_set_flip_vertically_on_load(int flag_true_if_should_flip)
{
   stbi__vertically_flip_on_load_global = flag_true_if_should_flip;
}

#ifndef STBI_THREAD_LOCAL
#define stbi__vertically_flip_on_load  stbi__vertically_flip_on_load_global
#else
static STBI_THREAD_LOCAL int stbi__vertically_flip_on_load_local, stbi__vertically_flip_on_load_set;

STBIDEF void stbi_set_flip_vertically_on_load_thread(int flag_true_if_should_flip)
{
   stbi__vertically_flip_on_load_local = flag_true_if_should_flip;
   stbi__vertically_flip_on_load_set = 1;
}

#define stbi__vertically_flip_on_load  (stbi__vertically_flip_on_load_set       \
                                         ? stbi__vertically_flip_on_load_local  \
                                         : stbi__vertically_flip_on_load_global)
#endif // STBI_THREAD_LOCAL

static void *stbi__load_main(stbi__context *s, int *x, int *y, int *comp, int req_comp, stbi__result_info *ri, int bpc)
{
   memset(ri, 0, sizeof(*ri)); // make sure it's initialized if we add new fields
   ri->bits_per_channel = 8; // default is 8 so most paths don't have to be changed
   ri->channel_order = STBI_ORDER_RGB; // all current input & output are this, but this is here so we can add BGR order
   ri->num_channels = 0;

   // test the formats with a very explicit header first (at least a FOURCC
   // or distinctive magic number first)
   #ifndef STBI_NO_PNG
   if (stbi__png_test(s))  return stbi__png_load(s,x,y,comp,req_comp, ri);
   #endif
   #ifndef STBI_NO_BMP
   if (stbi__bmp_test(s))  return stbi__bmp_load(s,x,y,comp,req_comp, ri);
   #endif
   #ifndef STBI_NO_GIF
   if (stbi__gif_test(s))  return stbi__gif_load(s,x,y,comp,req_comp, ri);
   #endif
   #ifndef STBI_NO_PSD
   if (stbi__psd_test(s))  return stbi__psd_load(s,x,y,comp,req_comp, ri, bpc);
   #else
   STBI_NOTUSED(bpc);
   #endif
   #ifndef STBI_NO_PIC
   if (stbi__pic_test(s))  return stbi__pic_load(s,x,y,comp,req_comp, ri);
   #endif
   #ifndef STBI_NO_ICO
   if (stbi__ico_test(s))  return stbi__ico_load(s,x,y,comp,req_comp, ri);
   #endif

   // then the formats that can end up attempting to load with just 1 or 2
   // bytes matching expectations; these are prone to false positives, so
   // try them later
   #ifndef STBI_NO_JPEG
   if (stbi__jpeg_test(s)) return stbi__jpeg_load(s,x,y,comp,req_comp, ri);
   #endif
   #ifndef STBI_NO_PNM
   if (stbi__pnm_test(s))  return stbi__pnm_load(s,x,y,comp,req_comp, ri);
   #endif

   #ifndef STBI_NO_HDR
   if (stbi__hdr_test(s)) {
      float *hdr = stbi__hdr_load(s, x,y,comp,req_comp, ri);
      return stbi__hdr_to_ldr(hdr, *x, *y, req_comp ? req_comp : *comp);
   }
   #endif

   #ifndef STBI_NO_TGA
   // test tga last because it's a crappy test!
   if (stbi__tga_test(s))
      return stbi__tga_load(s,x,y,comp,req_comp, ri);
   #endif

   return stbi__errpuc("unknown image type", "Image not of any known type, or corrupt");
}

static stbi_uc *stbi__convert_16_to_8(stbi__uint16 *orig, int w, int h, int channels)
{
   int i;
   int img_len = w * h * channels;
   stbi_uc *reduced;

   reduced = (stbi_uc *) stbi__malloc(img_len);
   if (reduced == NULL) return stbi__errpuc("outofmem", "Out of memory");

   for (i = 0; i < img_len; ++i)
      reduced[i] = (stbi_uc)((orig[i] >> 8) & 0xFF); // top half of each byte is sufficient approx of 16->8 bit scaling

   STBI_FREE(orig);
   return reduced;
}

static stbi__uint16 *stbi__convert_8_to_16(stbi_uc *orig, int w, int h, int channels)
{
   int i;
   int img_len = w * h * channels;
   stbi__uint16 *enlarged;

   enlarged = (stbi__uint16 *) stbi__malloc(img_len*2);
   if (enlarged == NULL) return (stbi__uint16 *) stbi__errpuc("outofmem", "Out of memory");

   for (i = 0; i < img_len; ++i)
      enlarged[i] = (stbi__uint16)((orig[i] << 8) + orig[i]); // replicate to high and low byte, maps 0->0, 255->0xffff

   STBI_FREE(orig);
   return enlarged;
}

static void stbi__vertical_flip(void *image, int w, int h, int bytes_per_pixel)
{
   int row;
   size_t bytes_per_row = (size_t)w * bytes_per_pixel;
   stbi_uc temp[2048];
   stbi_uc *bytes = (stbi_uc *)image;

   for (row = 0; row < (h>>1); row++) {
      stbi_uc *row0 = bytes + row*bytes_per_row;
      stbi_uc *row1 = bytes + (h - row - 1)*bytes_per_row;
      // swap row0 with row1
      size_t bytes_left = bytes_per_row;
      while (bytes_left) {
         size_t bytes_copy = (bytes_left < sizeof(temp)) ? bytes_left : sizeof(temp);
         memcpy(temp, row0, bytes_copy);
         memcpy(row0, row1, bytes_copy);
         memcpy(row1, temp, bytes_copy);
         row0 += bytes_copy;
         row1 += bytes_copy;
         bytes_left -= bytes_copy;
      }
   }
}

#ifndef STBI_NO_GIF
static void stbi__vertical_flip_slices(void *image, int w, int h, int z, int bytes_per_pixel)
{
   int slice;
   int slice_size = w * h * bytes_per_pixel;

   stbi_uc *bytes = (stbi_uc *)image;
   for (slice = 0; slice < z; ++slice) {
      stbi__vertical_flip(bytes, w, h, bytes_per_pixel);
      bytes += slice_size;
   }
}
#endif

static unsigned char *stbi__load_and_postprocess_8bit(stbi__context *s, int *x, int *y, int *comp, int req_comp)
{
   stbi__result_info ri;
   void *result = stbi__load_main(s, x, y, comp, req_comp, &ri, 8);

   if (result == NULL)
      return NULL;

   // it is the responsibility of the loaders to make sure we get either 8 or 16 bit.
   STBI_ASSERT(ri.bits_per_channel == 8 || ri.bits_per_channel == 16);

   if (ri.bits_per_channel != 8) {
      result = stbi__convert_16_to_8((stbi__uint16 *) result, *x, *y, req_comp == 0 ? *comp : req_comp);
      ri.bits_per_channel = 8;
   }

   // @TODO: move stbi__convert_format to here

   if (stbi__vertically_flip_on_load) {
      int channels = req_comp ? req_comp : *comp;
      stbi__vertical_flip(result, *x, *y, channels * sizeof(stbi_uc));
   }

   return (unsigned char *) result;
}

static stbi__uint16 *stbi__load_and_postprocess_16bit(stbi__context *s, int *x, int *y, int *comp, int req_comp)
{
   stbi__result_info ri;
   void *result = stbi__load_main(s, x, y, comp, req_comp, &ri, 16);

   if (result == NULL)
      return NULL;

   // it is the responsibility of the loaders to make sure we get either 8 or 16 bit.
   STBI_ASSERT(ri.bits_per_channel == 8 || ri.bits_per_channel == 16);

   if (ri.bits_per_channel != 16) {
      result = stbi__convert_8_to_16((stbi_uc *) result, *x, *y, req_comp == 0 ? *comp : req_comp);
      ri.bits_per_channel = 16;
   }

   // @TODO: move stbi__convert_format16 to here
   // @TODO: special case RGB-to-Y (and RGBA-to-YA) for 8-bit-to-16-bit case to keep more precision

   if (stbi__vertically_flip_on_load) {
      int channels = req_comp ? req_comp : *comp;
      stbi__vertical_flip(result, *x, *y, channels * sizeof(stbi__uint16));
   }

   return (stbi__uint16 *) result;
}

#if !defined(STBI_NO_HDR) && !defined(STBI_NO_LINEAR)
static void stbi__float_postprocess(float *result, int *x, int *y, int *comp, int req_comp)
{
   if (stbi__vertically_flip_on_load && result != NULL) {
      int channels = req_comp ? req_comp : *comp;
      stbi__vertical_flip(result, *x, *y, channels * sizeof(float));
   }
}
#endif

#ifndef STBI_NO_STDIO

#if defined(_WIN32) && defined(STBI_WINDOWS_UTF8)
STBI_EXTERN __declspec(dllimport) int __stdcall MultiByteToWideChar(unsigned int cp, unsigned long flags, const char *str, int cbmb, wchar_t *widestr, int cchwide);
STBI_EXTERN __declspec(dllimport) int __stdcall WideCharToMultiByte(unsigned int cp, unsigned long flags, const wchar_t *widestr, int cchwide, char *str, int cbmb, const char *defchar, int *used_default);
#endif

#if defined(_WIN32) && defined(STBI_WINDOWS_UTF8)
STBIDEF int stbi_convert_wchar_to_utf8(char *buffer, size_t bufferlen, const wchar_t* input)
{
	return WideCharToMultiByte(65001 /* UTF8 */, 0, input, -1, buffer, (int) bufferlen, NULL, NULL);
}
#endif

static FILE *stbi__fopen(char const *filename, char const *mode)
{
   FILE *f;
#if defined(_WIN32) && defined(STBI_WINDOWS_UTF8)
   wchar_t wMode[64];
   wchar_t wFilename[1024];
	if (0 == MultiByteToWideChar(65001 /* UTF8 */, 0, filename, -1, wFilename, sizeof(wFilename)/sizeof(*wFilename)))
      return 0;

	if (0 == MultiByteToWideChar(65001 /* UTF8 */, 0, mode, -1, wMode, sizeof(wMode)/sizeof(*wMode)))
      return 0;

#if defined(_MSC_VER) && _MSC_VER >= 1400
	if (0 != _wfopen_s(&f, wFilename, wMode))
		f = 0;
#else
   f = _wfopen(wFilename, wMode);
#endif

#elif defined(_MSC_VER) && _MSC_VER >= 1400
   if (0 != fopen_s(&f, filename, mode))
      f=0;
#else
   f = fopen(filename, mode);
#endif
   return f;
}


STBIDEF stbi_uc *stbi_load(char const *filename, int *x, int *y, int *comp, int req_comp)
{
   FILE *f = stbi__fopen(filename, "rb");
   unsigned char *result;
   if (!f) return stbi__errpuc("can't fopen", "Unable to open file");
   result = stbi_load_from_file(f,x,y,comp,req_comp);
   fclose(f);
   return result;
}

STBIDEF stbi_uc *stbi_load_from_file(FILE *f, int *x, int *y, int *comp, int req_comp)
{
   unsigned char *result;
   stbi__context s;
   stbi__start_file(&s,f);
   result = stbi__load_and_postprocess_8bit(&s,x,y,comp,req_comp);
   if (result) {
      // need to 'unget' all the characters in the IO buffer
      fseek(f, - (int) (s.img_buffer_end - s.img_buffer), SEEK_CUR);
   }
   return result;
}

STBIDEF stbi__uint16 *stbi_load_from_file_16(FILE *f, int *x, int *y, int *comp, int req_comp)
{
   stbi__uint16 *result;
   stbi__context s;
   stbi__start_file(&s,f);
   result = stbi__load_and_postprocess_16bit(&s,x,y,comp,req_comp);
   if (result) {
      // need to 'unget' all the characters in the IO buffer
      fseek(f, - (int) (s.img_buffer_end - s.img_buffer), SEEK_CUR);
   }
   return result;
}

STBIDEF stbi_us *stbi_load_16(char const *filename, int *x, int *y, int *comp, int req_comp)
{
   FILE *f = stbi__fopen(filename, "rb");
   stbi__uint16 *result;
   if (!f) return (stbi_us *) stbi__errpuc("can't fopen", "Unable to open file");
   result = stbi_load_from_file_16(f,x,y,comp,req_comp);
   fclose(f);
   return result;
}


#endif //!STBI_NO_STDIO

STBIDEF stbi_us *stbi_load_16_from_memory(stbi_uc const *buffer, int len, int *x, int *y, int *channels_in_file, int desired_channels)
{
   stbi__context s;
   stbi__start_mem(&s,buffer,len);
   return stbi__load_and_postprocess_16bit(&s,x,y,channels_in_file,desired_channels);
}

STBIDEF stbi_us *stbi_load_16_from_callbacks(stbi_io_callbacks const *clbk, void *user, int *x, int *y, int *channels_in_file, int desired_channels)
{
   stbi__context s;
   stbi__start_callbacks(&s, (stbi_io_callbacks *)clbk, user);
   return stbi__load_and_postprocess_16bit(&s,x,y,channels_in_file,desired_channels);
}

STBIDEF stbi_uc *stbi_load_from_memory(stbi_uc const *buffer, int len, int *x, int *y, int *comp, int req_comp)
{
   stbi__context s;
   stbi__start_mem(&s,buffer,len);
   return stbi__load_and_postprocess_8bit(&s,x,y,comp,req_comp);
}

STBIDEF stbi_uc *stbi_load_from_callbacks(stbi_io_callbacks const *clbk, void *user, int *x, int *y, int *comp, int req_comp)
{
   stbi__context s;
   stbi__start_callbacks(&s, (stbi_io_callbacks *) clbk, user);
   return stbi__load_and_postprocess_8bit(&s,x,y,comp,req_comp);
}

#ifndef STBI_NO_GIF
STBIDEF stbi_uc *stbi_load_gif_from_memory(stbi_uc const *buffer, int len, int **delays, int *x, int *y, int *z, int *comp, int req_comp)
{
   unsigned char *result;
   stbi__context s;
   stbi__start_mem(&s,buffer,len);

   result = (unsigned char*) stbi__load_gif_main(&s, delays, x, y, z, comp, req_comp);
   if (stbi__vertically_flip_on_load) {
      stbi__vertical_flip_slices( result, *x, *y, *z, *comp );
   }

   return result;
}
#endif

#ifndef STBI_NO_LINEAR
static float *stbi__loadf_main(stbi__context *s, int *x, int *y, int *comp, int req_comp)
{
   unsigned char *data;
   #ifndef STBI_NO_HDR
   if (stbi__hdr_test(s)) {
      stbi__result_info ri;
      float *hdr_data = stbi__hdr_load(s,x,y,comp,req_comp, &ri);
      if (hdr_data)
         stbi__float_postprocess(hdr_data,x,y,comp,req_comp);
      return hdr_data;
   }
   #endif
   data = stbi__load_and_postprocess_8bit(s, x, y, comp, req_comp);
   if (data)
      return stbi__ldr_to_hdr(data, *x, *y, req_comp ? req_comp : *comp);
   return stbi__errpf("unknown image type", "Image not of any known type, or corrupt");
}

STBIDEF float *stbi_loadf_from_memory(stbi_uc const *buffer, int len, int *x, int *y, int *comp, int req_comp)
{
   stbi__context s;
   stbi__start_mem(&s,buffer,len);
   return stbi__loadf_main(&s,x,y,comp,req_comp);
}

STBIDEF float *stbi_loadf_from_callbacks(stbi_io_callbacks const *clbk, void *user, int *x, int *y, int *comp, int req_comp)
{
   stbi__context s;
   stbi__start_callbacks(&s, (stbi_io_callbacks *) clbk, user);
   return stbi__loadf_main(&s,x,y,comp,req_comp);
}

#ifndef STBI_NO_STDIO
STBIDEF float *stbi_loadf(char const *filename, int *x, int *y, int *comp, int req_comp)
{
   float *result;
   FILE *f = stbi__fopen(filename, "rb");
   if (!f) return stbi__errpf("can't fopen", "Unable to open file");
   result = stbi_loadf_from_file(f,x,y,comp,req_comp);
   fclose(f);
   return result;
}

STBIDEF float *stbi_loadf_from_file(FILE *f, int *x, int *y, int *comp, int req_comp)
{
   stbi__context s;
   stbi__start_file(&s,f);
   return stbi__loadf_main(&s,x,y,comp,req_comp);
}
#endif // !STBI_NO_STDIO

#endif // !STBI_NO_LINEAR

// these is-hdr-or-not is defined independent of whether STBI_NO_LINEAR is
// defined, for API simplicity; if STBI_NO_LINEAR is defined, it always
// reports false!

STBIDEF int stbi_is_hdr_from_memory(stbi_uc const *buffer, int len)
{
   #ifndef STBI_NO_HDR
   stbi__context s;
   stbi__start_mem(&s,buffer,len);
   return stbi__hdr_test(&s);
   #else
   STBI_NOTUSED(buffer);
   STBI_NOTUSED(len);
   return 0;
   #endif
}

#ifndef STBI_NO_STDIO
STBIDEF int      stbi_is_hdr          (char const *filename)
{
   FILE *f = stbi__fopen(filename, "rb");
   int result=0;
   if (f) {
      result = stbi_is_hdr_from_file(f);
      fclose(f);
   }
   return result;
}

STBIDEF int stbi_is_hdr_from_file(FILE *f)
{
   #ifndef STBI_NO_HDR
   long pos = ftell(f);
   int res;
   stbi__context s;
   stbi__start_file(&s,f);
   res = stbi__hdr_test(&s);
   fseek(f, pos, SEEK_SET);
   return res;
   #else
   STBI_NOTUSED(f);
   return 0;
   #endif
}
#endif // !STBI_NO_STDIO

STBIDEF int      stbi_is_hdr_from_callbacks(stbi_io_callbacks const *clbk, void *user)
{
   #ifndef STBI_NO_HDR
   stbi__context s;
   stbi__start_callbacks(&s, (stbi_io_callbacks *) clbk, user);
   return stbi__hdr_test(&s);
   #else
   STBI_NOTUSED(clbk);
   STBI_NOTUSED(user);
   return 0;
   #endif
}

#ifndef STBI_NO_LINEAR
static float stbi__l2h_gamma=2.2f, stbi__l2h_scale=1.0f;

STBIDEF void   stbi_ldr_to_hdr_gamma(float gamma) { stbi__l2h_gamma = gamma; }
STBIDEF void   stbi_ldr_to_hdr_scale(float scale) { stbi__l2h_scale = scale; }
#endif

static float stbi__h2l_gamma_i=1.0f/2.2f, stbi__h2l_scale_i=1.0f;

STBIDEF void   stbi_hdr_to_ldr_gamma(float gamma) { stbi__h2l_gamma_i = 1/gamma; }
STBIDEF void   stbi_hdr_to_ldr_scale(float scale) { stbi__h2l_scale_i = 1/scale; }


//////////////////////////////////////////////////////////////////////////////
//
// Common code used by all image loaders
//

enum
{
   STBI__SCAN_load=0,
   STBI__SCAN_type,
   STBI__SCAN_header
};

static void stbi__refill_buffer(stbi__context *s)
{
   int n = (s->io.read)(s->io_user_data,(char*)s->buffer_start,s->buflen);
   s->callback_already_read += (int) (s->img_buffer - s->img_buffer_original);
   if (n == 0) {
      // at end of file, treat same as if from memory, but need to handle case
      // where s->img_buffer isn't pointing to safe memory, e.g. 0-byte file
      s->read_from_callbacks = 0;
      s->img_buffer = s->buffer_start;
      s->img_buffer_end = s->buffer_start+1;
      *s->img_buffer = 0;
   } else {
      s->img_buffer = s->buffer_start;
      s->img_buffer_end = s->buffer_start + n;
   }
}

stbi_inline static stbi_uc stbi__get8(stbi__context *s)
{
   if (s->img_buffer < s->img_buffer_end)
      return *s->img_buffer++;
   if (s->read_from_callbacks) {
      stbi__refill_buffer(s);
      return *s->img_buffer++;
   }
   return 0;
}

#if defined(STBI_NO_JPEG) && defined(STBI_NO_HDR) && defined(STBI_NO_PIC) && defined(STBI_NO_PNM)
// nothing
#else
stbi_inline static int stbi__at_eof(stbi__context *s)
{
   if (s->io.read) {
      if (!(s->io.eof)(s->io_user_data)) return 0;
      // if feof() is true, check if buffer = end
      // special case: we've only got the special 0 character at the end
      if (s->read_from_callbacks == 0) return 1;
   }

   return s->img_buffer >= s->img_buffer_end;
}
#endif

#if defined(STBI_NO_JPEG) && defined(STBI_NO_PNG) && defined(STBI_NO_BMP) && defined(STBI_NO_PSD) && defined(STBI_NO_TGA) && defined(STBI_NO_GIF) && defined(STBI_NO_PIC)
// nothing
#else
static void stbi__skip(stbi__context *s, int n)
{
   if (n == 0) return;  // already there!
   if (n < 0) {
      s->img_buffer = s->img_buffer_end;
      return;
   }
   if (s->io.read) {
      int blen = (int) (s->img_buffer_end - s->img_buffer);
      if (blen < n) {
         s->img_buffer = s->img_buffer_end;
         (s->io.skip)(s->io_user_data, n - blen);
         return;
      }
   }
   s->img_buffer += n;
}
#endif

#if defined(STBI_NO_PNG) && defined(STBI_NO_TGA) && defined(STBI_NO_HDR) && defined(STBI_NO_PNM)
// nothing
#else
static int stbi__getn(stbi__context *s, stbi_uc *buffer, int n)
{
   if (s->io.read) {
      int blen = (int) (s->img_buffer_end - s->img_buffer);
      if (blen < n) {
         int res, count;

         memcpy(buffer, s->img_buffer, blen);

         count = (s->io.read)(s->io_user_data, (char*) buffer + blen, n - blen);
         res = (count == (n-blen));
         s->img_buffer = s->img_buffer_end;
         return res;
      }
   }

   if (s->img_buffer+n <= s->img_buffer_end) {
      memcpy(buffer, s->img_buffer, n);
      s->img_buffer += n;
      return 1;
   } else
      return 0;
}
#endif

#if defined(STBI_NO_JPEG) && defined(STBI_NO_PNG) && defined(STBI_NO_PSD) && defined(STBI_NO_PIC)
// nothing
#else
static int stbi__get16be(stbi__context *s)
{
   int z = stbi__get8(s);
   return (z << 8) + stbi__get8(s);
}
#endif

#if defined(STBI_NO_PNG) && defined(STBI_NO_PSD) && defined(STBI_NO_PIC)
// nothing
#else
static stbi__uint32 stbi__get32be(stbi__context *s)
{
   stbi__uint32 z = stbi__get16be(s);
   return (z << 16) + stbi__get16be(s);
}
#endif

#if defined(STBI_NO_BMP) && defined(STBI_NO_TGA) && defined(STBI_NO_GIF)
// nothing
#else
static int stbi__get16le(stbi__context *s)
{
   int z = stbi__get8(s);
   return z + (stbi__get8(s) << 8);
}
#endif

#ifndef STBI_NO_BMP
static stbi__uint32 stbi__get32le(stbi__context *s)
{
   stbi__uint32 z = stbi__get16le(s);
   z += (stbi__uint32)stbi__get16le(s) << 16;
   return z;
}
#endif

#define STBI__BYTECAST(x)  ((stbi_uc) ((x) & 255))  // truncate int to byte without warnings

#if defined(STBI_NO_JPEG) && defined(STBI_NO_PNG) && defined(STBI_NO_BMP) && defined(STBI_NO_PSD) && defined(STBI_NO_TGA) && defined(STBI_NO_GIF) && defined(STBI_NO_PIC) && defined(STBI_NO_PNM)
// nothing
#else
//////////////////////////////////////////////////////////////////////////////
//
//  generic converter from built-in img_n to req_comp
//    individual types do this automatically as much as possible (e.g. jpeg
//    does all cases internally since it needs to colorspace convert anyway,
//    and it never has alpha, so very few cases ). png can automatically
//    interleave an alpha=255 channel, but falls back to this for other cases
//
//  assume data buffer is malloced, so malloc a new one and free that one
//  only failure mode is malloc failing

static stbi_uc stbi__compute_y(int r, int g, int b)
{
   return (stbi_uc) (((r*77) + (g*150) +  (29*b)) >> 8);
}
#endif

#if defined(STBI_NO_PNG) && defined(STBI_NO_BMP) && defined(STBI_NO_PSD) && defined(STBI_NO_TGA) && defined(STBI_NO_GIF) && defined(STBI_NO_PIC) && defined(STBI_NO_PNM)
// nothing
#else
static unsigned char *stbi__convert_format(unsigned char *data, int img_n, int req_comp, unsigned int x, unsigned int y)
{
   int i,j;
   unsigned char *good;

   if (req_comp == img_n) return data;
   STBI_ASSERT(req_comp >= 1 && req_comp <= 4);

   good = (unsigned char *) stbi__malloc_mad3(req_comp, x, y, 0);
   if (good == NULL) {
      STBI_FREE(data);
      return stbi__errpuc("outofmem", "Out of memory");
   }

   for (j=0; j < (int) y; ++j) {
      unsigned char *src  = data + j * x * img_n   ;
      unsigned char *dest = good + j * x * req_comp;

      #define STBI__COMBO(a,b)  ((a)*8+(b))
      #define STBI__CASE(a,b)   case STBI__COMBO(a,b): for(i=x-1; i >= 0; --i, src += a, dest += b)
      // convert source image with img_n components to one with req_comp components;
      // avoid switch per pixel, so use switch per scanline and massive macros
      switch (STBI__COMBO(img_n, req_comp)) {
         STBI__CASE(1,2) { dest[0]=src[0]; dest[1]=255;                                     } break;
         STBI__CASE(1,3) { dest[0]=dest[1]=dest[2]=src[0];                                  } break;
         STBI__CASE(1,4) { dest[0]=dest[1]=dest[2]=src[0]; dest[3]=255;                     } break;
         STBI__CASE(2,1) { dest[0]=src[0];                                                  } break;
         STBI__CASE(2,3) { dest[0]=dest[1]=dest[2]=src[0];                                  } break;
         STBI__CASE(2,4) { dest[0]=dest[1]=dest[2]=src[0]; dest[3]=src[1];                  } break;
         STBI__CASE(3,4) { dest[0]=src[0];dest[1]=src[1];dest[2]=src[2];dest[3]=255;        } break;
         STBI__CASE(3,1) { dest[0]=stbi__compute_y(src[0],src[1],src[2]);                   } break;
         STBI__CASE(3,2) { dest[0]=stbi__compute_y(src[0],src[1],src[2]); dest[1] = 255;    } break;
         STBI__CASE(4,1) { dest[0]=stbi__compute_y(src[0],src[1],src[2]);                   } break;
         STBI__CASE(4,2) { dest[0]=stbi__compute_y(src[0],src[1],src[2]); dest[1] = src[3]; } break;
         STBI__CASE(4,3) { dest[0]=src[0];dest[1]=src[1];dest[2]=src[2];                    } break;
         default: STBI_ASSERT(0); STBI_FREE(data); STBI_FREE(good); return stbi__errpuc("unsupported", "Unsupported format conversion");
      }
      #undef STBI__CASE
   }

   STBI_FREE(data);
   return good;
}
#endif

#if defined(STBI_NO_PNG) && defined(STBI_NO_PSD)
// nothing
#else
static stbi__uint16 stbi__compute_y_16(int r, int g, int b)
{
   return (stbi__uint16) (((r*77) + (g*150) +  (29*b)) >> 8);
}
#endif

#if defined(STBI_NO_PNG) && defined(STBI_NO_PSD)
// nothing
#else
static stbi__uint16 *stbi__convert_format16(stbi__uint16 *data, int img_n, int req_comp, unsigned int x, unsigned int y)
{
   int i,j;
   stbi__uint16 *good;

   if (req_comp == img_n) return data;
   STBI_ASSERT(req_comp >= 1 && req_comp <= 4);

   good = (stbi__uint16 *) stbi__malloc(req_comp * x * y * 2);
   if (good == NULL) {
      STBI_FREE(data);
      return (stbi__uint16 *) stbi__errpuc("outofmem", "Out of memory");
   }

   for (j=0; j < (int) y; ++j) {
      stbi__uint16 *src  = data + j * x * img_n   ;
      stbi__uint16 *dest = good + j * x * req_comp;

      #define STBI__COMBO(a,b)  ((a)*8+(b))
      #define STBI__CASE(a,b)   case STBI__COMBO(a,b): for(i=x-1; i >= 0; --i, src += a, dest += b)
      // convert source image with img_n components to one with req_comp components;
      // avoid switch per pixel, so use switch per scanline and massive macros
      switch (STBI__COMBO(img_n, req_comp)) {
         STBI__CASE(1,2) { dest[0]=src[0]; dest[1]=0xffff;                                     } break;
         STBI__CASE(1,3) { dest[0]=dest[1]=dest[2]=src[0];                                     } break;
         STBI__CASE(1,4) { dest[0]=dest[1]=dest[2]=src[0]; dest[3]=0xffff;                     } break;
         STBI__CASE(2,1) { dest[0]=src[0];                                                     } break;
         STBI__CASE(2,3) { dest[0]=dest[1]=dest[2]=src[0];                                     } break;
         STBI__CASE(2,4) { dest[0]=dest[1]=dest[2]=src[0]; dest[3]=src[1];                     } break;
         STBI__CASE(3,4) { dest[0]=src[0];dest[1]=src[1];dest[2]=src[2];dest[3]=0xffff;        } break;
         STBI__CASE(3,1) { dest[0]=stbi__compute_y_16(src[0],src[1],src[2]);                   } break;
         STBI__CASE(3,2) { dest[0]=stbi__compute_y_16(src[0],src[1],src[2]); dest[1] = 0xffff; } break;
         STBI__CASE(4,1) { dest[0]=stbi__compute_y_16(src[0],src[1],src[2]);                   } break;
         STBI__CASE(4,2) { dest[0]=stbi__compute_y_16(src[0],src[1],src[2]); dest[1] = src[3]; } break;
         STBI__CASE(4,3) { dest[0]=src[0];dest[1]=src[1];dest[2]=src[2];                       } break;
         default: STBI_ASSERT(0); STBI_FREE(data); STBI_FREE(good); return (stbi__uint16*) stbi__errpuc("unsupported", "Unsupported format conversion");
      }
      #undef STBI__CASE
   }

   STBI_FREE(data);
   return good;
}
#endif

#ifndef STBI_NO_LINEAR
static float   *stbi__ldr_to_hdr(stbi_uc *data, int x, int y, int comp)
{
   int i,k,n;
   float *output;
   if (!data) return NULL;
   output = (float *) stbi__malloc_mad4(x, y, comp, sizeof(float), 0);
   if (output == NULL) { STBI_FREE(data); return stbi__errpf("outofmem", "Out of memory"); }
   // compute number of non-alpha components
   if (comp & 1) n = comp; else n = comp-1;
   for (i=0; i < x*y; ++i) {
      for (k=0; k < n; ++k) {
         output[i*comp + k] = (float) (pow(data[i*comp+k]/255.0f, stbi__l2h_gamma) * stbi__l2h_scale);
      }
   }
   if (n < comp) {
      for (i=0; i < x*y; ++i) {
         output[i*comp + n] = data[i*comp + n]/255.0f;
      }
   }
   STBI_FREE(data);
   return output;
}
#endif

#ifndef STBI_NO_HDR
#define stbi__float2int(x)   ((int) (x))
static stbi_uc *stbi__hdr_to_ldr(float   *data, int x, int y, int comp)
{
   int i,k,n;
   stbi_uc *output;
   if (!data) return NULL;
   output = (stbi_uc *) stbi__malloc_mad3(x, y, comp, 0);
   if (output == NULL) { STBI_FREE(data); return stbi__errpuc("outofmem", "Out of memory"); }
   // compute number of non-alpha components
   if (comp & 1) n = comp; else n = comp-1;
   for (i=0; i < x*y; ++i) {
      for (k=0; k < n; ++k) {
         float z = (float) pow(data[i*comp+k]*stbi__h2l_scale_i, stbi__h2l_gamma_i) * 255 + 0.5f;
         if (z < 0) z = 0;
         if (z > 255) z = 255;
         output[i*comp + k] = (stbi_uc) stbi__float2int(z);
      }
      if (k < comp) {
         float z = data[i*comp+k] * 255 + 0.5f;
         if (z < 0) z = 0;
         if (z > 255) z = 255;
         output[i*comp + k] = (stbi_uc) stbi__float2int(z);
      }
   }
   STBI_FREE(data);
   return output;
}
#endif

//////////////////////////////////////////////////////////////////////////////
//
//  "baseline" JPEG/JFIF decoder
//
//    simple implementation
//      - doesn't support delayed output of y-dimension
//      - simple interface (only one output format: 8-bit interleaved RGB)
//      - doesn't try to recover corrupt jpegs
//      - doesn't allow partial loading, loading multiple at once
//      - still fast on x86 (copying globals into locals doesn't help x86)
//      - allocates lots of intermediate memory (full size of all components)
//        - non-interleaved case requires this anyway
//        - allows good upsampling (see next)
//    high-quality
//      - upsampled channels are bilinearly interpolated, even across blocks
//      - quality integer IDCT derived from IJG's 'slow'
//    performance
//      - fast huffman; reasonable integer IDCT
//      - some SIMD kernels for common paths on targets with SSE2/NEON
//      - uses a lot of intermediate memory, could cache poorly

#ifndef STBI_NO_JPEG

// huffman decoding acceleration
#define FAST_BITS   9  // larger handles more cases; smaller stomps less cache

typedef struct
{
   stbi_uc  fast[1 << FAST_BITS];
   // weirdly, repacking this into AoS is a 10% speed loss, instead of a win
   stbi__uint16 code[256];
   stbi_uc  values[256];
   stbi_uc  size[257];
   unsigned int maxcode[18];
   int    delta[17];   // old 'firstsymbol' - old 'firstcode'
} stbi__huffman;

typedef struct
{
   stbi__context *s;
   stbi__huffman huff_dc[4];
   stbi__huffman huff_ac[4];
   stbi__uint16 dequant[4][64];
   stbi__int16 fast_ac[4][1 << FAST_BITS];

// sizes for components, interleaved MCUs
   int img_h_max, img_v_max;
   int img_mcu_x, img_mcu_y;
   int img_mcu_w, img_mcu_h;

// definition of jpeg image component
   struct
   {
      int id;
      int h,v;
      int tq;
      int hd,ha;
      int dc_pred;

      int x,y,w2,h2;
      stbi_uc *data;
      void *raw_data, *raw_coeff;
      stbi_uc *linebuf;
      short   *coeff;   // progressive only
      int      coeff_w, coeff_h; // number of 8x8 coefficient blocks
   } img_comp[4];

   stbi__uint32   code_buffer; // jpeg entropy-coded buffer
   int            code_bits;   // number of valid bits
   unsigned char  marker;      // marker seen while filling entropy buffer
   int            nomore;      // flag if we saw a marker so must stop

   int            progressive;
   int            spec_start;
   int            spec_end;
   int            succ_high;
   int            succ_low;
   int            eob_run;
   int            jfif;
   int            app14_color_transform; // Adobe APP14 tag
   int            rgb;

   int scan_n, order[4];
   int restart_interval, todo;

// kernels
   void (*idct_block_kernel)(stbi_uc *out, int out_stride, short data[64]);
   void (*YCbCr_to_RGB_kernel)(stbi_uc *out, const stbi_uc *y, const stbi_uc *pcb, const stbi_uc *pcr, int count, int step);
   stbi_uc *(*resample_row_hv_2_kernel)(stbi_uc *out, stbi_uc *in_near, stbi_uc *in_far, int w, int hs);
} stbi__jpeg;

static int stbi__build_huffman(stbi__huffman *h, int *count)
{
   int i,j,k=0;
   unsigned int code;
   // build size list for each symbol (from JPEG spec)
   for (i=0; i < 16; ++i)
      for (j=0; j < count[i]; ++j)
         h->size[k++] = (stbi_uc) (i+1);
   h->size[k] = 0;

   // compute actual symbols (from jpeg spec)
   code = 0;
   k = 0;
   for(j=1; j <= 16; ++j) {
      // compute delta to add to code to compute symbol id
      h->delta[j] = k - code;
      if (h->size[k] == j) {
         while (h->size[k] == j)
            h->code[k++] = (stbi__uint16) (code++);
         if (code-1 >= (1u << j)) return stbi__err("bad code lengths","Corrupt JPEG");
      }
      // compute largest code + 1 for this size, preshifted as needed later
      h->maxcode[j] = code << (16-j);
      code <<= 1;
   }
   h->maxcode[j] = 0xffffffff;

   // build non-spec acceleration table; 255 is flag for not-accelerated
   memset(h->fast, 255, 1 << FAST_BITS);
   for (i=0; i < k; ++i) {
      int s = h->size[i];
      if (s <= FAST_BITS) {
         int c = h->code[i] << (FAST_BITS-s);
         int m = 1 << (FAST_BITS-s);
         for (j=0; j < m; ++j) {
            h->fast[c+j] = (stbi_uc) i;
         }
      }
   }
   return 1;
}

// build a table that decodes both magnitude and value of small ACs in
// one go.
static void stbi__build_fast_ac(stbi__int16 *fast_ac, stbi__huffman *h)
{
   int i;
   for (i=0; i < (1 << FAST_BITS); ++i) {
      stbi_uc fast = h->fast[i];
      fast_ac[i] = 0;
      if (fast < 255) {
         int rs = h->values[fast];
         int run = (rs >> 4) & 15;
         int magbits = rs & 15;
         int len = h->size[fast];

         if (magbits && len + magbits <= FAST_BITS) {
            // magnitude code followed by receive_extend code
            int k = ((i << len) & ((1 << FAST_BITS) - 1)) >> (FAST_BITS - magbits);
            int m = 1 << (magbits - 1);
            if (k < m) k += (~0U << magbits) + 1;
            // if the result is small enough, we can fit it in fast_ac table
            if (k >= -128 && k <= 127)
               fast_ac[i] = (stbi__int16) ((k * 256) + (run * 16) + (len + magbits));
         }
      }
   }
}

static void stbi__grow_buffer_unsafe(stbi__jpeg *j)
{
   do {
      unsigned int b = j->nomore ? 0 : stbi__get8(j->s);
      if (b == 0xff) {
         int c = stbi__get8(j->s);
         while (c == 0xff) c = stbi__get8(j->s); // consume fill bytes
         if (c != 0) {
            j->marker = (unsigned char) c;
            j->nomore = 1;
            return;
         }
      }
      j->code_buffer |= b << (24 - j->code_bits);
      j->code_bits += 8;
   } while (j->code_bits <= 24);
}

// (1 << n) - 1
static const stbi__uint32 stbi__bmask[17]={0,1,3,7,15,31,63,127,255,511,1023,2047,4095,8191,16383,32767,65535};

// decode a jpeg huffman value from the bitstream
stbi_inline static int stbi__jpeg_huff_decode(stbi__jpeg *j, stbi__huffman *h)
{
   unsigned int temp;
   int c,k;

   if (j->code_bits < 16) stbi__grow_buffer_unsafe(j);

   // look at the top FAST_BITS and determine what symbol ID it is,
   // if the code is <= FAST_BITS
   c = (j->code_buffer >> (32 - FAST_BITS)) & ((1 << FAST_BITS)-1);
   k = h->fast[c];
   if (k < 255) {
      int s = h->size[k];
      if (s > j->code_bits)
         return -1;
      j->code_buffer <<= s;
      j->code_bits -= s;
      return h->values[k];
   }

   // naive test is to shift the code_buffer down so k bits are
   // valid, then test against maxcode. To speed this up, we've
   // preshifted maxcode left so that it has (16-k) 0s at the
   // end; in other words, regardless of the number of bits, it
   // wants to be compared against something shifted to have 16;
   // that way we don't need to shift inside the loop.
   temp = j->code_buffer >> 16;
   for (k=FAST_BITS+1 ; ; ++k)
      if (temp < h->maxcode[k])
         break;
   if (k == 17) {
      // error! code not found
      j->code_bits -= 16;
      return -1;
   }

   if (k > j->code_bits)
      return -1;

   // convert the huffman code to the symbol id
   c = ((j->code_buffer >> (32 - k)) & stbi__bmask[k]) + h->delta[k];
   STBI_ASSERT((((j->code_buffer) >> (32 - h->size[c])) & stbi__bmask[h->size[c]]) == h->code[c]);

   // convert the id to a symbol
   j->code_bits -= k;
   j->code_buffer <<= k;
   return h->values[c];
}

// bias[n] = (-1<<n) + 1
static const int stbi__jbias[16] = {0,-1,-3,-7,-15,-31,-63,-127,-255,-511,-1023,-2047,-4095,-8191,-16383,-32767};

// combined JPEG 'receive' and JPEG 'extend', since baseline
// always extends everything it receives.
stbi_inline static int stbi__extend_receive(stbi__jpeg *j, int n)
{
   unsigned int k;
   int sgn;
   if (j->code_bits < n) stbi__grow_buffer_unsafe(j);

   sgn = j->code_buffer >> 31; // sign bit always in MSB; 0 if MSB clear (positive), 1 if MSB set (negative)
   k = stbi_lrot(j->code_buffer, n);
   j->code_buffer = k & ~stbi__bmask[n];
   k &= stbi__bmask[n];
   j->code_bits -= n;
   return k + (stbi__jbias[n] & (sgn - 1));
}

// get some unsigned bits
stbi_inline static int stbi__jpeg_get_bits(stbi__jpeg *j, int n)
{
   unsigned int k;
   if (j->code_bits < n) stbi__grow_buffer_unsafe(j);
   k = stbi_lrot(j->code_buffer, n);
   j->code_buffer = k & ~stbi__bmask[n];
   k &= stbi__bmask[n];
   j->code_bits -= n;
   return k;
}

stbi_inline static int stbi__jpeg_get_bit(stbi__jpeg *j)
{
   unsigned int k;
   if (j->code_bits < 1) stbi__grow_buffer_unsafe(j);
   k = j->code_buffer;
   j->code_buffer <<= 1;
   --j->code_bits;
   return k & 0x80000000;
}

// given a value that's at position X in the zigzag stream,
// where does it appear in the 8x8 matrix coded as row-major?
static const stbi_uc stbi__jpeg_dezigzag[64+15] =
{
    0,  1,  8, 16,  9,  2,  3, 10,
   17, 24, 32, 25, 18, 11,  4,  5,
   12, 19, 26, 33, 40, 48, 41, 34,
   27, 20, 13,  6,  7, 14, 21, 28,
   35, 42, 49, 56, 57, 50, 43, 36,
   29, 22, 15, 23, 30, 37, 44, 51,
   58, 59, 52, 45, 38, 31, 39, 46,
   53, 60, 61, 54, 47, 55, 62, 63,
   // let corrupt input sample past end
   63, 63, 63, 63, 63, 63, 63, 63,
   63, 63, 63, 63, 63, 63, 63
};

// decode one 64-entry block--
static int stbi__jpeg_decode_block(stbi__jpeg *j, short data[64], stbi__huffman *hdc, stbi__huffman *hac, stbi__int16 *fac, int b, stbi__uint16 *dequant)
{
   int diff,dc,k;
   int t;

   if (j->code_bits < 16) stbi__grow_buffer_unsafe(j);
   t = stbi__jpeg_huff_decode(j, hdc);
   if (t < 0 || t > 15) return stbi__err("bad huffman code","Corrupt JPEG");

   // 0 all the ac values now so we can do it 32-bits at a time
   memset(data,0,64*sizeof(data[0]));

   diff = t ? stbi__extend_receive(j, t) : 0;
   dc = j->img_comp[b].dc_pred + diff;
   j->img_comp[b].dc_pred = dc;
   data[0] = (short) (dc * dequant[0]);

   // decode AC components, see JPEG spec
   k = 1;
   do {
      unsigned int zig;
      int c,r,s;
      if (j->code_bits < 16) stbi__grow_buffer_unsafe(j);
      c = (j->code_buffer >> (32 - FAST_BITS)) & ((1 << FAST_BITS)-1);
      r = fac[c];
      if (r) { // fast-AC path
         k += (r >> 4) & 15; // run
         s = r & 15; // combined length
         j->code_buffer <<= s;
         j->code_bits -= s;
         // decode into unzigzag'd location
         zig = stbi__jpeg_dezigzag[k++];
         data[zig] = (short) ((r >> 8) * dequant[zig]);
      } else {
         int rs = stbi__jpeg_huff_decode(j, hac);
         if (rs < 0) return stbi__err("bad huffman code","Corrupt JPEG");
         s = rs & 15;
         r = rs >> 4;
         if (s == 0) {
            if (rs != 0xf0) break; // end block
            k += 16;
         } else {
            k += r;
            // decode into unzigzag'd location
            zig = stbi__jpeg_dezigzag[k++];
            data[zig] = (short) (stbi__extend_receive(j,s) * dequant[zig]);
         }
      }
   } while (k < 64);
   return 1;
}

static int stbi__jpeg_decode_block_prog_dc(stbi__jpeg *j, short data[64], stbi__huffman *hdc, int b)
{
   int diff,dc;
   int t;
   if (j->spec_end != 0) return stbi__err("can't merge dc and ac", "Corrupt JPEG");

   if (j->code_bits < 16) stbi__grow_buffer_unsafe(j);

   if (j->succ_high == 0) {
      // first scan for DC coefficient, must be first
      memset(data,0,64*sizeof(data[0])); // 0 all the ac values now
      t = stbi__jpeg_huff_decode(j, hdc);
      if (t < 0 || t > 15) return stbi__err("can't merge dc and ac", "Corrupt JPEG");
      diff = t ? stbi__extend_receive(j, t) : 0;

      dc = j->img_comp[b].dc_pred + diff;
      j->img_comp[b].dc_pred = dc;
      data[0] = (short) (dc * (1 << j->succ_low));
   } else {
      // refinement scan for DC coefficient
      if (stbi__jpeg_get_bit(j))
         data[0] += (short) (1 << j->succ_low);
   }
   return 1;
}

// @OPTIMIZE: store non-zigzagged during the decode passes,
// and only de-zigzag when dequantizing
static int stbi__jpeg_decode_block_prog_ac(stbi__jpeg *j, short data[64], stbi__huffman *hac, stbi__int16 *fac)
{
   int k;
   if (j->spec_start == 0) return stbi__err("can't merge dc and ac", "Corrupt JPEG");

   if (j->succ_high == 0) {
      int shift = j->succ_low;

      if (j->eob_run) {
         --j->eob_run;
         return 1;
      }

      k = j->spec_start;
      do {
         unsigned int zig;
         int c,r,s;
         if (j->code_bits < 16) stbi__grow_buffer_unsafe(j);
         c = (j->code_buffer >> (32 - FAST_BITS)) & ((1 << FAST_BITS)-1);
         r = fac[c];
         if (r) { // fast-AC path
            k += (r >> 4) & 15; // run
            s = r & 15; // combined length
            j->code_buffer <<= s;
            j->code_bits -= s;
            zig = stbi__jpeg_dezigzag[k++];
            data[zig] = (short) ((r >> 8) * (1 << shift));
         } else {
            int rs = stbi__jpeg_huff_decode(j, hac);
            if (rs < 0) return stbi__err("bad huffman code","Corrupt JPEG");
            s = rs & 15;
            r = rs >> 4;
            if (s == 0) {
               if (r < 15) {
                  j->eob_run = (1 << r);
                  if (r)
                     j->eob_run += stbi__jpeg_get_bits(j, r);
                  --j->eob_run;
                  break;
               }
               k += 16;
            } else {
               k += r;
               zig = stbi__jpeg_dezigzag[k++];
               data[zig] = (short) (stbi__extend_receive(j,s) * (1 << shift));
            }
         }
      } while (k <= j->spec_end);
   } else {
      // refinement scan for these AC coefficients

      short bit = (short) (1 << j->succ_low);

      if (j->eob_run) {
         --j->eob_run;
         for (k = j->spec_start; k <= j->spec_end; ++k) {
            short *p = &data[stbi__jpeg_dezigzag[k]];
            if (*p != 0)
               if (stbi__jpeg_get_bit(j))
                  if ((*p & bit)==0) {
                     if (*p > 0)
                        *p += bit;
                     else
                        *p -= bit;
                  }
         }
      } else {
         k = j->spec_start;
         do {
            int r,s;
            int rs = stbi__jpeg_huff_decode(j, hac); // @OPTIMIZE see if we can use the fast path here, advance-by-r is so slow, eh
            if (rs < 0) return stbi__err("bad huffman code","Corrupt JPEG");
            s = rs & 15;
            r = rs >> 4;
            if (s == 0) {
               if (r < 15) {
                  j->eob_run = (1 << r) - 1;
                  if (r)
                     j->eob_run += stbi__jpeg_get_bits(j, r);
                  r = 64; // force end of block
               } else {
                  // r=15 s=0 should write 16 0s, so we just do
                  // a run of 15 0s and then write s (which is 0),
                  // so we don't have to do anything special here
               }
            } else {
               if (s != 1) return stbi__err("bad huffman code", "Corrupt JPEG");
               // sign bit
               if (stbi__jpeg_get_bit(j))
                  s = bit;
               else
                  s = -bit;
            }

            // advance by r
            while (k <= j->spec_end) {
               short *p = &data[stbi__jpeg_dezigzag[k++]];
               if (*p != 0) {
                  if (stbi__jpeg_get_bit(j))
                     if ((*p & bit)==0) {
                        if (*p > 0)
                           *p += bit;
                        else
                           *p -= bit;
                     }
               } else {
                  if (r == 0) {
                     *p = (short) s;
                     break;
                  }
                  --r;
               }
            }
         } while (k <= j->spec_end);
      }
   }
   return 1;
}

// take a -128..127 value and stbi__clamp it and convert to 0..255
stbi_inline static stbi_uc stbi__clamp(int x)
{
   // trick to use a single test to catch both cases
   if ((unsigned int) x > 255) {
      if (x < 0) return 0;
      if (x > 255) return 255;
   }
   return (stbi_uc) x;
}

#define stbi__f2f(x)  ((int) (((x) * 4096 + 0.5)))
#define stbi__fsh(x)  ((x) * 4096)

// derived from jidctint -- DCT_ISLOW
#define STBI__IDCT_1D(s0,s1,s2,s3,s4,s5,s6,s7) \
   int t0,t1,t2,t3,p1,p2,p3,p4,p5,x0,x1,x2,x3; \
   p2 = s2;                                    \
   p3 = s6;                                    \
   p1 = (p2+p3) * stbi__f2f(0.5411961f);       \
   t2 = p1 + p3*stbi__f2f(-1.847759065f);      \
   t3 = p1 + p2*stbi__f2f( 0.765366865f);      \
   p2 = s0;                                    \
   p3 = s4;                                    \
   t0 = stbi__fsh(p2+p3);                      \
   t1 = stbi__fsh(p2-p3);                      \
   x0 = t0+t3;                                 \
   x3 = t0-t3;                                 \
   x1 = t1+t2;                                 \
   x2 = t1-t2;                                 \
   t0 = s7;                                    \
   t1 = s5;                                    \
   t2 = s3;                                    \
   t3 = s1;                                    \
   p3 = t0+t2;                                 \
   p4 = t1+t3;                                 \
   p1 = t0+t3;                                 \
   p2 = t1+t2;                                 \
   p5 = (p3+p4)*stbi__f2f( 1.175875602f);      \
   t0 = t0*stbi__f2f( 0.298631336f);           \
   t1 = t1*stbi__f2f( 2.053119869f);           \
   t2 = t2*stbi__f2f( 3.072711026f);           \
   t3 = t3*stbi__f2f( 1.501321110f);           \
   p1 = p5 + p1*stbi__f2f(-0.899976223f);      \
   p2 = p5 + p2*stbi__f2f(-2.562915447f);      \
   p3 = p3*stbi__f2f(-1.961570560f);           \
   p4 = p4*stbi__f2f(-0.390180644f);           \
   t3 += p1+p4;                                \
   t2 += p2+p3;                                \
   t1 += p2+p4;                                \
   t0 += p1+p3;

static void stbi__idct_block(stbi_uc *out, int out_stride, short data[64])
{
   int i,val[64],*v=val;
   stbi_uc *o;
   short *d = data;

   // columns
   for (i=0; i < 8; ++i,++d, ++v) {
      // if all zeroes, shortcut -- this avoids dequantizing 0s and IDCTing
      if (d[ 8]==0 && d[16]==0 && d[24]==0 && d[32]==0
           && d[40]==0 && d[48]==0 && d[56]==0) {
         //    no shortcut                 0     seconds
         //    (1|2|3|4|5|6|7)==0          0     seconds
         //    all separate               -0.047 seconds
         //    1 && 2|3 && 4|5 && 6|7:    -0.047 seconds
         int dcterm = d[0]*4;
         v[0] = v[8] = v[16] = v[24] = v[32] = v[40] = v[48] = v[56] = dcterm;
      } else {
         STBI__IDCT_1D(d[ 0],d[ 8],d[16],d[24],d[32],d[40],d[48],d[56])
         // constants scaled things up by 1<<12; let's bring them back
         // down, but keep 2 extra bits of precision
         x0 += 512; x1 += 512; x2 += 512; x3 += 512;
         v[ 0] = (x0+t3) >> 10;
         v[56] = (x0-t3) >> 10;
         v[ 8] = (x1+t2) >> 10;
         v[48] = (x1-t2) >> 10;
         v[16] = (x2+t1) >> 10;
         v[40] = (x2-t1) >> 10;
         v[24] = (x3+t0) >> 10;
         v[32] = (x3-t0) >> 10;
      }
   }

   for (i=0, v=val, o=out; i < 8; ++i,v+=8,o+=out_stride) {
      // no fast case since the first 1D IDCT spread components out
      STBI__IDCT_1D(v[0],v[1],v[2],v[3],v[4],v[5],v[6],v[7])
      // constants scaled things up by 1<<12, plus we had 1<<2 from first
      // loop, plus horizontal and vertical each scale by sqrt(8) so together
      // we've got an extra 1<<3, so 1<<17 total we need to remove.
      // so we want to round that, which means adding 0.5 * 1<<17,
      // aka 65536. Also, we'll end up with -128 to 127 that we want
      // to encode as 0..255 by adding 128, so we'll add that before the shift
      x0 += 65536 + (128<<17);
      x1 += 65536 + (128<<17);
      x2 += 65536 + (128<<17);
      x3 += 65536 + (128<<17);
      // tried computing the shifts into temps, or'ing the temps to see
      // if any were out of range, but that was slower
      o[0] = stbi__clamp((x0+t3) >> 17);
      o[7] = stbi__clamp((x0-t3) >> 17);
      o[1] = stbi__clamp((x1+t2) >> 17);
      o[6] = stbi__clamp((x1-t2) >> 17);
      o[2] = stbi__clamp((x2+t1) >> 17);
      o[5] = stbi__clamp((x2-t1) >> 17);
      o[3] = stbi__clamp((x3+t0) >> 17);
      o[4] = stbi__clamp((x3-t0) >> 17);
   }
}

#ifdef STBI_SSE2
// sse2 integer IDCT. not the fastest possible implementation but it
// produces bit-identical results to the generic C version so it's
// fully "transparent".
static void stbi__idct_simd(stbi_uc *out, int out_stride, short data[64])
{
   // This is constructed to match our regular (generic) integer IDCT exactly.
   __m128i row0, row1, row2, row3, row4, row5, row6, row7;
   __m128i tmp;

   // dot product constant: even elems=x, odd elems=y
   #define dct_const(x,y)  _mm_setr_epi16((x),(y),(x),(y),(x),(y),(x),(y))

   // out(0) = c0[even]*x + c0[odd]*y   (c0, x, y 16-bit, out 32-bit)
   // out(1) = c1[even]*x + c1[odd]*y
   #define dct_rot(out0,out1, x,y,c0,c1) \
      __m128i c0##lo = _mm_unpacklo_epi16((x),(y)); \
      __m128i c0##hi = _mm_unpackhi_epi16((x),(y)); \
      __m128i out0##_l = _mm_madd_epi16(c0##lo, c0); \
      __m128i out0##_h = _mm_madd_epi16(c0##hi, c0); \
      __m128i out1##_l = _mm_madd_epi16(c0##lo, c1); \
      __m128i out1##_h = _mm_madd_epi16(c0##hi, c1)

   // out = in << 12  (in 16-bit, out 32-bit)
   #define dct_widen(out, in) \
      __m128i out##_l = _mm_srai_epi32(_mm_unpacklo_epi16(_mm_setzero_si128(), (in)), 4); \
      __m128i out##_h = _mm_srai_epi32(_mm_unpackhi_epi16(_mm_setzero_si128(), (in)), 4)

   // wide add
   #define dct_wadd(out, a, b) \
      __m128i out##_l = _mm_add_epi32(a##_l, b##_l); \
      __m128i out##_h = _mm_add_epi32(a##_h, b##_h)

   // wide sub
   #define dct_wsub(out, a, b) \
      __m128i out##_l = _mm_sub_epi32(a##_l, b##_l); \
      __m128i out##_h = _mm_sub_epi32(a##_h, b##_h)

   // butterfly a/b, add bias, then shift by "s" and pack
   #define dct_bfly32o(out0, out1, a,b,bias,s) \
      { \
         __m128i abiased_l = _mm_add_epi32(a##_l, bias); \
         __m128i abiased_h = _mm_add_epi32(a##_h, bias); \
         dct_wadd(sum, abiased, b); \
         dct_wsub(dif, abiased, b); \
         out0 = _mm_packs_epi32(_mm_srai_epi32(sum_l, s), _mm_srai_epi32(sum_h, s)); \
         out1 = _mm_packs_epi32(_mm_srai_epi32(dif_l, s), _mm_srai_epi32(dif_h, s)); \
      }

   // 8-bit interleave step (for transposes)
   #define dct_interleave8(a, b) \
      tmp = a; \
      a = _mm_unpacklo_epi8(a, b); \
      b = _mm_unpackhi_epi8(tmp, b)

   // 16-bit interleave step (for transposes)
   #define dct_interleave16(a, b) \
      tmp = a; \
      a = _mm_unpacklo_epi16(a, b); \
      b = _mm_unpackhi_epi16(tmp, b)

   #define dct_pass(bias,shift) \
      { \
         /* even part */ \
         dct_rot(t2e,t3e, row2,row6, rot0_0,rot0_1); \
         __m128i sum04 = _mm_add_epi16(row0, row4); \
         __m128i dif04 = _mm_sub_epi16(row0, row4); \
         dct_widen(t0e, sum04); \
         dct_widen(t1e, dif04); \
         dct_wadd(x0, t0e, t3e); \
         dct_wsub(x3, t0e, t3e); \
         dct_wadd(x1, t1e, t2e); \
         dct_wsub(x2, t1e, t2e); \
         /* odd part */ \
         dct_rot(y0o,y2o, row7,row3, rot2_0,rot2_1); \
         dct_rot(y1o,y3o, row5,row1, rot3_0,rot3_1); \
         __m128i sum17 = _mm_add_epi16(row1, row7); \
         __m128i sum35 = _mm_add_epi16(row3, row5); \
         dct_rot(y4o,y5o, sum17,sum35, rot1_0,rot1_1); \
         dct_wadd(x4, y0o, y4o); \
         dct_wadd(x5, y1o, y5o); \
         dct_wadd(x6, y2o, y5o); \
         dct_wadd(x7, y3o, y4o); \
         dct_bfly32o(row0,row7, x0,x7,bias,shift); \
         dct_bfly32o(row1,row6, x1,x6,bias,shift); \
         dct_bfly32o(row2,row5, x2,x5,bias,shift); \
         dct_bfly32o(row3,row4, x3,x4,bias,shift); \
      }

   __m128i rot0_0 = dct_const(stbi__f2f(0.5411961f), stbi__f2f(0.5411961f) + stbi__f2f(-1.847759065f));
   __m128i rot0_1 = dct_const(stbi__f2f(0.5411961f) + stbi__f2f( 0.765366865f), stbi__f2f(0.5411961f));
   __m128i rot1_0 = dct_const(stbi__f2f(1.175875602f) + stbi__f2f(-0.899976223f), stbi__f2f(1.175875602f));
   __m128i rot1_1 = dct_const(stbi__f2f(1.175875602f), stbi__f2f(1.175875602f) + stbi__f2f(-2.562915447f));
   __m128i rot2_0 = dct_const(stbi__f2f(-1.961570560f) + stbi__f2f( 0.298631336f), stbi__f2f(-1.961570560f));
   __m128i rot2_1 = dct_const(stbi__f2f(-1.961570560f), stbi__f2f(-1.961570560f) + stbi__f2f( 3.072711026f));
   __m128i rot3_0 = dct_const(stbi__f2f(-0.390180644f) + stbi__f2f( 2.053119869f), stbi__f2f(-0.390180644f));
   __m128i rot3_1 = dct_const(stbi__f2f(-0.390180644f), stbi__f2f(-0.390180644f) + stbi__f2f( 1.501321110f));

   // rounding biases in column/row passes, see stbi__idct_block for explanation.
   __m128i bias_0 = _mm_set1_epi32(512);
   __m128i bias_1 = _mm_set1_epi32(65536 + (128<<17));

   // load
   row0 = _mm_load_si128((const __m128i *) (data + 0*8));
   row1 = _mm_load_si128((const __m128i *) (data + 1*8));
   row2 = _mm_load_si128((const __m128i *) (data + 2*8));
   row3 = _mm_load_si128((const __m128i *) (data + 3*8));
   row4 = _mm_load_si128((const __m128i *) (data + 4*8));
   row5 = _mm_load_si128((const __m128i *) (data + 5*8));
   row6 = _mm_load_si128((const __m128i *) (data + 6*8));
   row7 = _mm_load_si128((const __m128i *) (data + 7*8));

   // column pass
   dct_pass(bias_0, 10);

   {
      // 16bit 8x8 transpose pass 1
      dct_interleave16(row0, row4);
      dct_interleave16(row1, row5);
      dct_interleave16(row2, row6);
      dct_interleave16(row3, row7);

      // transpose pass 2
      dct_interleave16(row0, row2);
      dct_interleave16(row1, row3);
      dct_interleave16(row4, row6);
      dct_interleave16(row5, row7);

      // transpose pass 3
      dct_interleave16(row0, row1);
      dct_interleave16(row2, row3);
      dct_interleave16(row4, row5);
      dct_interleave16(row6, row7);
   }

   // row pass
   dct_pass(bias_1, 17);

   {
      // pack
      __m128i p0 = _mm_packus_epi16(row0, row1); // a0a1a2a3...a7b0b1b2b3...b7
      __m128i p1 = _mm_packus_epi16(row2, row3);
      __m128i p2 = _mm_packus_epi16(row4, row5);
      __m128i p3 = _mm_packus_epi16(row6, row7);

      // 8bit 8x8 transpose pass 1
      dct_interleave8(p0, p2); // a0e0a1e1...
      dct_interleave8(p1, p3); // c0g0c1g1...

      // transpose pass 2
      dct_interleave8(p0, p1); // a0c0e0g0...
      dct_interleave8(p2, p3); // b0d0f0h0...

      // transpose pass 3
      dct_interleave8(p0, p2); // a0b0c0d0...
      dct_interleave8(p1, p3); // a4b4c4d4...

      // store
      _mm_storel_epi64((__m128i *) out, p0); out += out_stride;
      _mm_storel_epi64((__m128i *) out, _mm_shuffle_epi32(p0, 0x4e)); out += out_stride;
      _mm_storel_epi64((__m128i *) out, p2); out += out_stride;
      _mm_storel_epi64((__m128i *) out, _mm_shuffle_epi32(p2, 0x4e)); out += out_stride;
      _mm_storel_epi64((__m128i *) out, p1); out += out_stride;
      _mm_storel_epi64((__m128i *) out, _mm_shuffle_epi32(p1, 0x4e)); out += out_stride;
      _mm_storel_epi64((__m128i *) out, p3); out += out_stride;
      _mm_storel_epi64((__m128i *) out, _mm_shuffle_epi32(p3, 0x4e));
   }

#undef dct_const
#undef dct_rot
#undef dct_widen
#undef dct_wadd
#undef dct_wsub
#undef dct_bfly32o
#undef dct_interleave8
#undef dct_interleave16
#undef dct_pass
}

#endif // STBI_SSE2

#ifdef STBI_NEON

// NEON integer IDCT. should produce bit-identical
// results to the generic C version.
static void stbi__idct_simd(stbi_uc *out, int out_stride, short data[64])
{
   int16x8_t row0, row1, row2, row3, row4, row5, row6, row7;

   int16x4_t rot0_0 = vdup_n_s16(stbi__f2f(0.5411961f));
   int16x4_t rot0_1 = vdup_n_s16(stbi__f2f(-1.847759065f));
   int16x4_t rot0_2 = vdup_n_s16(stbi__f2f( 0.765366865f));
   int16x4_t rot1_0 = vdup_n_s16(stbi__f2f( 1.175875602f));
   int16x4_t rot1_1 = vdup_n_s16(stbi__f2f(-0.899976223f));
   int16x4_t rot1_2 = vdup_n_s16(stbi__f2f(-2.562915447f));
   int16x4_t rot2_0 = vdup_n_s16(stbi__f2f(-1.961570560f));
   int16x4_t rot2_1 = vdup_n_s16(stbi__f2f(-0.390180644f));
   int16x4_t rot3_0 = vdup_n_s16(stbi__f2f( 0.298631336f));
   int16x4_t rot3_1 = vdup_n_s16(stbi__f2f( 2.053119869f));
   int16x4_t rot3_2 = vdup_n_s16(stbi__f2f( 3.072711026f));
   int16x4_t rot3_3 = vdup_n_s16(stbi__f2f( 1.501321110f));

#define dct_long_mul(out, inq, coeff) \
   int32x4_t out##_l = vmull_s16(vget_low_s16(inq), coeff); \
   int32x4_t out##_h = vmull_s16(vget_high_s16(inq), coeff)

#define dct_long_mac(out, acc, inq, coeff) \
   int32x4_t out##_l = vmlal_s16(acc##_l, vget_low_s16(inq), coeff); \
   int32x4_t out##_h = vmlal_s16(acc##_h, vget_high_s16(inq), coeff)

#define dct_widen(out, inq) \
   int32x4_t out##_l = vshll_n_s16(vget_low_s16(inq), 12); \
   int32x4_t out##_h = vshll_n_s16(vget_high_s16(inq), 12)

// wide add
#define dct_wadd(out, a, b) \
   int32x4_t out##_l = vaddq_s32(a##_l, b##_l); \
   int32x4_t out##_h = vaddq_s32(a##_h, b##_h)

// wide sub
#define dct_wsub(out, a, b) \
   int32x4_t out##_l = vsubq_s32(a##_l, b##_l); \
   int32x4_t out##_h = vsubq_s32(a##_h, b##_h)

// butterfly a/b, then shift using "shiftop" by "s" and pack
#define dct_bfly32o(out0,out1, a,b,shiftop,s) \
   { \
      dct_wadd(sum, a, b); \
      dct_wsub(dif, a, b); \
      out0 = vcombine_s16(shiftop(sum_l, s), shiftop(sum_h, s)); \
      out1 = vcombine_s16(shiftop(dif_l, s), shiftop(dif_h, s)); \
   }

#define dct_pass(shiftop, shift) \
   { \
      /* even part */ \
      int16x8_t sum26 = vaddq_s16(row2, row6); \
      dct_long_mul(p1e, sum26, rot0_0); \
      dct_long_mac(t2e, p1e, row6, rot0_1); \
      dct_long_mac(t3e, p1e, row2, rot0_2); \
      int16x8_t sum04 = vaddq_s16(row0, row4); \
      int16x8_t dif04 = vsubq_s16(row0, row4); \
      dct_widen(t0e, sum04); \
      dct_widen(t1e, dif04); \
      dct_wadd(x0, t0e, t3e); \
      dct_wsub(x3, t0e, t3e); \
      dct_wadd(x1, t1e, t2e); \
      dct_wsub(x2, t1e, t2e); \
      /* odd part */ \
      int16x8_t sum15 = vaddq_s16(row1, row5); \
      int16x8_t sum17 = vaddq_s16(row1, row7); \
      int16x8_t sum35 = vaddq_s16(row3, row5); \
      int16x8_t sum37 = vaddq_s16(row3, row7); \
      int16x8_t sumodd = vaddq_s16(sum17, sum35); \
      dct_long_mul(p5o, sumodd, rot1_0); \
      dct_long_mac(p1o, p5o, sum17, rot1_1); \
      dct_long_mac(p2o, p5o, sum35, rot1_2); \
      dct_long_mul(p3o, sum37, rot2_0); \
      dct_long_mul(p4o, sum15, rot2_1); \
      dct_wadd(sump13o, p1o, p3o); \
      dct_wadd(sump24o, p2o, p4o); \
      dct_wadd(sump23o, p2o, p3o); \
      dct_wadd(sump14o, p1o, p4o); \
      dct_long_mac(x4, sump13o, row7, rot3_0); \
      dct_long_mac(x5, sump24o, row5, rot3_1); \
      dct_long_mac(x6, sump23o, row3, rot3_2); \
      dct_long_mac(x7, sump14o, row1, rot3_3); \
      dct_bfly32o(row0,row7, x0,x7,shiftop,shift); \
      dct_bfly32o(row1,row6, x1,x6,shiftop,shift); \
      dct_bfly32o(row2,row5, x2,x5,shiftop,shift); \
      dct_bfly32o(row3,row4, x3,x4,shiftop,shift); \
   }

   // load
   row0 = vld1q_s16(data + 0*8);
   row1 = vld1q_s16(data + 1*8);
   row2 = vld1q_s16(data + 2*8);
   row3 = vld1q_s16(data + 3*8);
   row4 = vld1q_s16(data + 4*8);
   row5 = vld1q_s16(data + 5*8);
   row6 = vld1q_s16(data + 6*8);
   row7 = vld1q_s16(data + 7*8);

   // add DC bias
   row0 = vaddq_s16(row0, vsetq_lane_s16(1024, vdupq_n_s16(0), 0));

   // column pass
   dct_pass(vrshrn_n_s32, 10);

   // 16bit 8x8 transpose
   {
// these three map to a single VTRN.16, VTRN.32, and VSWP, respectively.
// whether compilers actually get this is another story, sadly.
#define dct_trn16(x, y) { int16x8x2_t t = vtrnq_s16(x, y); x = t.val[0]; y = t.val[1]; }
#define dct_trn32(x, y) { int32x4x2_t t = vtrnq_s32(vreinterpretq_s32_s16(x), vreinterpretq_s32_s16(y)); x = vreinterpretq_s16_s32(t.val[0]); y = vreinterpretq_s16_s32(t.val[1]); }
#define dct_trn64(x, y) { int16x8_t x0 = x; int16x8_t y0 = y; x = vcombine_s16(vget_low_s16(x0), vget_low_s16(y0)); y = vcombine_s16(vget_high_s16(x0), vget_high_s16(y0)); }

      // pass 1
      dct_trn16(row0, row1); // a0b0a2b2a4b4a6b6
      dct_trn16(row2, row3);
      dct_trn16(row4, row5);
      dct_trn16(row6, row7);

      // pass 2
      dct_trn32(row0, row2); // a0b0c0d0a4b4c4d4
      dct_trn32(row1, row3);
      dct_trn32(row4, row6);
      dct_trn32(row5, row7);

      // pass 3
      dct_trn64(row0, row4); // a0b0c0d0e0f0g0h0
      dct_trn64(row1, row5);
      dct_trn64(row2, row6);
      dct_trn64(row3, row7);

#undef dct_trn16
#undef dct_trn32
#undef dct_trn64
   }

   // row pass
   // vrshrn_n_s32 only supports shifts up to 16, we need
   // 17. so do a non-rounding shift of 16 first then follow
   // up with a rounding shift by 1.
   dct_pass(vshrn_n_s32, 16);

   {
      // pack and round
      uint8x8_t p0 = vqrshrun_n_s16(row0, 1);
      uint8x8_t p1 = vqrshrun_n_s16(row1, 1);
      uint8x8_t p2 = vqrshrun_n_s16(row2, 1);
      uint8x8_t p3 = vqrshrun_n_s16(row3, 1);
      uint8x8_t p4 = vqrshrun_n_s16(row4, 1);
      uint8x8_t p5 = vqrshrun_n_s16(row5, 1);
      uint8x8_t p6 = vqrshrun_n_s16(row6, 1);
      uint8x8_t p7 = vqrshrun_n_s16(row7, 1);

      // again, these can translate into one instruction, but often don't.
#define dct_trn8_8(x, y) { uint8x8x2_t t = vtrn_u8(x, y); x = t.val[0]; y = t.val[1]; }
#define dct_trn8_16(x, y) { uint16x4x2_t t = vtrn_u16(vreinterpret_u16_u8(x), vreinterpret_u16_u8(y)); x = vreinterpret_u8_u16(t.val[0]); y = vreinterpret_u8_u16(t.val[1]); }
#define dct_trn8_32(x, y) { uint32x2x2_t t = vtrn_u32(vreinterpret_u32_u8(x), vreinterpret_u32_u8(y)); x = vreinterpret_u8_u32(t.val[0]); y = vreinterpret_u8_u32(t.val[1]); }

      // sadly can't use interleaved stores here since we only write
      // 8 bytes to each scan line!

      // 8x8 8-bit transpose pass 1
      dct_trn8_8(p0, p1);
      dct_trn8_8(p2, p3);
      dct_trn8_8(p4, p5);
      dct_trn8_8(p6, p7);

      // pass 2
      dct_trn8_16(p0, p2);
      dct_trn8_16(p1, p3);
      dct_trn8_16(p4, p6);
      dct_trn8_16(p5, p7);

      // pass 3
      dct_trn8_32(p0, p4);
      dct_trn8_32(p1, p5);
      dct_trn8_32(p2, p6);
      dct_trn8_32(p3, p7);

      // store
      vst1_u8(out, p0); out += out_stride;
      vst1_u8(out, p1); out += out_stride;
      vst1_u8(out, p2); out += out_stride;
      vst1_u8(out, p3); out += out_stride;
      vst1_u8(out, p4); out += out_stride;
      vst1_u8(out, p5); out += out_stride;
      vst1_u8(out, p6); out += out_stride;
      vst1_u8(out, p7);

#undef dct_trn8_8
#undef dct_trn8_16
#undef dct_trn8_32
   }

#undef dct_long_mul
#undef dct_long_mac
#undef dct_widen
#undef dct_wadd
#undef dct_wsub
#undef dct_bfly32o
#undef dct_pass
}

#endif // STBI_NEON

#define STBI__MARKER_none  0xff
// if there's a pending marker from the entropy stream, return that
// otherwise, fetch from the stream and get a marker. if there's no
// marker, return 0xff, which is never a valid marker value
static stbi_uc stbi__get_marker(stbi__jpeg *j)
{
   stbi_uc x;
   if (j->marker != STBI__MARKER_none) { x = j->marker; j->marker = STBI__MARKER_none; return x; }
   x = stbi__get8(j->s);
   if (x != 0xff) return STBI__MARKER_none;
   while (x == 0xff)
      x = stbi__get8(j->s); // consume repeated 0xff fill bytes
   return x;
}

// in each scan, we'll have scan_n components, and the order
// of the components is specified by order[]
#define STBI__RESTART(x)     ((x) >= 0xd0 && (x) <= 0xd7)

// after a restart interval, stbi__jpeg_reset the entropy decoder and
// the dc prediction
static void stbi__jpeg_reset(stbi__jpeg *j)
{
   j->code_bits = 0;
   j->code_buffer = 0;
   j->nomore = 0;
   j->img_comp[0].dc_pred = j->img_comp[1].dc_pred = j->img_comp[2].dc_pred = j->img_comp[3].dc_pred = 0;
   j->marker = STBI__MARKER_none;
   j->todo = j->restart_interval ? j->restart_interval : 0x7fffffff;
   j->eob_run = 0;
   // no more than 1<<31 MCUs if no restart_interal? that's plenty safe,
   // since we don't even allow 1<<30 pixels
}

static int stbi__parse_entropy_coded_data(stbi__jpeg *z)
{
   stbi__jpeg_reset(z);
   if (!z->progressive) {
      if (z->scan_n == 1) {
         int i,j;
         STBI_SIMD_ALIGN(short, data[64]);
         int n = z->order[0];
         // non-interleaved data, we just need to process one block at a time,
         // in trivial scanline order
         // number of blocks to do just depends on how many actual "pixels" this
         // component has, independent of interleaved MCU blocking and such
         int w = (z->img_comp[n].x+7) >> 3;
         int h = (z->img_comp[n].y+7) >> 3;
         for (j=0; j < h; ++j) {
            for (i=0; i < w; ++i) {
               int ha = z->img_comp[n].ha;
               if (!stbi__jpeg_decode_block(z, data, z->huff_dc+z->img_comp[n].hd, z->huff_ac+ha, z->fast_ac[ha], n, z->dequant[z->img_comp[n].tq])) return 0;
               z->idct_block_kernel(z->img_comp[n].data+z->img_comp[n].w2*j*8+i*8, z->img_comp[n].w2, data);
               // every data block is an MCU, so countdown the restart interval
               if (--z->todo <= 0) {
                  if (z->code_bits < 24) stbi__grow_buffer_unsafe(z);
                  // if it's NOT a restart, then just bail, so we get corrupt data
                  // rather than no data
                  if (!STBI__RESTART(z->marker)) return 1;
                  stbi__jpeg_reset(z);
               }
            }
         }
         return 1;
      } else { // interleaved
         int i,j,k,x,y;
         STBI_SIMD_ALIGN(short, data[64]);
         for (j=0; j < z->img_mcu_y; ++j) {
            for (i=0; i < z->img_mcu_x; ++i) {
               // scan an interleaved mcu... process scan_n components in order
               for (k=0; k < z->scan_n; ++k) {
                  int n = z->order[k];
                  // scan out an mcu's worth of this component; that's just determined
                  // by the basic H and V specified for the component
                  for (y=0; y < z->img_comp[n].v; ++y) {
                     for (x=0; x < z->img_comp[n].h; ++x) {
                        int x2 = (i*z->img_comp[n].h + x)*8;
                        int y2 = (j*z->img_comp[n].v + y)*8;
                        int ha = z->img_comp[n].ha;
                        if (!stbi__jpeg_decode_block(z, data, z->huff_dc+z->img_comp[n].hd, z->huff_ac+ha, z->fast_ac[ha], n, z->dequant[z->img_comp[n].tq])) return 0;
                        z->idct_block_kernel(z->img_comp[n].data+z->img_comp[n].w2*y2+x2, z->img_comp[n].w2, data);
                     }
                  }
               }
               // after all interleaved components, that's an interleaved MCU,
               // so now count down the restart interval
               if (--z->todo <= 0) {
                  if (z->code_bits < 24) stbi__grow_buffer_unsafe(z);
                  if (!STBI__RESTART(z->marker)) return 1;
                  stbi__jpeg_reset(z);
               }
            }
         }
         return 1;
      }
   } else {
      if (z->scan_n == 1) {
         int i,j;
         int n = z->order[0];
         // non-interleaved data, we just need to process one block at a time,
         // in trivial scanline order
         // number of blocks to do just depends on how many actual "pixels" this
         // component has, independent of interleaved MCU blocking and such
         int w = (z->img_comp[n].x+7) >> 3;
         int h = (z->img_comp[n].y+7) >> 3;
         for (j=0; j < h; ++j) {
            for (i=0; i < w; ++i) {
               short *data = z->img_comp[n].coeff + 64 * (i + j * z->img_comp[n].coeff_w);
               if (z->spec_start == 0) {
                  if (!stbi__jpeg_decode_block_prog_dc(z, data, &z->huff_dc[z->img_comp[n].hd], n))
                     return 0;
               } else {
                  int ha = z->img_comp[n].ha;
                  if (!stbi__jpeg_decode_block_prog_ac(z, data, &z->huff_ac[ha], z->fast_ac[ha]))
                     return 0;
               }
               // every data block is an MCU, so countdown the restart interval
               if (--z->todo <= 0) {
                  if (z->code_bits < 24) stbi__grow_buffer_unsafe(z);
                  if (!STBI__RESTART(z->marker)) return 1;
                  stbi__jpeg_reset(z);
               }
            }
         }
         return 1;
      } else { // interleaved
         int i,j,k,x,y;
         for (j=0; j < z->img_mcu_y; ++j) {
            for (i=0; i < z->img_mcu_x; ++i) {
               // scan an interleaved mcu... process scan_n components in order
               for (k=0; k < z->scan_n; ++k) {
                  int n = z->order[k];
                  // scan out an mcu's worth of this component; that's just determined
                  // by the basic H and V specified for the component
                  for (y=0; y < z->img_comp[n].v; ++y) {
                     for (x=0; x < z->img_comp[n].h; ++x) {
                        int x2 = (i*z->img_comp[n].h + x);
                        int y2 = (j*z->img_comp[n].v + y);
                        short *data = z->img_comp[n].coeff + 64 * (x2 + y2 * z->img_comp[n].coeff_w);
                        if (!stbi__jpeg_decode_block_prog_dc(z, data, &z->huff_dc[z->img_comp[n].hd], n))
                           return 0;
                     }
                  }
               }
               // after all interleaved components, that's an interleaved MCU,
               // so now count down the restart interval
               if (--z->todo <= 0) {
                  if (z->code_bits < 24) stbi__grow_buffer_unsafe(z);
                  if (!STBI__RESTART(z->marker)) return 1;
                  stbi__jpeg_reset(z);
               }
            }
         }
         return 1;
      }
   }
}

static void stbi__jpeg_dequantize(short *data, stbi__uint16 *dequant)
{
   int i;
   for (i=0; i < 64; ++i)
      data[i] *= dequant[i];
}

static void stbi__jpeg_finish(stbi__jpeg *z)
{
   if (z->progressive) {
      // dequantize and idct the data
      int i,j,n;
      for (n=0; n < z->s->img_n; ++n) {
         int w = (z->img_comp[n].x+7) >> 3;
         int h = (z->img_comp[n].y+7) >> 3;
         for (j=0; j < h; ++j) {
            for (i=0; i < w; ++i) {
               short *data = z->img_comp[n].coeff + 64 * (i + j * z->img_comp[n].coeff_w);
               stbi__jpeg_dequantize(data, z->dequant[z->img_comp[n].tq]);
               z->idct_block_kernel(z->img_comp[n].data+z->img_comp[n].w2*j*8+i*8, z->img_comp[n].w2, data);
            }
         }
      }
   }
}

static int stbi__process_marker(stbi__jpeg *z, int m)
{
   int L;
   switch (m) {
      case STBI__MARKER_none: // no marker found
         return stbi__err("expected marker","Corrupt JPEG");

      case 0xDD: // DRI - specify restart interval
         if (stbi__get16be(z->s) != 4) return stbi__err("bad DRI len","Corrupt JPEG");
         z->restart_interval = stbi__get16be(z->s);
         return 1;

      case 0xDB: // DQT - define quantization table
         L = stbi__get16be(z->s)-2;
         while (L > 0) {
            int q = stbi__get8(z->s);
            int p = q >> 4, sixteen = (p != 0);
            int t = q & 15,i;
            if (p != 0 && p != 1) return stbi__err("bad DQT type","Corrupt JPEG");
            if (t > 3) return stbi__err("bad DQT table","Corrupt JPEG");

            for (i=0; i < 64; ++i)
               z->dequant[t][stbi__jpeg_dezigzag[i]] = (stbi__uint16)(sixteen ? stbi__get16be(z->s) : stbi__get8(z->s));
            L -= (sixteen ? 129 : 65);
         }
         return L==0;

      case 0xC4: // DHT - define huffman table
         L = stbi__get16be(z->s)-2;
         while (L > 0) {
            stbi_uc *v;
            int sizes[16],i,n=0;
            int q = stbi__get8(z->s);
            int tc = q >> 4;
            int th = q & 15;
            if (tc > 1 || th > 3) return stbi__err("bad DHT header","Corrupt JPEG");
            for (i=0; i < 16; ++i) {
               sizes[i] = stbi__get8(z->s);
               n += sizes[i];
            }
            L -= 17;
            if (tc == 0) {
               if (!stbi__build_huffman(z->huff_dc+th, sizes)) return 0;
               v = z->huff_dc[th].values;
            } else {
               if (!stbi__build_huffman(z->huff_ac+th, sizes)) return 0;
               v = z->huff_ac[th].values;
            }
            for (i=0; i < n; ++i)
               v[i] = stbi__get8(z->s);
            if (tc != 0)
               stbi__build_fast_ac(z->fast_ac[th], z->huff_ac + th);
            L -= n;
         }
         return L==0;
   }

   // check for comment block or APP blocks
   if ((m >= 0xE0 && m <= 0xEF) || m == 0xFE) {
      L = stbi__get16be(z->s);
      if (L < 2) {
         if (m == 0xFE)
            return stbi__err("bad COM len","Corrupt JPEG");
         else
            return stbi__err("bad APP len","Corrupt JPEG");
      }
      L -= 2;

      if (m == 0xE0 && L >= 5) { // JFIF APP0 segment
         static const unsigned char tag[5] = {'J','F','I','F','\0'};
         int ok = 1;
         int i;
         for (i=0; i < 5; ++i)
            if (stbi__get8(z->s) != tag[i])
               ok = 0;
         L -= 5;
         if (ok)
            z->jfif = 1;
      } else if (m == 0xEE && L >= 12) { // Adobe APP14 segment
         static const unsigned char tag[6] = {'A','d','o','b','e','\0'};
         int ok = 1;
         int i;
         for (i=0; i < 6; ++i)
            if (stbi__get8(z->s) != tag[i])
               ok = 0;
         L -= 6;
         if (ok) {
            stbi__get8(z->s); // version
            stbi__get16be(z->s); // flags0
            stbi__get16be(z->s); // flags1
            z->app14_color_transform = stbi__get8(z->s); // color transform
            L -= 6;
         }
      }

      stbi__skip(z->s, L);
      return 1;
   }

   return stbi__err("unknown marker","Corrupt JPEG");
}

// after we see SOS
static int stbi__process_scan_header(stbi__jpeg *z)
{
   int i;
   int Ls = stbi__get16be(z->s);
   z->scan_n = stbi__get8(z->s);
   if (z->scan_n < 1 || z->scan_n > 4 || z->scan_n > (int) z->s->img_n) return stbi__err("bad SOS component count","Corrupt JPEG");
   if (Ls != 6+2*z->scan_n) return stbi__err("bad SOS len","Corrupt JPEG");
   for (i=0; i < z->scan_n; ++i) {
      int id = stbi__get8(z->s), which;
      int q = stbi__get8(z->s);
      for (which = 0; which < z->s->img_n; ++which)
         if (z->img_comp[which].id == id)
            break;
      if (which == z->s->img_n) return 0; // no match
      z->img_comp[which].hd = q >> 4;   if (z->img_comp[which].hd > 3) return stbi__err("bad DC huff","Corrupt JPEG");
      z->img_comp[which].ha = q & 15;   if (z->img_comp[which].ha > 3) return stbi__err("bad AC huff","Corrupt JPEG");
      z->order[i] = which;
   }

   {
      int aa;
      z->spec_start = stbi__get8(z->s);
      z->spec_end   = stbi__get8(z->s); // should be 63, but might be 0
      aa = stbi__get8(z->s);
      z->succ_high = (aa >> 4);
      z->succ_low  = (aa & 15);
      if (z->progressive) {
         if (z->spec_start > 63 || z->spec_end > 63  || z->spec_start > z->spec_end || z->succ_high > 13 || z->succ_low > 13)
            return stbi__err("bad SOS", "Corrupt JPEG");
      } else {
         if (z->spec_start != 0) return stbi__err("bad SOS","Corrupt JPEG");
         if (z->succ_high != 0 || z->succ_low != 0) return stbi__err("bad SOS","Corrupt JPEG");
         z->spec_end = 63;
      }
   }

   return 1;
}

static int stbi__free_jpeg_components(stbi__jpeg *z, int ncomp, int why)
{
   int i;
   for (i=0; i < ncomp; ++i) {
      if (z->img_comp[i].raw_data) {
         STBI_FREE(z->img_comp[i].raw_data);
         z->img_comp[i].raw_data = NULL;
         z->img_comp[i].data = NULL;
      }
      if (z->img_comp[i].raw_coeff) {
         STBI_FREE(z->img_comp[i].raw_coeff);
         z->img_comp[i].raw_coeff = 0;
         z->img_comp[i].coeff = 0;
      }
      if (z->img_comp[i].linebuf) {
         STBI_FREE(z->img_comp[i].linebuf);
         z->img_comp[i].linebuf = NULL;
      }
   }
   return why;
}

static int stbi__process_frame_header(stbi__jpeg *z, int scan)
{
   stbi__context *s = z->s;
   int Lf,p,i,q, h_max=1,v_max=1,c;
   Lf = stbi__get16be(s);         if (Lf < 11) return stbi__err("bad SOF len","Corrupt JPEG"); // JPEG
   p  = stbi__get8(s);            if (p != 8) return stbi__err("only 8-bit","JPEG format not supported: 8-bit only"); // JPEG baseline
   s->img_y = stbi__get16be(s);   if (s->img_y == 0) return stbi__err("no header height", "JPEG format not supported: delayed height"); // Legal, but we don't handle it--but neither does IJG
   s->img_x = stbi__get16be(s);   if (s->img_x == 0) return stbi__err("0 width","Corrupt JPEG"); // JPEG requires
   if (s->img_y > STBI_MAX_DIMENSIONS) return stbi__err("too large","Very large image (corrupt?)");
   if (s->img_x > STBI_MAX_DIMENSIONS) return stbi__err("too large","Very large image (corrupt?)");
   c = stbi__get8(s);
   if (c != 3 && c != 1 && c != 4) return stbi__err("bad component count","Corrupt JPEG");
   s->img_n = c;
   for (i=0; i < c; ++i) {
      z->img_comp[i].data = NULL;
      z->img_comp[i].linebuf = NULL;
   }

   if (Lf != 8+3*s->img_n) return stbi__err("bad SOF len","Corrupt JPEG");

   z->rgb = 0;
   for (i=0; i < s->img_n; ++i) {
      static const unsigned char rgb[3] = { 'R', 'G', 'B' };
      z->img_comp[i].id = stbi__get8(s);
      if (s->img_n == 3 && z->img_comp[i].id == rgb[i])
         ++z->rgb;
      q = stbi__get8(s);
      z->img_comp[i].h = (q >> 4);  if (!z->img_comp[i].h || z->img_comp[i].h > 4) return stbi__err("bad H","Corrupt JPEG");
      z->img_comp[i].v = q & 15;    if (!z->img_comp[i].v || z->img_comp[i].v > 4) return stbi__err("bad V","Corrupt JPEG");
      z->img_comp[i].tq = stbi__get8(s);  if (z->img_comp[i].tq > 3) return stbi__err("bad TQ","Corrupt JPEG");
   }

   if (scan != STBI__SCAN_load) return 1;

   if (!stbi__mad3sizes_valid(s->img_x, s->img_y, s->img_n, 0)) return stbi__err("too large", "Image too large to decode");

   for (i=0; i < s->img_n; ++i) {
      if (z->img_comp[i].h > h_max) h_max = z->img_comp[i].h;
      if (z->img_comp[i].v > v_max) v_max = z->img_comp[i].v;
   }

   // check that plane subsampling factors are integer ratios; our resamplers can't deal with fractional ratios
   // and I've never seen a non-corrupted JPEG file actually use them
   for (i=0; i < s->img_n; ++i) {
      if (h_max % z->img_comp[i].h != 0) return stbi__err("bad H","Corrupt JPEG");
      if (v_max % z->img_comp[i].v != 0) return stbi__err("bad V","Corrupt JPEG");
   }

   // compute interleaved mcu info
   z->img_h_max = h_max;
   z->img_v_max = v_max;
   z->img_mcu_w = h_max * 8;
   z->img_mcu_h = v_max * 8;
   // these sizes can't be more than 17 bits
   z->img_mcu_x = (s->img_x + z->img_mcu_w-1) / z->img_mcu_w;
   z->img_mcu_y = (s->img_y + z->img_mcu_h-1) / z->img_mcu_h;

   for (i=0; i < s->img_n; ++i) {
      // number of effective pixels (e.g. for non-interleaved MCU)
      z->img_comp[i].x = (s->img_x * z->img_comp[i].h + h_max-1) / h_max;
      z->img_comp[i].y = (s->img_y * z->img_comp[i].v + v_max-1) / v_max;
      // to simplify generation, we'll allocate enough memory to decode
      // the bogus oversized data from using interleaved MCUs and their
      // big blocks (e.g. a 16x16 iMCU on an image of width 33); we won't
      // discard the extra data until colorspace conversion
      //
      // img_mcu_x, img_mcu_y: <=17 bits; comp[i].h and .v are <=4 (checked earlier)
      // so these muls can't overflow with 32-bit ints (which we require)
      z->img_comp[i].w2 = z->img_mcu_x * z->img_comp[i].h * 8;
      z->img_comp[i].h2 = z->img_mcu_y * z->img_comp[i].v * 8;
      z->img_comp[i].coeff = 0;
      z->img_comp[i].raw_coeff = 0;
      z->img_comp[i].linebuf = NULL;
      z->img_comp[i].raw_data = stbi__malloc_mad2(z->img_comp[i].w2, z->img_comp[i].h2, 15);
      if (z->img_comp[i].raw_data == NULL)
         return stbi__free_jpeg_components(z, i+1, stbi__err("outofmem", "Out of memory"));
      // align blocks for idct using mmx/sse
      z->img_comp[i].data = (stbi_uc*) (((size_t) z->img_comp[i].raw_data + 15) & ~15);
      if (z->progressive) {
         // w2, h2 are multiples of 8 (see above)
         z->img_comp[i].coeff_w = z->img_comp[i].w2 / 8;
         z->img_comp[i].coeff_h = z->img_comp[i].h2 / 8;
         z->img_comp[i].raw_coeff = stbi__malloc_mad3(z->img_comp[i].w2, z->img_comp[i].h2, sizeof(short), 15);
         if (z->img_comp[i].raw_coeff == NULL)
            return stbi__free_jpeg_components(z, i+1, stbi__err("outofmem", "Out of memory"));
         z->img_comp[i].coeff = (short*) (((size_t) z->img_comp[i].raw_coeff + 15) & ~15);
      }
   }

   return 1;
}

// use comparisons since in some cases we handle more than one case (e.g. SOF)
#define stbi__DNL(x)         ((x) == 0xdc)
#define stbi__SOI(x)         ((x) == 0xd8)
#define stbi__EOI(x)         ((x) == 0xd9)
#define stbi__SOF(x)         ((x) == 0xc0 || (x) == 0xc1 || (x) == 0xc2)
#define stbi__SOS(x)         ((x) == 0xda)

#define stbi__SOF_progressive(x)   ((x) == 0xc2)

static int stbi__decode_jpeg_header(stbi__jpeg *z, int scan)
{
   int m;
   z->jfif = 0;
   z->app14_color_transform = -1; // valid values are 0,1,2
   z->marker = STBI__MARKER_none; // initialize cached marker to empty
   m = stbi__get_marker(z);
   if (!stbi__SOI(m)) return stbi__err("no SOI","Corrupt JPEG");
   if (scan == STBI__SCAN_type) return 1;
   m = stbi__get_marker(z);
   while (!stbi__SOF(m)) {
      if (!stbi__process_marker(z,m)) return 0;
      m = stbi__get_marker(z);
      while (m == STBI__MARKER_none) {
         // some files have extra padding after their blocks, so ok, we'll scan
         if (stbi__at_eof(z->s)) return stbi__err("no SOF", "Corrupt JPEG");
         m = stbi__get_marker(z);
      }
   }
   z->progressive = stbi__SOF_progressive(m);
   if (!stbi__process_frame_header(z, scan)) return 0;
   return 1;
}

// decode image to YCbCr format
static int stbi__decode_jpeg_image(stbi__jpeg *j)
{
   int m;
   for (m = 0; m < 4; m++) {
      j->img_comp[m].raw_data = NULL;
      j->img_comp[m].raw_coeff = NULL;
   }
   j->restart_interval = 0;
   if (!stbi__decode_jpeg_header(j, STBI__SCAN_load)) return 0;
   m = stbi__get_marker(j);
   while (!stbi__EOI(m)) {
      if (stbi__SOS(m)) {
         if (!stbi__process_scan_header(j)) return 0;
         if (!stbi__parse_entropy_coded_data(j)) return 0;
         if (j->marker == STBI__MARKER_none ) {
            // handle 0s at the end of image data from IP Kamera 9060
            while (!stbi__at_eof(j->s)) {
               int x = stbi__get8(j->s);
               if (x == 255) {
                  j->marker = stbi__get8(j->s);
                  break;
               }
            }
            // if we reach eof without hitting a marker, stbi__get_marker() below will fail and we'll eventually return 0
         }
      } else if (stbi__DNL(m)) {
         int Ld = stbi__get16be(j->s);
         stbi__uint32 NL = stbi__get16be(j->s);
         if (Ld != 4) return stbi__err("bad DNL len", "Corrupt JPEG");
         if (NL != j->s->img_y) return stbi__err("bad DNL height", "Corrupt JPEG");
      } else {
         if (!stbi__process_marker(j, m)) return 0;
      }
      m = stbi__get_marker(j);
   }
   if (j->progressive)
      stbi__jpeg_finish(j);
   return 1;
}

// static jfif-centered resampling (across block boundaries)

typedef stbi_uc *(*resample_row_func)(stbi_uc *out, stbi_uc *in0, stbi_uc *in1,
                                    int w, int hs);

#define stbi__div4(x) ((stbi_uc) ((x) >> 2))

static stbi_uc *resample_row_1(stbi_uc *out, stbi_uc *in_near, stbi_uc *in_far, int w, int hs)
{
   STBI_NOTUSED(out);
   STBI_NOTUSED(in_far);
   STBI_NOTUSED(w);
   STBI_NOTUSED(hs);
   return in_near;
}

static stbi_uc* stbi__resample_row_v_2(stbi_uc *out, stbi_uc *in_near, stbi_uc *in_far, int w, int hs)
{
   // need to generate two samples vertically for every one in input
   int i;
   STBI_NOTUSED(hs);
   for (i=0; i < w; ++i)
      out[i] = stbi__div4(3*in_near[i] + in_far[i] + 2);
   return out;
}

static stbi_uc*  stbi__resample_row_h_2(stbi_uc *out, stbi_uc *in_near, stbi_uc *in_far, int w, int hs)
{
   // need to generate two samples horizontally for every one in input
   int i;
   stbi_uc *input = in_near;

   if (w == 1) {
      // if only one sample, can't do any interpolation
      out[0] = out[1] = input[0];
      return out;
   }

   out[0] = input[0];
   out[1] = stbi__div4(input[0]*3 + input[1] + 2);
   for (i=1; i < w-1; ++i) {
      int n = 3*input[i]+2;
      out[i*2+0] = stbi__div4(n+input[i-1]);
      out[i*2+1] = stbi__div4(n+input[i+1]);
   }
   out[i*2+0] = stbi__div4(input[w-2]*3 + input[w-1] + 2);
   out[i*2+1] = input[w-1];

   STBI_NOTUSED(in_far);
   STBI_NOTUSED(hs);

   return out;
}

#define stbi__div16(x) ((stbi_uc) ((x) >> 4))

static stbi_uc *stbi__resample_row_hv_2(stbi_uc *out, stbi_uc *in_near, stbi_uc *in_far, int w, int hs)
{
   // need to generate 2x2 samples for every one in input
   int i,t0,t1;
   if (w == 1) {
      out[0] = out[1] = stbi__div4(3*in_near[0] + in_far[0] + 2);
      return out;
   }

   t1 = 3*in_near[0] + in_far[0];
   out[0] = stbi__div4(t1+2);
   for (i=1; i < w; ++i) {
      t0 = t1;
      t1 = 3*in_near[i]+in_far[i];
      out[i*2-1] = stbi__div16(3*t0 + t1 + 8);
      out[i*2  ] = stbi__div16(3*t1 + t0 + 8);
   }
   out[w*2-1] = stbi__div4(t1+2);

   STBI_NOTUSED(hs);

   return out;
}

#if defined(STBI_SSE2) || defined(STBI_NEON)
static stbi_uc *stbi__resample_row_hv_2_simd(stbi_uc *out, stbi_uc *in_near, stbi_uc *in_far, int w, int hs)
{
   // need to generate 2x2 samples for every one in input
   int i=0,t0,t1;

   if (w == 1) {
      out[0] = out[1] = stbi__div4(3*in_near[0] + in_far[0] + 2);
      return out;
   }

   t1 = 3*in_near[0] + in_far[0];
   // process groups of 8 pixels for as long as we can.
   // note we can't handle the last pixel in a row in this loop
   // because we need to handle the filter boundary conditions.
   for (; i < ((w-1) & ~7); i += 8) {
#if defined(STBI_SSE2)
      // load and perform the vertical filtering pass
      // this uses 3*x + y = 4*x + (y - x)
      __m128i zero  = _mm_setzero_si128();
      __m128i farb  = _mm_loadl_epi64((__m128i *) (in_far + i));
      __m128i nearb = _mm_loadl_epi64((__m128i *) (in_near + i));
      __m128i farw  = _mm_unpacklo_epi8(farb, zero);
      __m128i nearw = _mm_unpacklo_epi8(nearb, zero);
      __m128i diff  = _mm_sub_epi16(farw, nearw);
      __m128i nears = _mm_slli_epi16(nearw, 2);
      __m128i curr  = _mm_add_epi16(nears, diff); // current row

      // horizontal filter works the same based on shifted vers of current
      // row. "prev" is current row shifted right by 1 pixel; we need to
      // insert the previous pixel value (from t1).
      // "next" is current row shifted left by 1 pixel, with first pixel
      // of next block of 8 pixels added in.
      __m128i prv0 = _mm_slli_si128(curr, 2);
      __m128i nxt0 = _mm_srli_si128(curr, 2);
      __m128i prev = _mm_insert_epi16(prv0, t1, 0);
      __m128i next = _mm_insert_epi16(nxt0, 3*in_near[i+8] + in_far[i+8], 7);

      // horizontal filter, polyphase implementation since it's convenient:
      // even pixels = 3*cur + prev = cur*4 + (prev - cur)
      // odd  pixels = 3*cur + next = cur*4 + (next - cur)
      // note the shared term.
      __m128i bias  = _mm_set1_epi16(8);
      __m128i curs = _mm_slli_epi16(curr, 2);
      __m128i prvd = _mm_sub_epi16(prev, curr);
      __m128i nxtd = _mm_sub_epi16(next, curr);
      __m128i curb = _mm_add_epi16(curs, bias);
      __m128i even = _mm_add_epi16(prvd, curb);
      __m128i odd  = _mm_add_epi16(nxtd, curb);

      // interleave even and odd pixels, then undo scaling.
      __m128i int0 = _mm_unpacklo_epi16(even, odd);
      __m128i int1 = _mm_unpackhi_epi16(even, odd);
      __m128i de0  = _mm_srli_epi16(int0, 4);
      __m128i de1  = _mm_srli_epi16(int1, 4);

      // pack and write output
      __m128i outv = _mm_packus_epi16(de0, de1);
      _mm_storeu_si128((__m128i *) (out + i*2), outv);
#elif defined(STBI_NEON)
      // load and perform the vertical filtering pass
      // this uses 3*x + y = 4*x + (y - x)
      uint8x8_t farb  = vld1_u8(in_far + i);
      uint8x8_t nearb = vld1_u8(in_near + i);
      int16x8_t diff  = vreinterpretq_s16_u16(vsubl_u8(farb, nearb));
      int16x8_t nears = vreinterpretq_s16_u16(vshll_n_u8(nearb, 2));
      int16x8_t curr  = vaddq_s16(nears, diff); // current row

      // horizontal filter works the same based on shifted vers of current
      // row. "prev" is current row shifted right by 1 pixel; we need to
      // insert the previous pixel value (from t1).
      // "next" is current row shifted left by 1 pixel, with first pixel
      // of next block of 8 pixels added in.
      int16x8_t prv0 = vextq_s16(curr, curr, 7);
      int16x8_t nxt0 = vextq_s16(curr, curr, 1);
      int16x8_t prev = vsetq_lane_s16(t1, prv0, 0);
      int16x8_t next = vsetq_lane_s16(3*in_near[i+8] + in_far[i+8], nxt0, 7);

      // horizontal filter, polyphase implementation since it's convenient:
      // even pixels = 3*cur + prev = cur*4 + (prev - cur)
      // odd  pixels = 3*cur + next = cur*4 + (next - cur)
      // note the shared term.
      int16x8_t curs = vshlq_n_s16(curr, 2);
      int16x8_t prvd = vsubq_s16(prev, curr);
      int16x8_t nxtd = vsubq_s16(next, curr);
      int16x8_t even = vaddq_s16(curs, prvd);
      int16x8_t odd  = vaddq_s16(curs, nxtd);

      // undo scaling and round, then store with even/odd phases interleaved
      uint8x8x2_t o;
      o.val[0] = vqrshrun_n_s16(even, 4);
      o.val[1] = vqrshrun_n_s16(odd,  4);
      vst2_u8(out + i*2, o);
#endif

      // "previous" value for next iter
      t1 = 3*in_near[i+7] + in_far[i+7];
   }

   t0 = t1;
   t1 = 3*in_near[i] + in_far[i];
   out[i*2] = stbi__div16(3*t1 + t0 + 8);

   for (++i; i < w; ++i) {
      t0 = t1;
      t1 = 3*in_near[i]+in_far[i];
      out[i*2-1] = stbi__div16(3*t0 + t1 + 8);
      out[i*2  ] = stbi__div16(3*t1 + t0 + 8);
   }
   out[w*2-1] = stbi__div4(t1+2);

   STBI_NOTUSED(hs);

   return out;
}
#endif

static stbi_uc *stbi__resample_row_generic(stbi_uc *out, stbi_uc *in_near, stbi_uc *in_far, int w, int hs)
{
   // resample with nearest-neighbor
   int i,j;
   STBI_NOTUSED(in_far);
   for (i=0; i < w; ++i)
      for (j=0; j < hs; ++j)
         out[i*hs+j] = in_near[i];
   return out;
}

// this is a reduced-precision calculation of YCbCr-to-RGB introduced
// to make sure the code produces the same results in both SIMD and scalar
#define stbi__float2fixed(x)  (((int) ((x) * 4096.0f + 0.5f)) << 8)
static void stbi__YCbCr_to_RGB_row(stbi_uc *out, const stbi_uc *y, const stbi_uc *pcb, const stbi_uc *pcr, int count, int step)
{
   int i;
   for (i=0; i < count; ++i) {
      int y_fixed = (y[i] << 20) + (1<<19); // rounding
      int r,g,b;
      int cr = pcr[i] - 128;
      int cb = pcb[i] - 128;
      r = y_fixed +  cr* stbi__float2fixed(1.40200f);
      g = y_fixed + (cr*-stbi__float2fixed(0.71414f)) + ((cb*-stbi__float2fixed(0.34414f)) & 0xffff0000);
      b = y_fixed                                     +   cb* stbi__float2fixed(1.77200f);
      r >>= 20;
      g >>= 20;
      b >>= 20;
      if ((unsigned) r > 255) { if (r < 0) r = 0; else r = 255; }
      if ((unsigned) g > 255) { if (g < 0) g = 0; else g = 255; }
      if ((unsigned) b > 255) { if (b < 0) b = 0; else b = 255; }
      out[0] = (stbi_uc)r;
      out[1] = (stbi_uc)g;
      out[2] = (stbi_uc)b;
      out[3] = 255;
      out += step;
   }
}

#if defined(STBI_SSE2) || defined(STBI_NEON)
static void stbi__YCbCr_to_RGB_simd(stbi_uc *out, stbi_uc const *y, stbi_uc const *pcb, stbi_uc const *pcr, int count, int step)
{
   int i = 0;

#ifdef STBI_SSE2
   // step == 3 is pretty ugly on the final interleave, and i'm not convinced
   // it's useful in practice (you wouldn't use it for textures, for example).
   // so just accelerate step == 4 case.
   if (step == 4) {
      // this is a fairly straightforward implementation and not super-optimized.
      __m128i signflip  = _mm_set1_epi8(-0x80);
      __m128i cr_const0 = _mm_set1_epi16(   (short) ( 1.40200f*4096.0f+0.5f));
      __m128i cr_const1 = _mm_set1_epi16( - (short) ( 0.71414f*4096.0f+0.5f));
      __m128i cb_const0 = _mm_set1_epi16( - (short) ( 0.34414f*4096.0f+0.5f));
      __m128i cb_const1 = _mm_set1_epi16(   (short) ( 1.77200f*4096.0f+0.5f));
      __m128i y_bias = _mm_set1_epi8((char) (unsigned char) 128);
      __m128i xw = _mm_set1_epi16(255); // alpha channel

      for (; i+7 < count; i += 8) {
         // load
         __m128i y_bytes = _mm_loadl_epi64((__m128i *) (y+i));
         __m128i cr_bytes = _mm_loadl_epi64((__m128i *) (pcr+i));
         __m128i cb_bytes = _mm_loadl_epi64((__m128i *) (pcb+i));
         __m128i cr_biased = _mm_xor_si128(cr_bytes, signflip); // -128
         __m128i cb_biased = _mm_xor_si128(cb_bytes, signflip); // -128

         // unpack to short (and left-shift cr, cb by 8)
         __m128i yw  = _mm_unpacklo_epi8(y_bias, y_bytes);
         __m128i crw = _mm_unpacklo_epi8(_mm_setzero_si128(), cr_biased);
         __m128i cbw = _mm_unpacklo_epi8(_mm_setzero_si128(), cb_biased);

         // color transform
         __m128i yws = _mm_srli_epi16(yw, 4);
         __m128i cr0 = _mm_mulhi_epi16(cr_const0, crw);
         __m128i cb0 = _mm_mulhi_epi16(cb_const0, cbw);
         __m128i cb1 = _mm_mulhi_epi16(cbw, cb_const1);
         __m128i cr1 = _mm_mulhi_epi16(crw, cr_const1);
         __m128i rws = _mm_add_epi16(cr0, yws);
         __m128i gwt = _mm_add_epi16(cb0, yws);
         __m128i bws = _mm_add_epi16(yws, cb1);
         __m128i gws = _mm_add_epi16(gwt, cr1);

         // descale
         __m128i rw = _mm_srai_epi16(rws, 4);
         __m128i bw = _mm_srai_epi16(bws, 4);
         __m128i gw = _mm_srai_epi16(gws, 4);

         // back to byte, set up for transpose
         __m128i brb = _mm_packus_epi16(rw, bw);
         __m128i gxb = _mm_packus_epi16(gw, xw);

         // transpose to interleave channels
         __m128i t0 = _mm_unpacklo_epi8(brb, gxb);
         __m128i t1 = _mm_unpackhi_epi8(brb, gxb);
         __m128i o0 = _mm_unpacklo_epi16(t0, t1);
         __m128i o1 = _mm_unpackhi_epi16(t0, t1);

         // store
         _mm_storeu_si128((__m128i *) (out + 0), o0);
         _mm_storeu_si128((__m128i *) (out + 16), o1);
         out += 32;
      }
   }
#endif

#ifdef STBI_NEON
   // in this version, step=3 support would be easy to add. but is there demand?
   if (step == 4) {
      // this is a fairly straightforward implementation and not super-optimized.
      uint8x8_t signflip = vdup_n_u8(0x80);
      int16x8_t cr_const0 = vdupq_n_s16(   (short) ( 1.40200f*4096.0f+0.5f));
      int16x8_t cr_const1 = vdupq_n_s16( - (short) ( 0.71414f*4096.0f+0.5f));
      int16x8_t cb_const0 = vdupq_n_s16( - (short) ( 0.34414f*4096.0f+0.5f));
      int16x8_t cb_const1 = vdupq_n_s16(   (short) ( 1.77200f*4096.0f+0.5f));

      for (; i+7 < count; i += 8) {
         // load
         uint8x8_t y_bytes  = vld1_u8(y + i);
         uint8x8_t cr_bytes = vld1_u8(pcr + i);
         uint8x8_t cb_bytes = vld1_u8(pcb + i);
         int8x8_t cr_biased = vreinterpret_s8_u8(vsub_u8(cr_bytes, signflip));
         int8x8_t cb_biased = vreinterpret_s8_u8(vsub_u8(cb_bytes, signflip));

         // expand to s16
         int16x8_t yws = vreinterpretq_s16_u16(vshll_n_u8(y_bytes, 4));
         int16x8_t crw = vshll_n_s8(cr_biased, 7);
         int16x8_t cbw = vshll_n_s8(cb_biased, 7);

         // color transform
         int16x8_t cr0 = vqdmulhq_s16(crw, cr_const0);
         int16x8_t cb0 = vqdmulhq_s16(cbw, cb_const0);
         int16x8_t cr1 = vqdmulhq_s16(crw, cr_const1);
         int16x8_t cb1 = vqdmulhq_s16(cbw, cb_const1);
         int16x8_t rws = vaddq_s16(yws, cr0);
         int16x8_t gws = vaddq_s16(vaddq_s16(yws, cb0), cr1);
         int16x8_t bws = vaddq_s16(yws, cb1);

         // undo scaling, round, convert to byte
         uint8x8x4_t o;
         o.val[0] = vqrshrun_n_s16(rws, 4);
         o.val[1] = vqrshrun_n_s16(gws, 4);
         o.val[2] = vqrshrun_n_s16(bws, 4);
         o.val[3] = vdup_n_u8(255);

         // store, interleaving r/g/b/a
         vst4_u8(out, o);
         out += 8*4;
      }
   }
#endif

   for (; i < count; ++i) {
      int y_fixed = (y[i] << 20) + (1<<19); // rounding
      int r,g,b;
      int cr = pcr[i] - 128;
      int cb = pcb[i] - 128;
      r = y_fixed + cr* stbi__float2fixed(1.40200f);
      g = y_fixed + cr*-stbi__float2fixed(0.71414f) + ((cb*-stbi__float2fixed(0.34414f)) & 0xffff0000);
      b = y_fixed                                   +   cb* stbi__float2fixed(1.77200f);
      r >>= 20;
      g >>= 20;
      b >>= 20;
      if ((unsigned) r > 255) { if (r < 0) r = 0; else r = 255; }
      if ((unsigned) g > 255) { if (g < 0) g = 0; else g = 255; }
      if ((unsigned) b > 255) { if (b < 0) b = 0; else b = 255; }
      out[0] = (stbi_uc)r;
      out[1] = (stbi_uc)g;
      out[2] = (stbi_uc)b;
      out[3] = 255;
      out += step;
   }
}
#endif

// set up the kernels
static void stbi__setup_jpeg(stbi__jpeg *j)
{
   j->idct_block_kernel = stbi__idct_block;
   j->YCbCr_to_RGB_kernel = stbi__YCbCr_to_RGB_row;
   j->resample_row_hv_2_kernel = stbi__resample_row_hv_2;

#ifdef STBI_SSE2
   if (stbi__sse2_available()) {
      j->idct_block_kernel = stbi__idct_simd;
      j->YCbCr_to_RGB_kernel = stbi__YCbCr_to_RGB_simd;
      j->resample_row_hv_2_kernel = stbi__resample_row_hv_2_simd;
   }
#endif

#ifdef STBI_NEON
   j->idct_block_kernel = stbi__idct_simd;
   j->YCbCr_to_RGB_kernel = stbi__YCbCr_to_RGB_simd;
   j->resample_row_hv_2_kernel = stbi__resample_row_hv_2_simd;
#endif
}

// clean up the temporary component buffers
static void stbi__cleanup_jpeg(stbi__jpeg *j)
{
   stbi__free_jpeg_components(j, j->s->img_n, 0);
}

typedef struct
{
   resample_row_func resample;
   stbi_uc *line0,*line1;
   int hs,vs;   // expansion factor in each axis
   int w_lores; // horizontal pixels pre-expansion
   int ystep;   // how far through vertical expansion we are
   int ypos;    // which pre-expansion row we're on
} stbi__resample;

// fast 0..255 * 0..255 => 0..255 rounded multiplication
static stbi_uc stbi__blinn_8x8(stbi_uc x, stbi_uc y)
{
   unsigned int t = x*y + 128;
   return (stbi_uc) ((t + (t >>8)) >> 8);
}

static stbi_uc *load_jpeg_image(stbi__jpeg *z, int *out_x, int *out_y, int *comp, int req_comp)
{
   int n, decode_n, is_rgb;
   z->s->img_n = 0; // make stbi__cleanup_jpeg safe

   // validate req_comp
   if (req_comp < 0 || req_comp > 4) return stbi__errpuc("bad req_comp", "Internal error");

   // load a jpeg image from whichever source, but leave in YCbCr format
   if (!stbi__decode_jpeg_image(z)) { stbi__cleanup_jpeg(z); return NULL; }

   // determine actual number of components to generate
   n = req_comp ? req_comp : z->s->img_n >= 3 ? 3 : 1;

   is_rgb = z->s->img_n == 3 && (z->rgb == 3 || (z->app14_color_transform == 0 && !z->jfif));

   if (z->s->img_n == 3 && n < 3 && !is_rgb)
      decode_n = 1;
   else
      decode_n = z->s->img_n;

   // nothing to do if no components requested; check this now to avoid
   // accessing uninitialized coutput[0] later
   if (decode_n <= 0) { stbi__cleanup_jpeg(z); return NULL; }

   // resample and color-convert
   {
      int k;
      unsigned int i,j;
      stbi_uc *output;
      stbi_uc *coutput[4] = { NULL, NULL, NULL, NULL };

      stbi__resample res_comp[4];

      for (k=0; k < decode_n; ++k) {
         stbi__resample *r = &res_comp[k];

         // allocate line buffer big enough for upsampling off the edges
         // with upsample factor of 4
         z->img_comp[k].linebuf = (stbi_uc *) stbi__malloc(z->s->img_x + 3);
         if (!z->img_comp[k].linebuf) { stbi__cleanup_jpeg(z); return stbi__errpuc("outofmem", "Out of memory"); }

         r->hs      = z->img_h_max / z->img_comp[k].h;
         r->vs      = z->img_v_max / z->img_comp[k].v;
         r->ystep   = r->vs >> 1;
         r->w_lores = (z->s->img_x + r->hs-1) / r->hs;
         r->ypos    = 0;
         r->line0   = r->line1 = z->img_comp[k].data;

         if      (r->hs == 1 && r->vs == 1) r->resample = resample_row_1;
         else if (r->hs == 1 && r->vs == 2) r->resample = stbi__resample_row_v_2;
         else if (r->hs == 2 && r->vs == 1) r->resample = stbi__resample_row_h_2;
         else if (r->hs == 2 && r->vs == 2) r->resample = z->resample_row_hv_2_kernel;
         else                               r->resample = stbi__resample_row_generic;
      }

      // can't error after this so, this is safe
      output = (stbi_uc *) stbi__malloc_mad3(n, z->s->img_x, z->s->img_y, 1);
      if (!output) { stbi__cleanup_jpeg(z); return stbi__errpuc("outofmem", "Out of memory"); }

      // now go ahead and resample
      for (j=0; j < z->s->img_y; ++j) {
         stbi_uc *out = output + n * z->s->img_x * j;
         for (k=0; k < decode_n; ++k) {
            stbi__resample *r = &res_comp[k];
            int y_bot = r->ystep >= (r->vs >> 1);
            coutput[k] = r->resample(z->img_comp[k].linebuf,
                                     y_bot ? r->line1 : r->line0,
                                     y_bot ? r->line0 : r->line1,
                                     r->w_lores, r->hs);
            if (++r->ystep >= r->vs) {
               r->ystep = 0;
               r->line0 = r->line1;
               if (++r->ypos < z->img_comp[k].y)
                  r->line1 += z->img_comp[k].w2;
            }
         }
         if (n >= 3) {
            stbi_uc *y = coutput[0];
            if (z->s->img_n == 3) {
               if (is_rgb) {
                  for (i=0; i < z->s->img_x; ++i) {
                     out[0] = y[i];
                     out[1] = coutput[1][i];
                     out[2] = coutput[2][i];
                     out[3] = 255;
                     out += n;
                  }
               } else {
                  z->YCbCr_to_RGB_kernel(out, y, coutput[1], coutput[2], z->s->img_x, n);
               }
            } else if (z->s->img_n == 4) {
               if (z->app14_color_transform == 0) { // CMYK
                  for (i=0; i < z->s->img_x; ++i) {
                     stbi_uc m = coutput[3][i];
                     out[0] = stbi__blinn_8x8(coutput[0][i], m);
                     out[1] = stbi__blinn_8x8(coutput[1][i], m);
                     out[2] = stbi__blinn_8x8(coutput[2][i], m);
                     out[3] = 255;
                     out += n;
                  }
               } else if (z->app14_color_transform == 2) { // YCCK
                  z->YCbCr_to_RGB_kernel(out, y, coutput[1], coutput[2], z->s->img_x, n);
                  for (i=0; i < z->s->img_x; ++i) {
                     stbi_uc m = coutput[3][i];
                     out[0] = stbi__blinn_8x8(255 - out[0], m);
                     out[1] = stbi__blinn_8x8(255 - out[1], m);
                     out[2] = stbi__blinn_8x8(255 - out[2], m);
                     out += n;
                  }
               } else { // YCbCr + alpha?  Ignore the fourth channel for now
                  z->YCbCr_to_RGB_kernel(out, y, coutput[1], coutput[2], z->s->img_x, n);
               }
            } else
               for (i=0; i < z->s->img_x; ++i) {
                  out[0] = out[1] = out[2] = y[i];
                  out[3] = 255; // not used if n==3
                  out += n;
               }
         } else {
            if (is_rgb) {
               if (n == 1)
                  for (i=0; i < z->s->img_x; ++i)
                     *out++ = stbi__compute_y(coutput[0][i], coutput[1][i], coutput[2][i]);
               else {
                  for (i=0; i < z->s->img_x; ++i, out += 2) {
                     out[0] = stbi__compute_y(coutput[0][i], coutput[1][i], coutput[2][i]);
                     out[1] = 255;
                  }
               }
            } else if (z->s->img_n == 4 && z->app14_color_transform == 0) {
               for (i=0; i < z->s->img_x; ++i) {
                  stbi_uc m = coutput[3][i];
                  stbi_uc r = stbi__blinn_8x8(coutput[0][i], m);
                  stbi_uc g = stbi__blinn_8x8(coutput[1][i], m);
                  stbi_uc b = stbi__blinn_8x8(coutput[2][i], m);
                  out[0] = stbi__compute_y(r, g, b);
                  out[1] = 255;
                  out += n;
               }
            } else if (z->s->img_n == 4 && z->app14_color_transform == 2) {
               for (i=0; i < z->s->img_x; ++i) {
                  out[0] = stbi__blinn_8x8(255 - coutput[0][i], coutput[3][i]);
                  out[1] = 255;
                  out += n;
               }
            } else {
               stbi_uc *y = coutput[0];
               if (n == 1)
                  for (i=0; i < z->s->img_x; ++i) out[i] = y[i];
               else
                  for (i=0; i < z->s->img_x; ++i) { *out++ = y[i]; *out++ = 255; }
            }
         }
      }
      stbi__cleanup_jpeg(z);
      *out_x = z->s->img_x;
      *out_y = z->s->img_y;
      if (comp) *comp = z->s->img_n >= 3 ? 3 : 1; // report original components, not output
      return output;
   }
}

static void *stbi__jpeg_load(stbi__context *s, int *x, int *y, int *comp, int req_comp, stbi__result_info *ri)
{
   unsigned char* result;
   stbi__jpeg* j = (stbi__jpeg*) stbi__malloc(sizeof(stbi__jpeg));
   if (!j) return stbi__errpuc("outofmem", "Out of memory");
   STBI_NOTUSED(ri);
   j->s = s;
   stbi__setup_jpeg(j);
   result = load_jpeg_image(j, x,y,comp,req_comp);
   STBI_FREE(j);
   return result;
}

static int stbi__jpeg_test(stbi__context *s)
{
   int r;
   stbi__jpeg* j = (stbi__jpeg*)stbi__malloc(sizeof(stbi__jpeg));
   if (!j) return stbi__err("outofmem", "Out of memory");
   j->s = s;
   stbi__setup_jpeg(j);
   r = stbi__decode_jpeg_header(j, STBI__SCAN_type);
   stbi__rewind(s);
   STBI_FREE(j);
   return r;
}

static int stbi__jpeg_info_raw(stbi__jpeg *j, int *x, int *y, int *comp)
{
   if (!stbi__decode_jpeg_header(j, STBI__SCAN_header)) {
      stbi__rewind( j->s );
      return 0;
   }
   if (x) *x = j->s->img_x;
   if (y) *y = j->s->img_y;
   if (comp) *comp = j->s->img_n >= 3 ? 3 : 1;
   return 1;
}

static int stbi__jpeg_info(stbi__context *s, int *x, int *y, int *comp)
{
   int result;
   stbi__jpeg* j = (stbi__jpeg*) (stbi__malloc(sizeof(stbi__jpeg)));
   if (!j) return stbi__err("outofmem", "Out of memory");
   j->s = s;
   result = stbi__jpeg_info_raw(j, x, y, comp);
   STBI_FREE(j);
   return result;
}
#endif

// public domain zlib decode    v0.2  Sean Barrett 2006-11-18
//    simple implementation
//      - all input must be provided in an upfront buffer
//      - all output is written to a single output buffer (can malloc/realloc)
//    performance
//      - fast huffman

#ifndef STBI_NO_ZLIB

// fast-way is faster to check than jpeg huffman, but slow way is slower
#define STBI__ZFAST_BITS  9 // accelerate all cases in default tables
#define STBI__ZFAST_MASK  ((1 << STBI__ZFAST_BITS) - 1)
#define STBI__ZNSYMS 288 // number of symbols in literal/length alphabet

// zlib-style huffman encoding
// (jpegs packs from left, zlib from right, so can't share code)
typedef struct
{
   stbi__uint16 fast[1 << STBI__ZFAST_BITS];
   stbi__uint16 firstcode[16];
   int maxcode[17];
   stbi__uint16 firstsymbol[16];
   stbi_uc  size[STBI__ZNSYMS];
   stbi__uint16 value[STBI__ZNSYMS];
} stbi__zhuffman;

stbi_inline static int stbi__bitreverse16(int n)
{
  n = ((n & 0xAAAA) >>  1) | ((n & 0x5555) << 1);
  n = ((n & 0xCCCC) >>  2) | ((n & 0x3333) << 2);
  n = ((n & 0xF0F0) >>  4) | ((n & 0x0F0F) << 4);
  n = ((n & 0xFF00) >>  8) | ((n & 0x00FF) << 8);
  return n;
}

stbi_inline static int stbi__bit_reverse(int v, int bits)
{
   STBI_ASSERT(bits <= 16);
   // to bit reverse n bits, reverse 16 and shift
   // e.g. 11 bits, bit reverse and shift away 5
   return stbi__bitreverse16(v) >> (16-bits);
}

static int stbi__zbuild_huffman(stbi__zhuffman *z, const stbi_uc *sizelist, int num)
{
   int i,k=0;
   int code, next_code[16], sizes[17];

   // DEFLATE spec for generating codes
   memset(sizes, 0, sizeof(sizes));
   memset(z->fast, 0, sizeof(z->fast));
   for (i=0; i < num; ++i)
      ++sizes[sizelist[i]];
   sizes[0] = 0;
   for (i=1; i < 16; ++i)
      if (sizes[i] > (1 << i))
         return stbi__err("bad sizes", "Corrupt PNG");
   code = 0;
   for (i=1; i < 16; ++i) {
      next_code[i] = code;
      z->firstcode[i] = (stbi__uint16) code;
      z->firstsymbol[i] = (stbi__uint16) k;
      code = (code + sizes[i]);
      if (sizes[i])
         if (code-1 >= (1 << i)) return stbi__err("bad codelengths","Corrupt PNG");
      z->maxcode[i] = code << (16-i); // preshift for inner loop
      code <<= 1;
      k += sizes[i];
   }
   z->maxcode[16] = 0x10000; // sentinel
   for (i=0; i < num; ++i) {
      int s = sizelist[i];
      if (s) {
         int c = next_code[s] - z->firstcode[s] + z->firstsymbol[s];
         stbi__uint16 fastv = (stbi__uint16) ((s << 9) | i);
         z->size [c] = (stbi_uc     ) s;
         z->value[c] = (stbi__uint16) i;
         if (s <= STBI__ZFAST_BITS) {
            int j = stbi__bit_reverse(next_code[s],s);
            while (j < (1 << STBI__ZFAST_BITS)) {
               z->fast[j] = fastv;
               j += (1 << s);
            }
         }
         ++next_code[s];
      }
   }
   return 1;
}

// zlib-from-memory implementation for PNG reading
//    because PNG allows splitting the zlib stream arbitrarily,
//    and it's annoying structurally to have PNG call ZLIB call PNG,
//    we require PNG read all the IDATs and combine them into a single
//    memory buffer

typedef struct
{
   stbi_uc *zbuffer, *zbuffer_end;
   int num_bits;
   stbi__uint32 code_buffer;

   char *zout;
   char *zout_start;
   char *zout_end;
   int   z_expandable;

   stbi__zhuffman z_length, z_distance;
} stbi__zbuf;

stbi_inline static int stbi__zeof(stbi__zbuf *z)
{
   return (z->zbuffer >= z->zbuffer_end);
}

stbi_inline static stbi_uc stbi__zget8(stbi__zbuf *z)
{
   return stbi__zeof(z) ? 0 : *z->zbuffer++;
}

static void stbi__fill_bits(stbi__zbuf *z)
{
   do {
      if (z->code_buffer >= (1U << z->num_bits)) {
        z->zbuffer = z->zbuffer_end;  /* treat this as EOF so we fail. */
        return;
      }
      z->code_buffer |= (unsigned int) stbi__zget8(z) << z->num_bits;
      z->num_bits += 8;
   } while (z->num_bits <= 24);
}

stbi_inline static unsigned int stbi__zreceive(stbi__zbuf *z, int n)
{
   unsigned int k;
   if (z->num_bits < n) stbi__fill_bits(z);
   k = z->code_buffer & ((1 << n) - 1);
   z->code_buffer >>= n;
   z->num_bits -= n;
   return k;
}

static int stbi__zhuffman_decode_slowpath(stbi__zbuf *a, stbi__zhuffman *z)
{
   int b,s,k;
   // not resolved by fast table, so compute it the slow way
   // use jpeg approach, which requires MSbits at top
   k = stbi__bit_reverse(a->code_buffer, 16);
   for (s=STBI__ZFAST_BITS+1; ; ++s)
      if (k < z->maxcode[s])
         break;
   if (s >= 16) return -1; // invalid code!
   // code size is s, so:
   b = (k >> (16-s)) - z->firstcode[s] + z->firstsymbol[s];
   if (b >= STBI__ZNSYMS) return -1; // some data was corrupt somewhere!
   if (z->size[b] != s) return -1;  // was originally an assert, but report failure instead.
   a->code_buffer >>= s;
   a->num_bits -= s;
   return z->value[b];
}

stbi_inline static int stbi__zhuffman_decode(stbi__zbuf *a, stbi__zhuffman *z)
{
   int b,s;
   if (a->num_bits < 16) {
      if (stbi__zeof(a)) {
         return -1;   /* report error for unexpected end of data. */
      }
      stbi__fill_bits(a);
   }
   b = z->fast[a->code_buffer & STBI__ZFAST_MASK];
   if (b) {
      s = b >> 9;
      a->code_buffer >>= s;
      a->num_bits -= s;
      return b & 511;
   }
   return stbi__zhuffman_decode_slowpath(a, z);
}

static int stbi__zexpand(stbi__zbuf *z, char *zout, int n)  // need to make room for n bytes
{
   char *q;
   unsigned int cur, limit, old_limit;
   z->zout = zout;
   if (!z->z_expandable) return stbi__err("output buffer limit","Corrupt PNG");
   cur   = (unsigned int) (z->zout - z->zout_start);
   limit = old_limit = (unsigned) (z->zout_end - z->zout_start);
   if (UINT_MAX - cur < (unsigned) n) return stbi__err("outofmem", "Out of memory");
   while (cur + n > limit) {
      if(limit > UINT_MAX / 2) return stbi__err("outofmem", "Out of memory");
      limit *= 2;
   }
   q = (char *) STBI_REALLOC_SIZED(z->zout_start, old_limit, limit);
   STBI_NOTUSED(old_limit);
   if (q == NULL) return stbi__err("outofmem", "Out of memory");
   z->zout_start = q;
   z->zout       = q + cur;
   z->zout_end   = q + limit;
   return 1;
}

static const int stbi__zlength_base[31] = {
   3,4,5,6,7,8,9,10,11,13,
   15,17,19,23,27,31,35,43,51,59,
   67,83,99,115,131,163,195,227,258,0,0 };

static const int stbi__zlength_extra[31]=
{ 0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,0,0,0 };

static const int stbi__zdist_base[32] = { 1,2,3,4,5,7,9,13,17,25,33,49,65,97,129,193,
257,385,513,769,1025,1537,2049,3073,4097,6145,8193,12289,16385,24577,0,0};

static const int stbi__zdist_extra[32] =
{ 0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13};

static int stbi__parse_huffman_block(stbi__zbuf *a)
{
   char *zout = a->zout;
   for(;;) {
      int z = stbi__zhuffman_decode(a, &a->z_length);
      if (z < 256) {
         if (z < 0) return stbi__err("bad huffman code","Corrupt PNG"); // error in huffman codes
         if (zout >= a->zout_end) {
            if (!stbi__zexpand(a, zout, 1)) return 0;
            zout = a->zout;
         }
         *zout++ = (char) z;
      } else {
         stbi_uc *p;
         int len,dist;
         if (z == 256) {
            a->zout = zout;
            return 1;
         }
         z -= 257;
         len = stbi__zlength_base[z];
         if (stbi__zlength_extra[z]) len += stbi__zreceive(a, stbi__zlength_extra[z]);
         z = stbi__zhuffman_decode(a, &a->z_distance);
         if (z < 0) return stbi__err("bad huffman code","Corrupt PNG");
         dist = stbi__zdist_base[z];
         if (stbi__zdist_extra[z]) dist += stbi__zreceive(a, stbi__zdist_extra[z]);
         if (zout - a->zout_start < dist) return stbi__err("bad dist","Corrupt PNG");
         if (zout + len > a->zout_end) {
            if (!stbi__zexpand(a, zout, len)) return 0;
            zout = a->zout;
         }
         p = (stbi_uc *) (zout - dist);
         if (dist == 1) { // run of one byte; common in images.
            stbi_uc v = *p;
            if (len) { do *zout++ = v; while (--len); }
         } else {
            if (len) { do *zout++ = *p++; while (--len); }
         }
      }
   }
}

static int stbi__compute_huffman_codes(stbi__zbuf *a)
{
   static const stbi_uc length_dezigzag[19] = { 16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15 };
   stbi__zhuffman z_codelength;
   stbi_uc lencodes[286+32+137];//padding for maximum single op
   stbi_uc codelength_sizes[19];
   int i,n;

   int hlit  = stbi__zreceive(a,5) + 257;
   int hdist = stbi__zreceive(a,5) + 1;
   int hclen = stbi__zreceive(a,4) + 4;
   int ntot  = hlit + hdist;

   memset(codelength_sizes, 0, sizeof(codelength_sizes));
   for (i=0; i < hclen; ++i) {
      int s = stbi__zreceive(a,3);
      codelength_sizes[length_dezigzag[i]] = (stbi_uc) s;
   }
   if (!stbi__zbuild_huffman(&z_codelength, codelength_sizes, 19)) return 0;

   n = 0;
   while (n < ntot) {
      int c = stbi__zhuffman_decode(a, &z_codelength);
      if (c < 0 || c >= 19) return stbi__err("bad codelengths", "Corrupt PNG");
      if (c < 16)
         lencodes[n++] = (stbi_uc) c;
      else {
         stbi_uc fill = 0;
         if (c == 16) {
            c = stbi__zreceive(a,2)+3;
            if (n == 0) return stbi__err("bad codelengths", "Corrupt PNG");
            fill = lencodes[n-1];
         } else if (c == 17) {
            c = stbi__zreceive(a,3)+3;
         } else if (c == 18) {
            c = stbi__zreceive(a,7)+11;
         } else {
            return stbi__err("bad codelengths", "Corrupt PNG");
         }
         if (ntot - n < c) return stbi__err("bad codelengths", "Corrupt PNG");
         memset(lencodes+n, fill, c);
         n += c;
      }
   }
   if (n != ntot) return stbi__err("bad codelengths","Corrupt PNG");
   if (!stbi__zbuild_huffman(&a->z_length, lencodes, hlit)) return 0;
   if (!stbi__zbuild_huffman(&a->z_distance, lencodes+hlit, hdist)) return 0;
   return 1;
}

static int stbi__parse_uncompressed_block(stbi__zbuf *a)
{
   stbi_uc header[4];
   int len,nlen,k;
   if (a->num_bits & 7)
      stbi__zreceive(a, a->num_bits & 7); // discard
   // drain the bit-packed data into header
   k = 0;
   while (a->num_bits > 0) {
      header[k++] = (stbi_uc) (a->code_buffer & 255); // suppress MSVC run-time check
      a->code_buffer >>= 8;
      a->num_bits -= 8;
   }
   if (a->num_bits < 0) return stbi__err("zlib corrupt","Corrupt PNG");
   // now fill header the normal way
   while (k < 4)
      header[k++] = stbi__zget8(a);
   len  = header[1] * 256 + header[0];
   nlen = header[3] * 256 + header[2];
   if (nlen != (len ^ 0xffff)) return stbi__err("zlib corrupt","Corrupt PNG");
   if (a->zbuffer + len > a->zbuffer_end) return stbi__err("read past buffer","Corrupt PNG");
   if (a->zout + len > a->zout_end)
      if (!stbi__zexpand(a, a->zout, len)) return 0;
   memcpy(a->zout, a->zbuffer, len);
   a->zbuffer += len;
   a->zout += len;
   return 1;
}

static int stbi__parse_zlib_header(stbi__zbuf *a)
{
   int cmf   = stbi__zget8(a);
   int cm    = cmf & 15;
   /* int cinfo = cmf >> 4; */
   int flg   = stbi__zget8(a);
   if (stbi__zeof(a)) return stbi__err("bad zlib header","Corrupt PNG"); // zlib spec
   if ((cmf*256+flg) % 31 != 0) return stbi__err("bad zlib header","Corrupt PNG"); // zlib spec
   if (flg & 32) return stbi__err("no preset dict","Corrupt PNG"); // preset dictionary not allowed in png
   if (cm != 8) return stbi__err("bad compression","Corrupt PNG"); // DEFLATE required for png
   // window = 1 << (8 + cinfo)... but who cares, we fully buffer output
   return 1;
}

static const stbi_uc stbi__zdefault_length[STBI__ZNSYMS] =
{
   8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8, 8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
   8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8, 8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
   8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8, 8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
   8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8, 8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
   8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8, 9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
   9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9, 9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
   9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9, 9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
   9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9, 9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
   7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7, 7,7,7,7,7,7,7,7,8,8,8,8,8,8,8,8
};
static const stbi_uc stbi__zdefault_distance[32] =
{
   5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5
};
/*
Init algorithm:
{
   int i;   // use <= to match clearly with spec
   for (i=0; i <= 143; ++i)     stbi__zdefault_length[i]   = 8;
   for (   ; i <= 255; ++i)     stbi__zdefault_length[i]   = 9;
   for (   ; i <= 279; ++i)     stbi__zdefault_length[i]   = 7;
   for (   ; i <= 287; ++i)     stbi__zdefault_length[i]   = 8;

   for (i=0; i <=  31; ++i)     stbi__zdefault_distance[i] = 5;
}
*/

static int stbi__parse_zlib(stbi__zbuf *a, int parse_header)
{
   int final, type;
   if (parse_header)
      if (!stbi__parse_zlib_header(a)) return 0;
   a->num_bits = 0;
   a->code_buffer = 0;
   do {
      final = stbi__zreceive(a,1);
      type = stbi__zreceive(a,2);
      if (type == 0) {
         if (!stbi__parse_uncompressed_block(a)) return 0;
      } else if (type == 3) {
         return 0;
      } else {
         if (type == 1) {
            // use fixed code lengths
            if (!stbi__zbuild_huffman(&a->z_length  , stbi__zdefault_length  , STBI__ZNSYMS)) return 0;
            if (!stbi__zbuild_huffman(&a->z_distance, stbi__zdefault_distance,  32)) return 0;
         } else {
            if (!stbi__compute_huffman_codes(a)) return 0;
         }
         if (!stbi__parse_huffman_block(a)) return 0;
      }
   } while (!final);
   return 1;
}

static int stbi__do_zlib(stbi__zbuf *a, char *obuf, int olen, int exp, int parse_header)
{
   a->zout_start = obuf;
   a->zout       = obuf;
   a->zout_end   = obuf + olen;
   a->z_expandable = exp;

   return stbi__parse_zlib(a, parse_header);
}

STBIDEF char *stbi_zlib_decode_malloc_guesssize(const char *buffer, int len, int initial_size, int *outlen)
{
   stbi__zbuf a;
   char *p = (char *) stbi__malloc(initial_size);
   if (p == NULL) return NULL;
   a.zbuffer = (stbi_uc *) buffer;
   a.zbuffer_end = (stbi_uc *) buffer + len;
   if (stbi__do_zlib(&a, p, initial_size, 1, 1)) {
      if (outlen) *outlen = (int) (a.zout - a.zout_start);
      return a.zout_start;
   } else {
      STBI_FREE(a.zout_start);
      return NULL;
   }
}

STBIDEF char *stbi_zlib_decode_malloc(char const *buffer, int len, int *outlen)
{
   return stbi_zlib_decode_malloc_guesssize(buffer, len, 16384, outlen);
}

STBIDEF char *stbi_zlib_decode_malloc_guesssize_headerflag(const char *buffer, int len, int initial_size, int *outlen, int parse_header)
{
   stbi__zbuf a;
   char *p = (char *) stbi__malloc(initial_size);
   if (p == NULL) return NULL;
   a.zbuffer = (stbi_uc *) buffer;
   a.zbuffer_end = (stbi_uc *) buffer + len;
   if (stbi__do_zlib(&a, p, initial_size, 1, parse_header)) {
      if (outlen) *outlen = (int) (a.zout - a.zout_start);
      return a.zout_start;
   } else {
      STBI_FREE(a.zout_start);
      return NULL;
   }
}

STBIDEF int stbi_zlib_decode_buffer(char *obuffer, int olen, char const *ibuffer, int ilen)
{
   stbi__zbuf a;
   a.zbuffer = (stbi_uc *) ibuffer;
   a.zbuffer_end = (stbi_uc *) ibuffer + ilen;
   if (stbi__do_zlib(&a, obuffer, olen, 0, 1))
      return (int) (a.zout - a.zout_start);
   else
      return -1;
}

STBIDEF char *stbi_zlib_decode_noheader_malloc(char const *buffer, int len, int *outlen)
{
   stbi__zbuf a;
   char *p = (char *) stbi__malloc(16384);
   if (p == NULL) return NULL;
   a.zbuffer = (stbi_uc *) buffer;
   a.zbuffer_end = (stbi_uc *) buffer+len;
   if (stbi__do_zlib(&a, p, 16384, 1, 0)) {
      if (outlen) *outlen = (int) (a.zout - a.zout_start);
      return a.zout_start;
   } else {
      STBI_FREE(a.zout_start);
      return NULL;
   }
}

STBIDEF int stbi_zlib_decode_noheader_buffer(char *obuffer, int olen, const char *ibuffer, int ilen)
{
   stbi__zbuf a;
   a.zbuffer = (stbi_uc *) ibuffer;
   a.zbuffer_end = (stbi_uc *) ibuffer + ilen;
   if (stbi__do_zlib(&a, obuffer, olen, 0, 0))
      return (int) (a.zout - a.zout_start);
   else
      return -1;
}
#endif

// public domain "baseline" PNG decoder   v0.10  Sean Barrett 2006-11-18
//    simple implementation
//      - only 8-bit samples
//      - no CRC checking
//      - allocates lots of intermediate memory
//        - avoids problem of streaming data between subsystems
//        - avoids explicit window management
//    performance
//      - uses stb_zlib, a PD zlib implementation with fast huffman decoding

#ifndef STBI_NO_PNG
typedef struct
{
   stbi__uint32 length;
   stbi__uint32 type;
} stbi__pngchunk;

static stbi__pngchunk stbi__get_chunk_header(stbi__context *s)
{
   stbi__pngchunk c;
   c.length = stbi__get32be(s);
   c.type   = stbi__get32be(s);
   return c;
}

static int stbi__check_png_header(stbi__context *s)
{
   static const stbi_uc png_sig[8] = { 137,80,78,71,13,10,26,10 };
   int i;
   for (i=0; i < 8; ++i)
      if (stbi__get8(s) != png_sig[i]) return stbi__err("bad png sig","Not a PNG");
   return 1;
}

typedef struct
{
   stbi__context *s;
   stbi_uc *idata, *expanded, *out;
   int depth;
} stbi__png;


enum {
   STBI__F_none=0,
   STBI__F_sub=1,
   STBI__F_up=2,
   STBI__F_avg=3,
   STBI__F_paeth=4,
   // synthetic filters used for first scanline to avoid needing a dummy row of 0s
   STBI__F_avg_first,
   STBI__F_paeth_first
};

static stbi_uc first_row_filter[5] =
{
   STBI__F_none,
   STBI__F_sub,
   STBI__F_none,
   STBI__F_avg_first,
   STBI__F_paeth_first
};

static int stbi__paeth(int a, int b, int c)
{
   int p = a + b - c;
   int pa = abs(p-a);
   int pb = abs(p-b);
   int pc = abs(p-c);
   if (pa <= pb && pa <= pc) return a;
   if (pb <= pc) return b;
   return c;
}

static const stbi_uc stbi__depth_scale_table[9] = { 0, 0xff, 0x55, 0, 0x11, 0,0,0, 0x01 };

// create the png data from post-deflated data
static int stbi__create_png_image_raw(stbi__png *a, stbi_uc *raw, stbi__uint32 raw_len, int out_n, stbi__uint32 x, stbi__uint32 y, int depth, int color)
{
   int bytes = (depth == 16? 2 : 1);
   stbi__context *s = a->s;
   stbi__uint32 i,j,stride = x*out_n*bytes;
   stbi__uint32 img_len, img_width_bytes;
   int k;
   int img_n = s->img_n; // copy it into a local for later

   int output_bytes = out_n*bytes;
   int filter_bytes = img_n*bytes;
   int width = x;

   STBI_ASSERT(out_n == s->img_n || out_n == s->img_n+1);
   a->out = (stbi_uc *) stbi__malloc_mad3(x, y, output_bytes, 0); // extra bytes to write off the end into
   if (!a->out) return stbi__err("outofmem", "Out of memory");

   if (!stbi__mad3sizes_valid(img_n, x, depth, 7)) return stbi__err("too large", "Corrupt PNG");
   img_width_bytes = (((img_n * x * depth) + 7) >> 3);
   img_len = (img_width_bytes + 1) * y;

   // we used to check for exact match between raw_len and img_len on non-interlaced PNGs,
   // but issue #276 reported a PNG in the wild that had extra data at the end (all zeros),
   // so just check for raw_len < img_len always.
   if (raw_len < img_len) return stbi__err("not enough pixels","Corrupt PNG");

   for (j=0; j < y; ++j) {
      stbi_uc *cur = a->out + stride*j;
      stbi_uc *prior;
      int filter = *raw++;

      if (filter > 4)
         return stbi__err("invalid filter","Corrupt PNG");

      if (depth < 8) {
         if (img_width_bytes > x) return stbi__err("invalid width","Corrupt PNG");
         cur += x*out_n - img_width_bytes; // store output to the rightmost img_len bytes, so we can decode in place
         filter_bytes = 1;
         width = img_width_bytes;
      }
      prior = cur - stride; // bugfix: need to compute this after 'cur +=' computation above

      // if first row, use special filter that doesn't sample previous row
      if (j == 0) filter = first_row_filter[filter];

      // handle first byte explicitly
      for (k=0; k < filter_bytes; ++k) {
         switch (filter) {
            case STBI__F_none       : cur[k] = raw[k]; break;
            case STBI__F_sub        : cur[k] = raw[k]; break;
            case STBI__F_up         : cur[k] = STBI__BYTECAST(raw[k] + prior[k]); break;
            case STBI__F_avg        : cur[k] = STBI__BYTECAST(raw[k] + (prior[k]>>1)); break;
            case STBI__F_paeth      : cur[k] = STBI__BYTECAST(raw[k] + stbi__paeth(0,prior[k],0)); break;
            case STBI__F_avg_first  : cur[k] = raw[k]; break;
            case STBI__F_paeth_first: cur[k] = raw[k]; break;
         }
      }

      if (depth == 8) {
         if (img_n != out_n)
            cur[img_n] = 255; // first pixel
         raw += img_n;
         cur += out_n;
         prior += out_n;
      } else if (depth == 16) {
         if (img_n != out_n) {
            cur[filter_bytes]   = 255; // first pixel top byte
            cur[filter_bytes+1] = 255; // first pixel bottom byte
         }
         raw += filter_bytes;
         cur += output_bytes;
         prior += output_bytes;
      } else {
         raw += 1;
         cur += 1;
         prior += 1;
      }

      // this is a little gross, so that we don't switch per-pixel or per-component
      if (depth < 8 || img_n == out_n) {
         int nk = (width - 1)*filter_bytes;
         #define STBI__CASE(f) \
             case f:     \
                for (k=0; k < nk; ++k)
         switch (filter) {
            // "none" filter turns into a memcpy here; make that explicit.
            case STBI__F_none:         memcpy(cur, raw, nk); break;
            STBI__CASE(STBI__F_sub)          { cur[k] = STBI__BYTECAST(raw[k] + cur[k-filter_bytes]); } break;
            STBI__CASE(STBI__F_up)           { cur[k] = STBI__BYTECAST(raw[k] + prior[k]); } break;
            STBI__CASE(STBI__F_avg)          { cur[k] = STBI__BYTECAST(raw[k] + ((prior[k] + cur[k-filter_bytes])>>1)); } break;
            STBI__CASE(STBI__F_paeth)        { cur[k] = STBI__BYTECAST(raw[k] + stbi__paeth(cur[k-filter_bytes],prior[k],prior[k-filter_bytes])); } break;
            STBI__CASE(STBI__F_avg_first)    { cur[k] = STBI__BYTECAST(raw[k] + (cur[k-filter_bytes] >> 1)); } break;
            STBI__CASE(STBI__F_paeth_first)  { cur[k] = STBI__BYTECAST(raw[k] + stbi__paeth(cur[k-filter_bytes],0,0)); } break;
         }
         #undef STBI__CASE
         raw += nk;
      } else {
         STBI_ASSERT(img_n+1 == out_n);
         #define STBI__CASE(f) \
             case f:     \
                for (i=x-1; i >= 1; --i, cur[filter_bytes]=255,raw+=filter_bytes,cur+=output_bytes,prior+=output_bytes) \
                   for (k=0; k < filter_bytes; ++k)
         switch (filter) {
            STBI__CASE(STBI__F_none)         { cur[k] = raw[k]; } break;
            STBI__CASE(STBI__F_sub)          { cur[k] = STBI__BYTECAST(raw[k] + cur[k- output_bytes]); } break;
            STBI__CASE(STBI__F_up)           { cur[k] = STBI__BYTECAST(raw[k] + prior[k]); } break;
            STBI__CASE(STBI__F_avg)          { cur[k] = STBI__BYTECAST(raw[k] + ((prior[k] + cur[k- output_bytes])>>1)); } break;
            STBI__CASE(STBI__F_paeth)        { cur[k] = STBI__BYTECAST(raw[k] + stbi__paeth(cur[k- output_bytes],prior[k],prior[k- output_bytes])); } break;
            STBI__CASE(STBI__F_avg_first)    { cur[k] = STBI__BYTECAST(raw[k] + (cur[k- output_bytes] >> 1)); } break;
            STBI__CASE(STBI__F_paeth_first)  { cur[k] = STBI__BYTECAST(raw[k] + stbi__paeth(cur[k- output_bytes],0,0)); } break;
         }
         #undef STBI__CASE

         // the loop above sets the high byte of the pixels' alpha, but for
         // 16 bit png files we also need the low byte set. we'll do that here.
         if (depth == 16) {
            cur = a->out + stride*j; // start at the beginning of the row again
            for (i=0; i < x; ++i,cur+=output_bytes) {
               cur[filter_bytes+1] = 255;
            }
         }
      }
   }

   // we make a separate pass to expand bits to pixels; for performance,
   // this could run two scanlines behind the above code, so it won't
   // intefere with filtering but will still be in the cache.
   if (depth < 8) {
      for (j=0; j < y; ++j) {
         stbi_uc *cur = a->out + stride*j;
         stbi_uc *in  = a->out + stride*j + x*out_n - img_width_bytes;
         // unpack 1/2/4-bit into a 8-bit buffer. allows us to keep the common 8-bit path optimal at minimal cost for 1/2/4-bit
         // png guarante byte alignment, if width is not multiple of 8/4/2 we'll decode dummy trailing data that will be skipped in the later loop
         stbi_uc scale = (color == 0) ? stbi__depth_scale_table[depth] : 1; // scale grayscale values to 0..255 range

         // note that the final byte might overshoot and write more data than desired.
         // we can allocate enough data that this never writes out of memory, but it
         // could also overwrite the next scanline. can it overwrite non-empty data
         // on the next scanline? yes, consider 1-pixel-wide scanlines with 1-bit-per-pixel.
         // so we need to explicitly clamp the final ones

         if (depth == 4) {
            for (k=x*img_n; k >= 2; k-=2, ++in) {
               *cur++ = scale * ((*in >> 4)       );
               *cur++ = scale * ((*in     ) & 0x0f);
            }
            if (k > 0) *cur++ = scale * ((*in >> 4)       );
         } else if (depth == 2) {
            for (k=x*img_n; k >= 4; k-=4, ++in) {
               *cur++ = scale * ((*in >> 6)       );
               *cur++ = scale * ((*in >> 4) & 0x03);
               *cur++ = scale * ((*in >> 2) & 0x03);
               *cur++ = scale * ((*in     ) & 0x03);
            }
            if (k > 0) *cur++ = scale * ((*in >> 6)       );
            if (k > 1) *cur++ = scale * ((*in >> 4) & 0x03);
            if (k > 2) *cur++ = scale * ((*in >> 2) & 0x03);
         } else if (depth == 1) {
            for (k=x*img_n; k >= 8; k-=8, ++in) {
               *cur++ = scale * ((*in >> 7)       );
               *cur++ = scale * ((*in >> 6) & 0x01);
               *cur++ = scale * ((*in >> 5) & 0x01);
               *cur++ = scale * ((*in >> 4) & 0x01);
               *cur++ = scale * ((*in >> 3) & 0x01);
               *cur++ = scale * ((*in >> 2) & 0x01);
               *cur++ = scale * ((*in >> 1) & 0x01);
               *cur++ = scale * ((*in     ) & 0x01);
            }
            if (k > 0) *cur++ = scale * ((*in >> 7)       );
            if (k > 1) *cur++ = scale * ((*in >> 6) & 0x01);
            if (k > 2) *cur++ = scale * ((*in >> 5) & 0x01);
            if (k > 3) *cur++ = scale * ((*in >> 4) & 0x01);
            if (k > 4) *cur++ = scale * ((*in >> 3) & 0x01);
            if (k > 5) *cur++ = scale * ((*in >> 2) & 0x01);
            if (k > 6) *cur++ = scale * ((*in >> 1) & 0x01);
         }
         if (img_n != out_n) {
            int q;
            // insert alpha = 255
            cur = a->out + stride*j;
            if (img_n == 1) {
               for (q=x-1; q >= 0; --q) {
                  cur[q*2+1] = 255;
                  cur[q*2+0] = cur[q];
               }
            } else {
               STBI_ASSERT(img_n == 3);
               for (q=x-1; q >= 0; --q) {
                  cur[q*4+3] = 255;
                  cur[q*4+2] = cur[q*3+2];
                  cur[q*4+1] = cur[q*3+1];
                  cur[q*4+0] = cur[q*3+0];
               }
            }
         }
      }
   } else if (depth == 16) {
      // force the image data from big-endian to platform-native.
      // this is done in a separate pass due to the decoding relying
      // on the data being untouched, but could probably be done
      // per-line during decode if care is taken.
      stbi_uc *cur = a->out;
      stbi__uint16 *cur16 = (stbi__uint16*)cur;

      for(i=0; i < x*y*out_n; ++i,cur16++,cur+=2) {
         *cur16 = (cur[0] << 8) | cur[1];
      }
   }

   return 1;
}

static int stbi__create_png_image(stbi__png *a, stbi_uc *image_data, stbi__uint32 image_data_len, int out_n, int depth, int color, int interlaced)
{
   int bytes = (depth == 16 ? 2 : 1);
   int out_bytes = out_n * bytes;
   stbi_uc *final;
   int p;
   if (!interlaced)
      return stbi__create_png_image_raw(a, image_data, image_data_len, out_n, a->s->img_x, a->s->img_y, depth, color);

   // de-interlacing
   final = (stbi_uc *) stbi__malloc_mad3(a->s->img_x, a->s->img_y, out_bytes, 0);
   if (!final) return stbi__err("outofmem", "Out of memory");
   for (p=0; p < 7; ++p) {
      int xorig[] = { 0,4,0,2,0,1,0 };
      int yorig[] = { 0,0,4,0,2,0,1 };
      int xspc[]  = { 8,8,4,4,2,2,1 };
      int yspc[]  = { 8,8,8,4,4,2,2 };
      int i,j,x,y;
      // pass1_x[4] = 0, pass1_x[5] = 1, pass1_x[12] = 1
      x = (a->s->img_x - xorig[p] + xspc[p]-1) / xspc[p];
      y = (a->s->img_y - yorig[p] + yspc[p]-1) / yspc[p];
      if (x && y) {
         stbi__uint32 img_len = ((((a->s->img_n * x * depth) + 7) >> 3) + 1) * y;
         if (!stbi__create_png_image_raw(a, image_data, image_data_len, out_n, x, y, depth, color)) {
            STBI_FREE(final);
            return 0;
         }
         for (j=0; j < y; ++j) {
            for (i=0; i < x; ++i) {
               int out_y = j*yspc[p]+yorig[p];
               int out_x = i*xspc[p]+xorig[p];
               memcpy(final + out_y*a->s->img_x*out_bytes + out_x*out_bytes,
                      a->out + (j*x+i)*out_bytes, out_bytes);
            }
         }
         STBI_FREE(a->out);
         image_data += img_len;
         image_data_len -= img_len;
      }
   }
   a->out = final;

   return 1;
}

static int stbi__compute_transparency(stbi__png *z, stbi_uc tc[3], int out_n)
{
   stbi__context *s = z->s;
   stbi__uint32 i, pixel_count = s->img_x * s->img_y;
   stbi_uc *p = z->out;

   // compute color-based transparency, assuming we've
   // already got 255 as the alpha value in the output
   STBI_ASSERT(out_n == 2 || out_n == 4);

   if (out_n == 2) {
      for (i=0; i < pixel_count; ++i) {
         p[1] = (p[0] == tc[0] ? 0 : 255);
         p += 2;
      }
   } else {
      for (i=0; i < pixel_count; ++i) {
         if (p[0] == tc[0] && p[1] == tc[1] && p[2] == tc[2])
            p[3] = 0;
         p += 4;
      }
   }
   return 1;
}

static int stbi__compute_transparency16(stbi__png *z, stbi__uint16 tc[3], int out_n)
{
   stbi__context *s = z->s;
   stbi__uint32 i, pixel_count = s->img_x * s->img_y;
   stbi__uint16 *p = (stbi__uint16*) z->out;

   // compute color-based transparency, assuming we've
   // already got 65535 as the alpha value in the output
   STBI_ASSERT(out_n == 2 || out_n == 4);

   if (out_n == 2) {
      for (i = 0; i < pixel_count; ++i) {
         p[1] = (p[0] == tc[0] ? 0 : 65535);
         p += 2;
      }
   } else {
      for (i = 0; i < pixel_count; ++i) {
         if (p[0] == tc[0] && p[1] == tc[1] && p[2] == tc[2])
            p[3] = 0;
         p += 4;
      }
   }
   return 1;
}

static int stbi__expand_png_palette(stbi__png *a, stbi_uc *palette, int len, int pal_img_n)
{
   stbi__uint32 i, pixel_count = a->s->img_x * a->s->img_y;
   stbi_uc *p, *temp_out, *orig = a->out;

   p = (stbi_uc *) stbi__malloc_mad2(pixel_count, pal_img_n, 0);
   if (p == NULL) return stbi__err("outofmem", "Out of memory");

   // between here and free(out) below, exitting would leak
   temp_out = p;

   if (pal_img_n == 3) {
      for (i=0; i < pixel_count; ++i) {
         int n = orig[i]*4;
         p[0] = palette[n  ];
         p[1] = palette[n+1];
         p[2] = palette[n+2];
         p += 3;
      }
   } else {
      for (i=0; i < pixel_count; ++i) {
         int n = orig[i]*4;
         p[0] = palette[n  ];
         p[1] = palette[n+1];
         p[2] = palette[n+2];
         p[3] = palette[n+3];
         p += 4;
      }
   }
   STBI_FREE(a->out);
   a->out = temp_out;

   STBI_NOTUSED(len);

   return 1;
}

static int stbi__unpremultiply_on_load_global = 0;
static int stbi__de_iphone_flag_global = 0;

STBIDEF void stbi_set_unpremultiply_on_load(int flag_true_if_should_unpremultiply)
{
   stbi__unpremultiply_on_load_global = flag_true_if_should_unpremultiply;
}

STBIDEF void stbi_convert_iphone_png_to_rgb(int flag_true_if_should_convert)
{
   stbi__de_iphone_flag_global = flag_true_if_should_convert;
}

#ifndef STBI_THREAD_LOCAL
#define stbi__unpremultiply_on_load  stbi__unpremultiply_on_load_global
#define stbi__de_iphone_flag  stbi__de_iphone_flag_global
#else
static STBI_THREAD_LOCAL int stbi__unpremultiply_on_load_local, stbi__unpremultiply_on_load_set;
static STBI_THREAD_LOCAL int stbi__de_iphone_flag_local, stbi__de_iphone_flag_set;

STBIDEF void stbi__unpremultiply_on_load_thread(int flag_true_if_should_unpremultiply)
{
   stbi__unpremultiply_on_load_local = flag_true_if_should_unpremultiply;
   stbi__unpremultiply_on_load_set = 1;
}

STBIDEF void stbi_convert_iphone_png_to_rgb_thread(int flag_true_if_should_convert)
{
   stbi__de_iphone_flag_local = flag_true_if_should_convert;
   stbi__de_iphone_flag_set = 1;
}

#define stbi__unpremultiply_on_load  (stbi__unpremultiply_on_load_set           \
                                       ? stbi__unpremultiply_on_load_local      \
                                       : stbi__unpremultiply_on_load_global)
#define stbi__de_iphone_flag  (stbi__de_iphone_flag_set                         \
                                ? stbi__de_iphone_flag_local                    \
                                : stbi__de_iphone_flag_global)
#endif // STBI_THREAD_LOCAL

static void stbi__de_iphone(stbi__png *z)
{
   stbi__context *s = z->s;
   stbi__uint32 i, pixel_count = s->img_x * s->img_y;
   stbi_uc *p = z->out;

   if (s->img_out_n == 3) {  // convert bgr to rgb
      for (i=0; i < pixel_count; ++i) {
         stbi_uc t = p[0];
         p[0] = p[2];
         p[2] = t;
         p += 3;
      }
   } else {
      STBI_ASSERT(s->img_out_n == 4);
      if (stbi__unpremultiply_on_load) {
         // convert bgr to rgb and unpremultiply
         for (i=0; i < pixel_count; ++i) {
            stbi_uc a = p[3];
            stbi_uc t = p[0];
            if (a) {
               stbi_uc half = a / 2;
               p[0] = (p[2] * 255 + half) / a;
               p[1] = (p[1] * 255 + half) / a;
               p[2] = ( t   * 255 + half) / a;
            } else {
               p[0] = p[2];
               p[2] = t;
            }
            p += 4;
         }
      } else {
         // convert bgr to rgb
         for (i=0; i < pixel_count; ++i) {
            stbi_uc t = p[0];
            p[0] = p[2];
            p[2] = t;
            p += 4;
         }
      }
   }
}

#define STBI__PNG_TYPE(a,b,c,d)  (((unsigned) (a) << 24) + ((unsigned) (b) << 16) + ((unsigned) (c) << 8) + (unsigned) (d))

static int stbi__parse_png_file(stbi__png *z, int scan, int req_comp)
{
   stbi_uc palette[1024], pal_img_n=0;
   stbi_uc has_trans=0, tc[3]={0};
   stbi__uint16 tc16[3];
   stbi__uint32 ioff=0, idata_limit=0, i, pal_len=0;
   int first=1,k,interlace=0, color=0, is_iphone=0;
   stbi__context *s = z->s;

   z->expanded = NULL;
   z->idata = NULL;
   z->out = NULL;

   if (!stbi__check_png_header(s)) return 0;

   if (scan == STBI__SCAN_type) return 1;

   for (;;) {
      stbi__pngchunk c = stbi__get_chunk_header(s);
      switch (c.type) {
         case STBI__PNG_TYPE('C','g','B','I'):
            is_iphone = 1;
            stbi__skip(s, c.length);
            break;
         case STBI__PNG_TYPE('I','H','D','R'): {
            int comp,filter;
            if (!first) return stbi__err("multiple IHDR","Corrupt PNG");
            first = 0;
            if (c.length != 13) return stbi__err("bad IHDR len","Corrupt PNG");
            s->img_x = stbi__get32be(s);
            s->img_y = stbi__get32be(s);
            if (s->img_y > STBI_MAX_DIMENSIONS) return stbi__err("too large","Very large image (corrupt?)");
            if (s->img_x > STBI_MAX_DIMENSIONS) return stbi__err("too large","Very large image (corrupt?)");
            z->depth = stbi__get8(s);  if (z->depth != 1 && z->depth != 2 && z->depth != 4 && z->depth != 8 && z->depth != 16)  return stbi__err("1/2/4/8/16-bit only","PNG not supported: 1/2/4/8/16-bit only");
            color = stbi__get8(s);  if (color > 6)         return stbi__err("bad ctype","Corrupt PNG");
            if (color == 3 && z->depth == 16)                  return stbi__err("bad ctype","Corrupt PNG");
            if (color == 3) pal_img_n = 3; else if (color & 1) return stbi__err("bad ctype","Corrupt PNG");
            comp  = stbi__get8(s);  if (comp) return stbi__err("bad comp method","Corrupt PNG");
            filter= stbi__get8(s);  if (filter) return stbi__err("bad filter method","Corrupt PNG");
            interlace = stbi__get8(s); if (interlace>1) return stbi__err("bad interlace method","Corrupt PNG");
            if (!s->img_x || !s->img_y) return stbi__err("0-pixel image","Corrupt PNG");
            if (!pal_img_n) {
               s->img_n = (color & 2 ? 3 : 1) + (color & 4 ? 1 : 0);
               if ((1 << 30) / s->img_x / s->img_n < s->img_y) return stbi__err("too large", "Image too large to decode");
               if (scan == STBI__SCAN_header) return 1;
            } else {
               // if paletted, then pal_n is our final components, and
               // img_n is # components to decompress/filter.
               s->img_n = 1;
               if ((1 << 30) / s->img_x / 4 < s->img_y) return stbi__err("too large","Corrupt PNG");
               // if SCAN_header, have to scan to see if we have a tRNS
            }
            break;
         }

         case STBI__PNG_TYPE('P','L','T','E'):  {
            if (first) return stbi__err("first not IHDR", "Corrupt PNG");
            if (c.length > 256*3) return stbi__err("invalid PLTE","Corrupt PNG");
            pal_len = c.length / 3;
            if (pal_len * 3 != c.length) return stbi__err("invalid PLTE","Corrupt PNG");
            for (i=0; i < pal_len; ++i) {
               palette[i*4+0] = stbi__get8(s);
               palette[i*4+1] = stbi__get8(s);
               palette[i*4+2] = stbi__get8(s);
               palette[i*4+3] = 255;
            }
            break;
         }

         case STBI__PNG_TYPE('t','R','N','S'): {
            if (first) return stbi__err("first not IHDR", "Corrupt PNG");
            if (z->idata) return stbi__err("tRNS after IDAT","Corrupt PNG");
            if (pal_img_n) {
               if (scan == STBI__SCAN_header) { s->img_n = 4; return 1; }
               if (pal_len == 0) return stbi__err("tRNS before PLTE","Corrupt PNG");
               if (c.length > pal_len) return stbi__err("bad tRNS len","Corrupt PNG");
               pal_img_n = 4;
               for (i=0; i < c.length; ++i)
                  palette[i*4+3] = stbi__get8(s);
            } else {
               if (!(s->img_n & 1)) return stbi__err("tRNS with alpha","Corrupt PNG");
               if (c.length != (stbi__uint32) s->img_n*2) return stbi__err("bad tRNS len","Corrupt PNG");
               has_trans = 1;
               if (z->depth == 16) {
                  for (k = 0; k < s->img_n; ++k) tc16[k] = (stbi__uint16)stbi__get16be(s); // copy the values as-is
               } else {
                  for (k = 0; k < s->img_n; ++k) tc[k] = (stbi_uc)(stbi__get16be(s) & 255) * stbi__depth_scale_table[z->depth]; // non 8-bit images will be larger
               }
            }
            break;
         }

         case STBI__PNG_TYPE('I','D','A','T'): {
            if (first) return stbi__err("first not IHDR", "Corrupt PNG");
            if (pal_img_n && !pal_len) return stbi__err("no PLTE","Corrupt PNG");
            if (scan == STBI__SCAN_header) { s->img_n = pal_img_n; return 1; }
            if ((int)(ioff + c.length) < (int)ioff) return 0;
            if (ioff + c.length > idata_limit) {
               stbi__uint32 idata_limit_old = idata_limit;
               stbi_uc *p;
               if (idata_limit == 0) idata_limit = c.length > 4096 ? c.length : 4096;
               while (ioff + c.length > idata_limit)
                  idata_limit *= 2;
               STBI_NOTUSED(idata_limit_old);
               p = (stbi_uc *) STBI_REALLOC_SIZED(z->idata, idata_limit_old, idata_limit); if (p == NULL) return stbi__err("outofmem", "Out of memory");
               z->idata = p;
            }
            if (!stbi__getn(s, z->idata+ioff,c.length)) return stbi__err("outofdata","Corrupt PNG");
            ioff += c.length;
            break;
         }

         case STBI__PNG_TYPE('I','E','N','D'): {
            stbi__uint32 raw_len, bpl;
            if (first) return stbi__err("first not IHDR", "Corrupt PNG");
            if (scan != STBI__SCAN_load) return 1;
            if (z->idata == NULL) return stbi__err("no IDAT","Corrupt PNG");
            // initial guess for decoded data size to avoid unnecessary reallocs
            bpl = (s->img_x * z->depth + 7) / 8; // bytes per line, per component
            raw_len = bpl * s->img_y * s->img_n /* pixels */ + s->img_y /* filter mode per row */;
            z->expanded = (stbi_uc *) stbi_zlib_decode_malloc_guesssize_headerflag((char *) z->idata, ioff, raw_len, (int *) &raw_len, !is_iphone);
            if (z->expanded == NULL) return 0; // zlib should set error
            STBI_FREE(z->idata); z->idata = NULL;
            if ((req_comp == s->img_n+1 && req_comp != 3 && !pal_img_n) || has_trans)
               s->img_out_n = s->img_n+1;
            else
               s->img_out_n = s->img_n;
            if (!stbi__create_png_image(z, z->expanded, raw_len, s->img_out_n, z->depth, color, interlace)) return 0;
            if (has_trans) {
               if (z->depth == 16) {
                  if (!stbi__compute_transparency16(z, tc16, s->img_out_n)) return 0;
               } else {
                  if (!stbi__compute_transparency(z, tc, s->img_out_n)) return 0;
               }
            }
            if (is_iphone && stbi__de_iphone_flag && s->img_out_n > 2)
               stbi__de_iphone(z);
            if (pal_img_n) {
               // pal_img_n == 3 or 4
               s->img_n = pal_img_n; // record the actual colors we had
               s->img_out_n = pal_img_n;
               if (req_comp >= 3) s->img_out_n = req_comp;
               if (!stbi__expand_png_palette(z, palette, pal_len, s->img_out_n))
                  return 0;
            } else if (has_trans) {
               // non-paletted image with tRNS -> source image has (constant) alpha
               ++s->img_n;
            }
            STBI_FREE(z->expanded); z->expanded = NULL;
            // end of PNG chunk, read and skip CRC
            stbi__get32be(s);
            return 1;
         }

         default:
            // if critical, fail
            if (first) return stbi__err("first not IHDR", "Corrupt PNG");
            if ((c.type & (1 << 29)) == 0) {
               #ifndef STBI_NO_FAILURE_STRINGS
               // not threadsafe
               static char invalid_chunk[] = "XXXX PNG chunk not known";
               invalid_chunk[0] = STBI__BYTECAST(c.type >> 24);
               invalid_chunk[1] = STBI__BYTECAST(c.type >> 16);
               invalid_chunk[2] = STBI__BYTECAST(c.type >>  8);
               invalid_chunk[3] = STBI__BYTECAST(c.type >>  0);
               #endif
               return stbi__err(invalid_chunk, "PNG not supported: unknown PNG chunk type");
            }
            stbi__skip(s, c.length);
            break;
      }
      // end of PNG chunk, read and skip CRC
      stbi__get32be(s);
   }
}

static void *stbi__do_png(stbi__png *p, int *x, int *y, int *n, int req_comp, stbi__result_info *ri)
{
   void *result=NULL;
   if (req_comp < 0 || req_comp > 4) return stbi__errpuc("bad req_comp", "Internal error");
   if (stbi__parse_png_file(p, STBI__SCAN_load, req_comp)) {
      if (p->depth <= 8)
         ri->bits_per_channel = 8;
      else if (p->depth == 16)
         ri->bits_per_channel = 16;
      else
         return stbi__errpuc("bad bits_per_channel", "PNG not supported: unsupported color depth");
      result = p->out;
      p->out = NULL;
      if (req_comp && req_comp != p->s->img_out_n) {
         if (ri->bits_per_channel == 8)
            result = stbi__convert_format((unsigned char *) result, p->s->img_out_n, req_comp, p->s->img_x, p->s->img_y);
         else
            result = stbi__convert_format16((stbi__uint16 *) result, p->s->img_out_n, req_comp, p->s->img_x, p->s->img_y);
         p->s->img_out_n = req_comp;
         if (result == NULL) return result;
      }
      *x = p->s->img_x;
      *y = p->s->img_y;
      if (n) *n = p->s->img_n;
   }
   STBI_FREE(p->out);      p->out      = NULL;
   STBI_FREE(p->expanded); p->expanded = NULL;
   STBI_FREE(p->idata);    p->idata    = NULL;

   return result;
}

static void *stbi__png_load(stbi__context *s, int *x, int *y, int *comp, int req_comp, stbi__result_info *ri)
{
   stbi__png p;
   p.s = s;
   return stbi__do_png(&p, x,y,comp,req_comp, ri);
}

static int stbi__png_test(stbi__context *s)
{
   int r;
   r = stbi__check_png_header(s);
   stbi__rewind(s);
   return r;
}

static int stbi__png_info_raw(stbi__png *p, int *x, int *y, int *comp)
{
   if (!stbi__parse_png_file(p, STBI__SCAN_header, 0)) {
      stbi__rewind( p->s );
      return 0;
   }
   if (x) *x = p->s->img_x;
   if (y) *y = p->s->img_y;
   if (comp) *comp = p->s->img_n;
   return 1;
}

static int stbi__png_info(stbi__context *s, int *x, int *y, int *comp)
{
   stbi__png p;
   p.s = s;
   return stbi__png_info_raw(&p, x, y, comp);
}

static int stbi__png_is16(stbi__context *s)
{
   stbi__png p;
   p.s = s;
   if (!stbi__png_info_raw(&p, NULL, NULL, NULL))
	   return 0;
   if (p.depth != 16) {
      stbi__rewind(p.s);
      return 0;
   }
   return 1;
}
#endif

// Microsoft/Windows BMP image

#ifndef STBI_NO_BMP
static int stbi__bmp_test_raw(stbi__context *s)
{
   int r;
   int sz;
   if (stbi__get8(s) != 'B') return 0;
   if (stbi__get8(s) != 'M') return 0;
   stbi__get32le(s); // discard filesize
   stbi__get16le(s); // discard reserved
   stbi__get16le(s); // discard reserved
   stbi__get32le(s); // discard data offset
   sz = stbi__get32le(s);
   r = (sz == 12 || sz == 40 || sz == 56 || sz == 108 || sz == 124);
   return r;
}

static int stbi__bmp_test(stbi__context *s)
{
   int r = stbi__bmp_test_raw(s);
   stbi__rewind(s);
   return r;
}


// returns 0..31 for the highest set bit
static int stbi__high_bit(unsigned int z)
{
   int n=0;
   if (z == 0) return -1;
   if (z >= 0x10000) { n += 16; z >>= 16; }
   if (z >= 0x00100) { n +=  8; z >>=  8; }
   if (z >= 0x00010) { n +=  4; z >>=  4; }
   if (z >= 0x00004) { n +=  2; z >>=  2; }
   if (z >= 0x00002) { n +=  1;/* >>=  1;*/ }
   return n;
}

static int stbi__bitcount(unsigned int a)
{
   a = (a & 0x55555555) + ((a >>  1) & 0x55555555); // max 2
   a = (a & 0x33333333) + ((a >>  2) & 0x33333333); // max 4
   a = (a + (a >> 4)) & 0x0f0f0f0f; // max 8 per 4, now 8 bits
   a = (a + (a >> 8)); // max 16 per 8 bits
   a = (a + (a >> 16)); // max 32 per 8 bits
   return a & 0xff;
}

// extract an arbitrarily-aligned N-bit value (N=bits)
// from v, and then make it 8-bits long and fractionally
// extend it to full full range.
static int stbi__shiftsigned(unsigned int v, int shift, int bits)
{
   static unsigned int mul_table[9] = {
      0,
      0xff/*0b11111111*/, 0x55/*0b01010101*/, 0x49/*0b01001001*/, 0x11/*0b00010001*/,
      0x21/*0b00100001*/, 0x41/*0b01000001*/, 0x81/*0b10000001*/, 0x01/*0b00000001*/,
   };
   static unsigned int shift_table[9] = {
      0, 0,0,1,0,2,4,6,0,
   };
   if (shift < 0)
      v <<= -shift;
   else
      v >>= shift;
   STBI_ASSERT(v < 256);
   v >>= (8-bits);
   STBI_ASSERT(bits >= 0 && bits <= 8);
   return (int) ((unsigned) v * mul_table[bits]) >> shift_table[bits];
}

typedef struct
{
   int bpp, offset, hsz;
   unsigned int mr,mg,mb,ma, all_a;
   int extra_read;
} stbi__bmp_data;

static int stbi__bmp_set_mask_defaults(stbi__bmp_data *info, int compress)
{
   // BI_BITFIELDS specifies masks explicitly, don't override
   if (compress == 3)
      return 1;

   if (compress == 0) {
      if (info->bpp == 16) {
         info->mr = 31u << 10;
         info->mg = 31u <<  5;
         info->mb = 31u <<  0;
      } else if (info->bpp == 32) {
         info->mr = 0xffu << 16;
         info->mg = 0xffu <<  8;
         info->mb = 0xffu <<  0;
         info->ma = 0xffu << 24;
         info->all_a = 0; // if all_a is 0 at end, then we loaded alpha channel but it was all 0
      } else {
         // otherwise, use defaults, which is all-0
         info->mr = info->mg = info->mb = info->ma = 0;
      }
      return 1;
   }
   return 0; // error
}

static void *stbi__bmp_load_cont(stbi__context *s, int *x, int *y, int *comp, int req_comp, stbi__bmp_data *info, stbi__result_info *ri)
{
   stbi_uc *out;
   unsigned int mr=0,mg=0,mb=0,ma=0, all_a;
   stbi_uc pal[256][4];
   int psize=0,i,j,width;
   int flip_vertically, pad, target;

   STBI_NOTUSED(ri);

   flip_vertically = ((int) s->img_y) > 0;
   s->img_y = abs((int) s->img_y);

   mr = info->mr;
   mg = info->mg;
   mb = info->mb;
   ma = info->ma;
   all_a = info->all_a;

   if (info->hsz == 12) {
      if (info->bpp < 24)
         psize = (info->offset - 14 - 24) / 3;
   } else {
      if (info->bpp < 16)
         psize = (info->offset - 14 - info->hsz) >> 2;
   }

   s->img_n = ma ? 4 : 3;
   if (req_comp && req_comp >= 3) // we can directly decode 3 or 4
      target = req_comp;
   else
      target = s->img_n; // if they want monochrome, we'll post-convert

   // sanity-check size
   if (!stbi__mad3sizes_valid(target, s->img_x, s->img_y, 0))
      return stbi__errpuc("too large", "Corrupt BMP");

   out = (stbi_uc *) stbi__malloc_mad3(target, s->img_x, s->img_y, 0);
   if (!out) return stbi__errpuc("outofmem", "Out of memory");
   if (info->bpp < 16) {
      int z=0;
      if (psize == 0 || psize > 256) { STBI_FREE(out); return stbi__errpuc("invalid", "Corrupt BMP"); }
      for (i=0; i < psize; ++i) {
         pal[i][2] = stbi__get8(s);
         pal[i][1] = stbi__get8(s);
         pal[i][0] = stbi__get8(s);
         if (info->hsz != 12) stbi__get8(s);
         pal[i][3] = 255;
      }
      stbi__skip(s, info->offset - 14 - info->hsz - psize * (info->hsz == 12 ? 3 : 4));
      if (info->bpp == 1) width = (s->img_x + 7) >> 3;
      else if (info->bpp == 4) width = (s->img_x + 1) >> 1;
      else if (info->bpp == 8) width = s->img_x;
      else { STBI_FREE(out); return stbi__errpuc("bad bpp", "Corrupt BMP"); }
      pad = (-width)&3;
      if (info->bpp == 1) {
         for (j=0; j < (int) s->img_y; ++j) {
            int bit_offset = 7, v = stbi__get8(s);
            for (i=0; i < (int) s->img_x; ++i) {
               int color = (v>>bit_offset)&0x1;
               out[z++] = pal[color][0];
               out[z++] = pal[color][1];
               out[z++] = pal[color][2];
               if((--bit_offset) < 0) {
                  bit_offset = 7;
                  v = stbi__get8(s);
               }
            }
            stbi__skip(s, pad);
         }
      } else {
         for (j=0; j < (int) s->img_y; ++j) {
            for (i=0; i < (int) s->img_x; i += 2) {
               int v=stbi__get8(s),v2=0;
               if (info->bpp == 4) {
                  v2 = v & 15;
                  v >>= 4;
               }
               out[z++] = pal[v][0];
               out[z++] = pal[v][1];
               out[z++] = pal[v][2];
               if (target == 4) out[z++] = 255;
               if (i+1 == (int) s->img_x) break;
               v = (info->bpp == 8) ? stbi__get8(s) : v2;
               out[z++] = pal[v][0];
               out[z++] = pal[v][1];
               out[z++] = pal[v][2];
               if (target == 4) out[z++] = 255;
            }
            stbi__skip(s, pad);
         }
      }
   } else {
      int rshift=0,gshift=0,bshift=0,ashift=0,rcount=0,gcount=0,bcount=0,acount=0;
      int z = 0;
      int easy=0;
      stbi__skip(s, info->offset - 14 - info->hsz);
      if (info->bpp == 24) width = 3 * s->img_x;
      else if (info->bpp == 16) width = 2*s->img_x;
      else /* bpp = 32 and pad = 0 */ width=0;
      pad = (-width) & 3;
      if (info->bpp == 24) {
         easy = 1;
      } else if (info->bpp == 32) {
         if (mb == 0xff && mg == 0xff00 && mr == 0x00ff0000 && ma == 0xff000000)
            easy = 2;
      }
      if (!easy) {
         if (!mr || !mg || !mb) { STBI_FREE(out); return stbi__errpuc("bad masks", "Corrupt BMP"); }
         // right shift amt to put high bit in position #7
         rshift = stbi__high_bit(mr)-7; rcount = stbi__bitcount(mr);
         gshift = stbi__high_bit(mg)-7; gcount = stbi__bitcount(mg);
         bshift = stbi__high_bit(mb)-7; bcount = stbi__bitcount(mb);
         ashift = stbi__high_bit(ma)-7; acount = stbi__bitcount(ma);
      }
      for (j=0; j < (int) s->img_y; ++j) {
         if (easy) {
            for (i=0; i < (int) s->img_x; ++i) {
               unsigned char a;
               out[z+2] = stbi__get8(s);
               out[z+1] = stbi__get8(s);
               out[z+0] = stbi__get8(s);
               z += 3;
               a = (easy == 2 ? stbi__get8(s) : 255);
               all_a |= a;
               if (target == 4) out[z++] = a;
            }
         } else {
            int bpp = info->bpp;
            for (i=0; i < (int) s->img_x; ++i) {
               stbi__uint32 v = (bpp == 16 ? (stbi__uint32) stbi__get16le(s) : stbi__get32le(s));
               unsigned int a;
               out[z++] = STBI__BYTECAST(stbi__shiftsigned(v & mr, rshift, rcount));
               out[z++] = STBI__BYTECAST(stbi__shiftsigned(v & mg, gshift, gcount));
               out[z++] = STBI__BYTECAST(stbi__shiftsigned(v & mb, bshift, bcount));
               a = (ma ? stbi__shiftsigned(v & ma, ashift, acount) : 255);
               all_a |= a;
               if (target == 4) out[z++] = STBI__BYTECAST(a);
            }
         }
         stbi__skip(s, pad);
      }
   }

   // if alpha channel is all 0s, replace with all 255s
   if (target == 4 && all_a == 0)
      for (i=4*s->img_x*s->img_y-1; i >= 0; i -= 4)
         out[i] = 255;

   if (flip_vertically) {
      stbi_uc t;
      for (j=0; j < (int) s->img_y>>1; ++j) {
         stbi_uc *p1 = out +      j     *s->img_x*target;
         stbi_uc *p2 = out + (s->img_y-1-j)*s->img_x*target;
         for (i=0; i < (int) s->img_x*target; ++i) {
            t = p1[i], p1[i] = p2[i], p2[i] = t;
         }
      }
   }

   if (req_comp && req_comp != target) {
      out = stbi__convert_format(out, target, req_comp, s->img_x, s->img_y);
      if (out == NULL) return out; // stbi__convert_format frees input on failure
   }

   *x = s->img_x;
   *y = s->img_y;
   if (comp) *comp = s->img_n;
   return out;
}

static void *stbi__bmp_parse_header_cont(stbi__context *s, stbi__bmp_data *info) {
   int hsz = info->hsz;
   if (hsz != 12 && hsz != 40 && hsz != 56 && hsz != 108 && hsz != 124) return stbi__errpuc("unknown BMP", "BMP type not supported: unknown");
   if (hsz == 12) {
      s->img_x = stbi__get16le(s);
      s->img_y = stbi__get16le(s);
   } else {
      s->img_x = stbi__get32le(s);
      s->img_y = stbi__get32le(s);
   }
   if (stbi__get16le(s) != 1) return stbi__errpuc("bad BMP", "bad BMP");
   info->bpp = stbi__get16le(s);
   if (hsz != 12) {
      int compress = stbi__get32le(s);
      if (compress == 1 || compress == 2) return stbi__errpuc("BMP RLE", "BMP type not supported: RLE");
      stbi__get32le(s); // discard sizeof
      stbi__get32le(s); // discard hres
      stbi__get32le(s); // discard vres
      stbi__get32le(s); // discard colorsused
      stbi__get32le(s); // discard max important
      if (hsz == 40 || hsz == 56) {
         if (hsz == 56) {
            stbi__get32le(s);
            stbi__get32le(s);
            stbi__get32le(s);
            stbi__get32le(s);
         }
         if (info->bpp == 16 || info->bpp == 32) {
            if (compress == 0) {
               if (info->bpp == 32) {
                  info->mr = 0xffu << 16;
                  info->mg = 0xffu <<  8;
                  info->mb = 0xffu <<  0;
                  info->ma = 0xffu << 24;
                  info->all_a = 0; // if all_a is 0 at end, then we loaded alpha channel but it was all 0
               } else {
                  info->mr = 31u << 10;
                  info->mg = 31u <<  5;
                  info->mb = 31u <<  0;
               }
            } else if (compress == 3) {
               info->mr = stbi__get32le(s);
               info->mg = stbi__get32le(s);
               info->mb = stbi__get32le(s);
               // not documented, but generated by photoshop and handled by mspaint
               if (info->mr == info->mg && info->mg == info->mb) {
                  // ?!?!?
                  return stbi__errpuc("bad BMP", "bad BMP");
               }
            } else
               return stbi__errpuc("bad BMP", "bad BMP");
         }
      } else {
         int i;
         if (hsz != 108 && hsz != 124)
            return stbi__errpuc("bad BMP", "bad BMP");
         info->mr = stbi__get32le(s);
         info->mg = stbi__get32le(s);
         info->mb = stbi__get32le(s);
         info->ma = stbi__get32le(s);
         stbi__get32le(s); // discard color space
         for (i=0; i < 12; ++i)
            stbi__get32le(s); // discard color space parameters
         if (hsz == 124) {
            stbi__get32le(s); // discard rendering intent
            stbi__get32le(s); // discard offset of profile data
            stbi__get32le(s); // discard size of profile data
            stbi__get32le(s); // discard reserved
         }
      }
   }
   return (void *) 1;
}

static void *stbi__bmp_parse_header(stbi__context *s, stbi__bmp_data *info)
{
   int hsz;
   if (stbi__get8(s) != 'B' || stbi__get8(s) != 'M') return stbi__errpuc("not BMP", "Corrupt BMP");
   stbi__get32le(s); // discard filesize
   stbi__get16le(s); // discard reserved
   stbi__get16le(s); // discard reserved
   info->offset = stbi__get32le(s);
   info->hsz = hsz = stbi__get32le(s);
   info->mr = info->mg = info->mb = info->ma = 0;
   info->extra_read = 14;

   if (info->offset < 0) return stbi__errpuc("bad BMP", "bad BMP");

   if (hsz != 12 && hsz != 40 && hsz != 56 && hsz != 108 && hsz != 124) return stbi__errpuc("unknown BMP", "BMP type not supported: unknown");
   if (hsz == 12) {
      s->img_x = stbi__get16le(s);
      s->img_y = stbi__get16le(s);
   } else {
      s->img_x = stbi__get32le(s);
      s->img_y = stbi__get32le(s);
   }
   if (stbi__get16le(s) != 1) return stbi__errpuc("bad BMP", "bad BMP");
   info->bpp = stbi__get16le(s);
   if (hsz != 12) {
      int compress = stbi__get32le(s);
      if (compress == 1 || compress == 2) return stbi__errpuc("BMP RLE", "BMP type not supported: RLE");
      if (compress >= 4) return stbi__errpuc("BMP JPEG/PNG", "BMP type not supported: unsupported compression"); // this includes PNG/JPEG modes
      if (compress == 3 && info->bpp != 16 && info->bpp != 32) return stbi__errpuc("bad BMP", "bad BMP"); // bitfields requires 16 or 32 bits/pixel
      stbi__get32le(s); // discard sizeof
      stbi__get32le(s); // discard hres
      stbi__get32le(s); // discard vres
      stbi__get32le(s); // discard colorsused
      stbi__get32le(s); // discard max important
      if (hsz == 40 || hsz == 56) {
         if (hsz == 56) {
            stbi__get32le(s);
            stbi__get32le(s);
            stbi__get32le(s);
            stbi__get32le(s);
         }
         if (info->bpp == 16 || info->bpp == 32) {
            if (compress == 0) {
               stbi__bmp_set_mask_defaults(info, compress);
            } else if (compress == 3) {
               info->mr = stbi__get32le(s);
               info->mg = stbi__get32le(s);
               info->mb = stbi__get32le(s);
               info->extra_read += 12;
               // not documented, but generated by photoshop and handled by mspaint
               if (info->mr == info->mg && info->mg == info->mb) {
                  // ?!?!?
                  return stbi__errpuc("bad BMP", "bad BMP");
               }
            } else
               return stbi__errpuc("bad BMP", "bad BMP");
         }
      } else {
         // V4/V5 header
         int i;
         if (hsz != 108 && hsz != 124)
            return stbi__errpuc("bad BMP", "bad BMP");
         info->mr = stbi__get32le(s);
         info->mg = stbi__get32le(s);
         info->mb = stbi__get32le(s);
         info->ma = stbi__get32le(s);
         if (compress != 3) // override mr/mg/mb unless in BI_BITFIELDS mode, as per docs
            stbi__bmp_set_mask_defaults(info, compress);
         stbi__get32le(s); // discard color space
         for (i=0; i < 12; ++i)
            stbi__get32le(s); // discard color space parameters
         if (hsz == 124) {
            stbi__get32le(s); // discard rendering intent
            stbi__get32le(s); // discard offset of profile data
            stbi__get32le(s); // discard size of profile data
            stbi__get32le(s); // discard reserved
         }
      }
   }
   return (void *) 1;
}


static void *stbi__bmp_load(stbi__context *s, int *x, int *y, int *comp, int req_comp, stbi__result_info *ri)
{
   stbi_uc *out;
   unsigned int mr=0,mg=0,mb=0,ma=0, all_a;
   stbi_uc pal[256][4];
   int psize=0,i,j,width;
   int flip_vertically, pad, target;
   stbi__bmp_data info;
   STBI_NOTUSED(ri);

   info.all_a = 255;
   if (stbi__bmp_parse_header(s, &info) == NULL)
      return NULL; // error code already set

   flip_vertically = ((int) s->img_y) > 0;
   s->img_y = abs((int) s->img_y);

   if (s->img_y > STBI_MAX_DIMENSIONS) return stbi__errpuc("too large","Very large image (corrupt?)");
   if (s->img_x > STBI_MAX_DIMENSIONS) return stbi__errpuc("too large","Very large image (corrupt?)");

   mr = info.mr;
   mg = info.mg;
   mb = info.mb;
   ma = info.ma;
   all_a = info.all_a;

   if (info.hsz == 12) {
      if (info.bpp < 24)
         psize = (info.offset - info.extra_read - 24) / 3;
   } else {
      if (info.bpp < 16)
         psize = (info.offset - info.extra_read - info.hsz) >> 2;
   }
   if (psize == 0) {
      if (info.offset != s->callback_already_read + (s->img_buffer - s->img_buffer_original)) {
        return stbi__errpuc("bad offset", "Corrupt BMP");
      }
   }

   if (info.bpp == 24 && ma == 0xff000000)
      s->img_n = 3;
   else
      s->img_n = ma ? 4 : 3;
   if (req_comp && req_comp >= 3) // we can directly decode 3 or 4
      target = req_comp;
   else
      target = s->img_n; // if they want monochrome, we'll post-convert

   // sanity-check size
   if (!stbi__mad3sizes_valid(target, s->img_x, s->img_y, 0))
      return stbi__errpuc("too large", "Corrupt BMP");

   out = (stbi_uc *) stbi__malloc_mad3(target, s->img_x, s->img_y, 0);
   if (!out) return stbi__errpuc("outofmem", "Out of memory");
   if (info.bpp < 16) {
      int z=0;
      if (psize == 0 || psize > 256) { STBI_FREE(out); return stbi__errpuc("invalid", "Corrupt BMP"); }
      for (i=0; i < psize; ++i) {
         pal[i][2] = stbi__get8(s);
         pal[i][1] = stbi__get8(s);
         pal[i][0] = stbi__get8(s);
         if (info.hsz != 12) stbi__get8(s);
         pal[i][3] = 255;
      }
      stbi__skip(s, info.offset - info.extra_read - info.hsz - psize * (info.hsz == 12 ? 3 : 4));
      if (info.bpp == 1) width = (s->img_x + 7) >> 3;
      else if (info.bpp == 4) width = (s->img_x + 1) >> 1;
      else if (info.bpp == 8) width = s->img_x;
      else { STBI_FREE(out); return stbi__errpuc("bad bpp", "Corrupt BMP"); }
      pad = (-width)&3;
      if (info.bpp == 1) {
         for (j=0; j < (int) s->img_y; ++j) {
            int bit_offset = 7, v = stbi__get8(s);
            for (i=0; i < (int) s->img_x; ++i) {
               int color = (v>>bit_offset)&0x1;
               out[z++] = pal[color][0];
               out[z++] = pal[color][1];
               out[z++] = pal[color][2];
               if (target == 4) out[z++] = 255;
               if (i+1 == (int) s->img_x) break;
               if((--bit_offset) < 0) {
                  bit_offset = 7;
                  v = stbi__get8(s);
               }
            }
            stbi__skip(s, pad);
         }
      } else {
         for (j=0; j < (int) s->img_y; ++j) {
            for (i=0; i < (int) s->img_x; i += 2) {
               int v=stbi__get8(s),v2=0;
               if (info.bpp == 4) {
                  v2 = v & 15;
                  v >>= 4;
               }
               out[z++] = pal[v][0];
               out[z++] = pal[v][1];
               out[z++] = pal[v][2];
               if (target == 4) out[z++] = 255;
               if (i+1 == (int) s->img_x) break;
               v = (info.bpp == 8) ? stbi__get8(s) : v2;
               out[z++] = pal[v][0];
               out[z++] = pal[v][1];
               out[z++] = pal[v][2];
               if (target == 4) out[z++] = 255;
            }
            stbi__skip(s, pad);
         }
      }
   } else {
      int rshift=0,gshift=0,bshift=0,ashift=0,rcount=0,gcount=0,bcount=0,acount=0;
      int z = 0;
      int easy=0;
      stbi__skip(s, info.offset - info.extra_read - info.hsz);
      if (info.bpp == 24) width = 3 * s->img_x;
      else if (info.bpp == 16) width = 2*s->img_x;
      else /* bpp = 32 and pad = 0 */ width=0;
      pad = (-width) & 3;
      if (info.bpp == 24) {
         easy = 1;
      } else if (info.bpp == 32) {
         if (mb == 0xff && mg == 0xff00 && mr == 0x00ff0000 && ma == 0xff000000)
            easy = 2;
      }
      if (!easy) {
         if (!mr || !mg || !mb) { STBI_FREE(out); return stbi__errpuc("bad masks", "Corrupt BMP"); }
         // right shift amt to put high bit in position #7
         rshift = stbi__high_bit(mr)-7; rcount = stbi__bitcount(mr);
         gshift = stbi__high_bit(mg)-7; gcount = stbi__bitcount(mg);
         bshift = stbi__high_bit(mb)-7; bcount = stbi__bitcount(mb);
         ashift = stbi__high_bit(ma)-7; acount = stbi__bitcount(ma);
         if (rcount > 8 || gcount > 8 || bcount > 8 || acount > 8) { STBI_FREE(out); return stbi__errpuc("bad masks", "Corrupt BMP"); }
      }
      for (j=0; j < (int) s->img_y; ++j) {
         if (easy) {
            for (i=0; i < (int) s->img_x; ++i) {
               unsigned char a;
               out[z+2] = stbi__get8(s);
               out[z+1] = stbi__get8(s);
               out[z+0] = stbi__get8(s);
               z += 3;
               a = (easy == 2 ? stbi__get8(s) : 255);
               all_a |= a;
               if (target == 4) out[z++] = a;
            }
         } else {
            int bpp = info.bpp;
            for (i=0; i < (int) s->img_x; ++i) {
               stbi__uint32 v = (bpp == 16 ? (stbi__uint32) stbi__get16le(s) : stbi__get32le(s));
               unsigned int a;
               out[z++] = STBI__BYTECAST(stbi__shiftsigned(v & mr, rshift, rcount));
               out[z++] = STBI__BYTECAST(stbi__shiftsigned(v & mg, gshift, gcount));
               out[z++] = STBI__BYTECAST(stbi__shiftsigned(v & mb, bshift, bcount));
               a = (ma ? stbi__shiftsigned(v & ma, ashift, acount) : 255);
               all_a |= a;
               if (target == 4) out[z++] = STBI__BYTECAST(a);
            }
         }
         stbi__skip(s, pad);
      }
   }

   // if alpha channel is all 0s, replace with all 255s
   if (target == 4 && all_a == 0)
      for (i=4*s->img_x*s->img_y-1; i >= 0; i -= 4)
         out[i] = 255;

   if (flip_vertically) {
      stbi_uc t;
      for (j=0; j < (int) s->img_y>>1; ++j) {
         stbi_uc *p1 = out +      j     *s->img_x*target;
         stbi_uc *p2 = out + (s->img_y-1-j)*s->img_x*target;
         for (i=0; i < (int) s->img_x*target; ++i) {
            t = p1[i]; p1[i] = p2[i]; p2[i] = t;
         }
      }
   }

   if (req_comp && req_comp != target) {
      out = stbi__convert_format(out, target, req_comp, s->img_x, s->img_y);
      if (out == NULL) return out; // stbi__convert_format frees input on failure
   }

   *x = s->img_x;
   *y = s->img_y;
   if (comp) *comp = s->img_n;
   return out;
}
#endif

#ifndef STBI_NO_ICO
static int stbi__ico_test_raw(stbi__context *s)
{
   int bpp;
   if (stbi__get8(s) != 0) return 0;
   if (stbi__get8(s) != 0) return 0;
   if (stbi__get8(s) != 1) return 0;
   if (stbi__get8(s) != 0) return 0;
   if (stbi__get8(s) == 0) return 0; // accept any "number of images" from 1 to 255,
   if (stbi__get8(s) != 0) return 0; // so the MSB needs to be 0.
   stbi__get8(s); // discard width
   stbi__get8(s); // discard height
   stbi__get8(s); // discard number of pallete entries
   if (stbi__get8(s) != 0) return 0;
   if (stbi__get8(s) > 1) return 0;
   if (stbi__get8(s) != 0) return 0;
   bpp = stbi__get8(s); // accept only common bit depths
   if (bpp != 1 && bpp != 2 && bpp != 4 && bpp != 8 && bpp != 16 && bpp != 24 && bpp != 32) return 0;
   if (stbi__get8(s) != 0) return 0;
   return 1;
}

static int stbi__ico_test(stbi__context *s)
{
   int r = stbi__ico_test_raw(s);
   stbi__rewind(s);
   return r;
}

static void *stbi__ico_load(stbi__context *s, int *x, int *y, int *comp, int req_comp, stbi__result_info *ri)
{
   int n, planes, bpp, offset, hsz;
   n = stbi__get32le(s);
   if (n == 0x00020000) return stbi__errpuc("not ICO", "Unsupported CURsor file");
   if (n != 0x00010000) return stbi__errpuc("not ICO", "Corrupt ICO");
   n = stbi__get16le(s);
   if (n < 0 || n > 255) return stbi__errpuc("not ICO", "Corrupt ICO");
   s->img_x = stbi__get8(s);
   s->img_y = stbi__get8(s);
   if (!s->img_x) s->img_x = 256;
   if (!s->img_y) s->img_y = 256;
   n = stbi__get16le(s); // number of pallete entries
   if (n == 1) return stbi__errpuc("not ICO", "Corrupt ICO");
   planes = stbi__get16le(s);
   if (planes > 1) return stbi__errpuc("not ICO", "Corrupt ICO");
   bpp = stbi__get16le(s);
   if (bpp != 1 && bpp != 2 && bpp != 4 && bpp != 8 && bpp != 16 && bpp != 24 && bpp != 32)
       return stbi__errpuc("not ICO", "Corrupt ICO");
   stbi__get32le(s); // discard size
   offset = stbi__get32le(s);
   stbi__skip(s, offset - 22);
   hsz = stbi__get32le(s);

   if (hsz != 0x474e5089) {
     stbi__bmp_data info;
     info.all_a = 255;
     info.offset = n * 4 + 14 + hsz;
     info.hsz = hsz;
     info.mr = info.mg = info.mb = info.ma = 0;
     int true_y = s->img_y; // save before parse head
     if (stbi__bmp_parse_header_cont(s, &info) == NULL)
       return NULL; // error code already set

     s->img_y = true_y;
     return stbi__bmp_load_cont(s, x, y, comp, req_comp, &info, ri);
   }
#ifndef STBI_NO_PNG
   // Rewind and go to PNG start again
   stbi__rewind(s);
   stbi__skip(s, offset);
   return stbi__png_load(s, x, y, comp, req_comp, ri);
#else
   return stbi__errpuc("PNG", "PNG support not compiled");
#endif
}

#endif

#ifndef STBI_NO_ICO
static int stbi__ico_info(stbi__context *s, int *x, int *y, int *comp)
{
   unsigned r, n;
   r = stbi__get32le(s);
   n = stbi__get16le(s);
   if (n != 0x00010000 || n < 1 || n > 255 ) {
       stbi__rewind( s );
       return 0;
   }
   *x = stbi__get8(s);
   *y = stbi__get8(s);
   if (!*x) *x = 256;
   if (!*y) *y = 256;
   r = stbi__get8(s);
   n = stbi__get16le(s);
   if (r != 0 || n > 1 ) {
       stbi__rewind( s );
       return 0;
   }
   *comp = stbi__get8(s) / 8;
   return 1;
}
#endif

// Targa Truevision - TGA
// by Jonathan Dummer
#ifndef STBI_NO_TGA
// returns STBI_rgb or whatever, 0 on error
static int stbi__tga_get_comp(int bits_per_pixel, int is_grey, int* is_rgb16)
{
   // only RGB or RGBA (incl. 16bit) or grey allowed
   if (is_rgb16) *is_rgb16 = 0;
   switch(bits_per_pixel) {
      case 8:  return STBI_grey;
      case 16: if(is_grey) return STBI_grey_alpha;
               // fallthrough
      case 15: if(is_rgb16) *is_rgb16 = 1;
               return STBI_rgb;
      case 24: // fallthrough
      case 32: return bits_per_pixel/8;
      default: return 0;
   }
}

static int stbi__tga_info(stbi__context *s, int *x, int *y, int *comp)
{
    int tga_w, tga_h, tga_comp, tga_image_type, tga_bits_per_pixel, tga_colormap_bpp;
    int sz, tga_colormap_type;
    stbi__get8(s);                   // discard Offset
    tga_colormap_type = stbi__get8(s); // colormap type
    if( tga_colormap_type > 1 ) {
        stbi__rewind(s);
        return 0;      // only RGB or indexed allowed
    }
    tga_image_type = stbi__get8(s); // image type
    if ( tga_colormap_type == 1 ) { // colormapped (paletted) image
        if (tga_image_type != 1 && tga_image_type != 9) {
            stbi__rewind(s);
            return 0;
        }
        stbi__skip(s,4);       // skip index of first colormap entry and number of entries
        sz = stbi__get8(s);    //   check bits per palette color entry
        if ( (sz != 8) && (sz != 15) && (sz != 16) && (sz != 24) && (sz != 32) ) {
            stbi__rewind(s);
            return 0;
        }
        stbi__skip(s,4);       // skip image x and y origin
        tga_colormap_bpp = sz;
    } else { // "normal" image w/o colormap - only RGB or grey allowed, +/- RLE
        if ( (tga_image_type != 2) && (tga_image_type != 3) && (tga_image_type != 10) && (tga_image_type != 11) ) {
            stbi__rewind(s);
            return 0; // only RGB or grey allowed, +/- RLE
        }
        stbi__skip(s,9); // skip colormap specification and image x/y origin
        tga_colormap_bpp = 0;
    }
    tga_w = stbi__get16le(s);
    if( tga_w < 1 ) {
        stbi__rewind(s);
        return 0;   // test width
    }
    tga_h = stbi__get16le(s);
    if( tga_h < 1 ) {
        stbi__rewind(s);
        return 0;   // test height
    }
    tga_bits_per_pixel = stbi__get8(s); // bits per pixel
    stbi__get8(s); // ignore alpha bits
    if (tga_colormap_bpp != 0) {
        if((tga_bits_per_pixel != 8) && (tga_bits_per_pixel != 16)) {
            // when using a colormap, tga_bits_per_pixel is the size of the indexes
            // I don't think anything but 8 or 16bit indexes makes sense
            stbi__rewind(s);
            return 0;
        }
        tga_comp = stbi__tga_get_comp(tga_colormap_bpp, 0, NULL);
    } else {
        tga_comp = stbi__tga_get_comp(tga_bits_per_pixel, (tga_image_type == 3) || (tga_image_type == 11), NULL);
    }
    if(!tga_comp) {
      stbi__rewind(s);
      return 0;
    }
    if (x) *x = tga_w;
    if (y) *y = tga_h;
    if (comp) *comp = tga_comp;
    return 1;                   // seems to have passed everything
}

static int stbi__tga_test(stbi__context *s)
{
   int res = 0;
   int sz, tga_color_type;
   stbi__get8(s);      //   discard Offset
   tga_color_type = stbi__get8(s);   //   color type
   if ( tga_color_type > 1 ) goto errorEnd;   //   only RGB or indexed allowed
   sz = stbi__get8(s);   //   image type
   if ( tga_color_type == 1 ) { // colormapped (paletted) image
      if (sz != 1 && sz != 9) goto errorEnd; // colortype 1 demands image type 1 or 9
      stbi__skip(s,4);       // skip index of first colormap entry and number of entries
      sz = stbi__get8(s);    //   check bits per palette color entry
      if ( (sz != 8) && (sz != 15) && (sz != 16) && (sz != 24) && (sz != 32) ) goto errorEnd;
      stbi__skip(s,4);       // skip image x and y origin
   } else { // "normal" image w/o colormap
      if ( (sz != 2) && (sz != 3) && (sz != 10) && (sz != 11) ) goto errorEnd; // only RGB or grey allowed, +/- RLE
      stbi__skip(s,9); // skip colormap specification and image x/y origin
   }
   if ( stbi__get16le(s) < 1 ) goto errorEnd;      //   test width
   if ( stbi__get16le(s) < 1 ) goto errorEnd;      //   test height
   sz = stbi__get8(s);   //   bits per pixel
   if ( (tga_color_type == 1) && (sz != 8) && (sz != 16) ) goto errorEnd; // for colormapped images, bpp is size of an index
   if ( (sz != 8) && (sz != 15) && (sz != 16) && (sz != 24) && (sz != 32) ) goto errorEnd;

   res = 1; // if we got this far, everything's good and we can return 1 instead of 0

errorEnd:
   stbi__rewind(s);
   return res;
}

// read 16bit value and convert to 24bit RGB
static void stbi__tga_read_rgb16(stbi__context *s, stbi_uc* out)
{
   stbi__uint16 px = (stbi__uint16)stbi__get16le(s);
   stbi__uint16 fiveBitMask = 31;
   // we have 3 channels with 5bits each
   int r = (px >> 10) & fiveBitMask;
   int g = (px >> 5) & fiveBitMask;
   int b = px & fiveBitMask;
   // Note that this saves the data in RGB(A) order, so it doesn't need to be swapped later
   out[0] = (stbi_uc)((r * 255)/31);
   out[1] = (stbi_uc)((g * 255)/31);
   out[2] = (stbi_uc)((b * 255)/31);

   // some people claim that the most significant bit might be used for alpha
   // (possibly if an alpha-bit is set in the "image descriptor byte")
   // but that only made 16bit test images completely translucent..
   // so let's treat all 15 and 16bit TGAs as RGB with no alpha.
}

static void *stbi__tga_load(stbi__context *s, int *x, int *y, int *comp, int req_comp, stbi__result_info *ri)
{
   //   read in the TGA header stuff
   int tga_offset = stbi__get8(s);
   int tga_indexed = stbi__get8(s);
   int tga_image_type = stbi__get8(s);
   int tga_is_RLE = 0;
   int tga_palette_start = stbi__get16le(s);
   int tga_palette_len = stbi__get16le(s);
   int tga_palette_bits = stbi__get8(s);
   int tga_x_origin = stbi__get16le(s);
   int tga_y_origin = stbi__get16le(s);
   int tga_width = stbi__get16le(s);
   int tga_height = stbi__get16le(s);
   int tga_bits_per_pixel = stbi__get8(s);
   int tga_comp, tga_rgb16=0;
   int tga_inverted = stbi__get8(s);
   // int tga_alpha_bits = tga_inverted & 15; // the 4 lowest bits - unused (useless?)
   //   image data
   unsigned char *tga_data;
   unsigned char *tga_palette = NULL;
   int i, j;
   unsigned char raw_data[4] = {0};
   int RLE_count = 0;
   int RLE_repeating = 0;
   int read_next_pixel = 1;
   STBI_NOTUSED(ri);
   STBI_NOTUSED(tga_x_origin); // @TODO
   STBI_NOTUSED(tga_y_origin); // @TODO

   if (tga_height > STBI_MAX_DIMENSIONS) return stbi__errpuc("too large","Very large image (corrupt?)");
   if (tga_width > STBI_MAX_DIMENSIONS) return stbi__errpuc("too large","Very large image (corrupt?)");

   //   do a tiny bit of precessing
   if ( tga_image_type >= 8 )
   {
      tga_image_type -= 8;
      tga_is_RLE = 1;
   }
   tga_inverted = 1 - ((tga_inverted >> 5) & 1);

   //   If I'm paletted, then I'll use the number of bits from the palette
   if ( tga_indexed ) tga_comp = stbi__tga_get_comp(tga_palette_bits, 0, &tga_rgb16);
   else tga_comp = stbi__tga_get_comp(tga_bits_per_pixel, (tga_image_type == 3), &tga_rgb16);

   if(!tga_comp) // shouldn't really happen, stbi__tga_test() should have ensured basic consistency
      return stbi__errpuc("bad format", "Can't find out TGA pixelformat");

   //   tga info
   *x = tga_width;
   *y = tga_height;
   if (comp) *comp = tga_comp;

   if (!stbi__mad3sizes_valid(tga_width, tga_height, tga_comp, 0))
      return stbi__errpuc("too large", "Corrupt TGA");

   tga_data = (unsigned char*)stbi__malloc_mad3(tga_width, tga_height, tga_comp, 0);
   if (!tga_data) return stbi__errpuc("outofmem", "Out of memory");

   // skip to the data's starting position (offset usually = 0)
   stbi__skip(s, tga_offset );

   if ( !tga_indexed && !tga_is_RLE && !tga_rgb16 ) {
      for (i=0; i < tga_height; ++i) {
         int row = tga_inverted ? tga_height -i - 1 : i;
         stbi_uc *tga_row = tga_data + row*tga_width*tga_comp;
         stbi__getn(s, tga_row, tga_width * tga_comp);
      }
   } else  {
      //   do I need to load a palette?
      if ( tga_indexed)
      {
         if (tga_palette_len == 0) {  /* you have to have at least one entry! */
            STBI_FREE(tga_data);
            return stbi__errpuc("bad palette", "Corrupt TGA");
         }

         //   any data to skip? (offset usually = 0)
         stbi__skip(s, tga_palette_start );
         //   load the palette
         tga_palette = (unsigned char*)stbi__malloc_mad2(tga_palette_len, tga_comp, 0);
         if (!tga_palette) {
            STBI_FREE(tga_data);
            return stbi__errpuc("outofmem", "Out of memory");
         }
         if (tga_rgb16) {
            stbi_uc *pal_entry = tga_palette;
            STBI_ASSERT(tga_comp == STBI_rgb);
            for (i=0; i < tga_palette_len; ++i) {
               stbi__tga_read_rgb16(s, pal_entry);
               pal_entry += tga_comp;
            }
         } else if (!stbi__getn(s, tga_palette, tga_palette_len * tga_comp)) {
               STBI_FREE(tga_data);
               STBI_FREE(tga_palette);
               return stbi__errpuc("bad palette", "Corrupt TGA");
         }
      }
      //   load the data
      for (i=0; i < tga_width * tga_height; ++i)
      {
         //   if I'm in RLE mode, do I need to get a RLE stbi__pngchunk?
         if ( tga_is_RLE )
         {
            if ( RLE_count == 0 )
            {
               //   yep, get the next byte as a RLE command
               int RLE_cmd = stbi__get8(s);
               RLE_count = 1 + (RLE_cmd & 127);
               RLE_repeating = RLE_cmd >> 7;
               read_next_pixel = 1;
            } else if ( !RLE_repeating )
            {
               read_next_pixel = 1;
            }
         } else
         {
            read_next_pixel = 1;
         }
         //   OK, if I need to read a pixel, do it now
         if ( read_next_pixel )
         {
            //   load however much data we did have
            if ( tga_indexed )
            {
               // read in index, then perform the lookup
               int pal_idx = (tga_bits_per_pixel == 8) ? stbi__get8(s) : stbi__get16le(s);
               if ( pal_idx >= tga_palette_len ) {
                  // invalid index
                  pal_idx = 0;
               }
               pal_idx *= tga_comp;
               for (j = 0; j < tga_comp; ++j) {
                  raw_data[j] = tga_palette[pal_idx+j];
               }
            } else if(tga_rgb16) {
               STBI_ASSERT(tga_comp == STBI_rgb);
               stbi__tga_read_rgb16(s, raw_data);
            } else {
               //   read in the data raw
               for (j = 0; j < tga_comp; ++j) {
                  raw_data[j] = stbi__get8(s);
               }
            }
            //   clear the reading flag for the next pixel
            read_next_pixel = 0;
         } // end of reading a pixel

         // copy data
         for (j = 0; j < tga_comp; ++j)
           tga_data[i*tga_comp+j] = raw_data[j];

         //   in case we're in RLE mode, keep counting down
         --RLE_count;
      }
      //   do I need to invert the image?
      if ( tga_inverted )
      {
         for (j = 0; j*2 < tga_height; ++j)
         {
            int index1 = j * tga_width * tga_comp;
            int index2 = (tga_height - 1 - j) * tga_width * tga_comp;
            for (i = tga_width * tga_comp; i > 0; --i)
            {
               unsigned char temp = tga_data[index1];
               tga_data[index1] = tga_data[index2];
               tga_data[index2] = temp;
               ++index1;
               ++index2;
            }
         }
      }
      //   clear my palette, if I had one
      if ( tga_palette != NULL )
      {
         STBI_FREE( tga_palette );
      }
   }

   // swap RGB - if the source data was RGB16, it already is in the right order
   if (tga_comp >= 3 && !tga_rgb16)
   {
      unsigned char* tga_pixel = tga_data;
      for (i=0; i < tga_width * tga_height; ++i)
      {
         unsigned char temp = tga_pixel[0];
         tga_pixel[0] = tga_pixel[2];
         tga_pixel[2] = temp;
         tga_pixel += tga_comp;
      }
   }

   // convert to target component count
   if (req_comp && req_comp != tga_comp)
      tga_data = stbi__convert_format(tga_data, tga_comp, req_comp, tga_width, tga_height);

   //   the things I do to get rid of an error message, and yet keep
   //   Microsoft's C compilers happy... [8^(
   tga_palette_start = tga_palette_len = tga_palette_bits =
         tga_x_origin = tga_y_origin = 0;
   STBI_NOTUSED(tga_palette_start);
   //   OK, done
   return tga_data;
}
#endif

// *************************************************************************************************
// Photoshop PSD loader -- PD by Thatcher Ulrich, integration by Nicolas Schulz, tweaked by STB

#ifndef STBI_NO_PSD
static int stbi__psd_test(stbi__context *s)
{
   int r = (stbi__get32be(s) == 0x38425053);
   stbi__rewind(s);
   return r;
}

static int stbi__psd_decode_rle(stbi__context *s, stbi_uc *p, int pixelCount)
{
   int count, nleft, len;

   count = 0;
   while ((nleft = pixelCount - count) > 0) {
      len = stbi__get8(s);
      if (len == 128) {
         // No-op.
      } else if (len < 128) {
         // Copy next len+1 bytes literally.
         len++;
         if (len > nleft) return 0; // corrupt data
         count += len;
         while (len) {
            *p = stbi__get8(s);
            p += 4;
            len--;
         }
      } else if (len > 128) {
         stbi_uc   val;
         // Next -len+1 bytes in the dest are replicated from next source byte.
         // (Interpret len as a negative 8-bit int.)
         len = 257 - len;
         if (len > nleft) return 0; // corrupt data
         val = stbi__get8(s);
         count += len;
         while (len) {
            *p = val;
            p += 4;
            len--;
         }
      }
   }

   return 1;
}

static void *stbi__psd_load(stbi__context *s, int *x, int *y, int *comp, int req_comp, stbi__result_info *ri, int bpc)
{
   int pixelCount;
   int channelCount, compression;
   int channel, i;
   int bitdepth;
   int w,h;
   stbi_uc *out;
   STBI_NOTUSED(ri);

   // Check identifier
   if (stbi__get32be(s) != 0x38425053)   // "8BPS"
      return stbi__errpuc("not PSD", "Corrupt PSD image");

   // Check file type version.
   if (stbi__get16be(s) != 1)
      return stbi__errpuc("wrong version", "Unsupported version of PSD image");

   // Skip 6 reserved bytes.
   stbi__skip(s, 6 );

   // Read the number of channels (R, G, B, A, etc).
   channelCount = stbi__get16be(s);
   if (channelCount < 0 || channelCount > 16)
      return stbi__errpuc("wrong channel count", "Unsupported number of channels in PSD image");

   // Read the rows and columns of the image.
   h = stbi__get32be(s);
   w = stbi__get32be(s);

   if (h > STBI_MAX_DIMENSIONS) return stbi__errpuc("too large","Very large image (corrupt?)");
   if (w > STBI_MAX_DIMENSIONS) return stbi__errpuc("too large","Very large image (corrupt?)");

   // Make sure the depth is 8 bits.
   bitdepth = stbi__get16be(s);
   if (bitdepth != 8 && bitdepth != 16)
      return stbi__errpuc("unsupported bit depth", "PSD bit depth is not 8 or 16 bit");

   // Make sure the color mode is RGB.
   // Valid options are:
   //   0: Bitmap
   //   1: Grayscale
   //   2: Indexed color
   //   3: RGB color
   //   4: CMYK color
   //   7: Multichannel
   //   8: Duotone
   //   9: Lab color
   if (stbi__get16be(s) != 3)
      return stbi__errpuc("wrong color format", "PSD is not in RGB color format");

   // Skip the Mode Data.  (It's the palette for indexed color; other info for other modes.)
   stbi__skip(s,stbi__get32be(s) );

   // Skip the image resources.  (resolution, pen tool paths, etc)
   stbi__skip(s, stbi__get32be(s) );

   // Skip the reserved data.
   stbi__skip(s, stbi__get32be(s) );

   // Find out if the data is compressed.
   // Known values:
   //   0: no compression
   //   1: RLE compressed
   compression = stbi__get16be(s);
   if (compression > 1)
      return stbi__errpuc("bad compression", "PSD has an unknown compression format");

   // Check size
   if (!stbi__mad3sizes_valid(4, w, h, 0))
      return stbi__errpuc("too large", "Corrupt PSD");

   // Create the destination image.

   if (!compression && bitdepth == 16 && bpc == 16) {
      out = (stbi_uc *) stbi__malloc_mad3(8, w, h, 0);
      ri->bits_per_channel = 16;
   } else
      out = (stbi_uc *) stbi__malloc(4 * w*h);

   if (!out) return stbi__errpuc("outofmem", "Out of memory");
   pixelCount = w*h;

   // Initialize the data to zero.
   //memset( out, 0, pixelCount * 4 );

   // Finally, the image data.
   if (compression) {
      // RLE as used by .PSD and .TIFF
      // Loop until you get the number of unpacked bytes you are expecting:
      //     Read the next source byte into n.
      //     If n is between 0 and 127 inclusive, copy the next n+1 bytes literally.
      //     Else if n is between -127 and -1 inclusive, copy the next byte -n+1 times.
      //     Else if n is 128, noop.
      // Endloop

      // The RLE-compressed data is preceded by a 2-byte data count for each row in the data,
      // which we're going to just skip.
      stbi__skip(s, h * channelCount * 2 );

      // Read the RLE data by channel.
      for (channel = 0; channel < 4; channel++) {
         stbi_uc *p;

         p = out+channel;
         if (channel >= channelCount) {
            // Fill this channel with default data.
            for (i = 0; i < pixelCount; i++, p += 4)
               *p = (channel == 3 ? 255 : 0);
         } else {
            // Read the RLE data.
            if (!stbi__psd_decode_rle(s, p, pixelCount)) {
               STBI_FREE(out);
               return stbi__errpuc("corrupt", "bad RLE data");
            }
         }
      }

   } else {
      // We're at the raw image data.  It's each channel in order (Red, Green, Blue, Alpha, ...)
      // where each channel consists of an 8-bit (or 16-bit) value for each pixel in the image.

      // Read the data by channel.
      for (channel = 0; channel < 4; channel++) {
         if (channel >= channelCount) {
            // Fill this channel with default data.
            if (bitdepth == 16 && bpc == 16) {
               stbi__uint16 *q = ((stbi__uint16 *) out) + channel;
               stbi__uint16 val = channel == 3 ? 65535 : 0;
               for (i = 0; i < pixelCount; i++, q += 4)
                  *q = val;
            } else {
               stbi_uc *p = out+channel;
               stbi_uc val = channel == 3 ? 255 : 0;
               for (i = 0; i < pixelCount; i++, p += 4)
                  *p = val;
            }
         } else {
            if (ri->bits_per_channel == 16) {    // output bpc
               stbi__uint16 *q = ((stbi__uint16 *) out) + channel;
               for (i = 0; i < pixelCount; i++, q += 4)
                  *q = (stbi__uint16) stbi__get16be(s);
            } else {
               stbi_uc *p = out+channel;
               if (bitdepth == 16) {  // input bpc
                  for (i = 0; i < pixelCount; i++, p += 4)
                     *p = (stbi_uc) (stbi__get16be(s) >> 8);
               } else {
                  for (i = 0; i < pixelCount; i++, p += 4)
                     *p = stbi__get8(s);
               }
            }
         }
      }
   }

   // remove weird white matte from PSD
   if (channelCount >= 4) {
      if (ri->bits_per_channel == 16) {
         for (i=0; i < w*h; ++i) {
            stbi__uint16 *pixel = (stbi__uint16 *) out + 4*i;
            if (pixel[3] != 0 && pixel[3] != 65535) {
               float a = pixel[3] / 65535.0f;
               float ra = 1.0f / a;
               float inv_a = 65535.0f * (1 - ra);
               pixel[0] = (stbi__uint16) (pixel[0]*ra + inv_a);
               pixel[1] = (stbi__uint16) (pixel[1]*ra + inv_a);
               pixel[2] = (stbi__uint16) (pixel[2]*ra + inv_a);
            }
         }
      } else {
         for (i=0; i < w*h; ++i) {
            unsigned char *pixel = out + 4*i;
            if (pixel[3] != 0 && pixel[3] != 255) {
               float a = pixel[3] / 255.0f;
               float ra = 1.0f / a;
               float inv_a = 255.0f * (1 - ra);
               pixel[0] = (unsigned char) (pixel[0]*ra + inv_a);
               pixel[1] = (unsigned char) (pixel[1]*ra + inv_a);
               pixel[2] = (unsigned char) (pixel[2]*ra + inv_a);
            }
         }
      }
   }

   // convert to desired output format
   if (req_comp && req_comp != 4) {
      if (ri->bits_per_channel == 16)
         out = (stbi_uc *) stbi__convert_format16((stbi__uint16 *) out, 4, req_comp, w, h);
      else
         out = stbi__convert_format(out, 4, req_comp, w, h);
      if (out == NULL) return out; // stbi__convert_format frees input on failure
   }

   if (comp) *comp = 4;
   *y = h;
   *x = w;

   return out;
}
#endif

// *************************************************************************************************
// Softimage PIC loader
// by Tom Seddon
//
// See http://softimage.wiki.softimage.com/index.php/INFO:_PIC_file_format
// See http://ozviz.wasp.uwa.edu.au/~pbourke/dataformats/softimagepic/

#ifndef STBI_NO_PIC
static int stbi__pic_is4(stbi__context *s,const char *str)
{
   int i;
   for (i=0; i<4; ++i)
      if (stbi__get8(s) != (stbi_uc)str[i])
         return 0;

   return 1;
}

static int stbi__pic_test_core(stbi__context *s)
{
   int i;

   if (!stbi__pic_is4(s,"\x53\x80\xF6\x34"))
      return 0;

   for(i=0;i<84;++i)
      stbi__get8(s);

   if (!stbi__pic_is4(s,"PICT"))
      return 0;

   return 1;
}

typedef struct
{
   stbi_uc size,type,channel;
} stbi__pic_packet;

static stbi_uc *stbi__readval(stbi__context *s, int channel, stbi_uc *dest)
{
   int mask=0x80, i;

   for (i=0; i<4; ++i, mask>>=1) {
      if (channel & mask) {
         if (stbi__at_eof(s)) return stbi__errpuc("bad file","PIC file too short");
         dest[i]=stbi__get8(s);
      }
   }

   return dest;
}

static void stbi__copyval(int channel,stbi_uc *dest,const stbi_uc *src)
{
   int mask=0x80,i;

   for (i=0;i<4; ++i, mask>>=1)
      if (channel&mask)
         dest[i]=src[i];
}

static stbi_uc *stbi__pic_load_core(stbi__context *s,int width,int height,int *comp, stbi_uc *result)
{
   int act_comp=0,num_packets=0,y,chained;
   stbi__pic_packet packets[10];

   // this will (should...) cater for even some bizarre stuff like having data
    // for the same channel in multiple packets.
   do {
      stbi__pic_packet *packet;

      if (num_packets==sizeof(packets)/sizeof(packets[0]))
         return stbi__errpuc("bad format","too many packets");

      packet = &packets[num_packets++];

      chained = stbi__get8(s);
      packet->size    = stbi__get8(s);
      packet->type    = stbi__get8(s);
      packet->channel = stbi__get8(s);

      act_comp |= packet->channel;

      if (stbi__at_eof(s))          return stbi__errpuc("bad file","file too short (reading packets)");
      if (packet->size != 8)  return stbi__errpuc("bad format","packet isn't 8bpp");
   } while (chained);

   *comp = (act_comp & 0x10 ? 4 : 3); // has alpha channel?

   for(y=0; y<height; ++y) {
      int packet_idx;

      for(packet_idx=0; packet_idx < num_packets; ++packet_idx) {
         stbi__pic_packet *packet = &packets[packet_idx];
         stbi_uc *dest = result+y*width*4;

         switch (packet->type) {
            default:
               return stbi__errpuc("bad format","packet has bad compression type");

            case 0: {//uncompressed
               int x;

               for(x=0;x<width;++x, dest+=4)
                  if (!stbi__readval(s,packet->channel,dest))
                     return 0;
               break;
            }

            case 1://Pure RLE
               {
                  int left=width, i;

                  while (left>0) {
                     stbi_uc count,value[4];

                     count=stbi__get8(s);
                     if (stbi__at_eof(s))   return stbi__errpuc("bad file","file too short (pure read count)");

                     if (count > left)
                        count = (stbi_uc) left;

                     if (!stbi__readval(s,packet->channel,value))  return 0;

                     for(i=0; i<count; ++i,dest+=4)
                        stbi__copyval(packet->channel,dest,value);
                     left -= count;
                  }
               }
               break;

            case 2: {//Mixed RLE
               int left=width;
               while (left>0) {
                  int count = stbi__get8(s), i;
                  if (stbi__at_eof(s))  return stbi__errpuc("bad file","file too short (mixed read count)");

                  if (count >= 128) { // Repeated
                     stbi_uc value[4];

                     if (count==128)
                        count = stbi__get16be(s);
                     else
                        count -= 127;
                     if (count > left)
                        return stbi__errpuc("bad file","scanline overrun");

                     if (!stbi__readval(s,packet->channel,value))
                        return 0;

                     for(i=0;i<count;++i, dest += 4)
                        stbi__copyval(packet->channel,dest,value);
                  } else { // Raw
                     ++count;
                     if (count>left) return stbi__errpuc("bad file","scanline overrun");

                     for(i=0;i<count;++i, dest+=4)
                        if (!stbi__readval(s,packet->channel,dest))
                           return 0;
                  }
                  left-=count;
               }
               break;
            }
         }
      }
   }

   return result;
}

static void *stbi__pic_load(stbi__context *s,int *px,int *py,int *comp,int req_comp, stbi__result_info *ri)
{
   stbi_uc *result;
   int i, x,y, internal_comp;
   STBI_NOTUSED(ri);

   if (!comp) comp = &internal_comp;

   for (i=0; i<92; ++i)
      stbi__get8(s);

   x = stbi__get16be(s);
   y = stbi__get16be(s);

   if (y > STBI_MAX_DIMENSIONS) return stbi__errpuc("too large","Very large image (corrupt?)");
   if (x > STBI_MAX_DIMENSIONS) return stbi__errpuc("too large","Very large image (corrupt?)");

   if (stbi__at_eof(s))  return stbi__errpuc("bad file","file too short (pic header)");
   if (!stbi__mad3sizes_valid(x, y, 4, 0)) return stbi__errpuc("too large", "PIC image too large to decode");

   stbi__get32be(s); //skip `ratio'
   stbi__get16be(s); //skip `fields'
   stbi__get16be(s); //skip `pad'

   // intermediate buffer is RGBA
   result = (stbi_uc *) stbi__malloc_mad3(x, y, 4, 0);
   if (!result) return stbi__errpuc("outofmem", "Out of memory");
   memset(result, 0xff, x*y*4);

   if (!stbi__pic_load_core(s,x,y,comp, result)) {
      STBI_FREE(result);
      result=0;
   }
   *px = x;
   *py = y;
   if (req_comp == 0) req_comp = *comp;
   result=stbi__convert_format(result,4,req_comp,x,y);

   return result;
}

static int stbi__pic_test(stbi__context *s)
{
   int r = stbi__pic_test_core(s);
   stbi__rewind(s);
   return r;
}
#endif

// *************************************************************************************************
// GIF loader -- public domain by Jean-Marc Lienher -- simplified/shrunk by stb

#ifndef STBI_NO_GIF
typedef struct
{
   stbi__int16 prefix;
   stbi_uc first;
   stbi_uc suffix;
} stbi__gif_lzw;

typedef struct
{
   int w,h;
   stbi_uc *out;                 // output buffer (always 4 components)
   stbi_uc *background;          // The current "background" as far as a gif is concerned
   stbi_uc *history;
   int flags, bgindex, ratio, transparent, eflags;
   stbi_uc  pal[256][4];
   stbi_uc lpal[256][4];
   stbi__gif_lzw codes[8192];
   stbi_uc *color_table;
   int parse, step;
   int lflags;
   int start_x, start_y;
   int max_x, max_y;
   int cur_x, cur_y;
   int line_size;
   int delay;
} stbi__gif;

static int stbi__gif_test_raw(stbi__context *s)
{
   int sz;
   if (stbi__get8(s) != 'G' || stbi__get8(s) != 'I' || stbi__get8(s) != 'F' || stbi__get8(s) != '8') return 0;
   sz = stbi__get8(s);
   if (sz != '9' && sz != '7') return 0;
   if (stbi__get8(s) != 'a') return 0;
   return 1;
}

static int stbi__gif_test(stbi__context *s)
{
   int r = stbi__gif_test_raw(s);
   stbi__rewind(s);
   return r;
}

static void stbi__gif_parse_colortable(stbi__context *s, stbi_uc pal[256][4], int num_entries, int transp)
{
   int i;
   for (i=0; i < num_entries; ++i) {
      pal[i][2] = stbi__get8(s);
      pal[i][1] = stbi__get8(s);
      pal[i][0] = stbi__get8(s);
      pal[i][3] = transp == i ? 0 : 255;
   }
}

static int stbi__gif_header(stbi__context *s, stbi__gif *g, int *comp, int is_info)
{
   stbi_uc version;
   if (stbi__get8(s) != 'G' || stbi__get8(s) != 'I' || stbi__get8(s) != 'F' || stbi__get8(s) != '8')
      return stbi__err("not GIF", "Corrupt GIF");

   version = stbi__get8(s);
   if (version != '7' && version != '9')    return stbi__err("not GIF", "Corrupt GIF");
   if (stbi__get8(s) != 'a')                return stbi__err("not GIF", "Corrupt GIF");

   stbi__g_failure_reason = "";
   g->w = stbi__get16le(s);
   g->h = stbi__get16le(s);
   g->flags = stbi__get8(s);
   g->bgindex = stbi__get8(s);
   g->ratio = stbi__get8(s);
   g->transparent = -1;

   if (g->w > STBI_MAX_DIMENSIONS) return stbi__err("too large","Very large image (corrupt?)");
   if (g->h > STBI_MAX_DIMENSIONS) return stbi__err("too large","Very large image (corrupt?)");

   if (comp != 0) *comp = 4;  // can't actually tell whether it's 3 or 4 until we parse the comments

   if (is_info) return 1;

   if (g->flags & 0x80)
      stbi__gif_parse_colortable(s,g->pal, 2 << (g->flags & 7), -1);

   return 1;
}

static int stbi__gif_info_raw(stbi__context *s, int *x, int *y, int *comp)
{
   stbi__gif* g = (stbi__gif*) stbi__malloc(sizeof(stbi__gif));
   if (!g) return stbi__err("outofmem", "Out of memory");
   if (!stbi__gif_header(s, g, comp, 1)) {
      STBI_FREE(g);
      stbi__rewind( s );
      return 0;
   }
   if (x) *x = g->w;
   if (y) *y = g->h;
   STBI_FREE(g);
   return 1;
}

static void stbi__out_gif_code(stbi__gif *g, stbi__uint16 code)
{
   stbi_uc *p, *c;
   int idx;

   // recurse to decode the prefixes, since the linked-list is backwards,
   // and working backwards through an interleaved image would be nasty
   if (g->codes[code].prefix >= 0)
      stbi__out_gif_code(g, g->codes[code].prefix);

   if (g->cur_y >= g->max_y) return;

   idx = g->cur_x + g->cur_y;
   p = &g->out[idx];
   g->history[idx / 4] = 1;

   c = &g->color_table[g->codes[code].suffix * 4];
   if (c[3] > 128) { // don't render transparent pixels;
      p[0] = c[2];
      p[1] = c[1];
      p[2] = c[0];
      p[3] = c[3];
   }
   g->cur_x += 4;

   if (g->cur_x >= g->max_x) {
      g->cur_x = g->start_x;
      g->cur_y += g->step;

      while (g->cur_y >= g->max_y && g->parse > 0) {
         g->step = (1 << g->parse) * g->line_size;
         g->cur_y = g->start_y + (g->step >> 1);
         --g->parse;
      }
   }
}

static stbi_uc *stbi__process_gif_raster(stbi__context *s, stbi__gif *g)
{
   stbi_uc lzw_cs;
   stbi__int32 len, init_code;
   stbi__uint32 first;
   stbi__int32 codesize, codemask, avail, oldcode, bits, valid_bits, clear;
   stbi__gif_lzw *p;

   lzw_cs = stbi__get8(s);
   if (lzw_cs > 12) return NULL;
   clear = 1 << lzw_cs;
   first = 1;
   codesize = lzw_cs + 1;
   codemask = (1 << codesize) - 1;
   bits = 0;
   valid_bits = 0;
   for (init_code = 0; init_code < clear; init_code++) {
      g->codes[init_code].prefix = -1;
      g->codes[init_code].first = (stbi_uc) init_code;
      g->codes[init_code].suffix = (stbi_uc) init_code;
   }

   // support no starting clear code
   avail = clear+2;
   oldcode = -1;

   len = 0;
   for(;;) {
      if (valid_bits < codesize) {
         if (len == 0) {
            len = stbi__get8(s); // start new block
            if (len == 0)
               return g->out;
         }
         --len;
         bits |= (stbi__int32) stbi__get8(s) << valid_bits;
         valid_bits += 8;
      } else {
         stbi__int32 code = bits & codemask;
         bits >>= codesize;
         valid_bits -= codesize;
         // @OPTIMIZE: is there some way we can accelerate the non-clear path?
         if (code == clear) {  // clear code
            codesize = lzw_cs + 1;
            codemask = (1 << codesize) - 1;
            avail = clear + 2;
            oldcode = -1;
            first = 0;
         } else if (code == clear + 1) { // end of stream code
            stbi__skip(s, len);
            while ((len = stbi__get8(s)) > 0)
               stbi__skip(s,len);
            return g->out;
         } else if (code <= avail) {
            if (first) {
               return stbi__errpuc("no clear code", "Corrupt GIF");
            }

            if (oldcode >= 0) {
               p = &g->codes[avail++];
               if (avail > 8192) {
                  return stbi__errpuc("too many codes", "Corrupt GIF");
               }

               p->prefix = (stbi__int16) oldcode;
               p->first = g->codes[oldcode].first;
               p->suffix = (code == avail) ? p->first : g->codes[code].first;
            } else if (code == avail)
               return stbi__errpuc("illegal code in raster", "Corrupt GIF");

            stbi__out_gif_code(g, (stbi__uint16) code);

            if ((avail & codemask) == 0 && avail <= 0x0FFF) {
               codesize++;
               codemask = (1 << codesize) - 1;
            }

            oldcode = code;
         } else {
            return stbi__errpuc("illegal code in raster", "Corrupt GIF");
         }
      }
   }
}

// this function is designed to support animated gifs, although stb_image doesn't support it
// two back is the image from two frames ago, used for a very specific disposal format
static stbi_uc *stbi__gif_load_next(stbi__context *s, stbi__gif *g, int *comp, int req_comp, stbi_uc *two_back)
{
   int dispose;
   int first_frame;
   int pi;
   int pcount;
   STBI_NOTUSED(req_comp);

   // on first frame, any non-written pixels get the background colour (non-transparent)
   first_frame = 0;
   if (g->out == 0) {
      if (!stbi__gif_header(s, g, comp,0)) return 0; // stbi__g_failure_reason set by stbi__gif_header
      if (!stbi__mad3sizes_valid(4, g->w, g->h, 0))
         return stbi__errpuc("too large", "GIF image is too large");
      pcount = g->w * g->h;
      g->out = (stbi_uc *) stbi__malloc(4 * pcount);
      g->background = (stbi_uc *) stbi__malloc(4 * pcount);
      g->history = (stbi_uc *) stbi__malloc(pcount);
      if (!g->out || !g->background || !g->history)
         return stbi__errpuc("outofmem", "Out of memory");

      // image is treated as "transparent" at the start - ie, nothing overwrites the current background;
      // background colour is only used for pixels that are not rendered first frame, after that "background"
      // color refers to the color that was there the previous frame.
      memset(g->out, 0x00, 4 * pcount);
      memset(g->background, 0x00, 4 * pcount); // state of the background (starts transparent)
      memset(g->history, 0x00, pcount);        // pixels that were affected previous frame
      first_frame = 1;
   } else {
      // second frame - how do we dispose of the previous one?
      dispose = (g->eflags & 0x1C) >> 2;
      pcount = g->w * g->h;

      if ((dispose == 3) && (two_back == 0)) {
         dispose = 2; // if I don't have an image to revert back to, default to the old background
      }

      if (dispose == 3) { // use previous graphic
         for (pi = 0; pi < pcount; ++pi) {
            if (g->history[pi]) {
               memcpy( &g->out[pi * 4], &two_back[pi * 4], 4 );
            }
         }
      } else if (dispose == 2) {
         // restore what was changed last frame to background before that frame;
         for (pi = 0; pi < pcount; ++pi) {
            if (g->history[pi]) {
               memcpy( &g->out[pi * 4], &g->background[pi * 4], 4 );
            }
         }
      } else {
         // This is a non-disposal case eithe way, so just
         // leave the pixels as is, and they will become the new background
         // 1: do not dispose
         // 0:  not specified.
      }

      // background is what out is after the undoing of the previou frame;
      memcpy( g->background, g->out, 4 * g->w * g->h );
   }

   // clear my history;
   memset( g->history, 0x00, g->w * g->h );        // pixels that were affected previous frame

   for (;;) {
      int tag = stbi__get8(s);
      switch (tag) {
         case 0x2C: /* Image Descriptor */
         {
            stbi__int32 x, y, w, h;
            stbi_uc *o;

            x = stbi__get16le(s);
            y = stbi__get16le(s);
            w = stbi__get16le(s);
            h = stbi__get16le(s);
            if (((x + w) > (g->w)) || ((y + h) > (g->h)))
               return stbi__errpuc("bad Image Descriptor", "Corrupt GIF");

            g->line_size = g->w * 4;
            g->start_x = x * 4;
            g->start_y = y * g->line_size;
            g->max_x   = g->start_x + w * 4;
            g->max_y   = g->start_y + h * g->line_size;
            g->cur_x   = g->start_x;
            g->cur_y   = g->start_y;

            // if the width of the specified rectangle is 0, that means
            // we may not see *any* pixels or the image is malformed;
            // to make sure this is caught, move the current y down to
            // max_y (which is what out_gif_code checks).
            if (w == 0)
               g->cur_y = g->max_y;

            g->lflags = stbi__get8(s);

            if (g->lflags & 0x40) {
               g->step = 8 * g->line_size; // first interlaced spacing
               g->parse = 3;
            } else {
               g->step = g->line_size;
               g->parse = 0;
            }

            if (g->lflags & 0x80) {
               stbi__gif_parse_colortable(s,g->lpal, 2 << (g->lflags & 7), g->eflags & 0x01 ? g->transparent : -1);
               g->color_table = (stbi_uc *) g->lpal;
            } else if (g->flags & 0x80) {
               g->color_table = (stbi_uc *) g->pal;
            } else
               return stbi__errpuc("missing color table", "Corrupt GIF");

            o = stbi__process_gif_raster(s, g);
            if (!o) return NULL;

            // if this was the first frame,
            pcount = g->w * g->h;
            if (first_frame && (g->bgindex > 0)) {
               // if first frame, any pixel not drawn to gets the background color
               for (pi = 0; pi < pcount; ++pi) {
                  if (g->history[pi] == 0) {
                     g->pal[g->bgindex][3] = 255; // just in case it was made transparent, undo that; It will be reset next frame if need be;
                     memcpy( &g->out[pi * 4], &g->pal[g->bgindex], 4 );
                  }
               }
            }

            return o;
         }

         case 0x21: // Comment Extension.
         {
            int len;
            int ext = stbi__get8(s);
            if (ext == 0xF9) { // Graphic Control Extension.
               len = stbi__get8(s);
               if (len == 4) {
                  g->eflags = stbi__get8(s);
                  g->delay = 10 * stbi__get16le(s); // delay - 1/100th of a second, saving as 1/1000ths.

                  // unset old transparent
                  if (g->transparent >= 0) {
                     g->pal[g->transparent][3] = 255;
                  }
                  if (g->eflags & 0x01) {
                     g->transparent = stbi__get8(s);
                     if (g->transparent >= 0) {
                        g->pal[g->transparent][3] = 0;
                     }
                  } else {
                     // don't need transparent
                     stbi__skip(s, 1);
                     g->transparent = -1;
                  }
               } else {
                  stbi__skip(s, len);
                  break;
               }
            }
            while ((len = stbi__get8(s)) != 0) {
               stbi__skip(s, len);
            }
            break;
         }

         case 0x3B: // gif stream termination code
            return (stbi_uc *) s; // using '1' causes warning on some compilers

         default:
            return stbi__errpuc("unknown code", "Corrupt GIF");
      }
   }
}

static void *stbi__load_gif_main_outofmem(stbi__gif *g, stbi_uc *out, int **delays)
{
   STBI_FREE(g->out);
   STBI_FREE(g->history);
   STBI_FREE(g->background);

   if (out) STBI_FREE(out);
   if (delays && *delays) STBI_FREE(*delays);
   return stbi__errpuc("outofmem", "Out of memory");
}

static void *stbi__load_gif_main(stbi__context *s, int **delays, int *x, int *y, int *z, int *comp, int req_comp)
{
   if (stbi__gif_test(s)) {
      int layers = 0;
      stbi_uc *u = 0;
      stbi_uc *out = 0;
      stbi_uc *two_back = 0;
      stbi__gif g;
      int stride;
      int out_size = 0;
      int delays_size = 0;

      STBI_NOTUSED(out_size);
      STBI_NOTUSED(delays_size);

      memset(&g, 0, sizeof(g));
      if (delays) {
         *delays = 0;
      }

      do {
         u = stbi__gif_load_next(s, &g, comp, req_comp, two_back);
         if (u == (stbi_uc *) s) u = 0;  // end of animated gif marker

         if (u) {
            *x = g.w;
            *y = g.h;
            ++layers;
            stride = g.w * g.h * 4;

            if (out) {
               void *tmp = (stbi_uc*) STBI_REALLOC_SIZED( out, out_size, layers * stride );
               if (!tmp)
                  return stbi__load_gif_main_outofmem(&g, out, delays);
               else {
                   out = (stbi_uc*) tmp;
                   out_size = layers * stride;
               }

               if (delays) {
                  int *new_delays = (int*) STBI_REALLOC_SIZED( *delays, delays_size, sizeof(int) * layers );
                  if (!new_delays)
                     return stbi__load_gif_main_outofmem(&g, out, delays);
                  *delays = new_delays;
                  delays_size = layers * sizeof(int);
               }
            } else {
               out = (stbi_uc*)stbi__malloc( layers * stride );
               if (!out)
                  return stbi__load_gif_main_outofmem(&g, out, delays);
               out_size = layers * stride;
               if (delays) {
                  *delays = (int*) stbi__malloc( layers * sizeof(int) );
                  if (!*delays)
                     return stbi__load_gif_main_outofmem(&g, out, delays);
                  delays_size = layers * sizeof(int);
               }
            }
            memcpy( out + ((layers - 1) * stride), u, stride );
            if (layers >= 2) {
               two_back = out - 2 * stride;
            }

            if (delays) {
               (*delays)[layers - 1U] = g.delay;
            }
         }
      } while (u != 0);

      // free temp buffer;
      STBI_FREE(g.out);
      STBI_FREE(g.history);
      STBI_FREE(g.background);

      // do the final conversion after loading everything;
      if (req_comp && req_comp != 4)
         out = stbi__convert_format(out, 4, req_comp, layers * g.w, g.h);

      *z = layers;
      return out;
   } else {
      return stbi__errpuc("not GIF", "Image was not as a gif type.");
   }
}

static void *stbi__gif_load(stbi__context *s, int *x, int *y, int *comp, int req_comp, stbi__result_info *ri)
{
   stbi_uc *u = 0;
   stbi__gif g;
   memset(&g, 0, sizeof(g));
   STBI_NOTUSED(ri);

   u = stbi__gif_load_next(s, &g, comp, req_comp, 0);
   if (u == (stbi_uc *) s) u = 0;  // end of animated gif marker
   if (u) {
      *x = g.w;
      *y = g.h;

      // moved conversion to after successful load so that the same
      // can be done for multiple frames.
      if (req_comp && req_comp != 4)
         u = stbi__convert_format(u, 4, req_comp, g.w, g.h);
   } else if (g.out) {
      // if there was an error and we allocated an image buffer, free it!
      STBI_FREE(g.out);
   }

   // free buffers needed for multiple frame loading;
   STBI_FREE(g.history);
   STBI_FREE(g.background);

   return u;
}

static int stbi__gif_info(stbi__context *s, int *x, int *y, int *comp)
{
   return stbi__gif_info_raw(s,x,y,comp);
}
#endif

// *************************************************************************************************
// Radiance RGBE HDR loader
// originally by Nicolas Schulz
#ifndef STBI_NO_HDR
static int stbi__hdr_test_core(stbi__context *s, const char *signature)
{
   int i;
   for (i=0; signature[i]; ++i)
      if (stbi__get8(s) != signature[i])
          return 0;
   stbi__rewind(s);
   return 1;
}

static int stbi__hdr_test(stbi__context* s)
{
   int r = stbi__hdr_test_core(s, "#?RADIANCE\n");
   stbi__rewind(s);
   if(!r) {
       r = stbi__hdr_test_core(s, "#?RGBE\n");
       stbi__rewind(s);
   }
   return r;
}

#define STBI__HDR_BUFLEN  1024
static char *stbi__hdr_gettoken(stbi__context *z, char *buffer)
{
   int len=0;
   char c = '\0';

   c = (char) stbi__get8(z);

   while (!stbi__at_eof(z) && c != '\n') {
      buffer[len++] = c;
      if (len == STBI__HDR_BUFLEN-1) {
         // flush to end of line
         while (!stbi__at_eof(z) && stbi__get8(z) != '\n')
            ;
         break;
      }
      c = (char) stbi__get8(z);
   }

   buffer[len] = 0;
   return buffer;
}

static void stbi__hdr_convert(float *output, stbi_uc *input, int req_comp)
{
   if ( input[3] != 0 ) {
      float f1;
      // Exponent
      f1 = (float) ldexp(1.0f, input[3] - (int)(128 + 8));
      if (req_comp <= 2)
         output[0] = (input[0] + input[1] + input[2]) * f1 / 3;
      else {
         output[0] = input[0] * f1;
         output[1] = input[1] * f1;
         output[2] = input[2] * f1;
      }
      if (req_comp == 2) output[1] = 1;
      if (req_comp == 4) output[3] = 1;
   } else {
      switch (req_comp) {
         case 4: output[3] = 1; /* fallthrough */
         case 3: output[0] = output[1] = output[2] = 0;
                 break;
         case 2: output[1] = 1; /* fallthrough */
         case 1: output[0] = 0;
                 break;
      }
   }
}

static float *stbi__hdr_load(stbi__context *s, int *x, int *y, int *comp, int req_comp, stbi__result_info *ri)
{
   char buffer[STBI__HDR_BUFLEN];
   char *token;
   int valid = 0;
   int width, height;
   stbi_uc *scanline;
   float *hdr_data;
   int len;
   unsigned char count, value;
   int i, j, k, c1,c2, z;
   const char *headerToken;
   STBI_NOTUSED(ri);

   // Check identifier
   headerToken = stbi__hdr_gettoken(s,buffer);
   if (strcmp(headerToken, "#?RADIANCE") != 0 && strcmp(headerToken, "#?RGBE") != 0)
      return stbi__errpf("not HDR", "Corrupt HDR image");

   // Parse header
   for(;;) {
      token = stbi__hdr_gettoken(s,buffer);
      if (token[0] == 0) break;
      if (strcmp(token, "FORMAT=32-bit_rle_rgbe") == 0) valid = 1;
   }

   if (!valid)    return stbi__errpf("unsupported format", "Unsupported HDR format");

   // Parse width and height
   // can't use sscanf() if we're not using stdio!
   token = stbi__hdr_gettoken(s,buffer);
   if (strncmp(token, "-Y ", 3))  return stbi__errpf("unsupported data layout", "Unsupported HDR format");
   token += 3;
   height = (int) strtol(token, &token, 10);
   while (*token == ' ') ++token;
   if (strncmp(token, "+X ", 3))  return stbi__errpf("unsupported data layout", "Unsupported HDR format");
   token += 3;
   width = (int) strtol(token, NULL, 10);

   if (height > STBI_MAX_DIMENSIONS) return stbi__errpf("too large","Very large image (corrupt?)");
   if (width > STBI_MAX_DIMENSIONS) return stbi__errpf("too large","Very large image (corrupt?)");

   *x = width;
   *y = height;

   if (comp) *comp = 3;
   if (req_comp == 0) req_comp = 3;

   if (!stbi__mad4sizes_valid(width, height, req_comp, sizeof(float), 0))
      return stbi__errpf("too large", "HDR image is too large");

   // Read data
   hdr_data = (float *) stbi__malloc_mad4(width, height, req_comp, sizeof(float), 0);
   if (!hdr_data)
      return stbi__errpf("outofmem", "Out of memory");

   // Load image data
   // image data is stored as some number of sca
   if ( width < 8 || width >= 32768) {
      // Read flat data
      for (j=0; j < height; ++j) {
         for (i=0; i < width; ++i) {
            stbi_uc rgbe[4];
           main_decode_loop:
            stbi__getn(s, rgbe, 4);
            stbi__hdr_convert(hdr_data + j * width * req_comp + i * req_comp, rgbe, req_comp);
         }
      }
   } else {
      // Read RLE-encoded data
      scanline = NULL;

      for (j = 0; j < height; ++j) {
         c1 = stbi__get8(s);
         c2 = stbi__get8(s);
         len = stbi__get8(s);
         if (c1 != 2 || c2 != 2 || (len & 0x80)) {
            // not run-length encoded, so we have to actually use THIS data as a decoded
            // pixel (note this can't be a valid pixel--one of RGB must be >= 128)
            stbi_uc rgbe[4];
            rgbe[0] = (stbi_uc) c1;
            rgbe[1] = (stbi_uc) c2;
            rgbe[2] = (stbi_uc) len;
            rgbe[3] = (stbi_uc) stbi__get8(s);
            stbi__hdr_convert(hdr_data, rgbe, req_comp);
            i = 1;
            j = 0;
            STBI_FREE(scanline);
            goto main_decode_loop; // yes, this makes no sense
         }
         len <<= 8;
         len |= stbi__get8(s);
         if (len != width) { STBI_FREE(hdr_data); STBI_FREE(scanline); return stbi__errpf("invalid decoded scanline length", "corrupt HDR"); }
         if (scanline == NULL) {
            scanline = (stbi_uc *) stbi__malloc_mad2(width, 4, 0);
            if (!scanline) {
               STBI_FREE(hdr_data);
               return stbi__errpf("outofmem", "Out of memory");
            }
         }

         for (k = 0; k < 4; ++k) {
            int nleft;
            i = 0;
            while ((nleft = width - i) > 0) {
               count = stbi__get8(s);
               if (count > 128) {
                  // Run
                  value = stbi__get8(s);
                  count -= 128;
                  if (count > nleft) { STBI_FREE(hdr_data); STBI_FREE(scanline); return stbi__errpf("corrupt", "bad RLE data in HDR"); }
                  for (z = 0; z < count; ++z)
                     scanline[i++ * 4 + k] = value;
               } else {
                  // Dump
                  if (count > nleft) { STBI_FREE(hdr_data); STBI_FREE(scanline); return stbi__errpf("corrupt", "bad RLE data in HDR"); }
                  for (z = 0; z < count; ++z)
                     scanline[i++ * 4 + k] = stbi__get8(s);
               }
            }
         }
         for (i=0; i < width; ++i)
            stbi__hdr_convert(hdr_data+(j*width + i)*req_comp, scanline + i*4, req_comp);
      }
      if (scanline)
         STBI_FREE(scanline);
   }

   return hdr_data;
}

static int stbi__hdr_info(stbi__context *s, int *x, int *y, int *comp)
{
   char buffer[STBI__HDR_BUFLEN];
   char *token;
   int valid = 0;
   int dummy;

   if (!x) x = &dummy;
   if (!y) y = &dummy;
   if (!comp) comp = &dummy;

   if (stbi__hdr_test(s) == 0) {
       stbi__rewind( s );
       return 0;
   }

   for(;;) {
      token = stbi__hdr_gettoken(s,buffer);
      if (token[0] == 0) break;
      if (strcmp(token, "FORMAT=32-bit_rle_rgbe") == 0) valid = 1;
   }

   if (!valid) {
       stbi__rewind( s );
       return 0;
   }
   token = stbi__hdr_gettoken(s,buffer);
   if (strncmp(token, "-Y ", 3)) {
       stbi__rewind( s );
       return 0;
   }
   token += 3;
   *y = (int) strtol(token, &token, 10);
   while (*token == ' ') ++token;
   if (strncmp(token, "+X ", 3)) {
       stbi__rewind( s );
       return 0;
   }
   token += 3;
   *x = (int) strtol(token, NULL, 10);
   *comp = 3;
   return 1;
}
#endif // STBI_NO_HDR

#ifndef STBI_NO_BMP
static int stbi__bmp_info(stbi__context *s, int *x, int *y, int *comp)
{
   void *p;
   stbi__bmp_data info;

   info.all_a = 255;
   p = stbi__bmp_parse_header(s, &info);
   if (p == NULL) {
      stbi__rewind( s );
      return 0;
   }
   if (x) *x = s->img_x;
   if (y) *y = s->img_y;
   if (comp) {
      if (info.bpp == 24 && info.ma == 0xff000000)
         *comp = 3;
      else
         *comp = info.ma ? 4 : 3;
   }
   return 1;
}
#endif

#ifndef STBI_NO_PSD
static int stbi__psd_info(stbi__context *s, int *x, int *y, int *comp)
{
   int channelCount, dummy, depth;
   if (!x) x = &dummy;
   if (!y) y = &dummy;
   if (!comp) comp = &dummy;
   if (stbi__get32be(s) != 0x38425053) {
       stbi__rewind( s );
       return 0;
   }
   if (stbi__get16be(s) != 1) {
       stbi__rewind( s );
       return 0;
   }
   stbi__skip(s, 6);
   channelCount = stbi__get16be(s);
   if (channelCount < 0 || channelCount > 16) {
       stbi__rewind( s );
       return 0;
   }
   *y = stbi__get32be(s);
   *x = stbi__get32be(s);
   depth = stbi__get16be(s);
   if (depth != 8 && depth != 16) {
       stbi__rewind( s );
       return 0;
   }
   if (stbi__get16be(s) != 3) {
       stbi__rewind( s );
       return 0;
   }
   *comp = 4;
   return 1;
}

static int stbi__psd_is16(stbi__context *s)
{
   int channelCount, depth;
   if (stbi__get32be(s) != 0x38425053) {
       stbi__rewind( s );
       return 0;
   }
   if (stbi__get16be(s) != 1) {
       stbi__rewind( s );
       return 0;
   }
   stbi__skip(s, 6);
   channelCount = stbi__get16be(s);
   if (channelCount < 0 || channelCount > 16) {
       stbi__rewind( s );
       return 0;
   }
   STBI_NOTUSED(stbi__get32be(s));
   STBI_NOTUSED(stbi__get32be(s));
   depth = stbi__get16be(s);
   if (depth != 16) {
       stbi__rewind( s );
       return 0;
   }
   return 1;
}
#endif

#ifndef STBI_NO_PIC
static int stbi__pic_info(stbi__context *s, int *x, int *y, int *comp)
{
   int act_comp=0,num_packets=0,chained,dummy;
   stbi__pic_packet packets[10];

   if (!x) x = &dummy;
   if (!y) y = &dummy;
   if (!comp) comp = &dummy;

   if (!stbi__pic_is4(s,"\x53\x80\xF6\x34")) {
      stbi__rewind(s);
      return 0;
   }

   stbi__skip(s, 88);

   *x = stbi__get16be(s);
   *y = stbi__get16be(s);
   if (stbi__at_eof(s)) {
      stbi__rewind( s);
      return 0;
   }
   if ( (*x) != 0 && (1 << 28) / (*x) < (*y)) {
      stbi__rewind( s );
      return 0;
   }

   stbi__skip(s, 8);

   do {
      stbi__pic_packet *packet;

      if (num_packets==sizeof(packets)/sizeof(packets[0]))
         return 0;

      packet = &packets[num_packets++];
      chained = stbi__get8(s);
      packet->size    = stbi__get8(s);
      packet->type    = stbi__get8(s);
      packet->channel = stbi__get8(s);
      act_comp |= packet->channel;

      if (stbi__at_eof(s)) {
          stbi__rewind( s );
          return 0;
      }
      if (packet->size != 8) {
          stbi__rewind( s );
          return 0;
      }
   } while (chained);

   *comp = (act_comp & 0x10 ? 4 : 3);

   return 1;
}
#endif

// *************************************************************************************************

// Portable Gray Map and Portable Pixel Map loader
// by Ken Miller
//
// PGM: http://netpbm.sourceforge.net/doc/pgm.html
// PPM: http://netpbm.sourceforge.net/doc/ppm.html
//
// Known limitations:
//    Does not support comments in the header section
//    Does not support ASCII image data (formats P2 and P3)

#ifndef STBI_NO_PNM

static int      stbi__pnm_test(stbi__context *s)
{
   char p, t;
   p = (char) stbi__get8(s);
   t = (char) stbi__get8(s);
   if (p != 'P' || (t != '5' && t != '6')) {
       stbi__rewind( s );
       return 0;
   }
   return 1;
}

static void *stbi__pnm_load(stbi__context *s, int *x, int *y, int *comp, int req_comp, stbi__result_info *ri)
{
   stbi_uc *out;
   STBI_NOTUSED(ri);

   ri->bits_per_channel = stbi__pnm_info(s, (int *)&s->img_x, (int *)&s->img_y, (int *)&s->img_n);
   if (ri->bits_per_channel == 0)
      return 0;

   if (s->img_y > STBI_MAX_DIMENSIONS) return stbi__errpuc("too large","Very large image (corrupt?)");
   if (s->img_x > STBI_MAX_DIMENSIONS) return stbi__errpuc("too large","Very large image (corrupt?)");

   *x = s->img_x;
   *y = s->img_y;
   if (comp) *comp = s->img_n;

   if (!stbi__mad4sizes_valid(s->img_n, s->img_x, s->img_y, ri->bits_per_channel / 8, 0))
      return stbi__errpuc("too large", "PNM too large");

   out = (stbi_uc *) stbi__malloc_mad4(s->img_n, s->img_x, s->img_y, ri->bits_per_channel / 8, 0);
   if (!out) return stbi__errpuc("outofmem", "Out of memory");
   stbi__getn(s, out, s->img_n * s->img_x * s->img_y * (ri->bits_per_channel / 8));

   if (req_comp && req_comp != s->img_n) {
      out = stbi__convert_format(out, s->img_n, req_comp, s->img_x, s->img_y);
      if (out == NULL) return out; // stbi__convert_format frees input on failure
   }
   return out;
}

static int      stbi__pnm_isspace(char c)
{
   return c == ' ' || c == '\t' || c == '\n' || c == '\v' || c == '\f' || c == '\r';
}

static void     stbi__pnm_skip_whitespace(stbi__context *s, char *c)
{
   for (;;) {
      while (!stbi__at_eof(s) && stbi__pnm_isspace(*c))
         *c = (char) stbi__get8(s);

      if (stbi__at_eof(s) || *c != '#')
         break;

      while (!stbi__at_eof(s) && *c != '\n' && *c != '\r' )
         *c = (char) stbi__get8(s);
   }
}

static int      stbi__pnm_isdigit(char c)
{
   return c >= '0' && c <= '9';
}

static int      stbi__pnm_getinteger(stbi__context *s, char *c)
{
   int value = 0;

   while (!stbi__at_eof(s) && stbi__pnm_isdigit(*c)) {
      value = value*10 + (*c - '0');
      *c = (char) stbi__get8(s);
   }

   return value;
}

static int      stbi__pnm_info(stbi__context *s, int *x, int *y, int *comp)
{
   int maxv, dummy;
   char c, p, t;

   if (!x) x = &dummy;
   if (!y) y = &dummy;
   if (!comp) comp = &dummy;

   stbi__rewind(s);

   // Get identifier
   p = (char) stbi__get8(s);
   t = (char) stbi__get8(s);
   if (p != 'P' || (t != '5' && t != '6')) {
       stbi__rewind(s);
       return 0;
   }

   *comp = (t == '6') ? 3 : 1;  // '5' is 1-component .pgm; '6' is 3-component .ppm

   c = (char) stbi__get8(s);
   stbi__pnm_skip_whitespace(s, &c);

   *x = stbi__pnm_getinteger(s, &c); // read width
   stbi__pnm_skip_whitespace(s, &c);

   *y = stbi__pnm_getinteger(s, &c); // read height
   stbi__pnm_skip_whitespace(s, &c);

   maxv = stbi__pnm_getinteger(s, &c);  // read max value
   if (maxv > 65535)
      return stbi__err("max value > 65535", "PPM image supports only 8-bit and 16-bit images");
   else if (maxv > 255)
      return 16;
   else
      return 8;
}

static int stbi__pnm_is16(stbi__context *s)
{
   if (stbi__pnm_info(s, NULL, NULL, NULL) == 16)
	   return 1;
   return 0;
}
#endif

static int stbi__info_main(stbi__context *s, int *x, int *y, int *comp)
{
   #ifndef STBI_NO_JPEG
   if (stbi__jpeg_info(s, x, y, comp)) return 1;
   #endif

   #ifndef STBI_NO_PNG
   if (stbi__png_info(s, x, y, comp))  return 1;
   #endif

   #ifndef STBI_NO_GIF
   if (stbi__gif_info(s, x, y, comp))  return 1;
   #endif

   #ifndef STBI_NO_BMP
   if (stbi__bmp_info(s, x, y, comp))  return 1;
   #endif

   #ifndef STBI_NO_PSD
   if (stbi__psd_info(s, x, y, comp))  return 1;
   #endif

   #ifndef STBI_NO_PIC
   if (stbi__pic_info(s, x, y, comp))  return 1;
   #endif

   #ifndef STBI_NO_PNM
   if (stbi__pnm_info(s, x, y, comp))  return 1;
   #endif

   #ifndef STBI_NO_HDR
   if (stbi__hdr_info(s, x, y, comp))  return 1;
   #endif

   #ifndef STBI_NO_ICO
   if(stbi__ico_info(s, x, y, comp)) return 1;
   #endif

   // test tga last because it's a crappy test!
   #ifndef STBI_NO_TGA
   if (stbi__tga_info(s, x, y, comp))
       return 1;
   #endif
   return stbi__err("unknown image type", "Image not of any known type, or corrupt");
}

static int stbi__is_16_main(stbi__context *s)
{
   #ifndef STBI_NO_PNG
   if (stbi__png_is16(s))  return 1;
   #endif

   #ifndef STBI_NO_PSD
   if (stbi__psd_is16(s))  return 1;
   #endif

   #ifndef STBI_NO_PNM
   if (stbi__pnm_is16(s))  return 1;
   #endif
   return 0;
}

#ifndef STBI_NO_STDIO
STBIDEF int stbi_info(char const *filename, int *x, int *y, int *comp)
{
    FILE *f = stbi__fopen(filename, "rb");
    int result;
    if (!f) return stbi__err("can't fopen", "Unable to open file");
    result = stbi_info_from_file(f, x, y, comp);
    fclose(f);
    return result;
}

STBIDEF int stbi_info_from_file(FILE *f, int *x, int *y, int *comp)
{
   int r;
   stbi__context s;
   long pos = ftell(f);
   stbi__start_file(&s, f);
   r = stbi__info_main(&s,x,y,comp);
   fseek(f,pos,SEEK_SET);
   return r;
}

STBIDEF int stbi_is_16_bit(char const *filename)
{
    FILE *f = stbi__fopen(filename, "rb");
    int result;
    if (!f) return stbi__err("can't fopen", "Unable to open file");
    result = stbi_is_16_bit_from_file(f);
    fclose(f);
    return result;
}

STBIDEF int stbi_is_16_bit_from_file(FILE *f)
{
   int r;
   stbi__context s;
   long pos = ftell(f);
   stbi__start_file(&s, f);
   r = stbi__is_16_main(&s);
   fseek(f,pos,SEEK_SET);
   return r;
}
#endif // !STBI_NO_STDIO

STBIDEF int stbi_info_from_memory(stbi_uc const *buffer, int len, int *x, int *y, int *comp)
{
   stbi__context s;
   stbi__start_mem(&s,buffer,len);
   return stbi__info_main(&s,x,y,comp);
}

STBIDEF int stbi_info_from_callbacks(stbi_io_callbacks const *c, void *user, int *x, int *y, int *comp)
{
   stbi__context s;
   stbi__start_callbacks(&s, (stbi_io_callbacks *) c, user);
   return stbi__info_main(&s,x,y,comp);
}

STBIDEF int stbi_is_16_bit_from_memory(stbi_uc const *buffer, int len)
{
   stbi__context s;
   stbi__start_mem(&s,buffer,len);
   return stbi__is_16_main(&s);
}

STBIDEF int stbi_is_16_bit_from_callbacks(stbi_io_callbacks const *c, void *user)
{
   stbi__context s;
   stbi__start_callbacks(&s, (stbi_io_callbacks *) c, user);
   return stbi__is_16_main(&s);
}

#endif // STB_IMAGE_IMPLEMENTATION

/*
   revision history:
      2.20  (2019-02-07) support utf8 filenames in Windows; fix warnings and platform ifdefs
      2.19  (2018-02-11) fix warning
      2.18  (2018-01-30) fix warnings
      2.17  (2018-01-29) change sbti__shiftsigned to avoid clang -O2 bug
                         1-bit BMP
                         *_is_16_bit api
                         avoid warnings
      2.16  (2017-07-23) all functions have 16-bit variants;
                         STBI_NO_STDIO works again;
                         compilation fixes;
                         fix rounding in unpremultiply;
                         optimize vertical flip;
                         disable raw_len validation;
                         documentation fixes
      2.15  (2017-03-18) fix png-1,2,4 bug; now all Imagenet JPGs decode;
                         warning fixes; disable run-time SSE detection on gcc;
                         uniform handling of optional "return" values;
                         thread-safe initialization of zlib tables
      2.14  (2017-03-03) remove deprecated STBI_JPEG_OLD; fixes for Imagenet JPGs
      2.13  (2016-11-29) add 16-bit API, only supported for PNG right now
      2.12  (2016-04-02) fix typo in 2.11 PSD fix that caused crashes
      2.11  (2016-04-02) allocate large structures on the stack
                         remove white matting for transparent PSD
                         fix reported channel count for PNG & BMP
                         re-enable SSE2 in non-gcc 64-bit
                         support RGB-formatted JPEG
                         read 16-bit PNGs (only as 8-bit)
      2.10  (2016-01-22) avoid warning introduced in 2.09 by STBI_REALLOC_SIZED
      2.09  (2016-01-16) allow comments in PNM files
                         16-bit-per-pixel TGA (not bit-per-component)
                         info() for TGA could break due to .hdr handling
                         info() for BMP to shares code instead of sloppy parse
                         can use STBI_REALLOC_SIZED if allocator doesn't support realloc
                         code cleanup
      2.08  (2015-09-13) fix to 2.07 cleanup, reading RGB PSD as RGBA
      2.07  (2015-09-13) fix compiler warnings
                         partial animated GIF support
                         limited 16-bpc PSD support
                         #ifdef unused functions
                         bug with < 92 byte PIC,PNM,HDR,TGA
      2.06  (2015-04-19) fix bug where PSD returns wrong '*comp' value
      2.05  (2015-04-19) fix bug in progressive JPEG handling, fix warning
      2.04  (2015-04-15) try to re-enable SIMD on MinGW 64-bit
      2.03  (2015-04-12) extra corruption checking (mmozeiko)
                         stbi_set_flip_vertically_on_load (nguillemot)
                         fix NEON support; fix mingw support
      2.02  (2015-01-19) fix incorrect assert, fix warning
      2.01  (2015-01-17) fix various warnings; suppress SIMD on gcc 32-bit without -msse2
      2.00b (2014-12-25) fix STBI_MALLOC in progressive JPEG
      2.00  (2014-12-25) optimize JPG, including x86 SSE2 & NEON SIMD (ryg)
                         progressive JPEG (stb)
                         PGM/PPM support (Ken Miller)
                         STBI_MALLOC,STBI_REALLOC,STBI_FREE
                         GIF bugfix -- seemingly never worked
                         STBI_NO_*, STBI_ONLY_*
      1.48  (2014-12-14) fix incorrectly-named assert()
      1.47  (2014-12-14) 1/2/4-bit PNG support, both direct and paletted (Omar Cornut & stb)
                         optimize PNG (ryg)
                         fix bug in interlaced PNG with user-specified channel count (stb)
      1.46  (2014-08-26)
              fix broken tRNS chunk (colorkey-style transparency) in non-paletted PNG
      1.45  (2014-08-16)
              fix MSVC-ARM internal compiler error by wrapping malloc
      1.44  (2014-08-07)
              various warning fixes from Ronny Chevalier
      1.43  (2014-07-15)
              fix MSVC-only compiler problem in code changed in 1.42
      1.42  (2014-07-09)
              don't define _CRT_SECURE_NO_WARNINGS (affects user code)
              fixes to stbi__cleanup_jpeg path
              added STBI_ASSERT to avoid requiring assert.h
      1.41  (2014-06-25)
              fix search&replace from 1.36 that messed up comments/error messages
      1.40  (2014-06-22)
              fix gcc struct-initialization warning
      1.39  (2014-06-15)
              fix to TGA optimization when req_comp != number of components in TGA;
              fix to GIF loading because BMP wasn't rewinding (whoops, no GIFs in my test suite)
              add support for BMP version 5 (more ignored fields)
      1.38  (2014-06-06)
              suppress MSVC warnings on integer casts truncating values
              fix accidental rename of 'skip' field of I/O
      1.37  (2014-06-04)
              remove duplicate typedef
      1.36  (2014-06-03)
              convert to header file single-file library
              if de-iphone isn't set, load iphone images color-swapped instead of returning NULL
      1.35  (2014-05-27)
              various warnings
              fix broken STBI_SIMD path
              fix bug where stbi_load_from_file no longer left file pointer in correct place
              fix broken non-easy path for 32-bit BMP (possibly never used)
              TGA optimization by Arseny Kapoulkine
      1.34  (unknown)
              use STBI_NOTUSED in stbi__resample_row_generic(), fix one more leak in tga failure case
      1.33  (2011-07-14)
              make stbi_is_hdr work in STBI_NO_HDR (as specified), minor compiler-friendly improvements
      1.32  (2011-07-13)
              support for "info" function for all supported filetypes (SpartanJ)
      1.31  (2011-06-20)
              a few more leak fixes, bug in PNG handling (SpartanJ)
      1.30  (2011-06-11)
              added ability to load files via callbacks to accomidate custom input streams (Ben Wenger)
              removed deprecated format-specific test/load functions
              removed support for installable file formats (stbi_loader) -- would have been broken for IO callbacks anyway
              error cases in bmp and tga give messages and don't leak (Raymond Barbiero, grisha)
              fix inefficiency in decoding 32-bit BMP (David Woo)
      1.29  (2010-08-16)
              various warning fixes from Aurelien Pocheville
      1.28  (2010-08-01)
              fix bug in GIF palette transparency (SpartanJ)
      1.27  (2010-08-01)
              cast-to-stbi_uc to fix warnings
      1.26  (2010-07-24)
              fix bug in file buffering for PNG reported by SpartanJ
      1.25  (2010-07-17)
              refix trans_data warning (Won Chun)
      1.24  (2010-07-12)
              perf improvements reading from files on platforms with lock-heavy fgetc()
              minor perf improvements for jpeg
              deprecated type-specific functions so we'll get feedback if they're needed
              attempt to fix trans_data warning (Won Chun)
      1.23    fixed bug in iPhone support
      1.22  (2010-07-10)
              removed image *writing* support
              stbi_info support from Jetro Lauha
              GIF support from Jean-Marc Lienher
              iPhone PNG-extensions from James Brown
              warning-fixes from Nicolas Schulz and Janez Zemva (i.stbi__err. Janez (U+017D)emva)
      1.21    fix use of 'stbi_uc' in header (reported by jon blow)
      1.20    added support for Softimage PIC, by Tom Seddon
      1.19    bug in interlaced PNG corruption check (found by ryg)
      1.18  (2008-08-02)
              fix a threading bug (local mutable static)
      1.17    support interlaced PNG
      1.16    major bugfix - stbi__convert_format converted one too many pixels
      1.15    initialize some fields for thread safety
      1.14    fix threadsafe conversion bug
              header-file-only version (#define STBI_HEADER_FILE_ONLY before including)
      1.13    threadsafe
      1.12    const qualifiers in the API
      1.11    Support installable IDCT, colorspace conversion routines
      1.10    Fixes for 64-bit (don't use "unsigned long")
              optimized upsampling by Fabian "ryg" Giesen
      1.09    Fix format-conversion for PSD code (bad global variables!)
      1.08    Thatcher Ulrich's PSD code integrated by Nicolas Schulz
      1.07    attempt to fix C++ warning/errors again
      1.06    attempt to fix C++ warning/errors again
      1.05    fix TGA loading to return correct *comp and use good luminance calc
      1.04    default float alpha is 1, not 255; use 'void *' for stbi_image_free
      1.03    bugfixes to STBI_NO_STDIO, STBI_NO_HDR
      1.02    support for (subset of) HDR files, float interface for preferred access to them
      1.01    fix bug: possible bug in handling right-side up bmps... not sure
              fix bug: the stbi__bmp_load() and stbi__tga_load() functions didn't work at all
      1.00    interface to zlib that skips zlib header
      0.99    correct handling of alpha in palette
      0.98    TGA loader by lonesock; dynamically add loaders (untested)
      0.97    jpeg errors on too large a file; also catch another malloc failure
      0.96    fix detection of invalid v value - particleman@mollyrocket forum
      0.95    during header scan, seek to markers in case of padding
      0.94    STBI_NO_STDIO to disable stdio usage; rename all #defines the same
      0.93    handle jpegtran output; verbose errors
      0.92    read 4,8,16,24,32-bit BMP files of several formats
      0.91    output 24-bit Windows 3.0 BMP files
      0.90    fix a few more warnings; bump version number to approach 1.0
      0.61    bugfixes due to Marc LeBlanc, Christopher Lloyd
      0.60    fix compiling as c++
      0.59    fix warnings: merge Dave Moore's -Wall fixes
      0.58    fix bug: zlib uncompressed mode len/nlen was wrong endian
      0.57    fix bug: jpg last huffman symbol before marker was >9 bits but less than 16 available
      0.56    fix bug: zlib uncompressed mode len vs. nlen
      0.55    fix bug: restart_interval not initialized to 0
      0.54    allow NULL for 'int *comp'
      0.53    fix bug in png 3->4; speedup png decoding
      0.52    png handles req_comp=3,4 directly; minor cleanup; jpeg comments
      0.51    obey req_comp requests, 1-component jpegs return as 1-component,
              on 'test' only check type, not whether we support this variant
      0.50  (2006-11-19)
              first released version
*/


/*
------------------------------------------------------------------------------
This software is available under 2 licenses -- choose whichever you prefer.
------------------------------------------------------------------------------
ALTERNATIVE A - MIT License
Copyright (c) 2017 Sean Barrett
Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
------------------------------------------------------------------------------
ALTERNATIVE B - Public Domain (www.unlicense.org)
This is free and unencumbered software released into the public domain.
Anyone is free to copy, modify, publish, use, compile, sell, or distribute this
software, either in source code form or as a compiled binary, for any purpose,
commercial or non-commercial, and by any means.
In jurisdictions that recognize copyright laws, the author or authors of this
software dedicate any and all copyright interest in the software to the public
domain. We make this dedication for the benefit of the public at large and to
the detriment of our heirs and successors. We intend this dedication to be an
overt act of relinquishment in perpetuity of all present and future rights to
this software under copyright law.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
------------------------------------------------------------------------------
*/
#endif // LIBSTD_H
