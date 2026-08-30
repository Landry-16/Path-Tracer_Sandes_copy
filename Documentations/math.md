# Math Module : `include/rt/math/`

This document covers all five headers in the math layer of the raytracer.
Every type here is a plain-value type: no heap allocation, no virtual dispatch,
copyable and assignable by value.

---

## Table of contents

1. [Vec3](#1-vec3)
2. [Color](#2-color)
3. [Ray](#3-ray)
4. [Matrix4](#4-matrix4)
5. [AABB](#5-aabb)

---

## 1. Vec3

**File:** `include/rt/math/Vec3.hpp` / `src/math/Vec3.cpp`

A three-component vector over `double`. Used both as a geometric direction or
position and, through the `Color` alias, as an RGB triplet.

### Storage

```
double x, y, z;
```

### Arithmetic operators

All arithmetic is component-wise:

| Operator | Formule |
|---|---|
| `a + b` | `(ax+bx, ay+by, az+bz)` |
| `a - b` | `(ax-bx, ay-by, az-bz)` |
| `a * t` | `(ax*t, ay*t, az*t)` |
| `t * a` | same as `a * t` (free function) |
| `a * b` | `(ax*bx, ay*by, az*bz)` , component-wise product |
| `a / t` | `(ax/t, ay/t, az/t)` |
| `-a`    | `(-ax, -ay, -az)` |

The compound assignment variants (`+=`, `-=`, `*=`) mutate in place and return
`*this`.

### Length and normalization

**Euclidean length** (L2 norm):

$$\|v\| = \sqrt{x^2 + y^2 + z^2}$$

```cpp
double Vec3::length() const {
    return std::sqrt(x*x + y*y + z*z);
}
```

**Length squared** avoids the square root and is preferred for comparisons:

$$\|v\|^2 = x^2 + y^2 + z^2$$

**Unit vector** (normalization). Divides every component by the length. If the
vector is the zero vector the method returns it unchanged to avoid division by
zero:

$$\hat{v} = \frac{v}{\|v\|}$$

```cpp
Vec3 Vec3::normalized() const {
    double len = length();
    if (len > 0) return *this / len;
    return *this;
}
```

### Dot product

The dot product of two vectors $\mathbf{a}$ and $\mathbf{b}$ is a scalar:

$$\mathbf{a} \cdot \mathbf{b} = a_x b_x + a_y b_y + a_z b_z$$

Geometric interpretation: equals $\|\mathbf{a}\| \|\mathbf{b}\| \cos\theta$
where $\theta$ is the angle between them. Used throughout the renderer to:

- test whether two vectors face the same hemisphere ($\mathbf{a} \cdot \mathbf{b} > 0$),
- compute light attenuation via Lambert's cosine law,
- evaluate BRDF terms.

```cpp
double Vec3::dot(const Vec3& v) const {
    return x*v.x + y*v.y + z*v.z;
}
```

### Cross product

The cross product of $\mathbf{a}$ and $\mathbf{b}$ produces a vector
perpendicular to both:

$$\mathbf{a} \times \mathbf{b} =
\begin{pmatrix} a_y b_z - a_z b_y \\ a_z b_x - a_x b_z \\ a_x b_y - a_y b_x \end{pmatrix}$$

The magnitude is $\|\mathbf{a}\| \|\mathbf{b}\| \sin\theta$. Used to:

- compute surface normals from two edge vectors of a triangle,
- construct orthonormal bases (e.g. tangent-space frames for sampling).

```cpp
Vec3 Vec3::cross(const Vec3& v) const {
    return Vec3(
        y*v.z - z*v.y,
        z*v.x - x*v.z,
        x*v.y - y*v.x
    );
}
```

---

## 2. Color

**File:** `include/rt/math/Color.hpp`

```cpp
using Color = Vec3;
```

`Color` is simply a type alias for `Vec3`. The three components are interpreted
as `(R, G, B)` intensities in linear light space, each in the range $[0, 1]$.
Using the same type for both vectors and colors means all vector arithmetic
(component-wise multiply for tinting, `+` for additive light contributions)
works without any conversion.

---

## 3. Ray

**File:** `include/rt/math/Ray.hpp` / `src/math/Ray.cpp`

A ray is defined by an origin point and a direction vector.

### Storage

```
Vec3 origin;
Vec3 direction;
```

The direction is not enforced to be a unit vector at the type level. Many call
sites normalize it before constructing the ray, but some (e.g. BVH slab tests)
benefit from keeping the raw direction for cheaper arithmetic.

### Parametric form

A ray $r$ at parameter $t$ gives the world-space point:

$$r(t) = \text{origin} + t \cdot \text{direction}$$

```cpp
Vec3 Ray::at(double t) const {
    return origin + direction * t;
}
```

$t$ represents signed distance along the ray scaled by `direction`'s length.
For intersection tests, only values in a range $[t\min, t\max]$ with
$t\min > 0$ are accepted to avoid self-intersection and to limit search depth.

---

## 4. Matrix4

**File:** `include/rt/math/Matrix4.hpp` / `src/math/Matrix4.cpp`

A row-major $4 \times 4$ matrix of `double`. The 4th dimension is the
homogeneous coordinate used to encode affine transformations (translation,
rotation, scale) as matrix multiplications.

### Storage layout

```
double m[4][4];   // m[row][column]
```

### Identity

The identity matrix $I$ leaves any vector unchanged under multiplication:

$$I = \begin{pmatrix} 1 & 0 & 0 & 0 \\ 0 & 1 & 0 & 0 \\ 0 & 0 & 1 & 0 \\ 0 & 0 & 0 & 1 \end{pmatrix}$$

### Translation

Moves points by $(t_x, t_y, t_z)$:

$$T = \begin{pmatrix} 1 & 0 & 0 & t_x \\ 0 & 1 & 0 & t_y \\ 0 & 0 & 1 & t_z \\ 0 & 0 & 0 & 1 \end{pmatrix}$$

Directions (free vectors) are invariant under translation because their
homogeneous $w = 0$.

### Scale

Scales each axis independently:

$$S = \begin{pmatrix} s_x & 0 & 0 & 0 \\ 0 & s_y & 0 & 0 \\ 0 & 0 & s_z & 0 \\ 0 & 0 & 0 & 1 \end{pmatrix}$$

### Rotation around X

Rotates by angle $\theta$ (radians) in the YZ plane:

$$R_x(\theta) = \begin{pmatrix} 1 & 0 & 0 & 0 \\ 0 & \cos\theta & -\sin\theta & 0 \\ 0 & \sin\theta & \cos\theta & 0 \\ 0 & 0 & 0 & 1 \end{pmatrix}$$

### Rotation around Y

Rotates by angle $\theta$ in the XZ plane:

$$R_y(\theta) = \begin{pmatrix} \cos\theta & 0 & \sin\theta & 0 \\ 0 & 1 & 0 & 0 \\ -\sin\theta & 0 & \cos\theta & 0 \\ 0 & 0 & 0 & 1 \end{pmatrix}$$

### Rotation around Z

Rotates by angle $\theta$ in the XY plane:

$$R_z(\theta) = \begin{pmatrix} \cos\theta & -\sin\theta & 0 & 0 \\ \sin\theta & \cos\theta & 0 & 0 \\ 0 & 0 & 1 & 0 \\ 0 & 0 & 0 & 1 \end{pmatrix}$$

### Matrix multiplication

Standard row-by-column product:

$$(A \cdot B)_{ij} = \sum_{k=0}^{3} A_{ik} \cdot B_{kj}$$

```cpp
for (int i = 0; i < 4; ++i)
    for (int j = 0; j < 4; ++j)
        for (int k = 0; k < 4; ++k)
            result.m[i][j] += m[i][k] * other.m[k][j];
```

Transforms are composed by multiplying matrices. The rightmost matrix in a
product is applied first (i.e. `S * R * T` translates, then rotates, then
scales).

### Transforming points vs directions

**Point** (homogeneous $w = 1$): includes translation.

$$p' = M \cdot \begin{pmatrix} p_x \\ p_y \\ p_z \\ 1 \end{pmatrix}$$

Implementation reads only the first three output rows and ignores $w$
(implicitly assumes the matrix does not project):

```cpp
Vec3 transformPoint(const Vec3& p) const {
    // equivalent to M * (p, 1) discarding w
    return Vec3(
        m[0][0]*p.x + m[0][1]*p.y + m[0][2]*p.z + m[0][3],
        m[1][0]*p.x + m[1][1]*p.y + m[1][2]*p.z + m[1][3],
        m[2][0]*p.x + m[2][1]*p.y + m[2][2]*p.z + m[2][3]
    );
}
```

**Direction** (homogeneous $w = 0$): translation term is dropped.

```cpp
Vec3 transformDirection(const Vec3& d) const {
    // equivalent to M * (d, 0) , upper-left 3x3 only
    return Vec3(
        m[0][0]*d.x + m[0][1]*d.y + m[0][2]*d.z,
        m[1][0]*d.x + m[1][1]*d.y + m[1][2]*d.z,
        m[2][0]*d.x + m[2][1]*d.y + m[2][2]*d.z
    );
}
```

### Transpose

Swaps rows and columns: $M^T_{ij} = M_{ji}$.

Used primarily to transform surface normals correctly. When a primitive is
deformed by matrix $M$, normals must be transformed by the **inverse
transpose** $(M^{-T})$ to remain perpendicular to the surface. Skipping this
step causes normals to point in the wrong direction after non-uniform scaling.

### Inverse : Cofactor expansion

The inverse is computed using the **adjugate / cofactor matrix** method. For a
$4 \times 4$ matrix:

$$M^{-1} = \frac{1}{\det(M)} \cdot \text{adj}(M)$$

where $\text{adj}(M)_{ij} = (-1)^{i+j} M^{(ji)}$ and $M^{(ji)}$ is the $3
\times 3$ minor formed by deleting row $j$ and column $i$.

The determinant is recovered from the first row of the input and the first
column of the adjugate (already computed):

$$\det(M) = m_{00} \cdot C_{00} + m_{01} \cdot C_{10} + m_{02} \cdot C_{20} + m_{03} \cdot C_{30}$$

---

## 5. AABB

**File:** `include/rt/math/AABB.hpp`

An **Axis-Aligned Bounding Box** is the minimal box, with sides parallel to
the world axes, that encloses a geometric object. Used by the BVH acceleration
structure to quickly discard ray-object intersections.

### Storage

```
Vec3 min;   // corner with smallest x, y, z
Vec3 max;   // corner with largest  x, y, z
```

The default constructor initializes `min` to `+inf` and `max` to `-inf` so
that the first `expand()` call produces the correct single-point box.

### Ray-AABB intersection : Slab method

The box is the intersection of three pairs of infinite planes (slabs), one
pair per axis. For each axis $i$ (0 = X, 1 = Y, 2 = Z) the ray enters the
slab at parameter $t_0$ and exits at $t_1$:

$$t_0 = \frac{\text{min}_i - \text{origin}_i}{\text{direction}_i}$$
$$t_1 = \frac{\text{max}_i - \text{origin}_i}{\text{direction}_i}$$

When the ray direction component is negative the entry/exit order is swapped
(`std::swap`). The overall intersection interval is:

$$[t\min, t\max] = \bigcap_{i=0}^{2} [t_0^{(i)}, t_1^{(i)}]$$

The ray hits the box if and only if the interval is non-empty after all three
axis tests:

$$t\max > t\min$$

```cpp
double invD = 1.0 / ray.direction[i];     // multiply is faster than divide
double t0 = (min[i] - origin[i]) * invD;
double t1 = (max[i] - origin[i]) * invD;
if (invD < 0.0) std::swap(t0, t1);
tMin = std::max(tMin, t0);
tMax = std::min(tMax, t1);
if (tMax <= tMin) return false;           // miss
```

The reciprocal `invD` is computed once per axis to replace two divisions with
multiplications.

### Expand

`expand(const AABB& other)` returns the smallest AABB that contains both boxes
by taking the component-wise min/max of the two corners.

`expand(const Vec3& point)` does the same for a single point.

### Surface area

$$SA = 2 \bigl( d_x d_y + d_y d_z + d_z d_x \bigr), \quad d = \text{max} - \text{min}$$

Used by the BVH **Surface Area Heuristic (SAH)** to estimate the cost of
choosing a split candidate. Larger surface area means more likely to be hit by
a random ray.

### Centroid

$$c = \frac{\text{min} + \text{max}}{2}$$

Used to bin primitives into SAH buckets during BVH construction.

### Longest axis

Returns 0, 1, or 2 for X, Y, Z respectively. The BVH splits along the longest
axis of the parent node's bounding box to maximize spatial separation of
children.

### isValid

Returns `true` when `min` $\leq$ `max` on all axes. An invalid (inverted)
AABB is the sentinel state after default construction and can be used to detect
empty nodes.
