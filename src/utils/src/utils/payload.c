/* En los archivos (*.c) se pueden poner tanto DECLARACIONES como DEFINICIONES de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#include "payload.h"

void payload_init(t_Payload *payload) {
  if(payload == NULL)
    return;
  
  payload->size = 0;
  payload->stream = NULL;
  payload->offset = 0;
}

void payload_destroy(t_Payload *payload) {
  if(payload == NULL)
    return;
  
  free(payload->stream);

  payload->size = 0;
  payload->stream = NULL;
  payload->offset = 0;
}

int payload_add(t_Payload *payload, void *source, size_t sourceSize) {
  if (payload == NULL || sourceSize == 0 || sourceSize > (SIZE_MAX - payload->size)) {
    errno = EINVAL;
    return 1;
  }

  void *newStream = realloc(payload->stream, payload->size + sourceSize);
  if(newStream == NULL) {
    errno = ENOMEM;
    return 1;
  }
  payload->stream = newStream;

  if(payload->offset < payload->size)
    memmove((void *) (((uint8_t *) payload->stream) + payload->offset + sourceSize), (void *) (((uint8_t *) payload->stream) + payload->offset), payload->size - payload->offset);

  if(source != NULL)
    memcpy((void *) (((uint8_t *) payload->stream) + payload->offset), source, sourceSize);
  
  payload->offset += sourceSize;

  payload->size += sourceSize;

  return 0;
}

int payload_remove(t_Payload *payload, void *destination, size_t destinationSize) {
  if(payload == NULL || payload->stream == NULL || destinationSize == 0 || destinationSize > (payload->size - payload->offset) ) {
    errno = EINVAL;
    return 1;
  }

  if(destination != NULL)
    memcpy(destination, (void *) (((uint8_t *) payload->stream) + payload->offset), destinationSize);

  size_t newSize = payload->size - destinationSize;
  if(newSize > 0) {

    memmove(
      (void *) (((uint8_t *) payload->stream) + payload->offset),
      (void *) (((uint8_t *) payload->stream) + payload->offset + destinationSize),
      payload->size - payload->offset - destinationSize
    );

    void *newStream = realloc(payload->stream, newSize);
    if(newStream == NULL) {
      errno = ENOMEM;
      return 1;
    }

    payload->stream = newStream;
    payload->size = newSize;
  }
  else {
    payload_destroy(payload);
  }

  return 0;
}

int payload_write(t_Payload *payload, void *source, size_t sourceSize) {
  if (payload == NULL || sourceSize == 0) {
    errno = EINVAL;
    return 1;
  }

  if(sourceSize > (payload->size - payload->offset)) {

    if(sourceSize > (SIZE_MAX - payload->offset)) {
      errno = EINVAL;
      return 1;
    }

    void *newStream = realloc(payload->stream, payload->offset + sourceSize);
    if(newStream == NULL) {
      errno = ENOMEM;
      return 1;
    }

    payload->stream = newStream;
    payload->size = payload->offset + sourceSize;
  }

  if(source != NULL)
    memcpy((void *) (((uint8_t *) payload->stream) + payload->offset), source, sourceSize);
  
  payload->offset += sourceSize;

  return 0;
}

int payload_read(t_Payload *payload, void *destination, size_t destinationSize) {
  if (payload == NULL || payload->stream == NULL || destination == NULL || destinationSize == 0 || destinationSize > (payload->size - payload->offset)) {
    errno = EINVAL;
    return 1;
  }

  memcpy(destination, (void *) (((uint8_t *) payload->stream) + payload->offset), destinationSize);

  payload->offset += destinationSize;

  return 0;
}

int payload_seek(t_Payload *payload, long offset, int whence) {
  if(payload == NULL) {
    errno = EINVAL;
    return 1;
  }

  switch(whence) {

    case SEEK_SET:
      if(offset < 0 || (size_t) offset > payload->size) {
        errno = EINVAL;
        return 1;
      }
      payload->offset = (size_t) offset;
      break;

    case SEEK_CUR:
      if(offset < 0) {
        if((size_t) (-offset) > payload->offset) {
          errno = EINVAL;
          return 1;
        }
        payload->offset -= (size_t) (-offset);
      }
      else {
        if(((size_t) offset > (SIZE_MAX - payload->offset)) || ((payload->offset + (size_t) offset) > payload->size)) {
          errno = EINVAL;
          return 1;
        }
        payload->offset += (size_t) offset;
      }
      break;

    case SEEK_END:
      if(offset > 0 || (size_t) (-offset) > payload->size) {
        errno = EINVAL;
        return 1;
      }
      payload->offset = payload->size - (size_t) (-offset);
      break;

    default:
      errno = EINVAL;
      return 1;

  }

  return 0;
}