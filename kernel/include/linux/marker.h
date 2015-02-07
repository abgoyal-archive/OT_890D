
#ifndef _LINUX_MARKER_H
#define _LINUX_MARKER_H


#include <stdarg.h>
#include <linux/types.h>

struct module;
struct marker;

typedef void marker_probe_func(void *probe_private, void *call_private,
		const char *fmt, va_list *args);

struct marker_probe_closure {
	marker_probe_func *func;	/* Callback */
	void *probe_private;		/* Private probe data */
};

struct marker {
	const char *name;	/* Marker name */
	const char *format;	/* Marker format string, describing the
				 * variable argument list.
				 */
	char state;		/* Marker state. */
	char ptype;		/* probe type : 0 : single, 1 : multi */
				/* Probe wrapper */
	void (*call)(const struct marker *mdata, void *call_private, ...);
	struct marker_probe_closure single;
	struct marker_probe_closure *multi;
	const char *tp_name;	/* Optional tracepoint name */
	void *tp_cb;		/* Optional tracepoint callback */
} __attribute__((aligned(8)));

#ifdef CONFIG_MARKERS

#define _DEFINE_MARKER(name, tp_name_str, tp_cb, format)		\
		static const char __mstrtab_##name[]			\
		__attribute__((section("__markers_strings")))		\
		= #name "\0" format;					\
		static struct marker __mark_##name			\
		__attribute__((section("__markers"), aligned(8))) =	\
		{ __mstrtab_##name, &__mstrtab_##name[sizeof(#name)],	\
		  0, 0, marker_probe_cb, { __mark_empty_function, NULL},\
		  NULL, tp_name_str, tp_cb }

#define DEFINE_MARKER(name, format)					\
		_DEFINE_MARKER(name, NULL, NULL, format)

#define DEFINE_MARKER_TP(name, tp_name, tp_cb, format)			\
		_DEFINE_MARKER(name, #tp_name, tp_cb, format)

#define __trace_mark(generic, name, call_private, format, args...)	\
	do {								\
		DEFINE_MARKER(name, format);				\
		__mark_check_format(format, ## args);			\
		if (unlikely(__mark_##name.state)) {			\
			(*__mark_##name.call)				\
				(&__mark_##name, call_private, ## args);\
		}							\
	} while (0)

#define __trace_mark_tp(name, call_private, tp_name, tp_cb, format, args...) \
	do {								\
		void __check_tp_type(void)				\
		{							\
			register_trace_##tp_name(tp_cb);		\
		}							\
		DEFINE_MARKER_TP(name, tp_name, tp_cb, format);		\
		__mark_check_format(format, ## args);			\
		(*__mark_##name.call)(&__mark_##name, call_private,	\
					## args);			\
	} while (0)

extern void marker_update_probe_range(struct marker *begin,
	struct marker *end);

#define GET_MARKER(name)	(__mark_##name)

#else /* !CONFIG_MARKERS */
#define DEFINE_MARKER(name, tp_name, tp_cb, format)
#define __trace_mark(generic, name, call_private, format, args...) \
		__mark_check_format(format, ## args)
#define __trace_mark_tp(name, call_private, tp_name, tp_cb, format, args...) \
	do {								\
		void __check_tp_type(void)				\
		{							\
			register_trace_##tp_name(tp_cb);		\
		}							\
		__mark_check_format(format, ## args);			\
	} while (0)
static inline void marker_update_probe_range(struct marker *begin,
	struct marker *end)
{ }
#define GET_MARKER(name)
#endif /* CONFIG_MARKERS */

#define trace_mark(name, format, args...) \
	__trace_mark(0, name, NULL, format, ## args)

#define _trace_mark(name, format, args...) \
	__trace_mark(1, name, NULL, format, ## args)

#define trace_mark_tp(name, tp_name, tp_cb, format, args...)	\
	__trace_mark_tp(name, NULL, tp_name, tp_cb, format, ## args)

#define MARK_NOARGS " "

/* To be used for string format validity checking with gcc */
static inline void __printf(1, 2) ___mark_check_format(const char *fmt, ...)
{
}

#define __mark_check_format(format, args...)				\
	do {								\
		if (0)							\
			___mark_check_format(format, ## args);		\
	} while (0)

extern marker_probe_func __mark_empty_function;

extern void marker_probe_cb(const struct marker *mdata,
	void *call_private, ...);

extern int marker_probe_register(const char *name, const char *format,
				marker_probe_func *probe, void *probe_private);

extern int marker_probe_unregister(const char *name,
	marker_probe_func *probe, void *probe_private);
extern int marker_probe_unregister_private_data(marker_probe_func *probe,
	void *probe_private);

extern void *marker_get_private_data(const char *name, marker_probe_func *probe,
	int num);

#define marker_synchronize_unregister() synchronize_sched()

#endif
