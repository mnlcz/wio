#ifndef WIO_GUILE_H
#define WIO_GUILE_H

struct wio_server;
struct wio_view;
void wio_guile_init(struct wio_server *server);
char *wio_scheme_menu_dispatch(int index);
void wio_scheme_hide_requested(struct wio_view *view);
void wio_scheme_view_mapped(struct wio_view *view);

#endif
