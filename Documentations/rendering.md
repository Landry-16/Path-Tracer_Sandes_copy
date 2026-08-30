# Rendering Module : `include/rt/rendering/`

This document covers the five headers that form the rendering pipeline.
Together they go from a list of scene primitives to a final image file.

---

## Table of contents

1. [BVH](#1-bvh)
2. [Renderer](#2-renderer)
3. [PathTracer](#3-pathtracer)
4. [TileScheduler](#4-tilescheduler)
5. [PPMWriter](#5-ppmwriter)

---

## 1. BVH

**Files:** `include/rt/rendering/BVH.hpp` / `src/accel/BVH.cpp`

A **Bounding Volume Hierarchy** is a binary tree that partitions a set of
primitives into nested axis-aligned bounding boxes. It reduces the average ray
intersection test cost from $O(n)$ to $O(\log n)$.

### BVHNode

```
struct BVHNode {
    AABB bounds;                             // tight AABB enclosing all children
    unique_ptr<BVHNode> left, right;         // nullptr for leaf nodes
    vector<const IPrimitive*> primitives;    // non-empty only for leaf nodes
};
```

`isLeaf()` returns `true` when both children are `nullptr`. Leaf nodes hold at
most `MAX_PRIMS_PER_LEAF` (= 4) primitive pointers. The pointers are
**non-owning** : ownership remains with `Scene::primitives`.

### Construction : `build()`

Recursively partitions the primitive array in place:

1. Compute the AABB of $[start, end)$ via `computeBounds()`.
2. If the range has $\leq$ `MAX_PRIMS_PER_LEAF` primitives, create a leaf.
3. Otherwise pick the **longest axis** of the node's AABB and call
   `findBestSplit()` to find an optimal split point `mid`.
4. If no valid split is found (`mid == -1`, or `mid` equals `start` or `end`),
   fall back to a leaf.
5. Recurse: `left = build(start, mid)`, `right = build(mid, end)`.

### Split strategy : Surface Area Heuristic (SAH)

`findBestSplit()` implements the SAH bucket method. The idea is to search for
the partition that minimises the **expected number of ray-primitive tests**
weighted by the bounding box surface area.

**Step 1 : Compute the centroid range.**

`computeAxisRange()` builds the AABB of all primitive centroids along the
chosen axis. If the extent is below $10^{-8}$ the primitives are already
collapsed onto a plane and no split is possible.

**Step 2 : Bin primitives into buckets.**

`fillBuckets()` maps each primitive centroid to a bucket index:

$$\text{idx} = \left\lfloor N_b \cdot \frac{\text{centroid}_\text{axis} - \text{axis}\min}{\text{axis}\max - \text{axis}\min} \right\rfloor$$

where $N_b = 12$ (`NUM_BUCKETS`). Each bucket tracks its primitive count and
the union of its primitives' AABBs.

**Step 3 : Evaluate split cost for each bucket boundary.**

`computeBucketCosts()` evaluates the SAH cost at each of the $N_b - 1$
splitting planes. The cost of splitting after bucket $i$ is:

$$C_i = C_t + N_L \cdot SA(B_L) + N_R \cdot SA(B_R)$$

where:
- $C_t = 0.125$ is the traversal cost constant,
- $N_L$, $N_R$ are the primitive counts in the left / right partitions,
- $SA(B)$ is the surface area of the bounding box of each partition.

The underlying principle is that the probability a random ray hits a box is
proportional to its surface area. A cheap split with many primitives on one
side is worse than a balanced split with smaller children.

**Step 4 : Select the minimum-cost split.**

`findMinCostBucketIndex()` scans the cost array and returns the index of the
minimum. If that minimum cost is still $\geq N \cdot SA(\text{node})$ (i.e.
not splitting is cheaper) the function returns $-1$ and a leaf is created.

**Step 5 : Partition.**

`partitionAtBucket()` uses `std::partition` to reorder the primitive pointer
array so that all primitives that belong to the left half come before those
that belong to the right half. Returns the new `mid` index.

### Traversal : `intersect()`

`intersectNode()` traverses the tree recursively:

1. Test the node's AABB with the slab method. If the ray misses, return false
   immediately.
2. For a **leaf**: test every primitive and track the closest hit.
3. For an **interior node**: recurse into the left child first, then shrink
   `tMax` to `hit.t` before recursing into the right child. This culls the
   right subtree if the left child already found a closer hit.

### Shadow traversal : `intersectShadow()`

`intersectShadowNode()` is the same structure but returns `true` as soon as
any opaque hit is found, without tracking which hit is closest. This is cheaper
than full intersection because no `HitRecord` needs to be maintained.

---

## 2. Renderer

**Files:** `include/rt/rendering/Renderer.hpp` / `src/renderers/Renderer.cpp`

The `Renderer` implements **Whitted-style ray tracing**: direct illumination
with recursive specular reflections and refractions, but no global
illumination. All public methods are `static`.

### Anti-aliasing

Each pixel is sampled `samplesPerAxis x samplesPerAxis` times (default from
`scene.antialiasingSamples`). For each sample, a sub-pixel offset drawn from a
uniform distribution jitters the ray origin within the pixel:

$$\text{offset}_x = \frac{s_x + \xi}{\text{samplesPerAxis}}, \quad \xi \sim U(0,1)$$

The `samplesPerAxis x samplesPerAxis` color samples are averaged:

$$\text{pixel color} = \frac{1}{N^2} \sum_{s} C_s$$

### Depth-of-field

When `scene.aperture > 0`, rays are generated via the lens-based
`Camera::generateRay(x, y, rng)` overload. An aperture $> 0$ defocuses objects
outside the focus plane, simulating a thin lens.

### Sky gradient (background)

When a ray misses all geometry, the background is a linear interpolation
between white and sky blue based on the ray's vertical direction:

$$t = \frac{1}{2}(d_y + 1), \quad C_\text{bg} = (1-t)(1,1,1) + t(0.5, 0.7, 1.0)$$

### Direct lighting

`computeDirectLighting()` iterates over all lights. For each **directional or
point light**:

1. Cast a shadow ray from the hit point toward the light.
2. `castShadowRayThrough()` advances the shadow ray past transparent materials
   (up to 10 hops) before declaring the point shadowed.
3. If unoccluded, compute the **diffuse** (Lambertian) contribution and,
   optionally, the **specular** (Blinn-Phong) contribution:

$$C_\text{diffuse} = k_d \cdot I_\text{light} \cdot \max(0, \hat{n} \cdot \hat{l})$$

$$C_\text{specular} = k_s \cdot I_\text{light} \cdot \max(0, \hat{v} \cdot \hat{r})^{\alpha}$$

where $\hat{l}$ is the unit light direction, $\hat{v}$ the unit view direction,
$\hat{r}$ the unit reflection of $\hat{l}$ about $\hat{n}$, and $\alpha$ the
shininess exponent.

Ambient lights skip shadow testing and apply their intensity directly to the
diffuse color.

### Refraction : Snell's law

`computeTransparencyColor()` computes refracted and reflected rays and blends
them with the **Schlick approximation** of the Fresnel equations.

For a ray entering a surface with index of refraction $\eta_t$ from medium
$\eta_i$, let $\eta = \eta_i / \eta_t$ and $\cos\theta_i = |\hat{d} \cdot \hat{n}|$:

$$k = 1 - \eta^2 (1 - \cos^2\theta_i)$$

If $k < 0$ the angle exceeds the critical angle and **total internal
reflection** occurs (only the reflected ray is traced).

Otherwise, the refracted direction is:

$$\hat{t} = \eta \hat{d} - \hat{n} (\eta \cos\theta_i + \sqrt{k})$$

The Schlick Fresnel term blends reflection and refraction:

$$r_0 = \left(\frac{1 - \eta_t}{1 + \eta_t}\right)^2$$
$$F(\theta_i) = r_0 + (1 - r_0)(1 - \cos\theta_i)^5$$

$$C = F \cdot C_\text{reflected} + (1 - F) \cdot C_\text{refracted}$$

### Reflection

`applyReflection()` computes the specularly reflected ray direction:

$$\hat{r} = \hat{d} - 2(\hat{d} \cdot \hat{n})\hat{n}$$

and blends:

$$C = (1 - k_r) \cdot C_\text{direct} + k_r \cdot C_\text{reflected}$$

where $k_r$ is `material->getReflectivity()`.

### Depth limit

Recursive calls are clamped at `MAX_DEPTH = 8`. Rays that exceed this depth
return black `(0, 0, 0)`.

### Multithreaded rendering

`renderMultithreaded()` and `renderMultithreadedWithDisplay()` spin up
`normalizeThreadCount()` threads , defaulting to hardware concurrency , and
feed them tiles from a `TileScheduler`. Each thread owns its own RNG
(`std::mt19937` seeded from `std::random_device`). No mutex is needed on the
pixel buffer because the tile scheduler guarantees each tile is consumed by at
most one thread.

---

## 3. PathTracer

**Files:** `include/rt/rendering/PathTracer.hpp` / `src/renderers/PathTracer.cpp`

The `PathTracer` implements **unidirectional Monte Carlo path tracing** with
next-event estimation (explicit direct light sampling) and optional PBR
materials. All public methods are `static`.

### Core loop : `trace()`

`trace()` is called once per sample per pixel and recurses until a depth limit
(`MAX_DEPTH = 8`) is reached or Russian Roulette terminates the path.

1. Intersect the BVH. If no hit, return the sky gradient.
2. If the hit material is **emissive**, return the emission directly (allowing
   area lights).
3. Sample direct lighting via `sampleDirectLighting()`.
4. **Russian Roulette** (depth $> 3$): continue the path with probability
   $p_\text{RR} = 0.8$; divide the throughput by $p_\text{RR}$ to maintain
   unbiasedness.
5. Branch on material type and call the appropriate sub-trace function.

### Direct lighting : `sampleDirectLighting()`

For each non-ambient light, `computeOneLightContrib()`:

1. Builds a shadow ray toward the light via `computeLightRay()`.
2. Evaluates visibility: if `bvh.intersect` hits anything between the surface
   and the light, the contribution is zero.
3. Evaluates the BRDF via `evalBRDF()`.
4. For point lights, applies inverse-square attenuation:

$$I_\text{effective} = \frac{I}{d^2}$$

5. Computes the contribution:

$$L_\text{direct} = f_r(\omega_o, \omega_i) \cdot L_i \cdot \cos\theta_i$$

where $f_r$ is the BRDF, $L_i$ the light radiance, and $\theta_i$ the angle
between the surface normal and the light direction.

### BRDF evaluation : `evalBRDF()`

- For **PBR materials**: delegates to `material->evaluateBRDF()` (which
  implements a full Cook-Torrance microfacet model in the PBR plugin).
- For **diffuse materials**: returns the Lambertian BRDF:

$$f_r^\text{Lambertian} = \frac{\rho}{\pi}$$

where $\rho$ is the diffuse albedo (sampled from a texture if one is bound).

### Indirect lighting : diffuse path (`traceDiffuse()`)

The indirect contribution is estimated by tracing one random hemisphere sample
(Monte Carlo estimator):

$$L_\text{indirect} \approx \frac{f_r \cdot L(\omega_\text{random}) \cdot \cos\theta}{p(\omega)} \cdot \frac{1}{p_\text{RR}}$$

With a cosine-weighted uniform hemisphere sampler, $p(\omega) = 1/(2\pi)$, so:

$$L_\text{indirect} \approx f_r \cdot L(\omega_\text{random}) \cdot \cos\theta \cdot 2\pi$$

### Hemisphere sampling : `randomHemisphere()`

Generates a uniform random unit vector on the hemisphere aligned with a given
normal.

1. `randomUnitVector()` generates a uniform point on the unit sphere using
   the rejection-free parametric method:

$$z \sim U(-1, 1), \quad a \sim U(0, 2\pi)$$
$$r = \sqrt{1 - z^2}, \quad (x, y, z) = (r\cos a,\; r\sin a,\; z)$$

2. If the dot product with the normal is negative, negate the vector to ensure
   it lies in the correct hemisphere.

### Transparent path (`traceTransparent()`)

Identical Snell + Schlick formulation as `Renderer::computeTransparencyColor()`
(see Section 2). The only difference is that recursive calls go through
`PathTracer::trace()` so indirect light is also accounted for.

### Reflective path (`traceReflective()`)

Perfect specular reflection blended by `getReflectivity()`:

$$\hat{r} = \hat{d} - 2(\hat{d} \cdot \hat{n})\hat{n}$$
$$C = (1 - k_r) C_\text{direct} + k_r \cdot \text{trace}(\text{reflected ray})$$

### PBR path (`tracePBR()`)

Calls `material->scatter()` to importance-sample the next direction according
to the material's BRDF. If a valid scatter direction is returned with PDF $> 0$:

$$C = C_\text{direct} + \text{attenuation} \cdot \text{trace}(\text{scattered ray})$$

The attenuation and PDF are baked into `scatter.attenuation` by the PBR plugin.

### Tone mapping : `applyToneMapping()`

After averaging $N$ path samples per pixel, each channel is tone-mapped with a
variant of the Reinhard operator followed by a gamma approximation (square root
encodes $\gamma \approx 2$):

$$C_\text{out} = \sqrt{\frac{C}{1 + C}} \quad \text{(per channel)}$$

This compresses HDR values to $[0, 1)$ while preserving relative brightness.

### Multithreading

Same tile-based approach as `Renderer`. Each thread has its own `std::mt19937`
seeded independently. The pixel buffer is written without locking because tiles
are non-overlapping.

---

## 4. TileScheduler

**Files:** `include/rt/rendering/TileScheduler.hpp` / `src/accel/TileScheduler.cpp`

The `TileScheduler` partitions the image into a fixed grid of rectangular tiles
and distributes them to worker threads in a **lock-free first-come-first-served**
manner.

### Tile

```
struct Tile {
    int startX, startY;   // top-left corner (inclusive)
    int endX,   endY;     // bottom-right corner (exclusive)
};
```

### Construction

The constructor pre-computes all tiles row by row and stores them in a `vector`.
The last tile on each row and column may be narrower or shorter than `tileSize`
if the image dimensions are not multiples of `tileSize`:

```
endX = min(x + tileSize, imageWidth);
endY = min(y + tileSize, imageHeight);
```

### `getNextTile()`

Uses a `std::atomic<int>` index incremented with `fetch_add(1)`. This is the
standard lock-free work-stealing pattern: each call atomically claims the next
unclaimed tile. If the returned index is out of range, returns `false` and the
thread exits its loop.

Lock-free correctness relies on two properties:
- `fetch_add` is sequentially consistent by default.
- Each tile in the pre-computed vector is immutable after construction so reads
  are safe without synchronisation.

### Tile size

The default tile size used by both `Renderer` and `PathTracer` is 64 pixels.
Smaller tiles improve load balancing (more uniform distribution across threads)
but increase overhead per tile. 64 is a common production default that fits
several cache lines of floating-point data.

---

## 5. PPMWriter

**Files:** `include/rt/rendering/PPMWriter.hpp` / `src/image/PPMWriter.cpp`

Serialises a pixel buffer to a **Portable Pixmap (PPM P3)** file, the simplest
uncompressed RGB image format.

### PPM P3 format

```
P3
<width> <height>
255
R G B
R G B
...
```

Each `R G B` triplet is an ASCII integer in $[0, 255]$ written on a single
line, in left-to-right, top-to-bottom order.

### `toInt()`

Converts a linear `double` in $[0, 1]$ to an 8-bit integer:

$$i = \lfloor v \times 255.99 \rfloor$$

Clamped to $[0, 255]$ to handle occasional values outside the nominal range
due to floating-point accumulation (e.g. from additive Monte Carlo estimators
that have not yet converged).

Note that **no gamma correction is applied** here. The caller is responsible
for tone-mapping or gamma-encoding the pixel buffer before writing if needed.
`PathTracer` applies its own tone mapping in `applyToneMapping()` before the
pixels reach this writer; `Renderer` writes raw linear values.
