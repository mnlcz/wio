#define _POSIX_C_SOURCE 200809L

#include <libguile.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "guile.h"

static SCM
wio_ping(void)
{
	fprintf(stderr, "wio: wio-ping called from Scheme\n");
	return SCM_UNSPECIFIED;
}

static SCM
wio_echo(SCM msg)
{
	if(!scm_is_string(msg))
		scm_wrong_type_arg_msg("wio-echo", 1, msg, "string");

	char *str = scm_to_locale_string(msg);
	fprintf(stderr, "wio: wio-echo received: %s\n", str);
	free(str);

	return msg;
}

static SCM
wio_guile_repl_body(void *data)
{
	const char *repl_path = (const char *) data;

	scm_c_define("%wio-repl-path", scm_from_locale_string(repl_path));
	scm_c_eval_string("(use-modules (system repl server))"
			  "(spawn-server (make-unix-domain-server-socket #:path %wio-repl-path))");

	return SCM_BOOL_T;
}

static void
wio_guile_start_repl(void)
{
	const char *namespace = getenv("NAMESPACE");
	if(namespace == NULL){
		fprintf(stderr,
			"wio: NAMESPACE not set, REPL server not started\n");
		return;
	}

	char repl_path[PATH_MAX];
	int n =
	    snprintf(repl_path, sizeof(repl_path), "%s/wio.repl",
		     namespace);
	if(n < 0 || (size_t) n >= sizeof(repl_path)){
		fprintf(stderr,
			"wio: REPL socket path too long, REPL server not started\n");
		return;
	}

	SCM result = scm_c_catch(SCM_BOOL_T,
				 wio_guile_repl_body, (void *) repl_path,
				 scm_handle_by_message_noexit, "wio",
				 NULL, NULL);

	if(scm_is_eq(result, SCM_BOOL_T)){
		fprintf(stderr, "wio: REPL server listening at %s\n",
			repl_path);
	} else{
		fprintf(stderr, "wio: REPL server failed to start\n");
	}
}

static void
wio_guile_register_primitives(void)
{
	scm_c_define_gsubr("wio-ping", 0, 0, 0, wio_ping);
	scm_c_define_gsubr("wio-echo", 1, 0, 0, wio_echo);
}

static SCM
wio_guile_load_body(void *data)
{
	const char *path = data;
	scm_c_primitive_load(path);
	return SCM_UNSPECIFIED;
}

void
wio_guile_init(void)
{
	wio_guile_register_primitives();
	wio_guile_start_repl();

	const char *home = getenv("HOME");
	if(home == NULL){
		fprintf(stderr,
			"wio: HOME not set, skipping Scheme init\n");
		return;
	}

	char init_path[PATH_MAX];
	int n = snprintf(init_path, sizeof(init_path),
			 "%s/.config/wio/init.scm", home);
	if(n <= 0 || (size_t) n >= sizeof(init_path)){
		fprintf(stderr,
			"wio: HOME path too long, skipping Scheme init\n");
		return;
	}

	if(access(init_path, R_OK) != 0){
		fprintf(stderr,
			"wio: no init.scm found at %s, running without Scheme policy\n",
			init_path);
		return;
	}

	scm_c_catch(SCM_BOOL_T,
		    wio_guile_load_body, (void *) init_path,
		    scm_handle_by_message_noexit, "wio", NULL, NULL);
}
