#pragma once

#include <glib-object.h>

G_BEGIN_DECLS


#define TEST_TYPE_TILE_SERVER (test_tile_server_get_type())
G_DECLARE_FINAL_TYPE (TestTileServer, test_tile_server, TEST, TILE_SERVER, GObject)


TestTileServer *test_tile_server_new (void);

char *test_tile_server_start (TestTileServer *self);
void test_tile_server_assert_requests (TestTileServer *self, int times);
void test_tile_server_set_status (TestTileServer *self, int status);
void test_tile_server_set_data (TestTileServer *self, const char *data);
void test_tile_server_set_etag (TestTileServer *self, const char *etag);

G_END_DECLS
