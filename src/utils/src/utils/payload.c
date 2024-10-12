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
  if(sourceSize == 0) {
    return 0;
  }

  if(payload == NULL) {
    log_warning(SERIALIZE_LOGGER, "payload_add: %s", strerror(EINVAL));
    errno = EINVAL;
    return -1;
  }

  if(sourceSize > (SIZE_MAX - payload->size)) {
    log_warning(SERIALIZE_LOGGER, "payload_add: %s", strerror(ERANGE));
    errno = ERANGE;
    return -1;
  }

  pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
    void *newStream = realloc(payload->stream, payload->size + sourceSize);
    if(newStream == NULL) {
      pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
      log_warning(SERIALIZE_LOGGER, "realloc: No se pudo redimensionar de %zu bytes a %zu bytes", payload->size, payload->size + sourceSize);
      errno = ENOMEM;
      return -1;
    }
    payload->stream = newStream;
    payload->size += sourceSize;
  pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

  if(payload->offset < (payload->size - sourceSize))
    memmove((void *) (((uint8_t *) payload->stream) + payload->offset + sourceSize), (void *) (((uint8_t *) payload->stream) + payload->offset), payload->size - sourceSize - payload->offset);

  if(source != NULL)
    memcpy((void *) (((uint8_t *) payload->stream) + payload->offset), source, sourceSize);
  
  payload->offset += sourceSize;

  return 0;
}

int payload_remove(t_Payload *payload, void *destination, size_t destinationSize) {
  if(destinationSize == 0) {
    return 0;
  }

  if(payload == NULL || payload->stream == NULL) {
    log_warning(SERIALIZE_LOGGER, "payload_remove: %s", strerror(EINVAL));
    errno = EINVAL;
    return -1;
  }

  if(destinationSize > (payload->size - payload->offset)) {
    log_warning(SERIALIZE_LOGGER, "payload_remove: %s", strerror(EDOM));
    errno = EDOM;
    return -1;
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

    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
      void *newStream = realloc(payload->stream, newSize);
      if(newStream == NULL) {
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
        log_warning(SERIALIZE_LOGGER, "realloc: No se pudo redimensionar de %zu bytes a %zu bytes", payload->size, newSize);
        errno = ENOMEM;
        return -1;
      }

      payload->stream = newStream;
      payload->size = newSize;
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
  }
  else {
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
      payload_destroy(payload);
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
  }

  return 0;
}

int payload_write(t_Payload *payload, void *source, size_t sourceSize) {
  if(sourceSize == 0) {
    return 0;
  }

  if(payload == NULL) {
    log_warning(SERIALIZE_LOGGER, "payload_write: %s", strerror(EINVAL));
    errno = EINVAL;
    return -1;
  }

  if(sourceSize > (payload->size - payload->offset)) {

    if(sourceSize > (SIZE_MAX - payload->offset)) {
      log_warning(SERIALIZE_LOGGER, "payload_write: %s", strerror(ERANGE));
      errno = ERANGE;
      return -1;
    }

    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
      void *newStream = realloc(payload->stream, payload->offset + sourceSize);
      if(newStream == NULL) {
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
        log_warning(SERIALIZE_LOGGER, "realloc: No se pudo redimensionar de %zu bytes a %zu bytes", payload->size, payload->offset + sourceSize);
        errno = ENOMEM;
        return -1;
      }

      payload->stream = newStream;
      payload->size = payload->offset + sourceSize;
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
  }

  if(source != NULL)
    memcpy((void *) (((uint8_t *) payload->stream) + payload->offset), source, sourceSize);
  
  payload->offset += sourceSize;

  return 0;
}

int payload_read(t_Payload *payload, void *destination, size_t destinationSize) {
  if(destinationSize == 0) {
    return 0;
  }

  if(payload == NULL || payload->stream == NULL || destination == NULL) {
    log_warning(SERIALIZE_LOGGER, "payload_read: %s", strerror(EINVAL));
    errno = EINVAL;
    return -1;
  }

  if(destinationSize > (payload->size - payload->offset)) {
    log_warning(SERIALIZE_LOGGER, "payload_read: %s", strerror(EDOM));
    errno = EDOM;
    return -1;
  }

  memcpy(destination, (void *) (((uint8_t *) payload->stream) + payload->offset), destinationSize);

  payload->offset += destinationSize;

  return 0;
}

int payload_seek(t_Payload *payload, long offset, int whence) {
  if(payload == NULL) {
    log_warning(SERIALIZE_LOGGER, "payload_seek: %s", strerror(EINVAL));
    errno = EINVAL;
    return -1;
  }

  switch(whence) {

    case SEEK_SET:
      if(offset < 0) {
        log_warning(SERIALIZE_LOGGER, "payload_seek: %s", strerror(EINVAL));
        errno = EINVAL;
        return -1;
      }
      if((size_t) offset > payload->size) {
        log_warning(SERIALIZE_LOGGER, "payload_seek: %s", strerror(EDOM));
        errno = EDOM;
        return -1;
      }
      payload->offset = (size_t) offset;
      break;

    case SEEK_CUR:
      if(offset < 0) {
        if((size_t) (-offset) > payload->offset) {
          log_warning(SERIALIZE_LOGGER, "payload_seek: %s", strerror(EDOM));
          errno = EDOM;
          return -1;
        }
        payload->offset -= (size_t) (-offset);
      }
      else {
        if((size_t) offset > (SIZE_MAX - payload->offset)) {
          log_warning(SERIALIZE_LOGGER, "payload_seek: %s", strerror(ERANGE));
          errno = ERANGE;
          return -1;
        }
        if((payload->offset + (size_t) offset) > payload->size) {
          log_warning(SERIALIZE_LOGGER, "payload_seek: %s", strerror(EDOM));
          errno = EDOM;
          return -1;
        }
        payload->offset += (size_t) offset;
      }
      break;

    case SEEK_END:
      if(offset > 0) {
        log_warning(SERIALIZE_LOGGER, "payload_seek: %s", strerror(EINVAL));
        errno = EINVAL;
        return -1;
      }
      if((size_t) (-offset) > payload->size) {
        log_warning(SERIALIZE_LOGGER, "payload_seek: %s", strerror(EDOM));
        errno = EDOM;
        return -1;
      }
      payload->offset = payload->size - (size_t) (-offset);
      break;

    default:
      log_warning(SERIALIZE_LOGGER, "payload_seek: %s", strerror(EINVAL));
      errno = EINVAL;
      return -1;

  }

  return 0;
}