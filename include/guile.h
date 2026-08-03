#ifndef WIO_GUILE_H
#define WIO_GUILE_H

struct wio_server;
void wio_guile_init(struct wio_server *server);
char *wio_scheme_menu_dispatch(int index);

#endif
