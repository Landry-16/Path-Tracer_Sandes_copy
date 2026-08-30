# Path Tracing

This document gives a complete technical description of the Monte Carlo path
tracer implemented in `src/renderers/pathtracer/`. It covers the rendering
equation, the algorithm, every shading branch, sampling strategies, and
convergence.

---

## Table of contents

1. [Rendering equation](#1-rendering-equation)
2. [Architecture and data structures](#2-architecture-and-data-structures)
3. [Main trace loop](#3-main-trace-loop)
4. [Direct lighting (next-event estimation)](#4-direct-lighting-next-event-estimation)
5. [Indirect lighting per material branch](#5-indirect-lighting-per-material-branch)
6. [Russian Roulette path termination](#6-russian-roulette-path-termination)
7. [Hemisphere sampling](#7-hemisphere-sampling)
8. [Tone mapping](#8-tone-mapping)
9. [Rendering orchestration and multithreading](#9-rendering-orchestration-and-multithreading)
10. [Configuration fields affecting path tracing](#10-configuration-fields-affecting-path-tracing)
11. [Convergence guide](#11-convergence-guide)

---

## 1. Rendering equation

The path tracer solves the rendering equation numerically:

$$L_o(\mathbf{x}, \omega_o) = L_e(\mathbf{x}, \omega_o) + \int_{\Omega} f_r(\mathbf{x}, \omega_i, \omega_o)\, L_i(\mathbf{x}, \omega_i)\, (\hat{n} \cdot \omega_i)\, d\omega_i$$

where:
- $L_o$ is the outgoing radiance at surface point $\mathbf{x}$ in direction $\omega_o$
- $L_e$ is the emitted radiance (non-zero only for emissive materials)
- $f_r$ is the bidirectional reflectance distribution function (BRDF)
- $L_i$ is the incoming radiance
- $\hat{n} \cdot \omega_i$ is the cosine foreshortening factor

The integral over the hemisphere $\Omega$ is estimated by Monte Carlo sampling:
for each pixel, $N$ independent light paths are traced and their contributions
averaged. Each light path splits the integral into an explicit direct lighting
term (next-event estimation) and a single recursive indirect term.

---

## 2. Architecture and data structures

**Source files:**
- `src/renderers/pathtracer/Trace.cpp` - core trace dispatch and shading branches
- `src/renderers/pathtracer/Sampling.cpp` - sampling utilities and direct lighting
- `src/renderers/pathtracer/PathTracer.cpp` - render orchestration and tile workers
- `include/rt/rendering/PathTracer.hpp` - class declaration and context structs

### TraceCtx

`TraceCtx` is the per-ray context passed through the recursive calls:

```cpp
struct TraceCtx {
    const BVH &bvh;
    const std::vector<std::unique_ptr<ILight>> &lights;
    int depth;
    std::mt19937 &rng;

    TraceCtx next() const { return {bvh, lights, depth + 1, rng}; }
};
```

`next()` creates a child context with `depth + 1`. No heap allocation occurs:
the BVH, lights vector, and RNG are passed by reference through the full call
chain.

### SampleCtx

`SampleCtx` is the per-pixel context owned by each tile worker thread:

```cpp
struct SampleCtx {
    std::mt19937 &rng;
    std::uniform_real_distribution<double> &dist;
    int samplesPerPixel;
};
```

### HitRecord

`HitRecord` is the output of a BVH intersection query:

```cpp
struct HitRecord {
    Vec3 point;                           // world-space hit point
    Vec3 normal;                          // outward surface normal
    double t;                             // ray parameter
    std::shared_ptr<const IMaterial> material;
    double u, v;                          // surface UV coordinates
};
```

### ScatterResult

`ScatterResult` is returned by `IMaterial::scatter()` for PBR materials:

```cpp
struct ScatterResult {
    Vec3  direction;    // sampled outgoing direction
    Color attenuation;  // BRDF * cos(theta) / pdf, pre-divided for the caller
    double pdf;         // probability density of the chosen direction
    bool  valid;        // false if sampling failed (degenerate geometry, etc.)
};
```

---

## 3. Main trace loop

**Source:** `src/renderers/pathtracer/Trace.cpp`, function `PathTracer::trace()`

```
trace(ray, ctx):
  1. depth guard: if depth >= 8, return black
  2. BVH intersection: if miss, return skyColor(ray)
  3. null material guard: if no material, return black
  4. emission check: if emission component > 0.01, return emission (area light hit)
  5. direct = sampleDirectLighting(hit, wo, bvh, lights)
  6. Russian Roulette if depth > 3 (see Section 6)
  7. material dispatch:
       isPBR()         -> tracePBR()
       isTransparent() -> traceTransparent()
       isReflective()  -> traceReflective()
       default         -> traceDiffuse()
```

### Sky background

When no geometry is hit, the background color is a linear interpolation between
white at the horizon and light blue at the zenith, parameterised by the ray's
vertical direction:

$$t = \tfrac{1}{2}(d_y + 1), \quad C_\text{sky} = (1 - t)(1, 1, 1) + t(0.5, 0.7, 1.0)$$

### Emissive surfaces

If the hit material returns a non-zero emission ($> 0.01$ on any channel),
`trace()` returns the emission color immediately without tracing further. This
is how area lights (any geometry with an emissive material) contribute to the
scene: they terminate the path and inject radiance directly.

Note that this means emissive geometry does not also receive direct-lighting
contributions from other lights. The emission entirely determines the color.

### Depth limit

`MAX_DEPTH = 8`. Paths exceeding this depth return `Color(0, 0, 0)`. Combined
with Russian Roulette this limit is rarely reached, but it guarantees the
recursion terminates.

---

## 4. Direct lighting (next-event estimation)

**Source:** `src/renderers/pathtracer/Sampling.cpp`

At every non-emissive surface hit, the direct contribution from all lights is
computed explicitly before sampling an indirect direction. This is called
next-event estimation (NEE) or shadow connection.

### Algorithm for each light

```
computeOneLightContrib(hit, wo, bvh, light):
  1. Compute light direction and distance via computeLightRay():
       - Directional: dir = -light.direction, dist = 1e10
       - Point: dir = normalize(light.position - hit.point), dist = ||light.position - hit.point||
  2. cosTheta = dot(hit.normal, dir); skip if cosTheta < 1e-6
  3. Cast shadow ray from hit.point along dir with tMax = dist - 0.001
     If bvh.intersect returns a hit: return black (occluded)
  4. Retrieve light intensity
     For point lights: intensity /= max(dist^2, 0.01)
  5. Evaluate BRDF: evalBRDF(hit, wo, dir)
  6. Return BRDF * intensity * cosTheta
```

Shadow rays use `bvh.intersect()` rather than `bvh.intersectShadow()` because
the path tracer needs to support transparent shadow occlusion logic at the
surface level. A single intersection test is performed; no transparent material
chain traversal is done in the path tracer's shadow pass (unlike the Whitted
renderer which chains up to 10 transparent hops).

### BRDF evaluation in direct lighting

`evalBRDF()` selects between two cases:

- **PBR material** (`isPBR() == true`): calls `material->evaluateBRDF(wo, wi, normal, u, v)`
  which returns the full Cook-Torrance value $f_r(\omega_o, \omega_i)$ without
  the cosine factor (the factor is applied by the caller).
- **All other materials**: uses the Lambertian BRDF:

$$f_r^\text{Lambertian} = \frac{\rho}{\pi}$$

where $\rho$ is the diffuse albedo, optionally sampled from a texture at UV
coordinates $(u, v)$.

### Ambient lights are excluded from NEE

`sampleDirectLighting()` skips any light for which
`dynamic_cast<const AmbientLight*>(light.get())` succeeds. Ambient lights have
no direction and cannot be used for shadow testing.

---

## 5. Indirect lighting per material branch

After computing the direct term, one indirect direction is sampled and the path
continues recursively. The material type determines which strategy is used.

### 5.1 Diffuse path (`traceDiffuse`)

A random direction is drawn uniformly from the hemisphere aligned with the
surface normal (see Section 7). The full Monte Carlo estimator is:

$$L_\text{indirect} = \frac{\rho}{\pi} \cdot L\left(\omega_\text{sample}\right) \cdot \cos\theta \cdot \frac{2\pi}{p_\text{RR}}$$

The $2\pi$ factor comes from the hemisphere solid angle: with a uniform
hemisphere sampler $p(\omega) = 1/(2\pi)$, so the importance-sampling weight
$1/p(\omega) = 2\pi$. $p_\text{RR} \in \{0.8, 1.0\}$ is the Russian Roulette
survival probability (1.0 at depth $\leq 3$).

The diffuse color $\rho$ is retrieved via the texture if one is bound, otherwise
from `material->getDiffuse()`.

The final pixel contribution is:

$$L_o = L_\text{direct} + \frac{\rho}{\pi} \cdot L_\text{indirect} \cdot \cos\theta \cdot 2\pi$$

which simplifies to $L_\text{direct} + 2 \rho \cdot L_\text{indirect} \cdot \cos\theta$.

### 5.2 Reflective path (`traceReflective`)

Perfect specular reflection. The reflected ray direction is computed as:

$$\hat{r} = \hat{d} - 2(\hat{d} \cdot \hat{n})\hat{n}$$

The contribution blends direct lighting and the recursive reflected radiance
by the material's reflectivity $k_r$:

$$L_o = L_\text{direct} \cdot (1 - k_r) + k_r \cdot L(\hat{r})$$

For mirror materials $k_r = 1.0$, which reduces to $L_o = L(\hat{r})$.

### 5.3 Transparent path (`traceTransparent`)

Handles both refraction and reflection using Snell's law and the Schlick
approximation. No direct lighting is computed for transparent surfaces; the
entire radiance comes from the transmitted and reflected secondary rays.

**Entry/exit handling:**
- If $\hat{d} \cdot \hat{n} < 0$, the ray is entering the material
  ($\eta = 1/\text{IOR}$, $\hat{n}$ points inward).
- Otherwise the ray is exiting ($\eta = \text{IOR}$, $\hat{n}$ flipped).

**Refracted direction:**

Let $c = |\hat{d} \cdot \hat{n}|$ and $k = 1 - \eta^2(1 - c^2)$.

$$\hat{t} = \eta\hat{d} - \hat{n}(\eta c + \sqrt{k})$$

If $k < 0$, total internal reflection occurs and only the reflected ray is
returned.

**Fresnel blending via Schlick:**

$$r_0 = \left(\frac{1 - \text{IOR}}{1 + \text{IOR}}\right)^2, \quad F = r_0 + (1 - r_0)(1 - c)^5$$

$$L_o = F \cdot L(\hat{r}) + (1 - F) \cdot L(\hat{t})$$

### 5.4 PBR path (`tracePBR`)

PBR materials implement their own importance-sampled scatter direction via
`IMaterial::scatter()`. This is the Cook-Torrance GGX microfacet model with
multiple importance sampling between the specular and diffuse lobes (see
[lights.md](lights.md) Section 9 for the full BRDF derivation).

`scatter()` returns a `ScatterResult` that pre-divides the full BRDF weight by
the PDF, so the caller only needs to multiply by the recursive radiance:

$$L_o = L_\text{direct} + \text{scatter.attenuation} \cdot L(\text{scatter.direction})$$

where `scatter.attenuation` encodes $f_r(\omega_o, \omega_i) \cdot \cos\theta / p(\omega_i)$.

If the scatter result is invalid (degenerate geometry, zero PDF), only the
direct term is returned.

---

## 6. Russian Roulette path termination

**Source:** `src/renderers/pathtracer/Trace.cpp`, inside `PathTracer::trace()`

At depths greater than 3, each path is terminated stochastically to reduce
computational cost.

```cpp
if (ctx.depth > 3) {
    rrProb = 0.8;
    if (randomDouble(ctx.rng) > rrProb)
        return direct;     // path terminated early, return only direct term
}
```

The survival probability is $p_\text{RR} = 0.8$, meaning 20% of paths are cut
at depth 4 and beyond.

**Unbiasedness guarantee.** When a path survives ($80\%$ of the time), its
contribution is scaled up by $1/p_\text{RR} = 1.25$ to compensate. In the
diffuse branch this factor appears explicitly in the denominator of the
estimator weight. The expected value of the estimator is therefore unchanged:

$$E\left[\frac{X}{p_\text{RR}} \cdot \mathbf{1}[\text{survive}]\right] = \frac{E[X]}{p_\text{RR}} \cdot p_\text{RR} = E[X]$$

**Variance trade-off.** Russian Roulette does not reduce variance; it reduces
mean computation time. Paths that are terminated early contribute only their
direct term, which introduces variance. The benefit is that long, low-energy
paths are cut proportionally more because they contribute little energy relative
to the cost of tracing them.

---

## 7. Hemisphere sampling

**Source:** `src/renderers/pathtracer/Sampling.cpp`

### Uniform sphere sampling (`randomUnitVector`)

Generates a uniform random direction on the unit sphere using a
rejection-free parametric construction:

$$z \sim U(-1, 1), \quad \phi \sim U(0, 2\pi)$$
$$r = \sqrt{1 - z^2}, \quad \hat{v} = (r\cos\phi,\; r\sin\phi,\; z)$$

This is sometimes called the "longitude-latitude" parametrisation. It avoids
the clustering artefacts of naive $(u, v)$ sampling.

### Hemisphere sampling (`randomHemisphere`)

The sphere sample is reflected into the correct hemisphere by the surface
normal:

```cpp
Vec3 v = randomUnitVector(rng);
return (v.dot(normal) > 0.0) ? v : (v * -1.0);
```

This produces a **uniform** distribution over the hemisphere (all directions
equally probable), not a cosine-weighted one. The cosine foreshortening is
applied explicitly in `traceDiffuse()`.

### GGX microfacet sampling (PBR path)

PBR materials use importance sampling of the GGX normal distribution function
to draw the half-vector $\hat{H}$, then reflect $\hat{V}$ about $\hat{H}$ to
obtain the incident direction. The derivation is inside `PBRMaterial::sampleGGX()`:

Given $\alpha^2 = \text{roughness}^4$ and uniform samples $(r_1, r_2) \in [0,1)^2$:

$$\cos\theta_H = \sqrt{\frac{1 - r_2}{1 + (\alpha^2 - 1) r_2}}, \quad \phi = 2\pi r_1$$

$$\hat{H}^{\text{local}} = (\sin\theta_H \cos\phi,\; \sin\theta_H \sin\phi,\; \cos\theta_H)$$

$\hat{H}$ is transformed from the local frame (aligned to the surface normal)
back to world space using an orthonormal basis built from the surface normal.

The incident direction is then:

$$\hat{l} = 2(\hat{V} \cdot \hat{H})\hat{H} - \hat{V}$$

The GGX PDF for this sample is:

$$p(\hat{l}) = \frac{D(\hat{H}) \cdot (\hat{n} \cdot \hat{H})}{4 (\hat{H} \cdot \hat{V})}$$

where $D(\hat{H})$ is the GGX normal distribution function evaluated at the
half-vector.

### Cosine-weighted hemisphere sampling (PBR diffuse lobe)

For the diffuse lobe in PBR scatter, the sample is drawn proportional to
$\cos\theta$:

$$\cos\theta = \sqrt{1 - r_2}, \quad \phi = 2\pi r_1$$
$$\hat{l}^{\text{local}} = (\sin\theta \cos\phi,\; \sin\theta \sin\phi,\; \cos\theta)$$

This importance-samples the $\cos\theta$ factor in the rendering equation,
reducing variance compared to the uniform hemisphere sampler.

---

## 8. Tone mapping

**Source:** `src/renderers/pathtracer/PathTracer.cpp`, `applyToneMapping()`

After accumulating $N$ path samples and averaging, each pixel undergoes tone
mapping before being written to the output buffer.

The operator used is a modified Reinhard combined with an approximate gamma
correction:

$$C_\text{out} = \sqrt{\frac{C}{1 + C}} \quad \text{(applied per channel)}$$

The $C / (1 + C)$ step compresses HDR values in $[0, \infty)$ to $[0, 1)$. It
is the Reinhard operator. The outer square root approximates gamma $\approx 2$
encoding, matching the sRGB convention that most displays assume.

This operator has the following properties:
- $C = 0 \Rightarrow C_\text{out} = 0$ (black maps to black)
- $C \to \infty \Rightarrow C_\text{out} \to 1$ (over-bright values saturate without clipping)
- $C = 1 \Rightarrow C_\text{out} = 1/\sqrt{2} \approx 0.707$ (mid-grey stays in mid range)

No color grading or white balance is applied.

---

## 9. Rendering orchestration and multithreading

**Source:** `src/renderers/pathtracer/PathTracer.cpp`

### Entry points

| Method | Description |
|---|---|
| `PathTracer::render(scene, N)` | Renders offline to a pixel buffer, no live preview |
| `PathTracer::renderWithDisplay(scene, display, N)` | Same but updates a `ProgressiveDisplay` window after each tile |

### BVH construction

Before spawning threads, `buildAndLogBVH()` constructs the BVH from
`scene.primitives`. Construction time is printed to stdout. Primitives with
non-finite bounding boxes are silently excluded (e.g., infinite planes that
have no AABB).

### Tile scheduler

The image is divided into $64 \times 64$ pixel tiles. A `TileScheduler` holds
the pre-computed tile list and distributes them to threads via a
`std::atomic<int>` counter (`fetch_add`). This is a lock-free work-stealing
pattern: each thread atomically claims the next unclaimed tile until the list
is exhausted.

### Thread count

`getThreadCount()` returns `std::thread::hardware_concurrency()`, falling back
to 4 if the value is unavailable.

### Per-thread state

Each worker thread owns:
- An independent `std::mt19937` RNG seeded from `std::random_device`. This
  avoids contention and ensures that each thread's samples are statistically
  independent.
- A local `std::uniform_real_distribution<double>`.

No mutex is required on the pixel buffer because tiles are non-overlapping. The
`ProgressiveDisplay::updateRegion()` call does lock internally.

### Pixel sampling loop

```
samplePixel(scene, bvh, i, j, ctx):
  for s in range(samplesPerPixel):
    u = (i + rng_float) / (scene.width  - 1)
    v = (j + rng_float) / (scene.height - 1)
    ray = camera.generateRay(u * width, height - 1 - v * height)
    accumulated += trace(ray, {bvh, lights, depth=0, rng})
  return applyToneMapping(accumulated / samplesPerPixel)
```

The random offsets per sample produce stochastic anti-aliasing: no regular grid
sub-pixel pattern is used. The jitter is fully random, which converges faster
per sample than a regular grid but can show noise at very low sample counts.

---

## 10. Configuration fields affecting path tracing

The path tracer uses the standard scene configuration format. A selection of
fields has a stronger effect on path-traced output than on the Whitted renderer.

### Camera (path tracing relevant fields)

| Field | Effect |
|---|---|
| `antialiasing` | Not used by the path tracer; the per-sample jitter already provides anti-aliasing |
| `aperture` | Enables depth-of-field via thin-lens model |
| `focusDist` | Distance to the sharp focus plane |

### Command-line sample count

The number of samples per pixel is set on the command line at launch time, not
in the scene file. Typical values:

| Samples | Use case |
|---|---|
| 16-64 | Quick preview with visible noise |
| 256 | Moderate quality, most features visible |
| 1024+ | High quality, near-converged |
| 4096+ | Reference images |

### Material selection

Path-traced output is most useful with:
- **PBR materials** (`pbr_*` presets): full Cook-Torrance BRDF with
  importance sampling, producing physically plausible metalness and roughness.
- **Emissive materials** (`emissive`, `emissive_diffuse`): turn geometry into
  area lights, enabling soft shadows and indirect bounce lighting.
- **Transparent materials** (`glass`, `diamond`, etc.): caustic-like effects
  through transmission, although caustics are not captured without bidirectional
  methods.

---

## 11. Convergence guide

### Noise sources

| Noise source | Cause | Mitigation |
|---|---|---|
| Low samples per pixel | Insufficient Monte Carlo samples | Increase sample count |
| High-intensity point lights | Large variance from rare direct hits | Spread light energy across multiple lights or use emissive geometry |
| Thin transparent objects | Refraction rays can flip rapidly | Ensure normals are consistent; high IOR increases variance |
| Highly specular GGX ($\alpha \ll 1$) | GGX PDF has a very narrow specular peak | MIS mitigates this; avoid $\text{roughness} < 0.03$ |

### Variance and sample count

The standard deviation of the Monte Carlo estimator decreases as $1/\sqrt{N}$
where $N$ is the number of samples. Doubling samples reduces noise by $\approx 30\%$.
To halve noise, four times as many samples are required.

### Denoising

The project includes two denoisers:
- **Simple denoiser** (`simple_denoiser` plugin): a spatial bilateral-style
  filter, fast but limited.
- **OIDN denoiser** (`oidn_denoiser` plugin): Intel Open Image Denoise,
  a neural network that can reconstruct clean images from 16-64 samples.

The denoiser is invoked via the `denoise_tool` binary on a `.ppm` output file.
