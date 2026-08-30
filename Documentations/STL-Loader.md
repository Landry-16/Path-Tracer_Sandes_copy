# STL Loader Guide

STL stands for **“Stereolithography”** but there are also several backronyms attributed such as **“Standard Triangle Language”** and **“Standard Tessellation Language”**.

A STL file only contain informations about surfaces, geometry of an object, there is no data about color textures and properties.

# How STL Works

It use **Tessellation** to encode data about the surface geometry of a 3D object.

Tessellation is a tiling ofa surface with geometric shapes without gaps and overlaps. like Honeycombs and tiled walls.

# STL Formats

There is two STL file formats :
- The ASCII method: start with the line `solid <name>`
- The binary method: start with a header of 80 characters then 4-bytes unsigned interger indicates the number of trianlges.The rest is just the information about the triangles, represented as 32-bit floating-point numbers.

Each of them is able to store the types of information about the triangles/facets:
- Coordinates of the tirangles' vertices
- The data regarding the normal vector. It should always point outwards related to the model.

# The rules
- Each trianlge must share two vertices with each of its adjacent triangles.
- The orientation of the facet must be specified in two ways.
    - The rule implies a couple of things :
        1. The normal should point outwards.
        2. The vertices must be listed in a couterclockwise order if you look at the model from the outside. This is called the "right-hand rule"

- The coordinates of the triangle vertices must all be positive. This means that the 3D object must exist in the all-positive octant of the 3D Cartesian coordinates system.
-The triangles should appear in ascending z-value order. This is not a strict rule per se, but it's recommended and allows the slicing software to work faster.


```txt
[80 bytes]  Header (text free, ignored)
[4 bytes]   uint32 — number of triangles
then for each triangle :
    [12 bytes]  normal vector  (3x float32)
    [12 bytes]  vertex 1       (3x float32)
    [12 bytes]  vertex 2       (3x float32)
    [12 bytes]  vertex 3       (3x float32)
    [2 bytes]   attribute byte count (ignored in general)
= 50 bytes per triangle
```