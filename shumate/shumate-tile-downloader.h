/*
 * Copyright (C) 2021 James Westman <james@jwestman.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <shumate/shumate-data-source.h>

G_BEGIN_DECLS


#define SHUMATE_TYPE_TILE_DOWNLOADER (shumate_tile_downloader_get_type())
G_DECLARE_FINAL_TYPE (ShumateTileDownloader, shumate_tile_downloader, SHUMATE, TILE_DOWNLOADER, ShumateDataSource)

ShumateTileDownloader *shumate_tile_downloader_new (const char *url_template);


/**
 * SHUMATE_TILE_DOWNLOADER_ERROR:
 *
 * Error domain for errors that may occur while fetching tiles from the network
 * using [class@TileDownloader]. Errors in this domain will be from the
 * [enum@TileDownloaderError] enum.
 */
#define SHUMATE_TILE_DOWNLOADER_ERROR shumate_tile_downloader_error_quark ()
GQuark shumate_tile_downloader_error_quark (void);

/**
 * ShumateTileDownloaderError:
 * @SHUMATE_TILE_DOWNLOADER_ERROR_FAILED: An unspecified error occurred during the operation.
 * @SHUMATE_TILE_DOWNLOADER_ERROR_BAD_RESPONSE: An unsuccessful HTTP response was received from the server.
 * @SHUMATE_TILE_DOWNLOADER_ERROR_COULD_NOT_CONNECT: The server could not be reached.
 * @SHUMATE_TILE_DOWNLOADER_ERROR_MALFORMED_URL: The provided URL isn't valid
 * @SHUMATE_TILE_DOWNLOADER_ERROR_OFFLINE: The tile source has been marked as offline.
 *
 * Error codes in the #SHUMATE_TILE_DOWNLOADER_ERROR domain.
 */
typedef enum {
  SHUMATE_TILE_DOWNLOADER_ERROR_FAILED,
  SHUMATE_TILE_DOWNLOADER_ERROR_BAD_RESPONSE,
  SHUMATE_TILE_DOWNLOADER_ERROR_COULD_NOT_CONNECT,
  SHUMATE_TILE_DOWNLOADER_ERROR_MALFORMED_URL,
  SHUMATE_TILE_DOWNLOADER_ERROR_OFFLINE,
} ShumateTileDownloaderError;


G_END_DECLS
