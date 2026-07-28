#include <libguile.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "guile.h"

static SCM wio_ping(void) {
    fprintf(stderr, "wio: wio-ping called from Scheme\n");
    return SCM_UNSPECIFIED;
}

static void wio_guile_register_primitives(void) {
    scm_c_define_gsubr("wio-ping", 0, 0, 0, wio_ping);
}

void wio_guile_init(void) {
    wio_guile_register_primitives();

    const char *home = getenv("HOME");
    if (home == NULL) {
        fprintf(stderr, "wio: HOME not set, skipping Scheme init\n");
        return;
    }

    char init_path[PATH_MAX];
    int n = snprintf(init_path, sizeof(init_path), "%s/.config/wio/init.scm", home);
    if (n <= 0 || (size_t)n >= sizeof(init_path)) {
        fprintf(stderr, "wio: HOME path too long, skipping Scheme init\n");
        return;
    }

    if (access(init_path, R_OK) != 0) {
        fprintf(stderr, "wio: no init.scm found at %s, running without Scheme policy\n", init_path);
        return;
    }

    scm_c_primitive_load(init_path);
}