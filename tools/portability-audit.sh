#!/bin/sh
set -eu

if rg -n \
  --glob '*.c' --glob '*.h' \
  --glob '!platform_posix.c' --glob '!platform_windows.c' \
  --glob '!platform_portable.c' --glob '!platform_provider.h' \
  --glob '!t_*.c' \
  '#include <(pthread|search|sys/|unistd|arpa/)|\b(clock_gettime|vasprintf|asprintf|strdup)\s*\(' \
  src; then
    echo "portable-core audit failed" >&2
    exit 1
fi
