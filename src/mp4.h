#ifndef MP4_H
#define MP4_H

//https://web.archive.org/web/20180219054429/http://l.web.umkc.edu/lizhu/teaching/2016sp.video-communication/ref/mp4.pdf

#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>
#include <stdbool.h>

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

#endif // TYPES_H

#ifndef MP4_DEF
#  define MP4_DEF static inline
#endif // MP4_DEF

#ifndef MP4_LOG
#  ifdef MP4_QUIET
#    define MP4_LOG(...)
#  else
#    include <stdio.h>
#    define MP4_LOG(...) fprintf(stderr, "MP4_LOG: "); fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n")
#  endif // MP4_QUIET
#endif // MP4_QUIET

typedef enum{
  MP4_TYPE_NONE = 0,
  MP4_TYPE_FTYP,
  MP4_TYPE_MOOV,
  MP4_TYPE_MVHD,
  MP4_TYPE_MVEX,
  MP4_TYPE_TREX,
  MP4_TYPE_TRAK,
  MP4_TYPE_TKHD,
  MP4_TYPE_MDIA,
  MP4_TYPE_MDHD,
  MP4_TYPE_HDLR,
  MP4_TYPE_MINF,
  MP4_TYPE_DINF,
  MP4_TYPE_DREF,
  MP4_TYPE_STBL,
  MP4_TYPE_STSD,
  MP4_TYPE_STTS,
  MP4_TYPE_STSC,
  MP4_TYPE_STCO,
  MP4_TYPE_STSZ,
  MP4_TYPE_SMHD,
  MP4_TYPE_SIDX,
  MP4_TYPE_MOOF,
  MP4_TYPE_MFHD,
  MP4_TYPE_TRAF,
  MP4_TYPE_TFHD,
  MP4_TYPE_TFDT,
  MP4_TYPE_TRUN,
  MP4_TYPE_MDAT,
  MP4_TYPE_UDTA,
}Mp4_Type;

typedef char Mp4_Word[5]; // 4 + '\0'

typedef enum {
  MP4_MEDIA_TYPE_NONE = 0,
  MP4_MEDIA_TYPE_AUDIO,
  MP4_MEDIA_TYPE_VIDEO,
}Mp4_Media_Type;

typedef enum {
  MP4_CODEC_TYPE_NONE = 0,
  MP4_CODEC_TYPE_AAC,
  MP4_CODEC_TYPE_H264,
  MP4_CODEC_TYPE_UNKNOWN,
}Mp4_Codec_Type;

typedef struct{
  u8 *data;
  u64 len;
}Mp4;

typedef struct{
  u32 track_id;  
  Mp4_Media_Type type;
  Mp4_Codec_Type codec;
  Mp4_Word coding_name;
  f64 duration; // in seconds
  
  bool tables_are_empty;

  u32 sample_count; // stsz
  Mp4 sample_sizes;

  u32 chunk_count; // stco
  u32 chunk_index;
  Mp4 chunk_offsets;
  
  u32 entry_count; // stsc
  Mp4 entries;
  u32 current_sample_count;

  //Audio
  u16 channels;
  u32 sample_rate;

  //Video
  u16 width, height;
}Mp4_Media;

#define mp4_from(data, len) (Mp4) {(data), (len)}

MP4_DEF bool mp4_next_media(Mp4 *m, Mp4_Media *media);
MP4_DEF bool mp4_has_audio(Mp4 m, Mp4_Media *media);
MP4_DEF bool mp4_next_fragmented_data(Mp4 *m, u32 track_id, u8 **data, u64 *data_len);
MP4_DEF bool mp4_media_next_data(Mp4_Media *media, u32 *offset, u64 *len);

MP4_DEF bool mp4_find(Mp4 m, Mp4_Type type, Mp4 *dest);
MP4_DEF bool mp4_find_next(Mp4 *m, Mp4_Type type, Mp4 *dest);
MP4_DEF bool mp4_next(Mp4 *m, u32 *size, Mp4_Type *type);

const char *mp4_type_name(Mp4_Type t);

#ifdef MP4_IMPLEMENTATION

// 0xaa 0xbb 0xcc 0xdd 0xee 0xff 0x11 0x22
//    |
//    v
// 0x22 0x11 0xff 0xee 0xdd 0xcc 0xbb 0xaa
static inline u64 mp4_swap_u64(u64 in) {
  return
    ((in & 0x00000000000000ff) << 56) |
    ((in & 0x000000000000ff00) << 48) |
    ((in & 0x0000000000ff0000) << 32) |
    ((in & 0x00000000ff000000) <<  8) |    
    ((in & 0x000000ff00000000) >>  8) |
    ((in & 0x0000ff0000000000) >> 32) |
    ((in & 0x00ff000000000000) >> 48) |
    ((in & 0xff00000000000000) >> 56);
}

// 0xaa 0xbb 0xcc 0xdd
//    |
//    v
// 0xdd 0xcc 0xbb 0xaa
static inline u32 mp4_swap_u32(u32 in) {
  return
    ((in & 0x000000ff) << 24) |
    ((in & 0x0000ff00) << 8) |
    ((in & 0x00ff0000) >> 8) |
    ((in & 0xff000000) >> 24);
}

// 0xaa 0xbb
// |
// v
// 0xbb 0xaa
static inline u16 mp4_swap_u16(u16 in) {
  return
    ((in & 0x00ff) << 8) |
    ((in & 0xff00) >> 8);
}

#define __MP4_READ(ptr, m, n) do{					\
    if((m).len < (n)) {							\
      MP4_LOG("Unexpected eof");					\
      return false;							\
    }									\
    memcpy((ptr), (m).data, (n)); (m).data += (n); (m).len -= (n);	\
  }while(0)
#define __MP4_DISCARD(m, n) do{			\
    if((m).len < (n)) {				\
      MP4_LOG("Unexpected eof");		\
      return false;				\
    }						\
    (m).data += (n); (m).len -= (n);		\
  }while(0)
#define __MP4_READ_U64(ptr, m) do{		\
    __MP4_READ(ptr, m, 8);			\
    *ptr = mp4_swap_u64(*(ptr));		\
  }while(0)
#define __MP4_READ_U32(ptr, m) do{		\
    __MP4_READ(ptr, m, 4);			\
    *ptr = mp4_swap_u32(*(ptr));		\
  }while(0)
#define __MP4_READ_U16(ptr, m) do{		\
    __MP4_READ(ptr, m, 2);			\
    *ptr = mp4_swap_u16(*(ptr));		\
  }while(0)


#ifndef return_defer
#  define return_defer(n) do{ result = (n); goto defer; }while(0)
#endif // return_defer

static const Mp4_Word mp4_box_names[] = {
  "NONE", "ftyp", "moov", "mvhd", "mvex", "trex", "trak", "tkhd", "mdia", "mdhd", "hdlr", "minf", "dinf", "dref", "stbl", "stsd", "stts", "stsc", "stco", "stsz", "smhd", "sidx", "moof", "mfhd", "traf", "tfhd", "tfdt", "trun", "mdat", "udta"
};

static u64 mp4_box_names_len = sizeof(mp4_box_names) / sizeof(*mp4_box_names);

#define mp4_type_name(t) mp4_box_names[(t)]

MP4_DEF bool mp4_next_media(Mp4 *m, Mp4_Media *media) {

  u32 size;
  Mp4_Type type;
  if(!mp4_next(m, &size, &type)) {
    return false;
  }

  Mp4 trak;
  if(type == MP4_TYPE_FTYP) {        // Beginning of FILE. Advance to 'moov'. Advance to 'trak'
    if(!mp4_find(*m, MP4_TYPE_MOOV, m)) {
      MP4_LOG("Can not find 'moov' in mp4. Mp4-File is propably damaged");
      return false;
    }

    if(!mp4_find_next(m, MP4_TYPE_TRAK, &trak)) {
      return false;
    }
  } else if(type == MP4_TYPE_TRAK) { // Inside 'moov'. Update 'trak'
    trak = mp4_from(m->data - size + 8, size - 8);
  } else {                           // Inside 'moov'. Advance to 'trak'
    if(!mp4_find_next(m, MP4_TYPE_TRAK, &trak)) {
      return false;
    }
  }

  Mp4 tkhd;
  if(!mp4_find(trak, MP4_TYPE_TKHD, &tkhd)) {
    MP4_LOG("Can not find 'tkhd' inside 'trak'. Mp4-File is propably damaged");
    return false;
  }

  // 'TrackHeaderBox'
  // only look for track_id, for now
  u8 version;
  __MP4_READ(&version, tkhd, 1);

  if(version == 1) {
    __MP4_DISCARD(tkhd, 3 + 8 + 8);
  } else { // version == 0
    __MP4_DISCARD(tkhd, 3 + 4 + 4);
  }
      
  __MP4_READ_U32(&media->track_id, tkhd);

  Mp4 mdia;
  if(!mp4_find(trak, MP4_TYPE_MDIA, &mdia)) {
    MP4_LOG("Can not find 'mdia' inside 'trak'. Mp4-File is propably damaged");
    return false;
  }

  Mp4 mdhd;
  if(!mp4_find(mdia, MP4_TYPE_MDHD, &mdhd)) {
    MP4_LOG("Can not find 'mdhd' inside 'mdia'. Mp4-File is propably damaged");
    return false;
  }

  // 'MediaHeaderBox'
  // only look for duration, for now

  __MP4_READ(&version, mdhd, 1);
  
  u32 timescale;    //5198848  //44100
  if(version == 1) {
    __MP4_DISCARD(mdhd, 3 + 8 + 8);
    __MP4_READ_U32(&timescale, mdhd);
    
    u64 duration;
    __MP4_READ_U64(&duration, mdhd);
    media->duration = (f64) duration / (f64) timescale;
  } else { // version == 0
    __MP4_DISCARD(mdhd, 3 + 4 + 4);
    __MP4_READ_U32(&timescale, mdhd);
    
    u32 duration;
    __MP4_READ_U32(&duration, mdhd);
    media->duration = (f64) duration / (f64) timescale;
  }  

  Mp4 hdlr;
  if(!mp4_find(mdia, MP4_TYPE_HDLR, &hdlr)) {
    MP4_LOG("Can not find 'hdlr' inside 'mdia'. Mp4-File is propably damaged");
    return false;
  }
  
  // 'HandlerBox'
  // only look for handler_type, for now
  __MP4_DISCARD(hdlr, 4 + 4);

  u8 buf[4];
  __MP4_READ(buf, hdlr, 4);
  if(memcmp(buf, "soun", 4) == 0) {
    media->type = MP4_MEDIA_TYPE_AUDIO;
  } else if(memcmp(buf, "vide", 4) == 0) {
    media->type = MP4_MEDIA_TYPE_VIDEO;
  } else {
    MP4_LOG("Unknown handler_type: '%.*s'", 4, buf);
    return false;
  }

  Mp4 minf;
  if(!mp4_find(mdia, MP4_TYPE_MINF, &minf)) {
    MP4_LOG("Can not find 'minf' inside 'mdia'. Mp4-File is propably damaged");
  }

  Mp4 stbl;
  if(!mp4_find(minf, MP4_TYPE_STBL, &stbl)) {
    MP4_LOG("Can not find 'stbl' inside 'minf'. Mp4-File is propably damaged");
  }

  Mp4 stsd;
  if(!mp4_find(stbl, MP4_TYPE_STSD, &stsd)) {
    MP4_LOG("Can not find 'stsd' inside 'stbl'. Mp4-File is propably damaged");
  }

  // 'SampleDescriptionBox'
  // This my be more complicated than this. A Sample Description Box (stsd) may contain
  // multiple entries. This considers only the first one.
  
  // Depending on the 'coding_name', the 'AudioSampleEntry' or 'VideoSampleEntry' may
  // contain more box's. 

  __MP4_DISCARD(stsd, 4);
  u32 entry_count;
  __MP4_READ_U32(&entry_count, stsd);
  if(entry_count != 1) {
    MP4_LOG("There are multiple entries in the Sample Description Box (stsd). entry_count: %u",
	    entry_count);
  }
  
  if(media->type == MP4_MEDIA_TYPE_AUDIO) {
    // 'AudioSampleEntry'
    // only look for coding_name, channel_count and sample_rate, for now
            
    __MP4_DISCARD(stsd, 4);
    __MP4_READ(media->coding_name, stsd, 4);
    media->coding_name[4] = '\0';
    __MP4_DISCARD(stsd, 6 + 2 + 8);
    __MP4_READ_U16(&media->channels, stsd);
    __MP4_DISCARD(stsd, 4 + 2);
    __MP4_READ_U32(&media->sample_rate, stsd);
    media->sample_rate >>= 16;
    
  } else if(media->type == MP4_MEDIA_TYPE_VIDEO) {
    // 'VideoSampleEntry'
    // only look for codingname, width, and height, for now

    __MP4_DISCARD(stsd, 4);
    __MP4_READ(media->coding_name, stsd, 4);
    media->coding_name[4] = '\0';
    __MP4_DISCARD(stsd, 6 + 2 + 4 + 12);
    __MP4_READ_U16(&media->width, stsd);
    __MP4_READ_U16(&media->height, stsd);
    __MP4_DISCARD(stsd, 4 + 4 + 4 + 2);
    
  } else {
    MP4_LOG("Unreachable state. Update your code");
  }

  // TODO: make this an cstr-array. Just like mp4-boxs
  if(strcmp(media->coding_name, "mp4a") == 0) {
    media->codec = MP4_CODEC_TYPE_AAC;
  } else if(strcmp(media->coding_name, "avc1") == 0) {
    media->codec = MP4_CODEC_TYPE_H264;
  } else {
    media->codec = MP4_CODEC_TYPE_UNKNOWN;
    MP4_LOG("Unknown coding_name: '%s'", media->coding_name);
  }

  Mp4 stsz;
  if(!mp4_find(stbl, MP4_TYPE_STSZ, &stsz)) {
    MP4_LOG("Can not find 'stsz' inside 'stbl'. Mp4-File is propably damaged");
  }

  // 'SampleSizeBox'
  // only look for sample_count and sample_sizes, for now

  __MP4_DISCARD(stsz, 4);
  u32 sample_size;
  __MP4_READ_U32(&sample_size, stsz);
  __MP4_READ_U32(&media->sample_count, stsz);

  if(sample_size != 0) {
    MP4_LOG("Unimplemented: sample_size != 0, is not handled inside 'SampleSizeBox'");
    return false;
  }
  if(media->sample_count > 0) {
    media->sample_sizes = mp4_from(stsz.data, media->sample_count * 4);
  }

  Mp4 stco;
  if(!mp4_find(stbl, MP4_TYPE_STCO, &stco)) {
    MP4_LOG("Can not find 'stco' inside 'stbl'. Mp4-File is propably damaged");
  }

  // 'ChunkOffsetBox'
  // look for entry_count and chunk_offset's

  __MP4_DISCARD(stco, 4);
  __MP4_READ_U32(&media->chunk_count, stco);
  if(media->chunk_count > 0) {
    media->chunk_index = 0;
    media->chunk_offsets = mp4_from(stco.data, media->chunk_count * 4);
  }

  Mp4 stsc;
  if(!mp4_find(stbl, MP4_TYPE_STSC, &stsc)) {
    MP4_LOG("Can not find 'stco' inside 'stbl'. Mp4-File is propably damaged");
  }

  // 'SampleToChunkBox'
  // look for entry_count and entries

  __MP4_DISCARD(stsc, 4);
  __MP4_READ_U32(&media->entry_count, stsc);
  if(media->entry_count > 0) {
    media->entries = mp4_from(stsc.data, media->entry_count * 4 * 3);
    __MP4_DISCARD(media->entries, 4);
    __MP4_READ_U32(&media->current_sample_count, media->entries);
    __MP4_DISCARD(media->entries, 4);

  }

  // Assume that you than can find 'moof's and 'mdat's
  media->tables_are_empty =
    media->sample_count == 0 &&
    media->entry_count == 0 &&
    media->chunk_count == 0;

  return true;
}

MP4_DEF bool mp4_has_audio(Mp4 m, Mp4_Media *media) {

  media->type = MP4_MEDIA_TYPE_NONE;
  while(media->type != MP4_MEDIA_TYPE_AUDIO && mp4_next_media(&m, media)) ;
  if(media->type == MP4_MEDIA_TYPE_AUDIO) {
    return true;
  } else {
    return false;
  }
}

MP4_DEF bool mp4_next_fragmented_data(Mp4 *m, u32 track_id, u8 **data, u64 *data_len) {

  u32 current_track_id;
  while(m->len) {
    Mp4 moof;
    if(!mp4_find_next(m, MP4_TYPE_MOOF, &moof)) {
      return false;
    }

    Mp4 traf;
    if(!mp4_find(moof, MP4_TYPE_TRAF, &traf)) {
      MP4_LOG("Exepcted 'traf' inside 'moof'");
      return false;
    }

    Mp4 tfhd;
    if(!mp4_find(traf, MP4_TYPE_TFHD, &tfhd)) {
      MP4_LOG("Can not find 'tfhd' inside 'traf'. Mp4-File is propably damaged");
      return false;
    }

    // only look for handler_type, for now
    __MP4_DISCARD(tfhd, 4);
    __MP4_READ_U32(&current_track_id, tfhd);

    if(current_track_id == track_id) {
      break;
    }
  }
  if(!m->len) {
    return false;
  }

  Mp4 mdat;
  if(!mp4_find_next(m, MP4_TYPE_MDAT, &mdat)) {
    return false;
  }
  *data = mdat.data;
  *data_len = mdat.len;
  
  return true;
}

MP4_DEF bool mp4_media_next_data(Mp4_Media *media, u32 *offset, u64 *len) {
  if(media->chunk_index == media->chunk_count) {
    return false;
  }

  // Consume chunk_offset  
  __MP4_READ_U32(offset, media->chunk_offsets);
  media->chunk_index++; 

  // Peek sample_entry
  u32 first_chunk, sample_count;
  bool could_peek = media->entries.len > 0;
  if(could_peek) {
    __MP4_READ_U32(&first_chunk, media->entries);
    __MP4_READ_U32(&sample_count, media->entries);
    __MP4_DISCARD(media->entries, 4);
  }

  // Look at fist 'sample_entry'.
  if( could_peek &&
      media->chunk_index < first_chunk ) {   // 'first_chunk' is indexed from 1

    // If 'first_chunk' applies to 'chunk_index',
    // then 'media.current_sample_count' is valid

    media->entries.data -= 12; media->entries.len += 12;
  } else {

    // If 'first_chunk' does not apply to 'chunk_index',
    // then update 'media.current_sample_count'.

    media->current_sample_count = sample_count;
  }

  *len = 0;
  u32 sample_size;
  for(u32 i=0;i<media->current_sample_count;i++) {
    if(media->sample_count == 0) {
      MP4_LOG("Media.sample_count underflows. Something is wrong");
      return false;
    }

    __MP4_READ_U32(&sample_size, media->sample_sizes);
    media->sample_count--;
    *len += (u64) sample_size;
  }

  return true;
}

MP4_DEF bool mp4_find(Mp4 m, Mp4_Type type, Mp4 *dest) {
  Mp4_Type t = MP4_TYPE_NONE;

  u32 size = 0;
  while(t != type && mp4_next(&m, &size, &t)) ;
  if(t != type) {
    return false;
  }

  *dest = mp4_from(m.data - size + 8, size - 8);
  return true;
}

MP4_DEF bool mp4_find_next(Mp4 *m, Mp4_Type type, Mp4 *dest) {
  Mp4_Type t = MP4_TYPE_NONE;

  u32 size = 0;
  while(t != type && mp4_next(m, &size, &t)) ;
  if(t != type) {
    return false;
  }

  *dest = mp4_from(m->data - size + 8, size - 8);
  return true;

}

MP4_DEF bool mp4_next(Mp4 *m, u32 *size, Mp4_Type *t) {

  if(m->len < 8) {
    return false;
  }

  // Get Box-Size
  memcpy(size, m->data, 4);
  *size = mp4_swap_u32(*size);

  // Get Box-Name
  Mp4_Word word;
  memcpy(word, m->data + 4, 4);

  // Advance
  m->data += *size;
  m->len  -= *size;    

  // Lookup Box-Name
  bool found = false;
  for(u64 i=0;!found && i<mp4_box_names_len;i++) {
    if(memcmp(mp4_box_names[i], word, 4) == 0) {
      *t = (Mp4_Type) i;
      found = true;
    }
  }

  if(!found) {
    MP4_LOG("Unknown MP4-Box: '%.*s'", 4, word);
  }
  
  return found;
}

#endif // MP4_IMPLEMENTATION

#endif // MP4_H
