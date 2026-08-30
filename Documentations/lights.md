# Lights and Materials

This document covers the three light types and all material types supported by
the raytracer. Lights control how surfaces are illuminated. Materials control
how surfaces scatter, reflect, refract, or emit light.

---

## Table of contents

**Lights**

1. [AmbientLight](#1-ambientlight)
2. [PointLight](#2-pointlight)
3. [DirectionalLight](#3-directionallight)

**Materials**

4. [FlatMaterial (builtin)](#4-flatmaterial-builtin)
5. [ReflectiveMaterial (builtin)](#5-reflectivematerial-builtin)
6. [GlossyMaterial (plugin)](#6-glossymaterial-plugin)
7. [TransparentMaterial (plugin)](#7-transparentmaterial-plugin)
8. [EmissiveMaterial and EmissiveDiffuseMaterial (plugin)](#8-emissivematerial-and-emissivediuffusematerial-plugin)
9. [PBRMaterial (plugin)](#9-pbrmaterial-plugin)
10. [Texture types](#10-texture-types)

---

## 1. AmbientLight

**Source:** `src/lights/AmbientLight.cpp`  
**Type key:** `"ambient"`

Ambient light contributes a constant background illumination to every visible
surface, regardless of orientation or occlusion. It approximates the indirect
light that arrives from the environment after many bounces.

### Behavior

- `getDirection()` returns `Vec3(0, 0, 0)`. There is no light direction.
- `getIntensity()` returns `color * intensity`.
- `isDirectional()` returns `false`.
- No shadow ray is cast. The ambient contribution is added unconditionally to
  every lit surface.

In the path tracer ambient lights are skipped during direct lighting
accumulation and are instead used as a scene-level emission floor.

### Configuration fields

| Field | Type | Description |
|---|---|---|
| `type` | `"ambient"` | Required |
| `color` | `[r, g, b]` | Light color (0-1) |
| `intensity` | `double` | Brightness multiplier |

### Example

```cfg
{
    type = "ambient";
    color = [1.0, 1.0, 1.0];
    intensity = 0.2;
}
```

### Recommendations

Keep ambient intensity low (0.1-0.3). High ambient values wash out the scene
by preventing any surface from appearing dark. When using the path tracer,
ambient contribution is usually not necessary because global illumination
already provides indirect lighting.

---

## 2. PointLight

**Source:** `src/lights/PointLight.cpp`  
**Type key:** `"point"`

A point light emits light equally in all directions from a single position in
space. It attenuates with the inverse square of the distance, matching the
physical behaviour of a small isotropic emitter.

### Behavior

- `getDirection(point)` returns `position - point` (unnormalized). The length
  of this vector is the distance from the surface point to the light.
- `getIntensity()` returns the raw `color * intensity` value. Attenuation is
  applied by the renderer:

$$I_\text{eff} = \frac{I}{\max(d^2, 0.01)}$$

  where $d = \|\text{position} - \text{point}\|$. The floor of $0.01$ prevents
  division by zero at very close range.
- `isDirectional()` returns `false`.
- A shadow ray is cast from the surface point to the light position. The ray
  covers the interval $[0.001, d - 0.001]$ to avoid self-intersection and
  over-shooting.

### Configuration fields

| Field | Type | Description |
|---|---|---|
| `type` | `"point"` | Required |
| `position` | `[x, y, z]` | World-space position of the light |
| `color` | `[r, g, b]` | Light color (0-1) |
| `intensity` | `double` | Brightness multiplier |

### Example

```cfg
{
    type = "point";
    position = [0.0, 4.0, 2.0];
    color = [1.0, 0.95, 0.9];
    intensity = 2.0;
}
```

### Choosing intensity values

Because of inverse-square attenuation, the perceived brightness scales
strongly with distance. A light with intensity 1.0 placed 2 units away provides
the same direct contribution as a light with intensity 4.0 placed 4 units away.
In path tracing scenes with small rooms, values of 2-10 are typical; for large
outdoor-scale scenes with emissive area lights driving the illumination, point
lights may require values in the hundreds.

---

## 3. DirectionalLight

**Source:** `src/lights/DirectionalLight.cpp`  
**Type key:** `"directional"`

A directional light simulates an infinitely distant source (such as the sun).
All rays from a directional light travel in a single fixed direction and carry
no distance attenuation.

### Behavior

- `getDirection(point)` returns the light direction negated (i.e., toward the
  light source), not dependent on `point`.
- `getIntensity()` returns `color * intensity` without attenuation.
- `isDirectional()` returns `true`.
- Shadow rays are fired with a large maximum distance (`tMax = 1e10`) so that
  any geometry, no matter how far away, can cast a shadow.

### Configuration fields

| Field | Type | Description |
|---|---|---|
| `type` | `"directional"` | Required |
| `direction` | `[x, y, z]` | Direction the light travels (need not be unit-length) |
| `color` | `[r, g, b]` | Light color (0-1) |
| `intensity` | `double` | Brightness multiplier |

### Example

```cfg
{
    type = "directional";
    direction = [0.3, -1.0, -0.5];
    color = [1.0, 1.0, 1.0];
    intensity = 0.8;
}
```

### Notes

Point the direction toward the lit surface: a sun near the horizon pointing
slightly downward and to the side ($[0.3, -1.0, -0.5]$ for instance) is a
common choice for outdoor scenes. The direction is normalised at construction.

---

## 4. FlatMaterial (builtin)

**Source:** `src/materials/FlatMaterial.cpp`  
**Type key:** Default when no `material` field is present (or `"flat"`)

The flat material is a pure Lambertian diffuse surface with no specular
component. It is the default when no explicit material type is given.

### Properties

- `getDiffuse()` returns the surface color (possibly sampled from a texture).
- `isReflective()` returns `false`.
- `isTransparent()` returns `false`.
- `getEmission()` returns `Color(0, 0, 0)`.

### Configuration

The material is selected implicitly. To receive a texture on a flat surface,
use the inline `texture` block inside the object definition.

---

## 5. ReflectiveMaterial (builtin)

**Source:** `src/materials/ReflectiveMaterial.cpp`  
**Type key:** `"reflective"`

A mirror-like material that mixes direct lighting with a perfect specular
reflection ray.

### Properties

- `isReflective()` returns `true`.
- `getReflectivity()` returns the configured value (typically 0.5-1.0).
- Direct lighting and the reflection contribution are blended:

$$C = (1 - k_r) \cdot C_\text{direct} + k_r \cdot C_\text{reflected}$$

### Configuration fields added to an object block

| Field | Type | Description |
|---|---|---|
| `material` | `"reflective"` | Activate the material |
| `reflectivity` | `double` | Blend factor (0 = no reflection, 1 = mirror) |

### Example

```cfg
{
    type = "sphere";
    center = [0.0, 1.0, 0.0];
    radius = 1.0;
    color = [0.9, 0.9, 0.9];
    material = "reflective";
    reflectivity = 0.9;
}
```

---

## 6. GlossyMaterial (plugin)

**Source:** `plugins/src/materials/glossy_material.cpp`  
**Type key:** `"glossy"`

The glossy material combines a diffuse color with specular reflection, blended
by a `reflectivity` parameter. Shininess is derived from a `roughness` value.

### Properties

- `getDiffuse()` returns `color * (1 - reflectivity)`.
- `getSpecular()` returns `Color(1, 1, 1) * reflectivity`.
- `getShininess()` returns `200 * (1 - roughness)`. A roughness of 0 gives
  maximum shininess (200), roughness of 1 gives a very broad highlight.
- `isReflective()` returns `true`.

### Configuration fields

| Field | Type | Description |
|---|---|---|
| `material` | `"glossy"` | Activate the material |
| `reflectivity` | `double` | Specular blend factor (0-1) |
| `refractiveIndex` | `double` | Repurposed to encode roughness: roughness = `refractiveIndex - 1.0` when the value is in (1.0, 2.0) |

### Example

```cfg
{
    type = "sphere";
    center = [0.0, 1.0, 0.0];
    radius = 1.0;
    color = [0.9, 0.7, 0.2];
    material = "glossy";
    reflectivity = 0.4;
    refractiveIndex = 1.2;
}
```

In the example above, `refractiveIndex = 1.2` maps to `roughness = 0.2`, giving
a moderately shiny highlight.

---

## 7. TransparentMaterial (plugin)

**Source:** `plugins/src/materials/transparent.cpp`  
**Type key:** `"transparent"`, `"glass"`, `"water"`, `"diamond"`

Simulates a dielectric (transparent) surface using Snell's law for refraction
and the Schlick approximation for the Fresnel reflectance.

### Properties

- `isTransparent()` returns `true`.
- `isReflective()` returns `true` (the renderer blends refraction and
  reflection using the Fresnel term).
- `getRefractiveIndex()` returns the configured IOR.

### Physics

For a ray entering a medium with IOR $\eta_t$ from a medium with IOR $\eta_i$,
let $\eta = \eta_i / \eta_t$ and $\cos\theta_i = |\hat{d} \cdot \hat{n}|$.

The refracted direction is:

$$\hat{t} = \eta \hat{d} - \hat{n}(\eta \cos\theta_i - \sqrt{1 - \eta^2(1 - \cos^2\theta_i)})$$

When the discriminant $1 - \eta^2(1 - \cos^2\theta_i) < 0$, total internal
reflection occurs and only the reflected ray is traced.

The Schlick approximation blends reflected and refracted contributions:

$$r_0 = \left(\frac{1 - \eta_t}{1 + \eta_t}\right)^2, \quad F = r_0 + (1 - r_0)(1 - \cos\theta_i)^5$$

$$C = F \cdot C_\text{reflected} + (1 - F) \cdot C_\text{refracted}$$

### Preset types and their IOR values

| Type key | IOR | Reflectivity |
|---|---|---|
| `"glass"` | 1.50 | 0.90 |
| `"water"` | 1.33 | 0.02 |
| `"diamond"` | 2.42 | 0.95 |
| `"transparent"` | user-defined (default 1.5) | user-defined |

### Configuration fields

| Field | Type | Description |
|---|---|---|
| `material` | `"transparent"` / `"glass"` / ... | Activate the material |
| `refractive_index` | `double` | IOR for `"transparent"` type only |
| `reflectivity` | `double` | Explicit reflectivity for `"transparent"` type |

### Example

```cfg
{
    type = "sphere";
    center = [0.4, 0.3, 0.3];
    radius = 0.3;
    color = [0.9, 0.9, 0.9];
    material = "glass";
}
```

---

## 8. EmissiveMaterial and EmissiveDiffuseMaterial (plugin)

**Source:** `plugins/src/materials/emissive_material.cpp`  
**Type keys:** `"emissive"`, `"emissive_diffuse"`, and named presets

Emissive materials turn surfaces into light sources. They are used in
combination with point or area light emitters to represent glowing geometry.

### EmissiveMaterial

A pure emitter with no diffuse scattering. When hit by a ray, it returns its
emission color scaled by `emissionStrength`. No secondary rays are traced from
an emissive surface.

### EmissiveDiffuseMaterial

Combines emission with Lambertian diffuse scattering (cosine-weighted hemisphere
sample). The surface both emits light and scatters incoming light, making it
behave like a lit matte surface.

### Configuration fields

| Field | Type | Description |
|---|---|---|
| `material` | `"emissive"` or `"emissive_diffuse"` | Activate the material |
| `color` | `[r, g, b]` | Emission color |
| `emissionStrength` | `double` | Emission intensity multiplier |

### Example

```cfg
{
    type = "sphere";
    center = [0.0, 3.0, 0.0];
    radius = 0.5;
    color = [1.0, 0.0, 0.3];
    material = "emissive";
    emissionStrength = 100.0;
}
```

### Built-in presets

The following named presets are registered and can be used as the `material`
value directly without specifying a color or emission strength:

| Preset key | Color (RGB) | Description |
|---|---|---|
| `"neon_red"` | (1.0, 0.1, 0.1) | Bright red neon |
| `"neon_blue"` | (0.1, 0.2, 1.0) | Bright blue neon |
| `"neon_cyan"` | (0.0, 0.9, 1.0) | Cyan neon |
| `"neon_pink"` | (1.0, 0.1, 0.7) | Magenta-pink neon |
| `"neon_green"` | (0.1, 1.0, 0.2) | Bright green neon |
| `"neon_orange"` | (1.0, 0.5, 0.0) | Warm orange neon |
| `"warm_light"` | (1.0, 0.85, 0.6) | Incandescent warm white |
| `"cool_light"` | (0.8, 0.9, 1.0) | Cool daylight white |

---

## 9. PBRMaterial (plugin)

**Source:** `plugins/src/materials/pbr_material.cpp`  
**Type keys:** `"pbr"` and named presets (e.g. `"pbr_gold"`, `"pbr_silver"`, ...)

A physically-based material implementing the Cook-Torrance microfacet BRDF. It
supports metallic workflow, roughness, and optional emission.

### Cook-Torrance BRDF

The reflectance model is:

$$f_r = \frac{k_d \rho}{\pi} + \frac{D \cdot G \cdot F}{4 \cdot (\hat{n}\cdot\hat{v})(\hat{n}\cdot\hat{l})}$$

where the terms are:

**Normal Distribution Function (NDF) - GGX/Trowbridge-Reitz:**

$$D = \frac{\alpha^4}{\pi \bigl[(\hat{n}\cdot\hat{h})^2(\alpha^4 - 1) + 1\bigr]^2}$$

with $\alpha = \text{roughness}^2$.

**Geometry term - Smith-Schlick-GGX:**

$$G = G_1(\hat{n}\cdot\hat{v}) \cdot G_1(\hat{n}\cdot\hat{l}), \quad G_1(x) = \frac{x}{x(1-k)+k}, \quad k = \frac{(\text{roughness}+1)^2}{8}$$

**Fresnel term - Schlick approximation:**

$$F = F_0 + (1 - F_0)(1 - \hat{h}\cdot\hat{v})^5$$

For metallic surfaces $F_0$ is the albedo color; for dielectrics $F_0 = (0.04, 0.04, 0.04)$. The blend is:

$$F_0 = (0.04)(1 - \text{metallic}) + \text{albedo} \cdot \text{metallic}$$

**Diffuse term:**

$$k_d = (1 - F)(1 - \text{metallic})$$

Only dielectric surfaces have a diffuse component. Pure metals $(m = 1)$ have
$k_d = 0$.

### Importance sampling

The `scatter()` method uses multiple importance sampling (MIS) to choose
between a GGX specular sample and a cosine-weighted diffuse sample:

- The mixing weight $w_s$ is derived from the average Fresnel term $F_0$
  relative to the total energy.
- A uniform random number selects specular sampling with probability $w_s$ and
  diffuse sampling otherwise.
- For highly metallic surfaces ($m > 0.9$) specular sampling is always used.
- The returned `ScatterResult.attenuation` bakes in the full BRDF evaluation
  divided by the PDF so that the caller trajectory sample needs only multiply
  by the recursive radiance.

### Configuration fields

| Field | Type | Description |
|---|---|---|
| `material` | `"pbr"` or a preset key | Activate the material |
| `color` | `[r, g, b]` | Albedo |
| `metallic` | `double` | Metallic factor (0 = dielectric, 1 = full metal) |
| `roughness` | `double` | Micro-surface roughness (0 = mirror, 1 = fully diffuse) |
| `emissionStrength` | `double` | Optional emission intensity |

### Fixed presets (full color + metallic + roughness)

| Preset | Albedo (RGB) | Metallic | Roughness |
|---|---|---|---|
| `pbr_gold` | (1.00, 0.77, 0.34) | 1.0 | 0.10 |
| `pbr_silver` | (0.97, 0.96, 0.92) | 1.0 | 0.05 |
| `pbr_copper` | (0.96, 0.64, 0.54) | 1.0 | 0.15 |
| `pbr_aluminum` | (0.91, 0.92, 0.92) | 1.0 | 0.20 |
| `pbr_iron` | (0.56, 0.57, 0.58) | 1.0 | 0.40 |
| `pbr_chrome` | (0.55, 0.56, 0.55) | 1.0 | 0.02 |
| `pbr_jade` | (0.05, 0.30, 0.20) | 0.0 | 0.25 |
| `pbr_marble` | (0.90, 0.90, 0.88) | 0.0 | 0.12 |
| `pbr_wood` | (0.40, 0.25, 0.15) | 0.0 | 0.70 |
| `pbr_glass_rough` | (1.00, 1.00, 1.00) | 0.0 | 0.02 |

### Parametric presets (metallic + roughness only, albedo from `color` field)

| Preset | Metallic | Roughness |
|---|---|---|
| `pbr_plastic` | 0.0 | 0.30 |
| `pbr_glossy_plastic` | 0.0 | 0.05 |
| `pbr_rubber` | 0.0 | 0.80 |
| `pbr_ceramic` | 0.0 | 0.15 |
| `pbr_rough_metal` | 1.0 | 0.60 |
| `pbr_polished_metal` | 1.0 | 0.05 |

### Example

```cfg
{
    type = "sphere";
    center = [2.5, 1.1, -1.5];
    radius = 0.6;
    color = [1.0, 0.8, 0.3];
    material = "pbr_gold";
}
```

---

## 10. Texture types

Textures can be assigned to any primitive via an inline `texture` block inside
the object definition, or via the `material` system when the material supports
textures.

### SolidColorTexture

A constant uniform color. This is the fallback texture created automatically
from the object's `color` field.

### CheckerTexture

An alternating two-color checkerboard pattern.

| Field | Type | Description |
|---|---|---|
| `type` | `"checker"` | Required |
| `color1` | `[r, g, b]` | First color |
| `color2` | `[r, g, b]` | Second color |
| `scale` | `double` | Controls checker tile size |

```cfg
texture = {
    type = "checker";
    color1 = [0.1, 0.1, 0.1];
    color2 = [0.9, 0.9, 0.9];
    scale = 1.5;
};
```

### ImageTexture

Loads an image file and maps it onto the primitive's UV coordinates.

| Field | Type | Description |
|---|---|---|
| `type` | `"image"` | Required |
| `path` | `string` | Path to the image file (PNG, JPG, etc.) |

```cfg
texture = {
    type = "image";
    path = "textures/earth.png";
};
```
