# PSA Implementation Details

## Parser Implementation Notes

### Version Validation

The PSA parser validates version numbers and provides appropriate warnings:

```c
// Read and validate version
version = read_u32(f);
if (version != 1) {
    fprintf(stderr, "Warning: Unsupported PSA version %u (expected 1)\n", version);
}
```

Currently, only PSA version 1 is supported by the game engine.

### Bone Count Validation

Based on the official PSAConvert.cpp implementation, bone count should not exceed 192:

```c
// Validation: Check bone count limit (192 max according to PSAConvert.cpp)
if (anim->numBones > 192) {
    fprintf(stderr, "Warning: Too many bones (%u > 192 max) - skeleton may have issues\n", anim->numBones);
}
```

**Rationale:**
- 192 bones is the practical limit enforced by the official converter
- Beyond this limit, skeletal animations may have performance or memory issues
- This is a softer limit than PMD's 254 bone hard limit

### Debug Output

The parser provides informative debug output matching the PMD parser style:

```c
printf("Valid PSAv%u: Bones=%u, Frames=%u, Name='%s'\n", 
       version, anim->numBones, anim->numFrames, anim->name);
```

This helps identify animation properties during conversion.

## Format Understanding

### Unused Fields Discovery

Through analysis of PSAConvert.cpp, we discovered that several PSA fields are **legacy and unused**:

1. **Name field**: 
   - Present in file format but ignored by game engine
   - Animation names are derived from filenames instead
   - Example: `horse_walk.psa` becomes animation "walk"

2. **Frame length field**:
   - Stored in file but not used for playback timing
   - Game uses fixed 30 FPS for all animations
   - Actual timing controlled by actor XML files

### Frame Rate Standardization

PSA animations are created at a fixed **30 FPS**:

```cpp
// From PSAConvert.cpp
float frameLength = 1.f / 30.f; // 30fps
write(output, 1000.f/30.f);     // 33.33ms per frame
```

This standardization ensures consistent playback across all animations.

### Coordinate System Consistency

Like PMD files, PSA animations store bone transforms in **world space**:
- No coordinate transformation needed during reading
- Transformations in PSAConvert.cpp apply during creation phase
- This maintains consistency with PMD coordinate handling

## Memory Layout

### Bone State Storage

Bone states are stored in frame-major order:

```c
// The state of bone b at frame f is stored in:
boneStates[f * numBones + b]
```

This layout is optimized for sequential frame playback.

### Data Structure

```c
typedef struct {
    char *name;           // Animation name (derived from filename)
    float frameLength;    // Legacy field (unused)
    uint32_t numBones;    // Must match PMD bone count
    uint32_t numFrames;   // Number of keyframes
    BoneState *boneStates; // Array of size numBones * numFrames
} PSAAnimation;
```

## Integration with PMD

### Bone Count Matching

PSA animations must have the same bone count as their target PMD model:

```c
// During conversion, bone counts are validated
if (pmd->numBones != psa->numBones) {
    // Warning or error - animations won't work correctly
}
```

### Animation Naming Convention

Animation names are extracted from PSA filenames:

```c
// Pattern: basename_animname.psa -> "animname"
// horse_walk.psa with basename "horse" -> "walk"
char* extract_anim_name(const char *psa_file, const char *basename);
```

This convention allows automatic animation discovery and naming.

## Reference Implementation

### Official C++ Implementation
- Location: `docs/collada/PSAConvert.cpp`
- Used for PSA creation from COLLADA
- Contains frame rate and bone limit specifications

### This C Implementation
- Focus: PSA reading for glTF conversion
- Handles validation and debug output
- Preserves world space coordinate system
- Compatible with all standard PSA v1 files