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
