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

#define WAV_FMT_PCM 0x1
#define WAV_FMT_IEEE 0x3

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
WAV_DEF bool wav_slurp(Wav *wav_header, const char *filepath, unsigned char **data, uint64_t *size);
WAV_DEF bool wav_read(Wav *wav_header, unsigned char *memory, size_t memory_len, unsigned char **data, uint64_t *size);

#ifdef WAV_IMPLEMENTATION

WAV_DEF bool wav_slurp(Wav *wav_header, const char *filepath, unsigned char **data, uint64_t *size) {
  FILE* f = fopen(filepath, "rb");
  if(!f) {
    return false;
  }

  size_t n = fread(wav_header, sizeof(*wav_header), 1, f);
  if(n != 1) {
    fclose(f);
    return false;
  }

  uint64_t off = 32;
  *size = (uint64_t) wav_header->chunkSize - sizeof(*wav_header) - off;
  *data = (unsigned char *) malloc(*size);
  if(!(*data)) {
    fclose(f);
    return false;
  }
  fseek(f, (long) off, SEEK_CUR);
  size_t m = fread(*data, 1, *size, f);
  if(m != *size) {
    fclose(f);
    return false;
  }

  fclose(f);
  return true;
}

WAV_DEF bool wav_read(Wav *wav_header, unsigned char *memory, size_t memory_len, unsigned char **data, uint64_t *size) {

  size_t header_size = sizeof(*wav_header);

  if(memory_len < header_size) {
    return false;
  }
  
  memcpy(wav_header, memory, header_size);
  
  memory += header_size;
  memory_len -= header_size;
  
  uint64_t off = 32;
  memory += off;
  memory_len -= off;

  *size = (uint64_t) wav_header->chunkSize - header_size - off;
  *data = memory;  

  return true;
}

#endif //WAV_IMPLEMENTATION

#endif //WAV_H
