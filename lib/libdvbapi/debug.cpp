/* debug functions */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/prctl.h>
#include <string.h>

int debuglevel = 0;

static const char *dvbapi_facility[] =
{
	"audio ",
	"video ",
	"demux ",
	"play  ",
	"cec   ",
	"init  ",
	"ca    ",
	"record",
	NULL
};

void _dvbapi_info(int facility, const void *func, const char *fmt, ...)
{
	/* %p does print "(nil)" instead of 0x00000000 for NULL */
	fprintf(stderr, "[DVBAPI:%08lx:%s] ", (long) func, dvbapi_facility[facility]);
	va_list args;
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
}


void _dvbapi_debug(int facility, const void *func, const char *fmt, ...)
{
	if (debuglevel < 0)
		fprintf(stderr, "dvbapi_debug: debuglevel not initialized!\n");

	if (!((1 << facility) & debuglevel))
		return;

	fprintf(stderr, "[DVBAPI:%08lx:%s] ", (long)func, dvbapi_facility[facility]);
	va_list args;
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
}

void dvbapi_debug_init(void)
{
	int i = 0;
	char *tmp = getenv("DVBAPI_DEBUG");
	if (! tmp)
		debuglevel = 0;
	else
		debuglevel = (int) strtol(tmp, NULL, 0);

	if (debuglevel == 0)
	{
		fprintf(stderr, "dvbapi debug options can be set by exporting DVBAPI_DEBUG.\n");
		fprintf(stderr, "The following values (or bitwise OR combinations) are valid:\n");
		while (dvbapi_facility[i])
		{
			fprintf(stderr, "\tcomponent: %s  0x%02x\n", dvbapi_facility[i], 1 << i);
			i++;
		}
		fprintf(stderr, "\tall components:    0x%02x\n", (1 << i) - 1);
	}
	else
	{
		fprintf(stderr, "dvbapi debug is active for the following components:\n");
		while (dvbapi_facility[i])
		{
			if (debuglevel & (1 << i))
				fprintf(stderr, "%s ", dvbapi_facility[i]);
			i++;
		}
		fprintf(stderr, "\n");
	}
}

void dvbapi_set_threadname(const char *name)
{
	char threadname[17];
	strncpy(threadname, name, sizeof(threadname));
	threadname[16] = 0;
	prctl(PR_SET_NAME, (unsigned long)&threadname);
}
