# Raytracing Pipeline

This document describes the complete rendering pipeline of the raytracer: how a
scene configuration is loaded, how rays are generated from the camera, how
geometry is accelerated with the BVH, and how two different renderers (a
Whitted raytracer and a path tracer) produce a final image.

---

## Table of contents

1. [Architecture overview](#1-architecture-overview)
2. [Scene configuration](#2-scene-configuration)
3. [Camera model and ray generation](#3-camera-model-and-ray-generation)
4. [BVH acceleration structure](#4-bvh-acceleration-structure)
5. [Whitted raytracer](#5-whitted-raytracer)
6. [Path tracer](#6-path-tracer)
7. [Tone mapping](#7-tone-mapping)
8. [Multithreaded rendering](#8-multithreaded-rendering)
9. [Denoising](#9-denoising)

---

## 1. Architecture overview

The renderer is split into two independent render modes selectable by a
command-line flag:

- **Renderer** (`-r` flag or default): Whitted-style recursive raytracer.
  Fast, deterministic, supports shadows, reflections, and refractions, but
  produces no global illumination.
- **PathTracer** (`-p` flag): Unidirectional Monte Carlo path tracer. Slower
  due to many samples per pixel, but produces physically accurate soft
  shadows, color bleeding, and indirect illumination.

Both modes share:
- The same scene graph (`Scene`, `Camera`, `IPrimitive` list, `ILight` list).
- The same BVH acceleration structure.
- The same material and light system.
- The same tile-based multithreaded output loop.

### High-level call flow

```
main()
  -> load scene file via LibconfigLoader
  -> build Scene
  -> select Renderer or PathTracer
  -> renderer.render(scene, samples)
      -> build BVH
      -> spawn N threads
          -> for each tile:
              -> for each pixel:
                  -> for each sample:
                      -> camera.generateRay(x, y)
                      -> trace ray through BVH
                      -> accumulate color
              -> tone-map pixel
  -> PPMWriter::write() -> output.ppm
```

---

## 2. Scene configuration

Scene files use the libconfig format (`.cfg` extension). The parser is
implemented in `src/parser/LibconfigLoader.cpp`.

### Camera block

```cfg
camera = {
    position  = [0.0, 1.0, 4.5];
    lookAt    = [0.0, 1.0, 0.0];
    up        = [0.0, 1.0, 0.0];
    fov       = 40.0;
    width     = 800;
    height    = 800;
    antialiasing = 4;          # samples per axis (optional, default 1)
    aperture  = 0.05;          # enables depth-of-field (optional)
    focusDist = 4.0;           # focus plane distance (optional)
};
```

### Objects list

```cfg
objects = (
    {
        type = "sphere";
        center = [0.0, 1.0, 0.0];
        radius = 1.0;
        color  = [0.9, 0.3, 0.3];
        material = "pbr_gold";   # optional
    },
    ...
);
```

See [primitives.md](primitives.md) for the full list of type keys and their
fields.

### Lights list

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
        color     = [1.0, 1.0, 1.0];
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

See [lights.md](lights.md) for the full description of each light type.

---

## 3. Camera model and ray generation

**Source:** `src/scene/Camera.cpp`, `include/rt/scene/Camera.hpp`

The camera uses a right-handed coordinate system with a virtual viewport.

### Basis construction

Given position $E$, look-at point $A$, and up vector $\mathbf{up}$:

$$\hat{w} = \text{normalize}(E - A), \quad \hat{u} = \text{normalize}(\mathbf{up} \times \hat{w}), \quad \hat{v} = \hat{w} \times \hat{u}$$

The viewport has dimensions derived from the field of view and aspect ratio:

$$h = \tan(\text{fov}/2), \quad \text{vpHeight} = 2h, \quad \text{vpWidth} = \text{aspect} \times \text{vpHeight}$$

The lower-left corner of the viewport in world space is:

$$P_\text{ll} = E - \frac{\text{vpWidth}}{2}\hat{u} - \frac{\text{vpHeight}}{2}\hat{v} - \hat{w}$$

### Basic ray generation

For pixel $(x, y)$:

$$s = x / (W - 1), \quad t = y / (H - 1)$$
$$P = P_\text{ll} + s \cdot \text{vpWidth}\,\hat{u} + t \cdot \text{vpHeight}\,\hat{v}$$
$$\mathbf{d} = \text{normalize}(P - E)$$

### Anti-aliasing

When `antialiasing = N` is set in the camera block, the renderer fires $N \times N$
jittered samples per pixel. Each sample adds a sub-pixel random offset
$(\xi_x, \xi_y) \in [0, 1)^2$ before normalising by $N$:

$$s_j = (x + \xi_x / N) / (W - 1), \quad t_j = (y + \xi_y / N) / (H - 1)$$

The $N^2$ samples are averaged to produce the final pixel color.

### Depth-of-field

When `aperture > 0` is set, the camera uses a thin-lens model. A focus point is
computed along the ray at distance `focusDist`:

$$P_\text{focus} = E + \hat{d} \cdot \text{focusDist}$$

The ray origin is then jittered uniformly within a disk of radius
$\text{aperture}/2$ in the $\hat{u}$-$\hat{v}$ plane:

$$E' = E + \hat{u} \cdot r_x + \hat{v} \cdot r_y, \quad (r_x, r_y) \sim \text{disk}(\text{aperture}/2)$$

$$\mathbf{d}' = \text{normalize}(P_\text{focus} - E')$$

Objects at `focusDist` from the camera remain sharp. Objects closer or farther
accumulate a blur whose magnitude is proportional to the aperture.

---

## 4. BVH acceleration structure

**Source:** `src/accel/BVH.cpp`, `include/rt/rendering/BVH.hpp`

A Bounding Volume Hierarchy is a binary tree where each node stores an
axis-aligned bounding box (AABB) enclosing all primitives in its subtree.
Leaf nodes hold at most `MAX_PRIMS_PER_LEAF = 4` primitives. Interior nodes
hold no primitives directly; they only store the bounding box and pointers to
the left and right child.

### Construction

The tree is built recursively over the flat primitive array. At each level:

1. Compute the AABB of the primitive set.
2. If the count is at or below the leaf threshold, create a leaf node.
3. Pick the longest axis of the AABB as the split axis.
4. Run the SAH (Surface Area Heuristic) bucket method to find the optimal split.
5. Partition the array with `std::partition` and recurse into both halves.

### Surface Area Heuristic (SAH)

The SAH bucket method partitions centroids into $N_b = 12$ axis-aligned bins
and evaluates the expected ray traversal cost at each of the $N_b - 1$ possible
split planes:

$$C_i = C_t + N_L \cdot SA(B_L) + N_R \cdot SA(B_R)$$

where $C_t = 0.125$ is the traversal cost, $N_L$/$N_R$ are the primitive counts
in the two halves, and $SA(B)$ is the surface area of each half's AABB. The
key insight is that the probability of a random ray intersecting a box is
proportional to its surface area, so minimising this expression minimises
expected work.

If the best split cost is not cheaper than making a leaf directly, a leaf is
created instead.

### Traversal

```
intersectNode(node, ray, tMin, tMax):
  if node.bounds does not intersect ray: return false
  if node is leaf:
    test each primitive, keep closest hit
  else:
    hitLeft  = intersectNode(left,  ray, tMin, tMax)
    rightMax = hitLeft ? hit.t : tMax
    hitRight = intersectNode(right, ray, tMin, rightMax)
    return hitLeft or hitRight
```

After finding a hit in the left subtree, `tMax` is tightened to `hit.t` before
testing the right subtree. Any right-subtree geometry farther than the current
best hit is therefore culled without being tested.

### Shadow traversal

`intersectShadow()` uses the same AABB test but returns `true` as soon as any
opaque hit is found, without tracking which hit is closest. This is faster than
full intersection because no `HitRecord` needs to be updated.

---

## 5. Whitted raytracer

**Source:** `src/renderers/Renderer.cpp`, `src/renderers/raytracer/Shading.cpp`,
`src/renderers/raytracer/Surface.cpp`

The Whitted renderer computes direct illumination with recursive specular
reflections and refractions. It does not simulate indirect diffuse bounces and
therefore has no global illumination or color bleeding.

### Per-pixel algorithm

```
renderPixel(x, y):
  for each anti-aliasing sample:
    ray = camera.generateRay(x + jitter, y + jitter)
    color += castRay(ray, depth=0)
  return color / numSamples
```

### castRay(ray, depth)

```
castRay(ray, depth):
  if depth >= MAX_DEPTH (8): return black
  if not bvh.intersect(ray, ...): return skyGradient()
  color = computeDirectLighting(hit)
  if material.isTransparent(): color = computeTransparencyColor(color, hit, depth)
  else if material.isReflective(): color = applyReflection(color, hit, depth)
  return color
```

### Direct lighting

For each light in the scene `computeDirectLighting()` accumulates:

**Ambient lights:** added directly without shadow testing.

**Point lights and directional lights:**
1. Compute the light direction and distance.
2. Cast a shadow ray with `castShadowRayThrough()`. This advances the shadow ray
   past transparent materials (up to 10 hops) before declaring a surface
   shadowed by an opaque blocker.
3. Apply the Lambertian diffuse term:

$$C_\text{diff} = k_d \cdot I \cdot \max(0, \hat{n} \cdot \hat{l})$$

4. Apply the Phong specular term using the reflection of the light direction
   about the surface normal:

$$\hat{r} = 2(\hat{l} \cdot \hat{n})\hat{n} - \hat{l}$$
$$C_\text{spec} = k_s \cdot I \cdot \max(0, \hat{v} \cdot \hat{r})^\alpha$$

   where $\hat{v}$ is the view direction and $\alpha$ is the shininess exponent.

5. For point lights, apply inverse-square attenuation:

$$I_\text{eff} = \frac{I}{\max(d^2, 0.01)}$$

### Reflection

The reflected ray direction is:

$$\hat{r} = \hat{d} - 2(\hat{d} \cdot \hat{n})\hat{n}$$

The contribution is blended by the reflectivity factor $k_r$:

$$C = (1 - k_r) C_\text{direct} + k_r \cdot \text{castRay}(\text{reflected ray}, \text{depth}+1)$$

### Refraction (Snell + Schlick)

For a ray hitting a transparent material with IOR $\eta_t$ from IOR $\eta_i$,
let $\eta = \eta_i / \eta_t$ and $c = |\hat{d} \cdot \hat{n}|$:

$$k = 1 - \eta^2(1 - c^2)$$

If $k < 0$: total internal reflection (only the reflected ray is traced).

Otherwise the refracted direction is:

$$\hat{t} = \eta\hat{d} - \hat{n}(\eta c - \sqrt{k})$$

The Schlick Fresnel term blends the two contributions:

$$r_0 = \left(\frac{1 - \eta_t}{1 + \eta_t}\right)^2, \quad F = r_0 + (1 - r_0)(1 - c)^5$$

$$C = F \cdot C_\text{reflected} + (1 - F) \cdot C_\text{refracted}$$

### Sky background

When a ray misses all geometry the background color is:

$$t = \tfrac{1}{2}(d_y + 1), \quad C_\text{sky} = (1 - t)(1,1,1) + t(0.5, 0.7, 1.0)$$

---

## 6. Path tracer

**Source:** `src/renderers/PathTracer.cpp`, `src/renderers/pathtracer/Trace.cpp`,
`src/renderers/pathtracer/Sampling.cpp`

The path tracer implements unidirectional Monte Carlo path tracing with
next-event estimation (direct lighting sampled explicitly at each bounce). It
produces physically accurate global illumination but requires many samples per
pixel to converge.

### trace(ray, ctx)

```
trace(ray, ctx):
  if ctx.depth >= MAX_DEPTH (8): return black
  if not bvh.intersect(ray, ...): return skyGradient()
  if emission > 0.01: return emission color   // emissive surface exit
  direct = sampleDirectLighting(hit, ...)
  if ctx.depth > 3:                           // Russian Roulette
    if random() > 0.8: return direct
    throughput /= 0.8
  indirect = branch on material type:
    PBR        -> tracePBR()
    Transparent -> traceTransparent()
    Reflective  -> traceReflective()
    Default    -> traceDiffuse()
  return direct + indirect
```

### Direct lighting (next-event estimation)

For each non-ambient light `sampleDirectLighting()` evaluates:

1. Compute the direction and distance to the light.
2. Trace a shadow ray. If anything occludes the path, skip this light.
3. Evaluate the BRDF at the hit point:
   - PBR materials: full Cook-Torrance evaluation via `material->evaluateBRDF()`.
   - Other materials: Lambertian BRDF $f_r = \rho/\pi$.
4. For point lights apply inverse-square attenuation.
5. Accumulate:

$$L_\text{direct} = f_r(\omega_o, \omega_i) \cdot L_i \cdot \cos\theta_i$$

### Indirect lighting: diffuse path

A cosine-weighted hemisphere direction is sampled. With PDF $p(\omega) = 1/(2\pi)$,
the single-sample Monte Carlo estimator for indirect radiance is:

$$L_\text{indirect} \approx f_r \cdot L(\omega_\text{sample}) \cdot \cos\theta \cdot 2\pi$$

### Russian Roulette

After depth 3, each bounce is terminated with probability $1 - 0.8 = 0.2$. To
keep the estimator unbiased, surviving paths have their contribution divided by
the survival probability (0.8). This reduces the mean path length without
introducing bias.

### Reflective path

$$C = C_\text{direct} + (1 - k_r) \cdot C_\text{diffuse} + k_r \cdot \text{trace}(\text{reflected ray})$$

but reflectivity is typically set to 1.0 for mirrors, making it:

$$C = C_\text{direct} + k_r \cdot \text{trace}(\text{reflected ray})$$

### Transparent path

Same Snell + Schlick calculation as the Whitted renderer (see Section 5), but
recursive calls go through `PathTracer::trace()` so indirect illumination is
captured through the glass as well.

### PBR path

The material's `scatter()` method uses multiple importance sampling (MIS) to
draw the next direction:

1. Evaluate the Fresnel term $F$ at the macro-level surface ($\hat{n} \cdot \hat{v}$).
2. Compute the mixing weight: $w_s = F_\text{avg} / (F_\text{avg} + (1 - F_\text{avg})(1 - m))$
   where $m$ is the metallic factor.
3. With probability $w_s$ sample the GGX NDF (specular lobe); otherwise sample
   a cosine-weighted hemisphere (diffuse lobe).
4. Evaluate the full Cook-Torrance BRDF at the chosen direction.
5. Compute the blended PDF:

$$p = w_s \cdot p_\text{GGX} + (1 - w_s) \cdot p_\text{cosine}$$

6. Return `attenuation = BRDF * cos(theta) / pdf` for use by the caller.

### Hemisphere sampling

`randomUnitVector()` generates a uniform point on the unit sphere using a
rejection-free parametric method:

$$z \sim U(-1, 1), \quad \phi \sim U(0, 2\pi)$$
$$r = \sqrt{1 - z^2}, \quad (x, y, z) = (r\cos\phi, r\sin\phi, z)$$

`randomHemisphere(normal, rng)` flips the vector if $\hat{v} \cdot \hat{n} < 0$
so that the sample always lies in the hemisphere facing the surface normal.

---

## 7. Tone mapping

**Source:** `src/renderers/PathTracer.cpp` (`applyToneMapping`)

After accumulating $N$ path samples, raw HDR floating-point radiance values are
tone-mapped to bring them into the displayable $[0, 1]$ range.

The operator used is a per-channel modified Reinhard operator followed by a
square-root gamma approximation ($\gamma \approx 2$):

$$C_\text{out} = \sqrt{\frac{C}{1 + C}}$$

Properties:
- Values near 0 map close to 0 (no visible change in dark areas).
- Very bright values (HDR highlights) map smoothly toward 1 instead of clipping.
- The square root applies an implicit $\gamma = 2$ correction, which approximates
  the sRGB display transfer function.

The Whitted renderer does not apply tone mapping; output colors are clamped to
$[0, 1]$ per channel directly.

---

## 8. Multithreaded rendering

**Sources:** `src/renderers/PathTracer.cpp`, `src/renderers/Renderer.cpp`,
`src/accel/TileScheduler.cpp`

Both renderers use the same tile-based multithreading strategy.

### TileScheduler

The image is divided into $64 \times 64$ pixel tiles (the last tile on each
border may be smaller). All tiles are pre-computed into a vector at construction
time. A `std::atomic<int>` index is incremented with `fetch_add(1)` to distribute
tiles to threads without a mutex. This is the standard lock-free work-stealing
pattern.

### Thread count

The thread count defaults to `std::thread::hardware_concurrency()`, with a
fallback of 4 if the system query returns 0.

### Thread safety

Each thread owns its own `std::mt19937` RNG seeded from `std::random_device`.
The pixel buffer is accessed without synchronisation because the tile scheduler
guarantees that each tile is processed by at most one thread.

---

## 9. Denoising

**Source:** `src/denoise_tool.cpp`, `plugins/src/denoisers/`

A standalone `denoise_tool` binary is provided for post-process denoising of
rendered PPM images.

Two denoiser plugins are available:

**SimpleDenoiser** (`simple_denoiser.so`): A spatial box filter / bilateral
filter that blurs high-frequency noise without requiring a GPU or external SDK.

**OIDNDenoiser** (`oidn_denoiser.so`): Wraps Intel Open Image Denoise (OIDN).
Requires OIDN to be installed at build time (path configured in `CMakeLists.txt`).
Produces higher-quality results by using a pre-trained neural network aware of
typical Monte Carlo noise patterns.

### Usage

```bash
./denoise_tool input.ppm output.ppm [--plugin simple|oidn]
```
