/* Copyright (c) 2026 Riverside Research */
#include "params.h"

size_t h_get_param_k(void *param) {
    uintptr_t params_int;

    params_int = (uintptr_t)param;
    return (size_t)params_int;
}

char *h_format_description_with_param_k(HAllocator *mm__, const char *backend_name, size_t k) {
    const char *format_str = "%s(%zu) parser backend";

    const char *generic_descr_format_str = "%s(k) parser backend (default k is %zu)";
    size_t len;
    char *descr = NULL;

    if (k > 0) {
        /* A specific k was given */
        /* Measure how big a buffer we need */
        len = snprintf(NULL, 0, format_str, backend_name, k);
        /* Allocate it and do the real snprintf */
        descr = h_new(char, len + 1);
        if (descr) {
            snprintf(descr, len + 1, format_str, backend_name, k);
        }
    } else {
        /*
         * No specific k, would use DEFAULT_KMAX.  We say what DEFAULT_KMAX
         * was compiled in in the description.
         */
        len = snprintf(NULL, 0, generic_descr_format_str, backend_name, DEFAULT_KMAX);
        /* Allocate and do the real snprintf */
        descr = h_new(char, len + 1);
        if (descr) {
            snprintf(descr, len + 1, generic_descr_format_str, backend_name, DEFAULT_KMAX);
        }
    }

    return descr;
}

char *h_format_name_with_param_k(HAllocator *mm__, const char *backend_name, size_t k) {
    const char *format_str = "%s(%zu)", *generic_name = "%s(k)";
    size_t len;
    char *name = NULL;

    if (k > 0) {
        /* A specific k was given */
        /* Measure how big a buffer we need */
        len = snprintf(NULL, 0, format_str, backend_name, k);
        /* Allocate it and do the real snprintf */
        name = h_new(char, len + 1);
        if (name) {
            snprintf(name, len + 1, format_str, backend_name, k);
        }
    } else {
        /* No specific k */
        len = snprintf(NULL, 0, generic_name, backend_name, k);
        name = h_new(char, len + 1);
        if (name) {
            snprintf(name, len + 1, generic_name, backend_name);
        }
    }

    return name;
}

#define MAX_LENGTH 10 // On a 32-bit system ASCII representation of 2^32 is 10 digits.

int h_extract_param_k(HParserBackendWithParams *be_with_params,
                      backend_with_params_t *be_with_params_t) {

    if (!be_with_params || !be_with_params_t)
        return -1; // NULL input

    be_with_params->params = NULL;

    int param_0 = -1;
    int success = 0;
    uintptr_t param;

    backend_params_t params_t = be_with_params_t->params;

    if (params_t.params == NULL || params_t.len == 0) {
        return -2; // NULL params in be_with_params_t->params
    }

    size_t num_of_params = params_t.len;

    if (num_of_params > 0) {
        backend_param_with_name_t param_t = params_t.params[0];
        if (param_t.param.param == NULL)
            return -3; // NULL param

        size_t len = param_t.param.len; // length of the param string
        if (len > MAX_LENGTH && len > 0)
            return -5; // param_t.param.len is too large or 0

        // char's can sometimes be non NUL-terminated and will cause an overflow on sscanf, so
        // nul-termination can be added inside a temp copy to avoid overwriting unowned memory
        char tmp[10];
        memcpy(tmp, param_t.param.param, len);
        tmp[len] = '\0';
        success = sscanf((char *)tmp, "%d", &param_0);
    } else
        return -4; // params_t.len is 0

    if ((size_t)success <= num_of_params && success > 0) {
        param = (uintptr_t)param_0;
        be_with_params->params = (void *)param;
    }

    return success;
}
