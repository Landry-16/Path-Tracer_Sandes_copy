# Primitives

This document covers every geometric primitive available in the raytracer, both
built-in and plugin-based. For each primitive the following aspects are
described: the type key used in scene configuration files, the accepted
configuration fields, the intersection algorithm, the UV coordinate mapping, and
transform support.

---

## Table of contents

1. [Sphere](#1-sphere)
2. [Plane](#2-plane)
3. [Box](#3-box)
4. [Cone](#4-cone)
5. [Cylinder](#5-cylinder)
6. [Torus](#6-torus)
7. [Pyramid](#7-pyramid)
8. [Triangle](#8-triangle)
9. [OBJ Mesh (plugin)](#9-obj-mesh-plugin)
10. [Transform fields](#10-transform-fields)
11. [Quick-reference table](#11-quick-reference-table)

---

## 1. Sphere

**Source:** `src/primitives/Sphere.cpp`  
**Type key:** `"sphere"`

### Configuration fields

| Field | Type | Description |
|---|---|---|
| `center` | `[x, y, z]` | Centre of the sphere in world space |
| `radius` | `double` | Radius |
| `color` | `[r, g, b]` | Base diffuse color (0-1 per channel) |
| `material` | `string` | Optional material override |
| `reflectivity` | `double` | Blend factor for specular reflection (0-1) |
| `transparent` | `bool` | Enable refraction |
| `refractive_index` | `double` | Index of refraction (e.g. 1.5 for glass) |
| `rotation` | `[rx, ry, rz]` | Euler angles in degrees |
| `scale` | `[sx, sy, sz]` | Per-axis scale |

### Example

```cfg
{
    type = "sphere";
    center = [0.0, 1.0, 0.0];
    radius = 1.0;
    color = [0.9, 0.3, 0.3];
    material = "transparent";
    refractive_index = 1.5;
}
```


### Intersection algorithm

The sphere is defined in local space as all points $p$ with $\|p\| = r$. Ray
$\mathbf{r}(t) = \mathbf{o} + t\mathbf{d}$ is first transformed into local
space using the stored inverse transform, producing $\mathbf{o}' + t\mathbf{d}'$.

Let $\mathbf{oc} = \mathbf{o}' - \mathbf{c}$ where $\mathbf{c}$ is the sphere
centre. The intersection solves:

$$a = \mathbf{d}' \cdot \mathbf{d}', \quad h = \mathbf{oc} \cdot \mathbf{d}', \quad c = \mathbf{oc} \cdot \mathbf{oc} - r^2$$

$$\Delta = h^2 - a \cdot c$$

If $\Delta < 0$ the ray misses. Otherwise both roots are:

$$t = \frac{-h \pm \sqrt{\Delta}}{a}$$

The smaller root is preferred if it falls in $[t\min, t\max]$, otherwise the
larger root is tried. The hit point and outward normal are then transformed back
to world space. The normal is transformed by the transpose of the inverse
transform matrix (i.e., the inverse-transpose), which is the correct method for
non-uniform scales.

### UV mapping

After computing the local-space surface normal $\hat{n}$:

$$u = \frac{1}{2} + \frac{\text{atan2}(n_z, n_x)}{2\pi}, \quad v = \frac{1}{2} - \frac{\text{asin}(n_y)}{\pi}$$

This is a standard spherical (equirectangular) parameterisation where $u$
increases east and $v$ increases from north to south.

---

## 2. Plane

**Source:** `src/primitives/Plane.cpp`  
**Type key:** `"plane"`

### Configuration fields

| Field | Type | Description |
|---|---|---|
| `point` | `[x, y, z]` | Any point on the plane |
| `normal` | `[x, y, z]` | Outward surface normal (need not be unit-length) |
| `color` | `[r, g, b]` | Base diffuse color |
| `material` | `string` | Optional material override |
| `texture` | `{ type; ... }` | Optional inline texture block |

### Example

```cfg
{
    type = "plane";
    point = [0.0, 0.0, 0.0];
    normal = [0.0, 1.0, 0.0];
    color = [0.8, 0.8, 0.8];
    texture = {
        type = "checker";
        color1 = [0.1, 0.1, 0.1];
        color2 = [0.9, 0.9, 0.9];
        scale = 1.0;
    };
}
```

### Intersection algorithm

Given point $p_0$ on the plane and unit normal $\hat{n}$, the intersection
parameter satisfies:

$$t = \frac{(p_0 - \mathbf{o}) \cdot \hat{n}}{\mathbf{d} \cdot \hat{n}}$$

If $|\mathbf{d} \cdot \hat{n}| < 10^{-8}$ the ray is nearly parallel to the
plane and no intersection is recorded.

### UV mapping

The dominant component of the normal vector determines which two world axes
serve as the UV axes:

| Dominant normal axis | U axis | V axis |
|---|---|---|
| X (`normal.x` largest) | world Z | world Y |
| Y (`normal.y` largest) | world X | world Z |
| Z (`normal.z` largest) | world X | world Y |

UV coordinates are the hit point projected onto those axes, divided by a `uvScale`
factor (default 2.0) to control texture tiling density.

---

## 3. Box

**Source:** `plugins/src/primitives/box.cpp`  
**Type key:** `"box"`

A box is an axis-aligned rectangular cuboid (AABB) centred at a given position.
Half-extents are equal on all axes and equal to the `radius` field.

### Configuration fields

| Field | Type | Description |
|---|---|---|
| `center` | `[x, y, z]` | Centre of the box |
| `radius` | `double` | Half-side length (box extends `radius` in each direction) |
| `color` | `[r, g, b]` | Base diffuse color |
| `material` | `string` | Optional material override |

### Example

```cfg
{
    type = "box";
    center = [0.0, 0.5, 0.0];
    radius = 0.5;
    color = [0.4, 0.6, 0.9];
}
```

### Intersection algorithm

Uses the standard slab test. For each axis $k \in \{x, y, z\}$:

$$t_{\text{near},k} = \frac{\min_k - o_k}{\text{inv}_k}, \quad t_{\text{far},k} = \frac{\max_k - o_k}{\text{inv}_k}$$

where $\text{inv}_k = 1/d_k$. Signs are swapped when $d_k < 0$ so that
$t_{\text{near},k} \leq t_{\text{far},k}$ always holds.

The overall interval is $[\max_k(t_{\text{near},k}),\; \min_k(t_{\text{far},k})]$. A
hit occurs when $t_\text{near} \leq t_\text{far}$ and the interval overlaps
$[t\min, t\max]$.

### Face normal and UV

The face hit is determined by the dominant axis of the normalised local hit
point. The face normal is $\pm\hat{e}_k$ where the sign matches the side the
ray entered from. UV coordinates are then read from the two remaining axes
using the mapping:

| Hit axis | U component | V component |
|---|---|---|
| X | Z component of hit point | Y component of hit point |
| Y | X component of hit point | Z component of hit point |
| Z | X component of hit point | Y component of hit point |

---

## 4. Cone

**Source:** `plugins/src/primitives/cone.cpp`  
**Type key:** `"cone"`

A finite cone with a circular base cap and a pointed apex.

### Configuration fields

| Field | Type | Description |
|---|---|---|
| `center` | `[x, y, z]` | Centre of the cone's base disk |
| `radius` | `double` | Radius of the base disk; height = `radius * 2` |
| `color` | `[r, g, b]` | Base diffuse color |
| `material` | `string` | Optional material override |

The apex is automatically placed at `center + Vec3(0, radius * 2, 0)`.

### Example

```cfg
{
    type = "cone";
    center = [2.5, 0.6, 1.0];
    radius = 0.6;
    color = [0.9, 0.9, 0.3];
}
```

### Intersection algorithm

Let the apex be $A$ and let $k = r^2/h^2$ where $r$ is the base radius and $h$
is the height.

**Lateral surface.** The quadratic in $t$ for the body is:

$$\alpha = d_x^2 + d_z^2 - k \cdot d_y^2$$
$$\beta = 2[(o_x - A_x)d_x + (o_z - A_z)d_z - k(o_y - A_y)d_y]$$
$$\gamma = (o_x-A_x)^2 + (o_z-A_z)^2 - k(o_y-A_y)^2$$

Both roots are tested. A root is valid only if the Y coordinate of the hit point
satisfies $A_y - h \leq p_y \leq A_y$ (i.e., within the vertical extent of the
cone). The surface normal at a lateral hit is:

$$\hat{n} = \text{normalize}\bigl(p_x - A_x,\; k \cdot (A_y - p_y),\; p_z - A_z\bigr)$$

**Base cap.** A disk intersection test is performed at $y = A_y - h$. A hit is
valid when $\|(p_x - C_x, p_z - C_z)\| \leq r$ where $C$ is the base centre.
The cap normal faces downward: $(0, -1, 0)$.

The closest valid hit across the lateral surface and the base cap is returned.

---

## 5. Cylinder

**Source:** `plugins/src/primitives/cylinder.cpp`  
**Type key:** `"cylinder"`

A finite cylinder with two flat end caps.

### Configuration fields

| Field | Type | Description |
|---|---|---|
| `center` | `[x, y, z]` | Centre of the cylinder (midpoint between the two caps) |
| `radius` | `double` | Radius of the cylinder barrel |
| `color` | `[r, g, b]` | Base diffuse color |
| `material` | `string` | Optional material override |

The height is automatically set to `radius * 2`.

### Example

```cfg
{
    type = "cylinder";
    center = [0.0, 1.1, 1.0];
    radius = 0.5;
    color = [0.3, 0.3, 0.9];
}
```

### Intersection algorithm

The cylinder is infinite along Y and clipped to $[c_y - h/2, c_y + h/2]$.

**Body.** The XZ projection reduces to a 2D circle test:

$$a = d_x^2 + d_z^2, \quad b = 2(oc_x d_x + oc_z d_z), \quad c = oc_x^2 + oc_z^2 - r^2$$

where $\mathbf{oc} = \mathbf{o} - \mathbf{centre}$. Both roots are tested; each
is accepted only if the Y coordinate of the hit point falls within the height
bounds.

**Caps.** Two flat disk tests are performed at $y = c_y \pm h/2$ using the
same disk-in-circle method as the cone. Cap normals are $(0, \pm 1, 0)$.

A `HitContext` structure accumulates the closest valid hit across all three
components (body + two caps). At the end the closest one is committed to the
output `HitRecord`.

---

## 6. Torus

**Source:** `plugins/src/primitives/torus.cpp`  
**Type key:** `"torus"`

A solid torus of revolution defined by a major radius $R$ (distance from the
centre of the tube to the centre of the torus) and a minor radius $r$ (radius
of the tube).

### Configuration fields

| Field | Type | Description |
|---|---|---|
| `center` | `[x, y, z]` | Centre of the torus |
| `radius` | `double` | Major radius $R$ |
| `direction` | `[minor, 0, 0]` | Minor radius $r$ taken from the X component |
| `color` | `[r, g, b]` | Base diffuse color |
| `material` | `string` | Optional material override |
| `rotation` | `[rx, ry, rz]` | Euler angles in degrees |

If `direction.x <= 0`, the minor radius defaults to `R * 0.3`.

### Example

```cfg
{
    type = "torus";
    center = [0.0, 1.5, 0.0];
    radius = 1.5;
    direction = [0.5, 0.0, 0.0];
    color = [0.6, 0.0, 0.0];
    rotation = [75.0, 30.0, 0.0];
}
```

### Intersection algorithm

The torus is intersected via **sphere tracing** (ray marching with a signed
distance function). The SDF of a torus centred at the origin in the XZ plane
is:

$$f(p) = \left\| \bigl(\sqrt{p_x^2 + p_z^2} - R,\; p_y\bigr) \right\| - r$$

Starting at the ray origin the marcher advances by the SDF value at each step.
The loop runs for at most 200 iterations with an early-exit when $|f(p)| < 10^{-3}$.

Surface normals are estimated analytically from the gradient of $f$:

$$\nabla f(p) = \frac{p - q}{\|p - q\|} \quad \text{where } q = \text{normalize}(p_x, 0, p_z) \cdot R$$

### UV mapping

$$\theta = \text{atan2}(p_z, p_x), \quad d_{xz} = \sqrt{p_x^2 + p_z^2}$$
$$\phi = \text{atan2}(p_y,\; d_{xz} - R)$$
$$u = \frac{\theta + \pi}{2\pi}, \quad v = \frac{\phi + \pi}{2\pi}$$

---

## 7. Pyramid

**Source:** `plugins/src/primitives/pyramid.cpp`  
**Type key:** `"pyramid"`

A square-base pyramid built from six triangular faces.

### Configuration fields

| Field | Type | Description |
|---|---|---|
| `center` | `[x, y, z]` | Centre of the base square |
| `radius` | `double` | Half-side length of the base; height = `radius * 2` |
| `color` | `[r, g, b]` | Base diffuse color |
| `material` | `string` | Optional material override |

The apex is at `center + Vec3(0, radius * 2, 0)`.

### Example

```cfg
{
    type = "pyramid";
    center = [5.0, 0.6, 1.0];
    radius = 0.6;
    color = [0.9, 0.3, 0.9];
}
```

### Intersection algorithm

The pyramid is decomposed into 6 `TriangleFace` objects: 4 lateral triangular
faces and 2 base triangles (the base square is split on the diagonal). Each
face is intersected individually using the Moller-Trumbore algorithm (see
Section 8). The closest hit across all faces is returned.

---

## 8. Triangle

**Source:** `plugins/src/primitives/triangle.cpp`  
**Type key:** `"triangle"`

A single flat triangle with a constant face normal.

### Configuration fields

| Field | Type | Description |
|---|---|---|
| `center` | `[x, y, z]` | Used as the first vertex `v0` |
| `radius` | `double` | Used to derive `v1` and `v2` from `v0` |
| `color` | `[r, g, b]` | Base diffuse color |
| `material` | `string` | Optional material override |

In practice, individual triangles are rarely placed manually. They appear as
the atom of OBJ mesh loading.

### Intersection algorithm: Moller-Trumbore

Given triangle vertices $v_0$, $v_1$, $v_2$ and ray $\mathbf{o} + t\mathbf{d}$:

$$\mathbf{e}_1 = v_1 - v_0, \quad \mathbf{e}_2 = v_2 - v_0$$
$$\mathbf{h} = \mathbf{d} \times \mathbf{e}_2, \quad a = \mathbf{e}_1 \cdot \mathbf{h}$$

If $|a| < 10^{-6}$ the ray is parallel to the triangle. Otherwise:

$$f = 1/a, \quad \mathbf{s} = \mathbf{o} - v_0$$
$$u = f \cdot (\mathbf{s} \cdot \mathbf{h})$$

If $u \notin [0, 1]$ the hit is outside the triangle.

$$\mathbf{q} = \mathbf{s} \times \mathbf{e}_1, \quad v = f \cdot (\mathbf{d} \cdot \mathbf{q})$$

If $v < 0$ or $u + v > 1$ the hit is outside. Finally:

$$t = f \cdot (\mathbf{e}_2 \cdot \mathbf{q})$$

The face normal is constant: $\hat{n} = \text{normalize}(\mathbf{e}_1 \times \mathbf{e}_2)$.

---

## 9. OBJ Mesh (plugin)

**Source:** `plugins/src/obj_loader.cpp`  
**Type key:** `"obj"`

Loads an external Wavefront OBJ file and registers all its triangles into the
scene. Each triangle of the mesh is treated as an individual triangle primitive.
The mesh inherits the material assigned to the object block.

### Configuration fields

| Field | Type | Description |
|---|---|---|
| `path` | `string` | Relative or absolute path to the `.obj` file |
| `color` | `[r, g, b]` | Fallback color if the OBJ has no material |
| `material` | `string` | Material type to apply to the whole mesh |
| `scale` | `double` | Uniform scale applied to all vertices |
| `rotation` | `[rx, ry, rz]` | Euler angles in degrees |

### Example

```cfg
{
    type = "obj";
    path = "obj/knight/knight.obj";
    color = [0.8, 0.85, 0.9];
    material = "pbr_chrome";
}
```

---

## 10. Transform fields

The following optional transform fields are accepted by most primitives. They
are applied at construction time to build a `Matrix4` transform and its inverse.

| Field | Type | Default | Description |
|---|---|---|---|
| `rotation` | `[rx, ry, rz]` | `[0, 0, 0]` | Euler rotation in degrees, applied as Rx * Ry * Rz |
| `scale` | `[sx, sy, sz]` or `double` | `[1, 1, 1]` | Per-axis or uniform scale |
| `translation` | `[tx, ty, tz]` | `[0, 0, 0]` | Additional world-space offset |

Rays are converted to local object space before intersection tests, and normals
are transformed back using the inverse-transpose of the matrix. This ensures
correct normals under non-uniform scaling.

---

## 11. Quick-reference table

| Type key | Category | Intersection method | UV mapping |
|---|---|---|---|
| `sphere` | builtin | Quadratic | Spherical (equirectangular) |
| `plane` | builtin | Dot-product formula | Dominant-axis planar |
| `box` | plugin | Slab test (AABB) | Per-face planar |
| `cone` | plugin | Quadratic + disk cap | Not UV-mapped |
| `cylinder` | plugin | Quadratic + two disk caps | Not UV-mapped |
| `torus` | plugin | Sphere marching (SDF) | Toroidal (theta/phi) |
| `pyramid` | plugin | Moller-Trumbore per face | Not UV-mapped |
| `triangle` | plugin | Moller-Trumbore | Not UV-mapped |
| `obj` | plugin | Moller-Trumbore per mesh triangle | Vertex UV if present |
