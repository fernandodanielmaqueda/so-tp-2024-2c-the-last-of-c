/* En los archivos de cabecera (header files) (*.h) poner DECLARACIONES (evitar DEFINICIONES) de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#ifndef UTILS_TIMESPEC_H
#define UTILS_TIMESPEC_H

#include <time.h>

#define NSEC_PER_SEC 1000000000

struct timespec timespec_add(struct timespec ts1, struct timespec ts2);

/** \fn struct timespec timespec_sub(struct timespec ts1, struct timespec ts2)
 *  \brief Returns the result of subtracting ts2 from ts1.
*/
struct timespec timespec_sub(struct timespec ts1, struct timespec ts2);

/** \fn struct timespec timespec_from_ms(long milliseconds)
 *  \brief Converts an integer number of milliseconds to a timespec.
*/
struct timespec timespec_from_ms(long milliseconds);

/** \fn struct timespec timespec_normalise(struct timespec ts)
 *  \brief Normalises a timespec structure.
 *
 * Returns a normalised version of a timespec structure, according to the
 * following rules:
 *
 * 1) If tv_nsec is >=1,000,000,00 or <=-1,000,000,000, flatten the surplus
 *    nanoseconds into the tv_sec field.
 *
 * 2) If tv_nsec is negative, decrement tv_sec and roll tv_nsec up to represent
 *    the same value attainable by ADDING nanoseconds to tv_sec.
*/
struct timespec timespec_normalise(struct timespec ts);

#endif // UTILS_TIMESPEC_H