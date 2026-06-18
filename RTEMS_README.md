# Porting Hammer to RTEMS 5

This document describes the port of the Hammer parser combinator library
to RTEMS 5.0.0 SPARC (GR712RC LEON3, tested under TSIM eval).

## Build requirements

- RTEMS 5.0.0 toolchain at a known prefix
  - SPARC: rcc-1.3.1-gcc (Gaisler RTEMS 5 toolchain)
- A target BSP installed alongside the toolchain
  - SPARC: `gr712rc`
- GNU Make
- A simulator or hardware to run the resulting executable
  - SPARC: TSIM used to test

## Build

The Makefile accepts a target via two variables:

    make RTEMS_PREFIX=<prefix> RTEMS_BSP=<arch>/<bsp>

Examples:
    # SPARC

    make RTEMS_PREFIX=/opt/rcc-1.3.1-gcc \
         RTEMS_BSP=sparc-gaisler-rtems5/gr712rc \


Run `make install` to stage `libhammer.a` and the public headers into
`$INSTALL_PREFIX` (default `$HOME/install`). Run `make install-tests`
to additionally stage `libhammer_tests.a`.

## Linking your application

    <gcc> <target-flags> \
      -B<RTEMS_PREFIX>/<arch>/<bsp>/lib \
      -specs bsp_specs -qrtems \
      -I<RTEMS_PREFIX>/<arch>/<bsp>/lib/include \
      -I$HOME/install/include \
      your_app.c \
      -L$HOME/install/lib -lhammer \
      -o your_app.exe

ex. (for code below)

    sparc-gaisler-rtems5-gcc -mcpu=leon3 -mfix-gr712rc \
      -B/opt/rcc-1.3.1-gcc/sparc-gaisler-rtems5/gr712rc/lib \
      -specs bsp_specs -qrtems \
      -I/opt/rcc-1.3.1-gcc/sparc-gaisler-rtems5/gr712rc/lib/include \
      -I$HOME/install/include \
      -I$HOME/install/tests \
      init.c \
      -L$HOME/install/tests \
      -L$HOME/install/lib -lhammer_tests -lhammer \
      -o hammer_smoke.exe

For test apps, add `-I$HOME/install/tests -L$HOME/install/tests -lhammer_tests`,
placing `-lhammer_tests` BEFORE `-lhammer`.

### NOTES
* When doing `make tests` you will see many warnings of functions being redefined in the different test files.
  This is intended, as glib is not supported on rtems, so any functions provided by it were redefined for the rtems build.
* There is a CFlag named SIMULATOR_LIMITED. This was added, as some tests would take too much time in the simulator, so the test suite was unable to fully run. This flag can be safely removed if not running on simulator.
* Running `make install` or `make install-tests` will by default install everything needed to link to `~/install/`, which the linking instructions above are based on.
* Some desugar tests were removed, as they did not verify any functionality, they only verified desugar output formatting.


## Example rtems test
```c
#include <rtems.h>
#include <stdio.h>
#include <stdlib.h>
#include "hammer.h"
#include "rtems_test_suite.h"
#include <sys/stat.h>

extern void register_all_hammer_tests(void);

rtems_task Init(rtems_task_argument arg) {
    (void) arg;
	int r = mkdir("/tmp", 0777);
    g_test_init(NULL, NULL, NULL);
    register_all_hammer_tests();
    g_run_test();
    exit(0);
}

/* ---- RTEMS configuration ---- */
#define CONFIGURE_APPLICATION_NEEDS_CLOCK_DRIVER
#define CONFIGURE_APPLICATION_NEEDS_CONSOLE_DRIVER

#define CONFIGURE_MAXIMUM_TASKS            4
#define CONFIGURE_RTEMS_INIT_TASKS_TABLE

#define CONFIGURE_INIT_TASK_STACK_SIZE     (128 * 1024) 

#define CONFIGURE_INIT

#define CONFIGURE_FILESYSTEM_IMFS
#define CONFIGURE_MAXIMUM_FILE_DESCRIPTORS 16

#define CONFIGURE_INIT_TASK_ATTRIBUTES RTEMS_FLOATING_POINT


#include <rtems/confdefs.h>
```