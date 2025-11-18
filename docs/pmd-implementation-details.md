# PMD Implementation Details

## Parser Implementation Notes

### Version Detection and Validation

The PMD parser validates version numbers and provides appropriate warnings:

```c
// Validation: Check supported version
if (model->version < 1 || model->version > 4) {
    fprintf(stderr, "Warning: Unsupported PMD version %u (expected 1-4)\n", model->version);
}
```

### Multi-UV Support (Version 4)

Version 4 adds support for multiple UV coordinate sets per vertex:

```c
// Version 4+ includes numTexCoords field, older versions default to 1
model->numTexCoords = (model->version >= 4) ? read_u32(f) : 1;

// Each vertex stores multiple UV pairs
for (uint32_t j = 0; j < model->numTexCoords; j++) {
    v->coords[j].u = read_float(f);
    v->coords[j].v = read_float(f);
}
```

### Bone Count Validation

Based on the official PMDConvert.cpp implementation, bone count is limited to 254:

```c
// Validation: Check bone count limit (254 max according to PMDConvert.cpp)
if (model->numBones > 254) {
    fprintf(stderr, "Error: Too many bones (%u > 254 max)\n", model->numBones);
    return NULL;
}
```

**Rationale:**
- Bone indices are stored as `uint8_t` (0-255)
- Value `0xFF` (255) is reserved as "no bone" marker
- Value equal to `jointCount` is reserved for bind-shape matrix special case
- This leaves 254 as the practical maximum

### Debug Output

The parser provides informative debug output matching the pmd2collada reference implementation:

```c
printf("Valid PMDv%u: Verts=%u, Faces=%u, Bones=%u, Props=%u\n", 
       model->version, model->numVertices, model->numFaces, 
       model->numBones, model->numPropPoints);
```

## Coordinate System Handling

### Important Discovery

**PMD files store coordinates in world space.** No coordinate transformation is needed when reading PMD files for glTF export.

The coordinate transformations mentioned in PMDConvert.cpp apply during:
- **PMD creation** from COLLADA sources
- **Not during PMD reading**

### Incorrect Assumptions

Initially, we assumed coordinate transformation was needed based on PMDConvert.cpp comments about "game coordinates" vs "right-handed Z_UP coordinates". However, the PMD format documentation clearly states:

> Vertex position: in world space, matching the restStates pose

This means PMD files already contain properly transformed coordinates.

## glTF Export Considerations

### Prop Point Naming

Prop points are exported with "prop-" prefix to distinguish them from regular bones:

```c
// Handle special "root" prop point according to PMD specifications
if (strcmp(prop_name, "root") == 0) {
    fprintf(f, "\"name\": \"prop-root\"");
} else {
    fprintf(f, "\"name\": \"prop-%s\"", prop_name);
}
```

### UV Coordinate Handling

glTF expects V origin at top, while PMD may store V origin at bottom:

```c
// glTF expects V origin at top; source data appears upside-down -> flip V
texcoords[i*2+0] = model->vertices[i].coords[0].u;
texcoords[i*2+1] = 1.0f - model->vertices[i].coords[0].v;
```

## Function Declarations

All PMD/PSA functions are properly declared in the header:

```c
// Function declarations
PMDModel* load_pmd(const char *filename);
void free_pmd(PMDModel *model);
PSAAnimation* load_psa(const char *filename);
void free_psa(PSAAnimation *anim);
```

## Reference Implementations

### Official C++ Implementation
- Location: `docs/collada/PMDConvert.cpp`
- Used for PMD creation from COLLADA
- Contains coordinate transformation logic for creation phase

### Python Reference Implementation  
- Location: `docs/pmd2collada/pmd2collada.py`
- Contains reverse transformation (PMD to COLLADA)
- Useful for understanding coordinate systems and data validation

### This C Implementation
- Focus: PMD to glTF conversion
- Handles all PMD versions (1-4)
- Preserves world space coordinates without transformation