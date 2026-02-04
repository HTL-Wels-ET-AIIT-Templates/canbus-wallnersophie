#include "ringbuffer.h"

#ifndef NO_MEM_COPY
#include <string.h>
#else
#ifndef memcpy
static void *memcpy(void *dst, const void *src, uint32_t n)
{
   uint32_t i = 0;
   /* Verify if n, and the pointers are word aligned.
    * If it's word aligned copy by word.
    */
   if((uintptr_t)dst % sizeof(uint32_t) == 0 &&
      (uintptr_t)src % sizeof(uint32_t) == 0 &&
      n % sizeof(uint32_t) == 0) {

      uint32_t *d = dst;
      const uint32_t *s = src;

      for (i=0; i<n/sizeof(uint32_t); i++) {
         d[i] = s[i];
      }
   }
   else {

      char *d = dst;
      const char *s = src;

      for (i=0; i<n; i++) {
         d[i] = s[i];
      }
   }

   return dst;
}
#endif
#endif

/**
 * Initialize ring buffer. Function needs a "context" struct and an array to use for the ring buffer
 * @param buffer Pointer to "context" struct to be used for ring buffer organization
 * @param data array, which will be used as ring buffer
 * @param len length of array. Has to be a power of 2!
 * @return
 */
int ringBufferInit(RingBuffer_t *buffer, uint8_t *data, uint32_t len) {
   if(!(len && !(len & (len - 1)))) {
      return 0;
   }

   buffer->tail = 0;
   buffer->head = 0;
   buffer->sizeMask = len-1;
   buffer->data = data;
   return 1;
}

/***
 * Returns number of bytes currently stored in the ring buffer
 * @param buffer "context" struct
 * @return number of bytes stored in the ring buffer
 */
uint32_t ringBufferLen(RingBuffer_t *buffer) {
   if(buffer->tail >= buffer->head) {
      return buffer->tail-buffer->head;
   }

   return buffer->sizeMask-(buffer->head-buffer->tail)+1;
}

/**
 * Check if ring buffer is empty
 * @param buffer "context" struct
 * @return 1 if ring buffer is empty, otherwise 0
 */
uint8_t ringBufferEmpty(RingBuffer_t *buffer) {
   return (buffer->tail == buffer->head);
}

/**
 * Return the number of bytes, which are currently free in the ring buffer
 * @param buffer "context" struct
 * @return number of bytes, which are currently free in the ring buffer
 */
uint32_t ringBufferLenAvailable(RingBuffer_t *buffer){
   return buffer->sizeMask - ringBufferLen(buffer);
}

/**
 * Full Size of the ring buffer
 * @param buffer "context" struct
 * @return size of ring buffer in bytes
 */
uint32_t ringBufferMaxSize(RingBuffer_t *buffer) {
   return buffer->sizeMask;
}

/**
 * Append one byte to the ring buffer. Function does not check if free space is available!
 * @param buffer "context" struct
 * @param data data value to put into the ring buffer
 */
void ringBufferAppendOne(RingBuffer_t *buffer, uint8_t data){
   buffer->data[buffer->tail] = data;
   buffer->tail = (buffer->tail + 1) & buffer->sizeMask;
}

/**
 * Delete one byte from the ring buffer.
 * @param buffer "context" struct
 */
void ringBufferDeleteOne(RingBuffer_t *buffer){
   buffer->tail = (buffer->tail - 1) & buffer->sizeMask;
}

/**
 * Append multiple bytes to the ring buffer. Function does not check if free space is available!
 * @param buffer "context" struct
 * @param data array of bytes to be stored in the ring buffer
 * @param len length of array data
 */
void ringBufferAppendMultiple(RingBuffer_t *buffer, uint8_t *data, uint32_t len){
   if(buffer->tail + len > buffer->sizeMask) {
      uint32_t lenToTheEnd = buffer->sizeMask - buffer->tail + 1;
      uint32_t lenFromBegin = len - lenToTheEnd;
      memcpy(buffer->data + buffer->tail, data, lenToTheEnd);
      memcpy(buffer->data, data + lenToTheEnd, lenFromBegin);
   }
   else {
      memcpy(buffer->data, data, len);
   }
   buffer->tail = (buffer->tail + len) & buffer->sizeMask;
}

/**
 * Read the next byte from the ring buffer
 * @param buffer "context" struct
 * @return next byte to be read
 */
uint8_t ringBufferGetOne(RingBuffer_t *buffer){
   uint8_t data =  buffer->data[buffer->head];
   buffer->head = (buffer->head + 1) & buffer->sizeMask;
   return data;
}

/**
 * Read multiple bytes from the ring buffer.
 * @param buffer "context" struct
 * @param dst copy bytes from the ring buffer to this array
 * @param len Number of bytes to read (lenght of array dst)
 */
void ringBufferGetMultiple(RingBuffer_t *buffer, uint8_t *dst, uint32_t len) {
   ringBufferPeekMultiple(buffer, dst, len);
   buffer->head = (buffer->head + len) & buffer->sizeMask;
}

/**
 * Return the next to-be-read byte from the ring buffer, but do not advance the read pointer.
 * @param buffer "context" struct
 * @return next to-be-read byte
 */
uint8_t ringBufferPeekOne(RingBuffer_t *buffer){
   return buffer->data[buffer->head];
}

/**
 * Return multiple to-be-read bytes from the ring buffer, but do not advance the read pointer.
 * @param buffer "context" struct
 * @param dst copy bytes from the ring buffer to this array
 * @param len Number of bytes to read (lenght of array dst)
 */
void ringBufferPeekMultiple(RingBuffer_t *buffer, uint8_t *dst, uint32_t len){
   if(buffer->head + len > buffer->sizeMask) {
      uint32_t lenToTheEnd = buffer->sizeMask - buffer->head + 1;
      uint32_t lenFromBegin = len - lenToTheEnd;
      memcpy(dst, buffer->data + buffer->head, lenToTheEnd);
      memcpy(dst + lenToTheEnd, buffer->data, lenFromBegin);
   }
   else {
      memcpy(dst, buffer->data, len);
   }
}

/**
 * Delete number of bytes from the ring buffer (advance read pointer without reading)
 * @param buffer "context" struct
 * @param len number of bytes to discard
 */
void ringBufferDiscardMultiple(RingBuffer_t *buffer, uint32_t len){
   buffer->head = (buffer->head + len) + buffer->sizeMask;
}

/**
 * Delete all bytes from the ring buffer
 * @param buffer "context" struct
 */
void ringBufferClear(RingBuffer_t *buffer){
   buffer->head = buffer->tail = 0;
}
