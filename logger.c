#include <errno.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <syslog.h>
#include <execinfo.h>

#include "logger.h"

struct nvmecli_logger g_logger = { NULL, 0, NULL, 0 };

static void append_stacktrace(FILE *fp, int frames_to_chop)
{
	enum { BT_SIZE = 256 };

	void* stack_trace[BT_SIZE];
	char** traces;
	int st_size;
    int i;

	fprintf(fp, "\n=== nvme-cli stack trace ===\n");

	if ((st_size = backtrace(stack_trace, BT_SIZE)) < 0) {
		fprintf(fp, "backtrace() failed. no stack trace");
		return;
	}

	if ((traces = backtrace_symbols(stack_trace, st_size)) == NULL) {
		fprintf(fp, "backtrace_symbols() failed. no stack trace");
		return;
	}

	for (i = frames_to_chop; i < st_size; ++i) {
		if (traces[i])
			fprintf(fp, "(%d) %s\n", i, traces[i]);
	}

	free(traces);
}

static void append_location_info(FILE *fp, const struct nvmecli_location *loc)
{
	fprintf(fp, " in %s:%s():L%d ", loc->fname, loc->func, loc->line);
}

static void append_perror(FILE *fp, int os_error)
{
    fprintf(fp, ": %s", strerror(os_error));
}

/*
 * From syslog.h
 *
 * LOG_EMERG	:	system is unusable
 * LOG_ALERT	:	action must be taken immediately
 * LOG_CRIT		:	critical conditions
 * LOG_ERR		:	error conditions
 * LOG_WARNING	:	warning conditions
 * LOG_NOTICE	:	normal but significant condition
 * LOG_INFO :	:	informational
 * LOG_DEBUG	:	debug-level messages
 */
static int sysloglevel(const struct nvmecli_location *loc)
{
	switch (loc->level) {
	case NVMECLI_LOGLEVEL_FATAL:
		return LOG_CRIT;
	case NVMECLI_LOGLEVEL_ERROR:
		return LOG_ERR;
	case NVMECLI_LOGLEVEL_WARN:
		return LOG_WARNING;
	case NVMECLI_LOGLEVEL_NOTICE:
		return LOG_NOTICE;
	case NVMECLI_LOGLEVEL_INFO:
		return LOG_INFO;
	case NVMECLI_LOGLEVEL_DEBUG:
	case NVMECLI_LOGLEVEL_TRACE:
	default:
		return LOG_DEBUG;
	}
}

static void send_syslog(char *message, const char *pfx, int level)
{
	char *line, *saveptr;

	line = strtok_r(message, "\n", &saveptr);
	while (line) {
		syslog(level | LOG_USER, "%s %s", pfx, line);
		line = strtok_r(NULL, "\n", &saveptr);
        pfx = "   ";
	}
}

int nvmecli_open_logger()
{
	g_logger.memstream = open_memstream(&g_logger.message, &g_logger.msglen);
	if (!g_logger.memstream)
		return errno;

	g_logger.logging_threshold = NVMECLI_LOGLEVEL_TRACE;

	openlog("nvme-cli", LOG_NDELAY, LOG_USER);

	nvmecli_info("=== Starting NVMe CLI Logger ===");

	return 0;
}

int nvmecli_close_logger()
{
    nvmecli_info("=== Closing NVMe CLI Logger ===");

	if (g_logger.memstream) {
        fclose(g_logger.memstream);
        g_logger.memstream = NULL;
    }

	if (g_logger.message) {
		free(g_logger.message);
		g_logger.message = NULL;
	}

	g_logger.logging_threshold = 0;
	g_logger.msglen = 0;

	closelog();

	return 0;
}

int nvmecli_log(struct nvmecli_logger *logger,
				const struct nvmecli_location *loc,
				const char *prefix,
                int os_error,
				const char *format,
				...)
{
	va_list ap1, ap2;
	va_start(ap1, format);
	va_copy(ap2, ap1);

	vfprintf(logger->memstream, format, ap1);
	va_end(ap1);

	switch (loc->level) {
	case NVMECLI_LOGLEVEL_FATAL:
        /* start at 2 since this and append function need not be printed */
		append_stacktrace(logger->memstream, 2);
		/* fall through */

	case NVMECLI_LOGLEVEL_ERROR:
	case NVMECLI_LOGLEVEL_WARN:
		vfprintf(stderr, format, ap2);
		va_end(ap2);
        if (os_error) {
            append_perror(stderr, os_error);
            append_perror(logger->memstream, os_error);
        }
		fputc('\n', stderr);
        append_location_info(logger->memstream, loc);
		break;

	case NVMECLI_LOGLEVEL_NOTICE:
		vfprintf(stdout, format, ap2);
		va_end(ap2);
        putchar('\n');
        break;
	case NVMECLI_LOGLEVEL_INFO:
	case NVMECLI_LOGLEVEL_DEBUG:
	case NVMECLI_LOGLEVEL_TRACE:
	default:
        break;
	}

    fflush(logger->memstream);
    if (logger->message) {
        logger->message[logger->msglen] = '\0';
        send_syslog(logger->message, prefix, sysloglevel(loc));
    }
    rewind(logger->memstream);

	return 0;
}

#if 1
const char* multiline_message =
    "hello world\n"
    "One two three four\n"
    "\n"
    "\n"
    "int n = nvmecli_open_logger();\n"
    "London does not wait for me";


void send_fatal()
{
	nvmecli_fatal("Fatal Message");
}

int main()
{
	int n = nvmecli_open_logger();
	if (n != 0) {
		printf("open_memstream: %s\n", strerror(n));
		return 1;
	}

	nvmecli_trace("Trace Message and very long message");
	nvmecli_debug("Debug Message");
    nvmecli_debug(multiline_message);
	nvmecli_info("Info Message");
	nvmecli_notice("Notice Message");
	nvmecli_warn("Warn Message");
	nvmecli_error("Error Message");
    errno = 114;
	nvmecli_perror("Perror Message");
    send_fatal();

	nvmecli_close_logger();
	return 0;
}
#endif
