#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <libguile.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "guile.h"
#include "server.h"
#include "view.h"

static struct
wio_server *the_server;

static SCM wio_view_type;

static SCM
wio_wrap_view(struct wio_view *view)
{
	return scm_make_foreign_object_1(wio_view_type, (void *) view->id);
}

static SCM
wio_views_list(void)
{
	SCM head = SCM_EOL;
	struct wio_view *view;

	wl_list_for_each_reverse(view, &the_server->views, link){
		head = scm_cons(wio_wrap_view(view), head);
	}

	return head;
}

static SCM
wio_view_title_scm(SCM view_scm)
{
	scm_assert_foreign_object_type(wio_view_type, view_scm);
	uint64_t id = (uint64_t) scm_foreign_object_ref(view_scm, 0);
	struct wio_view *view = wio_view_by_id(the_server, id);

	if(!view)
		return SCM_BOOL_F;

	const char *title = view->xdg_toplevel->title;
	if(!title)
		return SCM_BOOL_F;

	return scm_from_locale_string(title);
}

static SCM
wio_hide_view_scm(SCM view_scm)
{
	scm_assert_foreign_object_type(wio_view_type, view_scm);
	uint64_t id = (uint64_t) scm_foreign_object_ref(view_scm, 0);
	struct wio_view *view = wio_view_by_id(the_server, id);

	if(!view)
		return SCM_BOOL_F;
	wio_view_hide(view);

	return SCM_BOOL_T;
}

static SCM
wio_restore_view_scm(SCM view_scm)
{
	scm_assert_foreign_object_type(wio_view_type, view_scm);
	uint64_t id = (uint64_t) scm_foreign_object_ref(view_scm, 0);
	struct wio_view *view = wio_view_by_id(the_server, id);

	if(!view)
		return SCM_BOOL_F;
	wio_view_restore(view);

	return SCM_BOOL_T;
}

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
wio_menu_dispatch_body(void *data)
{
	int idx = *(int *) data;
	SCM proc = scm_variable_ref(scm_c_lookup("wio-menu-dispatch"));
	SCM result = scm_call_1(proc, scm_from_int(idx));

	if(scm_is_symbol(result))
		return scm_symbol_to_string(result);

	return SCM_BOOL_F;
}

char *
wio_scheme_menu_dispatch(int index)
{
	SCM result = scm_c_catch(SCM_BOOL_T,
				 wio_menu_dispatch_body, (void *) &index,
				 scm_handle_by_message_noexit, "wio",
				 NULL, NULL);

	if(!scm_is_string(result))
		return NULL;

	return scm_to_locale_string(result);
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
	int n = snprintf(repl_path, sizeof(repl_path), "%s/wio.repl",
			 namespace);
	if(n < 0 || (size_t) n >= sizeof(repl_path)){
		fprintf(stderr,
			"wio: REPL socket path too long, REPL server not started\n");
		return;
	}

	if(unlink(repl_path) != 0 && errno != ENOENT)
		fprintf(stderr,
			"wio: could not remove stale REPL socket at %s: %s\n",
			repl_path, strerror(errno));

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
	scm_c_define_gsubr("wio-views", 0, 0, 0, wio_views_list);
	scm_c_define_gsubr("wio-view-title", 1, 0, 0, wio_view_title_scm);
	scm_c_define_gsubr("wio-hide-view", 1, 0, 0, wio_hide_view_scm);
	scm_c_define_gsubr("wio-restore-view", 1, 0, 0,
			   wio_restore_view_scm);
}

static SCM
wio_guile_load_body(void *data)
{
	const char *path = data;
	scm_c_primitive_load(path);
	return SCM_UNSPECIFIED;
}

void
wio_guile_init(struct wio_server *server)
{
	the_server = server;
	wio_view_type =
	    scm_make_foreign_object_type(scm_from_locale_symbol
					 ("wio-view"),
					 scm_list_1(scm_from_locale_symbol
						    ("id")), NULL);

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
