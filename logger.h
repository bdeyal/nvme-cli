#ifndef NVMECLI_LOGGER
#define NVMECLI_LOGGER

#include <stdio.h>

/* Log levels */
enum {
	NVMECLI_LOGLEVEL_ALL    =   0,
	NVMECLI_LOGLEVEL_TRACE  =  10,
	NVMECLI_LOGLEVEL_DEBUG  =  20,
	NVMECLI_LOGLEVEL_INFO   =  50,
	NVMECLI_LOGLEVEL_NOTICE =  60,
	NVMECLI_LOGLEVEL_WARN   =  70,
	NVMECLI_LOGLEVEL_ERROR  =  80,
	NVMECLI_LOGLEVEL_FATAL  =  90,
	NVMECLI_LOGLEVEL_NONE   = 100
};

struct nvmecli_location {
	const char *fname;
	int line;
	const char *func;
	int level;
};

struct nvmecli_logger {
	char   *message;
	size_t	msglen;
	FILE   *memstream;
	int		logging_threshold;
};

/*
 *  init / destroy logger
 */
int nvmecli_open_logger();
int nvmecli_close_logger();

int nvmecli_log(struct nvmecli_logger* logger,
				const struct nvmecli_location *loc,
				const char *prefix,
                int os_error,
				const char *format,
				...) __attribute__ ((format (printf, 5, 6)));

#define _nvmecli_msg(indicator, level, fmt, ...)						\
	do {																\
		extern struct nvmecli_logger g_logger;							\
		if (level >= g_logger.logging_threshold) {                      \
			struct nvmecli_location loc = { __FILE__, __LINE__, __func__, level}; \
			nvmecli_log(&g_logger, &loc, indicator, (fmt), ##__VA_ARGS__);  \
		}																    \
	} while (0)

#define nvmecli_trace(_format, ...) \
	_nvmecli_msg("T:", NVMECLI_LOGLEVEL_TRACE, 0, _format, ##__VA_ARGS__)

#define nvmecli_debug(_format, ...) \
	_nvmecli_msg("D:", NVMECLI_LOGLEVEL_DEBUG, 0, _format, ##__VA_ARGS__)

#define nvmecli_info(_format, ...) \
	_nvmecli_msg("I:", NVMECLI_LOGLEVEL_INFO, 0, _format, ##__VA_ARGS__)

#define nvmecli_notice(_format, ...) \
	_nvmecli_msg("N:", NVMECLI_LOGLEVEL_NOTICE, 0, _format, ##__VA_ARGS__)

/*
 * Log statements with warn, error, fatal are sent to stderr too.
 * No need for separate logging and stderr printing
 */
#define nvmecli_warn(_format, ...) \
	_nvmecli_msg("W:", NVMECLI_LOGLEVEL_WARN, 0, _format, ##__VA_ARGS__)

#define nvmecli_error(_format, ...) \
	_nvmecli_msg("E:", NVMECLI_LOGLEVEL_ERROR, 0, _format, ##__VA_ARGS__)

/*
 * nvmecli_perror = nvmecli_error + strerror(errno)
 */
#define nvmecli_perror(_format, ...) \
	_nvmecli_msg("E:", NVMECLI_LOGLEVEL_ERROR, errno, _format, ##__VA_ARGS__)

#define nvmecli_fatal(_format, ...) \
	_nvmecli_msg("F:", NVMECLI_LOGLEVEL_FATAL, 0, _format, ##__VA_ARGS__)

/*
 *  Abort / Assert messages that are logged
 */
#define nvmecli_abort(_format, ...)                                            \
    do {                                                                       \
        _nvmecli_msg("F:", NVMECLI_LOGLEVEL_FATAL, 0, _format, ##__VA_ARGS__); \
        abort();                                                               \
    } while(0)

/*
 *  Specific assert macro that respects NDEBUG
 */
#ifdef NDEBUG
#define nvmecli_assert(expr)  ((void)0)
#else
#define nvmecli_assert(expr)                         \
    do {                                             \
        if (!(expr)) {                               \
            log_fatal("Assertion Failure: " #expr);  \
            abort();                                 \
        }                                            \
    } while (0)
#endif


#endif
