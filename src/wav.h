#ifndef WAV_H
#define WAV_H

#ifndef WAV_DEF
#  define WAV_DEF static inline
#endif // WAV_DEF

typedef unsigned char Wav_u8;
typedef char Wav_s8;
typedef unsigned short Wav_u16;
typedef int Wav_s32;
typedef unsigned int Wav_u32;
typedef unsigned long long Wav_u64;
#define s8 Wav_s8
#define u8 Wav_u8
#define u16 Wav_u16
#define s32 Wav_s32
#define u32 Wav_u32
#define u64 Wav_u64

WAV_DEF int wav_memcmp(const void *a, const void *b, u64 len);

typedef int (*Wav_Read)(void *opaque, u8 *data, u64 len);
typedef int (*Wav_Write)(void *opaque, u8 *data, u64 len);
typedef int (*Wav_Seek)(void *opaque, u64 offset);

typedef enum {
  WAV_FORMAT_PCM  = 0x1,
  WAV_FORMAT_IEEE = 0x3,
} Wav_Format;

// https://de.wikipedia.org/wiki/RIFF_WAVE
typedef struct {
  u8 RIFF[4];
  u32 file_size;
  u8 WAVE[4];
  
  u8 fmt[4];
  u32 fmt_len;
  u16 fmt_tag;
  u16 channels;
  u32 sample_rate;
  u32 bytes_per_second;
  u16 block_align;
  u16 bits_per_sample;

  u8 data[4];
  u32 data_size;
} Wav_Header;

typedef struct {
  u8 name[4];
  u32 size;
} Wav_Chunk_Name_And_Size;

typedef struct {
  // Input
  u16 channels;
  u32 sample_rate;
  Wav_Format format;
  void *opaque;
  Wav_Write write;
  Wav_Seek seek;

  // Internal
  u32 samples_len_bytes; // I would define this as 'u64' but 'file_size' is only 'u32'
} Wav_Encoder;

WAV_DEF void wav_encoder_init(Wav_Encoder *w,
			      u16 channels,
			      u32 sample_rate,
			      Wav_Format format,
			      void *opaque,
			      Wav_Write write,
			      Wav_Seek seek);
WAV_DEF int wav_encoder_encode(Wav_Encoder *w, u8 *samples, u32 samples_count);
WAV_DEF int wav_encoder_finalize(Wav_Encoder *w);

typedef struct {
  // Output
  u16 channels;
  u32 sample_rate;
  Wav_Format format;
  u32 samples_total;
  u32 duration_secs;

  // Input
  void *opaque;
  Wav_Read read;
  Wav_Seek seek;

  // Internal
  u32 samples_left;
} Wav_Decoder;

WAV_DEF int wav_decoder_open(Wav_Decoder *w,
			     void *opaque,
			     Wav_Read read,
			     Wav_Seek seek);
WAV_DEF int wav_decoder_decode(Wav_Decoder *w,
			       u8 *buffer,
			       u32 buffer_len_bytes,
			       u32 *samples_count);
WAV_DEF int wav_decoder_seek_to(Wav_Decoder *w, u32 to_second);

#ifdef WAV_IMPLEMENTATION

WAV_DEF s32 wav_memcmp(const void *a, const void *b, u64 len) {
  const s8 *pa = a;
  const s8 *pb = b;
  int d = 0;
  while(!d && len) {
    d = *pa++ - *pb++;
    len--;
  }
  return d;
}

u16 __wav_format_to_bits_per_sample[] = {
  [WAV_FORMAT_PCM] = 16,
  [WAV_FORMAT_IEEE] = 32,
};

u32 __wav_format_to_bytes_per_sample[] = {
  [WAV_FORMAT_PCM] = 2,
  [WAV_FORMAT_IEEE] = 4,
};

Wav_Header __wav_header_default = {
  .RIFF = "RIFF",
  // .file_size,
  .WAVE = "WAVE",
  
  .fmt = "fmt ",
  .fmt_len = (u32) 16,
  // .fmt_tag
  // .channels;
  // .sample_rate;
  // .bytes_per_second;
  // .block_align;
  // .bits_per_sample;

  .data = "data",
  // .data_size,
};

WAV_DEF void wav_encoder_init(Wav_Encoder *w,
			      u16 channels,
			      u32 sample_rate,
			      Wav_Format format,
			      void *opaque,
			      Wav_Write write,
			      Wav_Seek seek) {
  w->channels = channels;
  w->sample_rate = sample_rate;
  w->format = format;
  w->opaque = opaque;
  w->write = write;
  w->seek = seek;
}

WAV_DEF int wav_encoder_encode(Wav_Encoder *w, u8 *samples, u32 samples_count) {

  if(samples_count == 0) {
    return 1;
  }

  if(w->samples_len_bytes == 0) {
    Wav_Header h = __wav_header_default;
    
    h.bits_per_sample = __wav_format_to_bits_per_sample[w->format];
    h.block_align = w->channels * (h.bits_per_sample + 7/8) / 8;
    h.bytes_per_second = w->sample_rate * (u32) h.block_align;
    h.sample_rate = w->sample_rate;
    h.channels = w->channels;
    h.fmt_tag = w->format;

    if(!w->write(w->opaque, (u8 *) &h, sizeof(h))) {
      return 0;
    }
  }

  u32 bytes_to_write = samples_count * __wav_format_to_bytes_per_sample[w->format] * (u32) w->channels;
  w->samples_len_bytes += bytes_to_write;
  return w->write(w->opaque, samples, (u64) bytes_to_write);
  
}

// https://stackoverflow.com/questions/3553296/sizeof-single-struct-member-in-c
#define wav_member_size(type, member) (sizeof( ((type *)0)->member )) 

WAV_DEF int wav_encoder_finalize(Wav_Encoder *w) {

  // Maybe it is faster to write the header a second time,
  // instead of seek/write/seek/write.
  
  if(!w->seek(w->opaque, offsetof(Wav_Header, file_size))) {
    return 0;
  }
  u32 file_size = w->samples_len_bytes 
    - offsetof(Wav_Header, data_size)
    - wav_member_size(Wav_Header, RIFF)
    - wav_member_size(Wav_Header, data_size);  
  if(!w->write(w->opaque, (u8 *) &file_size, sizeof(u32))) {
    return 0;
  }

  if(!w->seek(w->opaque, offsetof(Wav_Header, data_size))) {
    return 0;
  }
  return w->write(w->opaque, (u8 *) &w->samples_len_bytes, sizeof(u32));
  
}

WAV_DEF int wav_decoder_open(Wav_Decoder *w,
			     void *opaque,
			     Wav_Read read,
			     Wav_Seek seek) {
  
  w->opaque = opaque;
  w->read = read;
  w->seek = seek;

  Wav_Header h;
  if(!w->read(w->opaque, (u8 *) &h, sizeof(h))) {
    return 0;
  }
  w->channels = h.channels;
  w->sample_rate = h.sample_rate;
  w->format = h.fmt_tag;

  if(wav_memcmp(h.data, "data", 4) != 0) {
    u32 size_to_skip = h.data_size;

    if(!w->seek(w->opaque, sizeof(h) + size_to_skip)) {
      return 0;
    }
    if(!w->read(w->opaque, (u8 *) &h.data, sizeof(h.data) + sizeof(h.data_size))) {
      return 0;
    }
  }

  w->samples_total = h.data_size / (__wav_format_to_bytes_per_sample[w->format] * (u32) w->channels);
  w->duration_secs = w->samples_total / w->sample_rate;
  w->samples_left = w->samples_total;

  return 1;
  
}

WAV_DEF int wav_decoder_decode(Wav_Decoder *w,
			       u8 *buffer,
			       u32 buffer_len_bytes,
			       u32 *samples_count) {

  u32 sample_size = __wav_format_to_bytes_per_sample[w->format] * (u32) w->channels;
  *samples_count = buffer_len_bytes / sample_size;
  if((*samples_count) > w->samples_left) {
    *samples_count = w->samples_left;
  }
  
  w->samples_left -= (*samples_count);

  if((*samples_count) == 0) {
    return 0;
  } else {
    return w->read(w->opaque, buffer, (u64) ((*samples_count) * sample_size));
  }  
  
}

WAV_DEF int wav_decoder_seek_to(Wav_Decoder *w, u32 to_second) {

  if(to_second > w->duration_secs) {
    return 0;
  }

  u32 sample_size = __wav_format_to_bytes_per_sample[w->format] * (u32) w->channels;
  u32 sample_to_seek_to = to_second * w->sample_rate;
  w->samples_left = w->samples_total - sample_to_seek_to;
  
  return w->seek(w->opaque, sizeof(Wav_Header) + sample_to_seek_to * sample_size);
}

#endif // WAV_IMPLEMENTATION

#undef s8
#undef u8
#undef u16
#undef s32
#undef u32
#undef u64

#endif // WAV_H
