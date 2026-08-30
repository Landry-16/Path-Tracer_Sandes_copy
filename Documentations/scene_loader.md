# Scene Loader

This document describes the scene configuration file format, its full syntax,
and the internal parsing pipeline implemented in `src/parser/LibconfigLoader.cpp`.

---

## Table of contents

1. [File format overview](#1-file-format-overview)
2. [Camera block](#2-camera-block)
3. [Objects list](#3-objects-list)
4. [Lights list](#4-lights-list)
5. [Material fields reference](#5-material-fields-reference)
6. [Transform fields reference](#6-transform-fields-reference)
7. [Texture block reference](#7-texture-block-reference)
8. [Internal parsing pipeline](#8-internal-parsing-pipeline)
9. [Error handling](#9-error-handling)

---

## 1. File format overview

Scene files use the **libconfig** format (`.cfg` extension). The syntax is
similar to JSON but more permissive: values can be bare, strings are quoted,
arrays use `[]`, lists use `()`, and groups use `{}`. Comments start with `#`
or `//`.

A valid scene file has three top-level sections:

```cfg
camera = { ... };

objects = ( { ... }, { ... }, ... );

lights  = ( { ... }, { ... }, ... );
```

The `objects` and `lights` sections are optional. A missing `camera` section
raises a parse error.

---

## 2. Camera block

```cfg
camera = {
    position      = [0.0, 1.0, 4.5];   # required
    lookAt        = [0.0, 1.0, 0.0];   # required
    up            = [0.0, 1.0, 0.0];   # required
    fov           = 40.0;              # required, vertical field of view in degrees
    width         = 800;               # optional, default 800
    height        = 600;               # optional, default 600
    antialiasing  = 4;                 # optional, samples per axis, default 1
    aperture      = 0.05;              # optional, enables depth-of-field
    focus_distance = 4.0;             # optional, focus plane distance, default 1.0
};
```

| Field | Type | Required | Description |
|---|---|---|---|
| `position` | `[x, y, z]` | Yes | Camera position in world space |
| `lookAt` | `[x, y, z]` | Yes | Point the camera aims at |
| `up` | `[x, y, z]` | Yes | World up vector (usually `[0, 1, 0]`) |
| `fov` | `double` | Yes | Vertical field of view in degrees |
| `width` | `int` | No (800) | Image width in pixels |
| `height` | `int` | No (600) | Image height in pixels |
| `antialiasing` | `int` | No (1) | Samples per axis for the Whitted renderer (NxN total) |
| `aperture` | `double` | No (0) | Lens aperture radius; 0 disables depth-of-field |
| `focus_distance` | `double` | No (1.0) | Distance from camera to the sharp focus plane |

Both the libconfig array syntax (`[x, y, z]`) and the struct syntax
(`{ x = ...; y = ...; z = ...; }`) are accepted for vector fields.

---

## 3. Objects list

The objects section is a libconfig **list** (parentheses). Each element is a
group (braces) with at minimum a `type` field.

```cfg
objects = (
    {
        type   = "sphere";
        center = [0.0, 1.0, 0.0];
        radius = 1.0;
        color  = [0.9, 0.3, 0.3];
    },
    {
        type   = "plane";
        point  = [0.0, 0.0, 0.0];
        normal = [0.0, 1.0, 0.0];
        color  = [0.8, 0.8, 0.8];
    }
);
```

### Geometry fields per type

| Type key | Position field | Direction / normal field | Size field |
|---|---|---|---|
| `sphere` | `center` | (none) | `radius` |
| `plane` | `point` | `normal` | (none) |
| `box` | `center` | (none) | `radius` (half-side) |
| `cone` | `center` | (none) | `radius` |
| `cylinder` | `center` | (none) | `radius` |
| `torus` | `center` | `direction` (minor radius in X) | `radius` (major) |
| `pyramid` | `center` | (none) | `radius` (half-base) |
| `triangle` | `center` | (none) | `radius` |
| `obj` | (none) | (none) | `scale` |

The `obj` type additionally requires a `path` field pointing to the `.obj` file.

### Common optional fields for all objects

These fields are accepted by every primitive:

| Field | Type | Description |
|---|---|---|
| `color` | `[r, g, b]` | Diffuse color (0-1) |
| `material` | `string` | Material type key (see lights.md) |
| `reflectivity` | `double` | Blend factor for reflective/glossy materials |
| `refractive_index` | `double` | Index of refraction for transparent materials |
| `emissionStrength` | `double` | Emission multiplier for emissive materials |
| `roughness` | `double` | Surface roughness for PBR/glossy materials |
| `specular` | `[r, g, b]` | Specular color for Phong shading |
| `shininess` | `double` | Specular exponent for Phong shading |
| `texture` | `{ ... }` | Inline texture block (see Section 7) |
| `rotation` | `[rx, ry, rz]` | Euler rotation in degrees |
| `scale` | `[sx, sy, sz]` | Per-axis scale |
| `translation` | `[tx, ty, tz]` | World-space position offset |

---

## 4. Lights list

The lights section follows the same list-of-groups format as objects.

```cfg
lights = (
    {
        type      = "ambient";
        color     = [1.0, 1.0, 1.0];
        intensity = 0.2;
    },
    {
        type      = "point";
        position  = [0.0, 4.0, 2.0];
        color     = [1.0, 0.95, 0.9];
        intensity = 3.0;
    },
    {
        type      = "directional";
        direction = [0.3, -1.0, -0.5];
        color     = [1.0, 1.0, 1.0];
        intensity = 0.6;
    }
);
```

All three fields (`color`, `intensity`, and the type-specific `position` or
`direction`) are required for the light to work correctly. Missing position or
direction fields fall back to `[0, 0, 0]` and `[0, -1, 0]` respectively.

---

## 5. Material fields reference

When a `material` key is present, the string value names a registered material
type. The associated behavior fields are read from the same object block:

| Scenario | Required fields |
|---|---|
| Default flat | Only `color` |
| `"reflective"` | `color`, `reflectivity` |
| `"glossy"` | `color`, `reflectivity`, `refractiveIndex` (encodes roughness) |
| `"transparent"` / `"glass"` / `"water"` / `"diamond"` | `color`, optionally `refractive_index` |
| `"emissive"` / `"emissive_diffuse"` | `color`, `emissionStrength` |
| `"pbr"` | `color`, `metallic`, `roughness`, optionally `emissionStrength` |
| PBR preset (`"pbr_gold"`, etc.) | Optionally `color` (overrides albedo for param-type presets) |
| Neon/warm presets | No extra fields needed |

Unknown material type strings produce a warning on stderr and the object keeps
no material (rendered black).

---

## 6. Transform fields reference

Transforms are applied at parse time to produce the primitive's `Matrix4`
transform stored at construction. The application order is:

1. Scale
2. Rotation (Z then Y then X, i.e. `Rz * Ry * Rx`)
3. Translation

```cfg
{
    type     = "sphere";
    center   = [0.0, 0.0, 0.0];
    radius   = 1.0;
    color    = [0.8, 0.2, 0.2];
    rotation    = [45.0, 0.0, 0.0];
    scale       = [1.0, 2.0, 1.0];
    translation = [0.0, 1.0, 0.0];
}
```

Note that `center`/`point` already moves the primitive in world space. The
`translation` field applies an additional offset on top of that, which is
rarely needed unless the object needs to orbit a pivot that differs from its
geometric centre.

---

## 7. Texture block reference

An inline `texture` block can be added to any object. If the specified texture
type is not registered the block is silently skipped and the flat color is used.

```cfg
texture = {
    type   = "checker";
    color1 = [0.1, 0.1, 0.1];
    color2 = [0.9, 0.9, 0.9];
    scale  = 1.5;
};
```

```cfg
texture = {
    type = "image";
    path = "textures/earth.png";
};
```

| Field | Used by | Description |
|---|---|---|
| `type` | all | Registered texture type key |
| `color1` | `checker` | First color; defaults to the object's `color` if absent |
| `color2` | `checker` | Second color; defaults to black if absent |
| `scale` | `checker` | Tile size scale factor |
| `path` | `image` | Filesystem path to the image |

---

## 8. Internal parsing pipeline

### `loadScene(filename)`

1. Opens the file with `libconfig::Config::readFile()`. Throws a
   `std::runtime_error` on IO or parse failure.
2. Verifies that a `camera` section exists.
3. Calls `loadCamera()`, then `loadObjects()` (if present), then
   `loadLights()` (if present).
4. Returns the populated `std::unique_ptr<Scene>`.

### `loadCamera()`

Reads all camera fields using `readOr<T>()` helpers that return a default value
when the key is absent. Creates a `Camera` object and calls
`camera->setDepthOfField()` when aperture is greater than zero.

### `loadObjects()`

Iterates over the objects list. For each entry:

1. Reads the `type` string.
2. Reads `color` (optional for OBJ meshes).
3. Calls `parseTextureSettings()` to build a `TextureParams` and check if the
   texture type is registered.
4. Constructs a `MaterialContext` from the collected fields.
5. Calls `createObjectMaterial()` to build and register the material.
6. Calls `parseObjectTransform()` to build the `Matrix4` transform.
7. Calls `createAndAddPrimitive()` to instantiate the primitive via the
   `PrimitiveRegistry` and push it into `scene.primitives`.

### `createObjectMaterial()`

Determines whether to create a material override:
- OBJ objects without an explicit `material` field, no explicit color, and no
  texture inherit material from the OBJ file directly.
- All other objects use the resolved material type (defaulting to `"flat"`).

The `MaterialParams` struct is filled with all relevant fields from the config
block and passed to `Factory::createMaterial()`.

### `loadLights()`

Iterates over the lights list. For each entry reads `type`, `color`,
`intensity`, `position` (point lights), and `direction` (directional lights).
Instantiates the light via `Factory::createLight()` and pushes it into
`scene.lights`.

### Helper functions

| Function | Purpose |
|---|---|
| `readVec3(setting)` | Reads a `[x, y, z]` or struct setting into `Vec3` |
| `readVec3Or(s, key, default)` | Same but returns a default if the key is absent |
| `readColor(setting)` | Alias for `readVec3`, reads RGB from `[r, g, b]` |
| `readOr<T>(s, key, default)` | Reads a scalar or returns a default |
| `readStringOr(s, key, default)` | Reads a string or returns a default |

---

## 9. Error handling

The loader raises `std::runtime_error` (caught by `main()`) for:
- File not found or unreadable.
- libconfig parse errors (with line/column from the exception).
- Missing required `camera` block.

Non-fatal issues print a warning to `stderr` and are skipped:
- Unknown `type` string for a primitive, material, light, or texture.
- A `texture` block whose type is not registered.
- A `Vec3Or` or `readOr` key that is absent (uses the default instead).
