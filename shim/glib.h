#ifndef RTEMS_GLIB_SHIM_H
#define RTEMS_GLIB_SHIM_H
#ifdef RTEMS_BUILD
#define GINT_TO_POINTER(i) ((void*)(long)(i))
#define GPOINTER_TO_INT(p) ((int)(long)(p))
typedef const void* gconstpointer;
#else
#error "RTEMS-only shim"
#endif
#endif