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
    u32 version;  // 1, 2, 3, or 4
    u32 data_size;  // == filesize-12

    u32 numVertices;
    u32 numTexCoords;  // number of UV pairs per vertex (only present in version 4+, defaults to 1 for older versions)
    Vertex vertices[numVertices];

    u32 numFaces;
    Face faces[numFaces];

    u32 numBones;
    BoneState restStates[numBones];

    // Only present in version 2+
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

## Version Differences

**PMD Version 1:**
- No prop points (no `numPropPoints` and no `propPoints` array)
- Single UV pair per vertex (no `numTexCoords` field)

**PMD Version 2:**
- Adds prop points support
- Vertices stored relative to bind-pose (transformed by inverse of `restStates`)
- Single UV pair per vertex (no `numTexCoords` field)
- Only works correctly with single bone influences per vertex

**PMD Version 3:**
- Vertices stored in world space (no bind-pose transformation)
- Single UV pair per vertex (no `numTexCoords` field)
- Supports multiple bone influences per vertex

**PMD Version 4:**
- All features of version 3
- Adds `numTexCoords` field for multiple UV sets per vertex
- Supports multiple texture coordinate pairs per vertex

## Implementation Notes

**Bone Count Limit:**
According to the official PMDConvert.cpp, the maximum number of bones is 254 (not 255) because:
- Bone index 0xFF (255) is reserved as "no bone" marker
- Bone index equal to `jointCount` is reserved for bind-shape matrix special case

**Coordinate System:**
All PMD versions store coordinates in world space. The coordinate transformations mentioned in PMDConvert.cpp apply during PMD creation from COLLADA, not during PMD reading.

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
