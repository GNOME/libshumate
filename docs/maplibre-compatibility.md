Title: MapLibre GL JS Compatibility
Slug: maplibre-compatibility

# MapLibre GL JS Compatibility

Vector tile support via [class@Shumate.VectorRenderer] is based on the [MapLibre GL JS style
specification](https://maplibre.org/maplibre-gl-js-docs/style-spec/).
A style is a JSON document describing the vector data source and the layers
that are drawn to render the map. It includes a JSON-based expression format
that allows style authors to do a wide range of data-based styling.

Not all of the features in the spec are implemented in libshumate, and some
features have important differences. These differences are described here.

## Rasterization

The most important architectural difference between MapLibre GL JS and
libshumate is that, while MapLibre uses WebGL to render vector data directly
to the screen, libshumate rasterizes the tiles first. This has broad effects
because once a tile is rendered, its pixel contents do not change as you zoom.

## [Style Root](https://maplibre.org/maplibre-gl-js-docs/style-spec/root/)

<table>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/root/#bearing">bearing</a></td>
    <td>❌ Not supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/root/#center">center</a></td>
    <td>❌ Not supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/root/#glyphs">glyphs</a></td>
    <td>❌ Not supported. Text is drawn using Pango and the system fonts.</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/root/#layers">layers</a></td>
    <td>✅ Supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/root/#light">light</a></td>
    <td>❌ Not supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/root/#name">name</a></td>
    <td>✅ Supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/root/#pitch">pitch</a></td>
    <td>❌ Not supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/root/#sources">sources</a></td>
    <td>✅ Supported, but see caveats below</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/root/#sprite">sprite</a></td>
    <td>❌ Not supported. Sprite sheets are set using [property@Shumate.VectorRenderer:sprite-sheet].
    Only one sprite sheet may be used.</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/root/#terrain">terrain</a></td>
    <td>❌ Not supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/root/#transition">transition</a></td>
    <td>❌ Not supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/root/#version">version</a></td>
    <td>❌ Not supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/root/#zoom">zoom</a></td>
    <td>❌ Not supported</td>
  </tr>
</table>

## [Sources](https://maplibre.org/maplibre-gl-js-docs/style-spec/sources)

Exactly one source must be provided. Only vector sources are supported;
for raster maps, use [class@Shumate.RasterRenderer]. TileJSON URLs via the
"url" property are not supported.

### [vector](https://maplibre.org/maplibre-gl-js-docs/style-spec/sources/#vector)

<table>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/sources/#vector-attribution">attribution</a></td>
    <td>❌ Not supported. Use [property@Shumate.MapSource:license] and [property@Shumate.MapSource:license-uri] instead.</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/sources/#vector-bounds">bounds</a></td>
    <td>❌ Not supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/sources/#vector-maxzoom">maxzoom</a></td>
    <td>✅ Supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/sources/#vector-minzoom">minzoom</a></td>
    <td>✅ Supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/sources/#vector-promoteId">promoteId</a></td>
    <td>❌ Not supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/sources/#vector-scheme">scheme</a></td>
    <td>❌ Not supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/sources/#vector-tiles">tiles</a></td>
    <td>✅ Supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/sources/#vector-url">url</a></td>
    <td>❌ Not supported. The stylesheet will fail to parse if this property is present.</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/sources/#vector-volatile">volatile</a></td>
    <td>❌ Not supported</td>
  </tr>
</table>

## [Layers](https://maplibre.org/maplibre-gl-js-docs/style-spec/layers)

<table>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#filter">filter</a></td>
    <td>✅ Supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#id">id</a></td>
    <td>✅ Supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#maxzoom">maxzoom</a></td>
    <td>✅ Supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#minzoom">minzoom</a></td>
    <td>✅ Supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#source">source</a></td>
    <td>❌ Not supported. [class@Shumate.VectorRenderer] only supports a single data source, so this property is ignored.</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#source-layer">source-layer</a></td>
    <td>✅ Supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#type">type</a></td>
    <td>✅ Supported</td>
  </tr>
</table>

### [background](https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#background)

<table>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#paint-background-background-color">background-color</a></td>
    <td>✅ Supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#paint-background-background-opacity">background-opacity</a></td>
    <td>✅ Supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#paint-background-background-pattern">background-pattern</a></td>
    <td>❌ Not supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#layout-background-visibility">visibility</a></td>
    <td>❌ Not supported</td>
  </tr>
</table>

### [fill](https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#fill)

<table>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#paint-fill-fill-antialias">fill-antialias</a></td>
    <td>❌ Not supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#paint-fill-fill-color">fill-color</a></td>
    <td>✅ Supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#paint-fill-fill-opacity">fill-opacity</a></td>
    <td>✅ Supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#paint-fill-fill-outline-color">fill-outline-color</a></td>
    <td>❌ Not supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#paint-fill-fill-pattern">fill-pattern</a></td>
    <td>✅ Supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#layout-fill-fill-sort-key">fill-sort-key</a></td>
    <td>❌ Not supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#paint-fill-fill-translate">fill-translate</a></td>
    <td>❌ Not supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#paint-fill-fill-translate-anchor">fill-translate-anchor</a></td>
    <td>❌ Not supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#layout-fill-visibility">visibility</a></td>
    <td>❌ Not supported</td>
  </tr>
</table>

### [line](https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#line)

<table>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#paint-line-line-blur">line-blur</a></td>
    <td>❌ Not supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#layout-line-line-cap">line-cap</a></td>
    <td>✅ Supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#paint-line-line-color">line-color</a></td>
    <td>✅ Supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#paint-line-line-dasharray">line-dasharray</a></td>
    <td>Supported, but does not support expressions.</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#paint-line-line-gap-width">line-gap-width</a></td>
    <td>❌ Not supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#paint-line-line-gradient">line-gradient</a></td>
    <td>❌ Not supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#layout-line-line-join">line-join</a></td>
    <td>✅ Supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#layout-line-line-miter-limit">line-miter-limit</a></td>
    <td>❌ Not supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#paint-line-line-offset">line-offset</a></td>
    <td>❌ Not supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#paint-line-line-opacity">line-opacity</a></td>
    <td>✅ Supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#paint-line-line-pattern">line-pattern</a></td>
    <td>❌ Not supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#layout-line-line-round-limit">line-round-limit</a></td>
    <td>❌ Not supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#layout-line-line-sort-key">line-sort-key</a></td>
    <td>❌ Not supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#paint-line-line-translate">line-translate</a></td>
    <td>❌ Not supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#paint-line-line-translate-anchor">line-translate-anchor</a></td>
    <td>❌ Not supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#paint-line-line-width">line-width</a></td>
    <td>✅ Supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#layout-line-visibility">visibility</a></td>
    <td>❌ Not supported</td>
  </tr>
</table>

### [symbol](https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#symbol)

Symbols are rendered using Pango and system fonts, not the glyphs defined in
the spec.

<table>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#layout-symbol-icon-allow-overlap">icon-allow-overlap</a></td>
    <td>✅ Supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#layout-symbol-icon-anchor">icon-anchor</a></td>
    <td>✅ Supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#paint-symbol-icon-color">icon-color</a></td>
    <td>✅ Supported. Only works for symbolic icons.</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#paint-symbol-icon-halo-blur">icon-halo-blur</a></td>
    <td>❌ Not supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#paint-symbol-icon-halo-color">icon-halo-color</a></td>
    <td>❌ Not supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#paint-symbol-icon-halo-width">icon-halo-width</a></td>
    <td>❌ Not supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#layout-symbol-icon-ignore-placement">icon-ignore-placement</a></td>
    <td>✅ Supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#layout-symbol-icon-image">icon-image</a></td>
    <td>✅ Supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#layout-symbol-icon-keep-upright">icon-keep-upright</a></td>
    <td>❌ Not supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#layout-symbol-icon-offset">icon-offset</a></td>
    <td>Expressions are not supported.</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#paint-symbol-icon-opacity">icon-opacity</a></td>
    <td>✅ Supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#layout-symbol-icon-optional">icon-optional</a></td>
    <td>✅ Supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#layout-symbol-icon-overlap">icon-overlap</a></td>
    <td>✅ Supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#layout-symbol-icon-padding">icon-padding</a></td>
    <td>✅ Supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#layout-symbol-icon-pitch-alignment">icon-pitch-alignment</a></td>
    <td>❌ Not supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#layout-symbol-icon-rotate">icon-rotate</a></td>
    <td>✅ Supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#layout-symbol-icon-rotation-alignment">icon-rotation-alignment</a></td>
    <td>✅ Supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#layout-symbol-icon-size">icon-size</a></td>
    <td>✅ Supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#layout-symbol-icon-text-fit">icon-text-fit</a></td>
    <td>❌ Not supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#layout-symbol-icon-text-fit-padding">icon-text-fit-padding</a></td>
    <td>❌ Not supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#paint-symbol-icon-translate">icon-translate</a></td>
    <td>❌ Not supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#paint-symbol-icon-translate-anchor">icon-translate-anchor</a></td>
    <td>❌ Not supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#layout-symbol-symbol-avoid-edges">symbol-avoid-edges</a></td>
    <td>❌ Not supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#layout-symbol-symbol-placement">symbol-placement</a></td>
    <td>"line-center" is treated like "line".</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#layout-symbol-symbol-sort-key">symbol-sort-key</a></td>
    <td>✅ Supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#layout-symbol-symbol-spacing">symbol-spacing</a></td>
    <td>✅ Supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#layout-symbol-symbol-z-order">symbol-z-order</a></td>
    <td>❌ Not supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#layout-symbol-text-allow-overlap">text-allow-overlap</a></td>
    <td>✅ Supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#layout-symbol-text-anchor">text-anchor</a></td>
    <td>Not supported when glyphs are positioned along a line geometry.</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#paint-symbol-text-color">text-color</a></td>
    <td>✅ Supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#layout-symbol-text-field">text-field</a></td>
    <td>✅ Supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#layout-symbol-text-font">text-font</a></td>
    <td>Expressions are not supported. Fallback fonts are not handled correctly.</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#paint-symbol-text-halo-blur">text-halo-blur</a></td>
    <td>❌ Not supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#paint-symbol-text-halo-color">text-halo-color</a></td>
    <td>❌ Not supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#paint-symbol-text-halo-width">text-halo-width</a></td>
    <td>❌ Not supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#layout-symbol-text-ignore-placement">text-ignore-placement</a></td>
    <td>✅ Supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#layout-symbol-text-justify">text-justify</a></td>
    <td>❌ Not supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#layout-symbol-text-keep-upright">text-keep-upright</a></td>
    <td>✅ Supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#layout-symbol-text-letter-spacing">text-letter-spacing</a></td>
    <td>✅ Supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#layout-symbol-text-line-height">text-line-height</a></td>
    <td>❌ Not supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#layout-symbol-text-max-angle">text-max-angle</a></td>
    <td>❌ Not supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#layout-symbol-text-max-width">text-max-width</a></td>
    <td>❌ Not supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#layout-symbol-text-offset">text-offset</a></td>
    <td>Not supported when glyphs are positioned along a line geometry. Expressions are not supported.</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#paint-symbol-text-opacity">text-opacity</a></td>
    <td>✅ Supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#layout-symbol-text-optional">text-optional</a></td>
    <td>✅ Supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#layout-symbol-text-overlap">text-overlap</a></td>
    <td>✅ Supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#layout-symbol-text-padding">text-padding</a></td>
    <td>✅ Supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#layout-symbol-text-pitch-alignment">text-pitch-alignment</a></td>
    <td>❌ Not supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#layout-symbol-text-radial-offset">text-radial-offset</a></td>
    <td>❌ Not supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#layout-symbol-text-rotate">text-rotate</a></td>
    <td>❌ Not supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#layout-symbol-text-rotation-alignment">text-rotation-alignment</a></td>
    <td>✅ Supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#layout-symbol-text-size">text-size</a></td>
    <td>✅ Supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#layout-symbol-text-transform">text-transform</a></td>
    <td>✅ Supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#paint-symbol-text-translate">text-translate</a></td>
    <td>❌ Not supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#paint-symbol-text-translate-anchor">text-translate-anchor</a></td>
    <td>❌ Not supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#layout-symbol-text-variable-anchor">text-variable-anchor</a></td>
    <td>❌ Not supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#layout-symbol-text-writing-mode">text-writing-mode</a></td>
    <td>❌ Not supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#layout-symbol-visibility">visibility</a></td>
    <td>❌ Not supported</td>
  </tr>
</table>

### [raster](https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#raster)

❌ Not supported. Use [class@Shumate.RasterRenderer] instead.

### [circle](https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#circle)

❌ Not supported

### [fill-extrusion](https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#fill-extrusion)

❌ Not supported

### [heatmap](https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#heatmap)

❌ Not supported

### [hillshade](https://maplibre.org/maplibre-gl-js-docs/style-spec/layers/#hillshade)

❌ Not supported

## [Expressions](https://maplibre.org/maplibre-gl-js-docs/style-spec/expressions)

Unlike MapLibre GL JS, libshumate does not check expression types when the
style is parsed, only when they are evaluated.

Due to limitations in libshumate's rendering pipeline, all expressions are
evaluated at integer zoom levels only.

### [Types](https://maplibre.org/maplibre-gl-js-docs/style-spec/expressions/#types)

<table>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/expressions/#types-array">array</a></td>
    <td>❌ Not supported. The expression engine in libshumate does not perform ahead-of-time type checking.</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/expressions/#types-boolean">boolean</a></td>
    <td>❌ Not supported. The expression engine in libshumate does not perform ahead-of-time type checking.</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/expressions/#types-collator">collator</a></td>
    <td>Only the case-sensitive property is supported. The locale property is always set to the current locale and may not be changed.</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/expressions/#types-format">format</a></td>
    <td>The text-font style override property is not supported.</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/expressions/#types-image">image</a></td>
    <td>✅ Supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/expressions/#types-literal">literal</a></td>
    <td>Only array literals are supported, not objects.</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/expressions/#types-number">number</a></td>
    <td>❌ Not supported. The expression engine in libshumate does not perform ahead-of-time type checking.</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/expressions/#types-number-format">number-format</a></td>
    <td>❌ Not supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/expressions/#types-object">object</a></td>
    <td>❌ Not supported. The expression engine in libshumate does not perform ahead-of-time type checking.</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/expressions/#types-string">string</a></td>
    <td>❌ Not supported. The expression engine in libshumate does not perform ahead-of-time type checking.</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/expressions/#types-to-boolean">to-boolean</a></td>
    <td>✅ Supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/expressions/#types-to-color">to-color</a></td>
    <td>✅ Supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/expressions/#types-to-number">to-number</a></td>
    <td>✅ Supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/expressions/#types-to-string">to-string</a></td>
    <td>✅ Supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/expressions/#types-typeof">typeof</a></td>
    <td>✅ Supported. Arrays always return "array", without information about the array's elements like in MapLibre GL JS. Additionally, because libshumate's type system is less static than MapLibre GL JS's, results may differ in some (likely not common) cases.</td>
  </tr>
</table>

### [Feature data](https://maplibre.org/maplibre-gl-js-docs/style-spec/expressions/#feature-data)

<table>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/expressions/#accumulated">accumulated</a></td>
    <td>❌ Not supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/expressions/#feature-state">feature-state</a></td>
    <td>❌ Not supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/expressions/#geometry-type">geometry-type</a></td>
    <td>✅ Supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/expressions/#id">id</a></td>
    <td>✅ Supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/expressions/#line-progress">line-progress</a></td>
    <td>❌ Not supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/expressions/#properties">properties</a></td>
    <td>❌ Not supported</td>
  </tr>
</table>

### [Lookup](https://maplibre.org/maplibre-gl-js-docs/style-spec/expressions/#lookup)

<table>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/expressions/#at">at</a></td>
    <td>✅ Supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/expressions/#get">get</a></td>
    <td>The object argument is not supported.</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/expressions/#has">has</a></td>
    <td>The object argument is not supported.</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/expressions/#in">in</a></td>
    <td>Only supports arrays, not substring search.</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/expressions/#index-of">index-of</a></td>
    <td>✅ Supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/expressions/#length">length</a></td>
    <td>✅ Supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/expressions/#slice">slice</a></td>
    <td>✅ Supported</td>
  </tr>
</table>

### [Decision](https://maplibre.org/maplibre-gl-js-docs/style-spec/expressions/#decision)

<table>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/expressions/#!">!</a></td>
    <td>✅ Supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/expressions/#!=">!=</a></td>
    <td>✅ Supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/expressions/#<">&lt;</a></td>
    <td>✅ Supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/expressions/#<=">&lt;=</a></td>
    <td>✅ Supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/expressions/#==">==</a></td>
    <td>✅ Supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/expressions/#&gt;">&gt;</a></td>
    <td>✅ Supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/expressions/#&gt;=">&gt;=</a></td>
    <td>✅ Supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/expressions/#all">all</a></td>
    <td>✅ Supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/expressions/#any">any</a></td>
    <td>✅ Supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/expressions/#case">case</a></td>
    <td>✅ Supported. The fallback argument is optional in libshumate; if no case matches,
    then the expression fails to evaluate.</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/expressions/#coalesce">coalesce</a></td>
    <td>✅ Supported. Arguments that fail to evaluate will be skipped rather than
    causing the entire expression to fail.</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/expressions/#match">match</a></td>
    <td>✅ Supported. The fallback argument is optional in libshumate; if no case matches,
    then the expression fails to evaluate.</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/expressions/#within">within</a></td>
    <td>❌ Not supported</td>
  </tr>
</table>

### [Ramps, scales, curves](https://maplibre.org/maplibre-gl-js-docs/style-spec/expressions/#ramps-scales-curves)

<table>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/expressions/#interpolate">interpolate</a></td>
    <td>Only linear and exponential interpolation is supported.</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/expressions/#interpolate-hcl">interpolate-hcl</a></td>
    <td>❌ Not supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/expressions/#interpolate-lab">interpolate-lab</a></td>
    <td>❌ Not supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/expressions/#step">step</a></td>
    <td>✅ Supported</td>
  </tr>
</table>

### [Variable binding](https://maplibre.org/maplibre-gl-js-docs/style-spec/expressions/#variable-binding)

<table>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/expressions/#let">let</a></td>
    <td>✅ Supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/expressions/#var">var</a></td>
    <td>✅ Supported</td>
  </tr>
</table>

### [String](https://maplibre.org/maplibre-gl-js-docs/style-spec/expressions/#string)

<table>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/expressions/#concat">concat</a></td>
    <td>✅ Supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/expressions/#downcase">downcase</a></td>
    <td>✅ Supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/expressions/#is-supported-script">is-supported-script</a></td>
    <td>❌ Not supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/expressions/#resolved-locale">resolved-locale</a></td>
    <td>✅ Supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/expressions/#upcase">upcase</a></td>
    <td>✅ Supported</td>
  </tr>
</table>

### [Color](https://maplibre.org/maplibre-gl-js-docs/style-spec/expressions/#color)

<table>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/expressions/#rgb">rgb</a></td>
    <td>❌ Not supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/expressions/#rgba">rgba</a></td>
    <td>❌ Not supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/expressions/#rgb">to-rgba</a></td>
    <td>❌ Not supported</td>
  </tr>
</table>

### [Math](https://maplibre.org/maplibre-gl-js-docs/style-spec/expressions/#math)

<table>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/expressions/#-">- (subtraction)</a></td>
    <td>✅ Supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/expressions/#*">* (multiplication)</a></td>
    <td>✅ Supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/expressions/#/">/ (division)</a></td>
    <td>✅ Supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/expressions/#%">% (remainder)</a></td>
    <td>✅ Supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/expressions/#^">^ (exponent)</a></td>
    <td>✅ Supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/expressions/#+">+ (sum)</a></td>
    <td>✅ Supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/expressions/#abs">abs</a></td>
    <td>✅ Supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/expressions/#acos">acos</a></td>
    <td>✅ Supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/expressions/#asin">asin</a></td>
    <td>✅ Supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/expressions/#atan">atan</a></td>
    <td>✅ Supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/expressions/#ceil">ceil</a></td>
    <td>✅ Supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/expressions/#cos">cos</a></td>
    <td>✅ Supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/expressions/#distance">distance</a></td>
    <td>❌ Not supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/expressions/#e">e</a></td>
    <td>✅ Supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/expressions/#floor">floor</a></td>
    <td>✅ Supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/expressions/#ln">ln</a></td>
    <td>✅ Supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/expressions/#ln2">ln2</a></td>
    <td>✅ Supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/expressions/#log10">log10</a></td>
    <td>✅ Supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/expressions/#log2">log2</a></td>
    <td>✅ Supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/expressions/#max">max</a></td>
    <td>✅ Supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/expressions/#min">min</a></td>
    <td>✅ Supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/expressions/#pi">pi</a></td>
    <td>✅ Supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/expressions/#round">round</a></td>
    <td>✅ Supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/expressions/#sin">sin</a></td>
    <td>✅ Supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/expressions/#sqrt">sqrt</a></td>
    <td>✅ Supported</td>
  </tr>
  <tr>
    <td><a href="https://maplibre.org/maplibre-gl-js-docs/style-spec/expressions/#tan">tan</a></td>
    <td>✅ Supported</td>
  </tr>
</table>

### [Zoom](https://maplibre.org/maplibre-gl-js-docs/style-spec/expressions/#zoom)

✅ Supported. Zoom expressions are supported anywhere in libshumate, not just
as the input to a top-level "step" or "interpolate" expression.

### [Heatmap](https://maplibre.org/maplibre-gl-js-docs/style-spec/expressions/#heatmap)

❌ Not supported
