#pragma once

#include <glib-object.h>

G_BEGIN_DECLS


#define TEST_TYPE_TILE_SERVER (test_tile_server_get_type())
G_DECLARE_FINAL_TYPE (TestTileServer, test_tile_server, TEST, TILE_SERVER, GObject)


TestTileServer *test_tile_server_new (void);

char *test_tile_server_start (TestTileServer *self);


G_END_DECLS
