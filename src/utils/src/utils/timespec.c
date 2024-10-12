/* En los archivos (*.c) se pueden poner tanto DECLARACIONES como DEFINICIONES de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#include "timespec.h"

struct timespec timespec_add(struct timespec ts1, struct timespec ts2) {
	/* Normalise inputs to prevent tv_nsec rollover if whole-second values
	 * are packed in it.
	*/
	ts1 = timespec_normalise(ts1);
	ts2 = timespec_normalise(ts2);
	
	ts1.tv_sec  += ts2.tv_sec;
	ts1.tv_nsec += ts2.tv_nsec;
	
	return timespec_normalise(ts1);
}

struct timespec timespec_sub(struct timespec ts1, struct timespec ts2) {
	/* Normalise inputs to prevent tv_nsec rollover if whole-second values
	 * are packed in it.
	*/
	ts1 = timespec_normalise(ts1);
	ts2 = timespec_normalise(ts2);

	ts1.tv_sec  -= ts2.tv_sec;
	ts1.tv_nsec -= ts2.tv_nsec;
	
	return timespec_normalise(ts1);
}

struct timespec timespec_from_ms(long milliseconds) {
	struct timespec ts = {
		.tv_sec  = (milliseconds / 1000),
		.tv_nsec = (milliseconds % 1000) * 1000000,
	};
	
	return timespec_normalise(ts);
}

struct timespec timespec_normalise(struct timespec ts) {
	while(ts.tv_nsec >= NSEC_PER_SEC)
	{
		++(ts.tv_sec);
		ts.tv_nsec -= NSEC_PER_SEC;
	}
	
	while(ts.tv_nsec <= -NSEC_PER_SEC)
	{
		--(ts.tv_sec);
		ts.tv_nsec += NSEC_PER_SEC;
	}
	
	if(ts.tv_nsec < 0)
	{
		/* Negative nanoseconds isn't valid according to POSIX.
		 * Decrement tv_sec and roll tv_nsec over.
		*/
		
		--(ts.tv_sec);
		ts.tv_nsec = (NSEC_PER_SEC + ts.tv_nsec);
	}
	
	return ts;
}