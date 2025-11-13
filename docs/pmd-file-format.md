PMD files store a model, which includes vertex positions, normals and texture coordinates, faces (all faces are triangles), rest state bone transformations (if bones are used), and prop point positions and transformations (where other models may be propped onto this one).

The following is the version 4 PMD format, which adds support for multiple UV sets per model (i.e. multiple texture coordinate pairs per vertex).

Basic types:
 * char = 8-bit character
 * u32 = 32-bit unsigned int
 * u16 = 16-bit unsigned int
 * u8 = 8-bit unsigned int
 * float = 32-bit single-precision float

All types are stored in little-endian format. Text is always ASCII.

```c
 PMD {
    char magic[4];  // == "PSMD"
    u32 version;  // == 4
    u32 data_size;  // == filesize-12

    u32 numVertices;
    u32 numTexCoords;  // number of UV pairs per vertex
    Vertex vertices[numVertices];

    u32 numFaces;
    Face faces[numFaces];

    u32 numBones;
    BoneState restStates[numBones];

    u32 numPropPoints;
    PropPoint propPoints[numPropPoints];  // note: PropPoint has variable size (see below)
 }

 Vertex {
    Vector3D position;  // in world space, matching the restStates pose
    Vector3D normal;
    TexCoord coords[numTextureCoords];
    VertexBlend blend;
 }

 TexCoord {
    float u, v;
 }

 VertexBlend {  // represents the set of bones affecting a vertex
    u8 bones[4];  // bone indices; 0xFF in any position indicates "no bone" for vertices with fewer than 4 bones
    float weights[4]; // add up to 1.0 (0 is used for unused bones)
 }

 Face {
    u16 vertices[3];  // vertex indices; all faces are triangles
 }

 BoneState {
    Vector3D translation;
    Quaternion rotation;

    // the bone's final transform matrix is translation * rotation
 }

 PropPoint {
    u32 nameLength;
    char name[nameLength];

    Vector3D translation;
    Quaternion rotation;

    u8 bone;

    // the prop point's final transform matrix is boneTransform * translation * rotation,
    // where boneTransform is that of the given bone or identity if no bones are used
 }

 Vector3D {
    float x, y, z;
 }

 Quaternion {
    float x, y, z, w;
 }
```

Some meshes also use PMD version 1. The only difference between this and version 2 is that no prop points are specified (there is no `numPropPoints` and no `propPoints` array).

Some use PMD version 2. These store vertex positions/normals already transformed by the inverse of `restStates`. That can only work correctly when `VertexBlend` has only a single non-zero weight, so PMD version 3 stores vertices in world space instead.

The algorithm for calculating vertex data under a skeletal animation is:

```c
 pos = (0,0,0)
 norm = (0,0,0)
 for i = 0 to 3:
   if blend.bones[i] != 0xFF:
     inverseRestMatrix = (restStates[blend.bones[i]].translation * restStates[blend.bones[i]].rotation) ^ -1
     animatedPoseMatrix = animatedBoneStates[blend.bones[i]].translation * animatedBoneStates[blend.bones[i]].rotation
     pos  += blend.weights[i] * animatedPoseMatrix * inverseRestMatrix * (position,1)
     norm += blend.weights[i] * animatedPoseMatrix * inverseRestMatrix * (normal,0)
 norm = normalise(norm)
```

PMD version 3 supported only one UV pair per vertex, so there was no `numTexCoords`.
