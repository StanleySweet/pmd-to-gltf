PSA files store a skeletal animation, which consists of bone states at a number of keyframes (see [PMD File Format](PMD_File_Format)). Animations can only be applied to models with the same number of bones in the same order (no bone names are used, only indices). Although PSA contains a name field and a frame length field, these are no longer used in the game since animation variants and total animation lengths (so the same animation can be reused with different lengths) are given in the actor XML files.

The following is the version 1 PSA format.

Basic types:
 * char = 8-bit character
 * u32 = 32-bit unsigned int
 * float = 32-bit single-precision floating point

All types are stored in little-endian format. Text is always ASCII.

```c
 PSA {
    char magic[4];  // == "PSSA"
    u32 version;  // == 1
    u32 data_size;  // == filesize-12

    u32 nameLength;
    char name[nameLength];  // no longer used in the game

    float frameLength;  // no longer used in the game, not valid for most of the animations

    u32 numBones;
    u32 numFrames;

    BoneState boneStates[numBones * numFrames];

    // the state of bone b at frame f is stored in boneStates[f * numBones + b]
 }

 BoneState {
    Vector3D translation;
    Quaternion rotation;

    // the bone's final transform matrix is translation * rotation, as in PMD
 }

 Vector3D {
    float x, y, z;
 }

 Quaternion {
    float x, y, z, w;
 }
```
