# Test Suite Documentation

This document describes the comprehensive test suite for the PMD/PSA to glTF converter, including technical and functional specifications.

## Overview

The test suite validates that:
1. PMD files can be loaded correctly (standalone)
2. PMD + PSA files can be converted to valid glTF 2.0 format
3. Vertex positions are preserved exactly during conversion
4. Bone rest positions and hierarchies are maintained
5. Animations are correctly applied to bones

## Test Data Files

### Cube Test Models

The test suite includes three programmatically generated cube models to test different configurations:

#### 1. Cube with No Bones (`cube_nobones.pmd`)
- **Purpose**: Test static mesh conversion without skeletal animation
- **Specifications**:
  - 8 vertices (forming a 2m × 2m × 2m cube)
  - 12 triangular faces (2 per cube face)
  - 0 bones
  - 0 prop points
  - All vertices have no bone assignments (0xFF markers)
- **Expected Behavior**: 
  - Vertices should remain at exact positions: (-1,-1,-1) to (1,1,1)
  - glTF should have valid mesh data with no skin/skeleton
  - Cube dimensions preserved: 2m in each dimension

#### 2. Cube with 4 Bones (`cube_4bones.pmd` + `cube_4bones_anim.psa`)
- **Purpose**: Test skeletal animation with bones at corners
- **Specifications**:
  - 8 vertices (2m × 2m × 2m cube)
  - 12 triangular faces
  - 4 bones positioned at bottom corners:
    - Bone 0: (-1, -1, -1)
    - Bone 1: ( 1, -1, -1)
    - Bone 2: (-1,  1, -1)
    - Bone 3: ( 1,  1, -1)
  - Each vertex is weighted to one bone (weight = 1.0)
  - Animation: 10 frames, bones rotate around Z axis
- **Expected Behavior**:
  - Bone rest positions match vertex positions at corners
  - Animation transforms are applied correctly
  - Vertex positions remain correct in rest pose

#### 3. Cube with 5 Bones Hierarchy (`cube_5bones.pmd` + `cube_5bones_anim.psa` + `cube_5bones.xml`)
- **Purpose**: Test hierarchical bone structures
- **Specifications**:
  - 8 vertices (2m × 2m × 2m cube)
  - 12 triangular faces
  - 5 bones in hierarchy:
    - Bone 0-3: Corner bones (same as 4-bone cube)
    - Bone 4: Center bone at (0, 0, 0) - parent of all corner bones
  - Skeleton XML defines parent-child relationships:
    - bone_center (root)
      - bone_corner_0
      - bone_corner_1
      - bone_corner_2
      - bone_corner_3
  - Vertices 0-3: Fully weighted to corner bones
  - Vertices 4-7: 50% weighted to corner + 50% to center
  - Animation: 10 frames, center bone rotates around Y axis
- **Expected Behavior**:
  - Hierarchy is preserved in glTF output
  - Local transforms computed correctly from world transforms
  - Center bone influences vertices with blended weights

## Technical Specifications

### PMD File Format (Version 4)
All test PMD files use version 4 of the PMD format with the following structure:

```
Header:
  - Magic: "PSMD" (4 bytes)
  - Version: 4 (uint32)
  - Data size: calculated (uint32)

Vertices Section:
  - numVertices: 8
  - numTexCoords: 1
  - Each vertex contains:
    - position: Vector3D (12 bytes)
    - normal: Vector3D (12 bytes)
    - texcoords: 1 × TexCoord (8 bytes)
    - blend.bones: 4 × uint8 (4 bytes)
    - blend.weights: 4 × float32 (16 bytes)

Faces Section:
  - numFaces: 12
  - Each face: 3 × uint16 vertex indices (6 bytes)

Bones Section:
  - numBones: 0, 4, or 5
  - Each bone state:
    - translation: Vector3D (12 bytes)
    - rotation: Quaternion (16 bytes)

PropPoints Section:
  - numPropPoints: 0
  - (empty for test cubes)
```

### PSA Animation Format (Version 1)
Test animations use PSA version 1:

```
Header:
  - Magic: "PSSA" (4 bytes)
  - Version: 1 (uint32)
  - Data size: calculated (uint32)

Animation Data:
  - nameLength: variable
  - name: "test_anim" (string)
  - frameLength: 0.03333 (30 fps)
  - numBones: 4 or 5
  - numFrames: 10

Bone States:
  - boneStates[numBones × numFrames]
  - Each state:
    - translation: Vector3D
    - rotation: Quaternion
  - Indexed as: boneStates[frame × numBones + bone]
```

### Skeleton XML Format
The 5-bone test includes a skeleton XML file:

```xml
<?xml version="1.0" encoding="UTF-8"?>
<skeleton target="cube_5bones">
  <bone name="bone_center">
    <bone name="bone_corner_0"/>
    <bone name="bone_corner_1"/>
    <bone name="bone_corner_2"/>
    <bone name="bone_corner_3"/>
  </bone>
</skeleton>
```

## Functional Specifications

### Test Categories

#### Unit Tests (`test_pmd_cubes`)
These tests validate that PMD/PSA files can be loaded and parsed correctly:

1. **test_load_cube_nobones**
   - Loads cube_nobones.pmd
   - Verifies: 8 vertices, 12 faces, 0 bones
   - Checks all vertices have 0xFF bone markers

2. **test_load_cube_4bones**
   - Loads cube_4bones.pmd
   - Verifies: 8 vertices, 12 faces, 4 bones
   - Checks bone positions at corners (-1,-1,-1), (1,-1,-1), etc.
   - Validates bone assignments and weights

3. **test_load_cube_5bones**
   - Loads cube_5bones.pmd
   - Verifies: 8 vertices, 12 faces, 5 bones
   - Checks center bone at origin (0,0,0)

4. **test_load_animation_4bones**
   - Loads cube_4bones_anim.psa
   - Verifies: 4 bones, 10 frames
   - Checks first frame has rest positions

5. **test_load_animation_5bones**
   - Loads cube_5bones_anim.psa
   - Verifies: 5 bones, 10 frames

6. **test_cube_dimensions**
   - Verifies cube is exactly 2m × 2m × 2m
   - Checks min/max bounds: (-1,-1,-1) to (1,1,1)
   - Ensures vertex positions are preserved

7. **test_bone_vertex_alignment**
   - For vertices with single bone weight = 1.0
   - Verifies bone is positioned at or near the vertex
   - Validates rest pose alignment

8. **test_face_validity**
   - Checks all vertex indices are within bounds
   - Ensures no degenerate triangles
   - Validates all 3 vertices per face are unique

#### Integration Tests
These tests run the full converter pipeline:

1. **integration_cube_nobones**
   - Converts `cube_nobones.pmd` → `cube_nobones.gltf`
   - Expected: Valid glTF with mesh but no skeleton

2. **integration_cube_4bones**
   - Converts `cube_4bones.pmd` + `cube_4bones_anim.psa` → glTF
   - Expected: Valid glTF with mesh, skeleton, skin, and 1 animation

3. **integration_cube_5bones**
   - Converts `cube_5bones.pmd` + `cube_5bones_anim.psa` + `cube_5bones.xml` → glTF
   - Expected: Valid glTF with hierarchical skeleton and animation

### Validation Criteria

#### Vertex Position Preservation
- **Requirement**: Vertex positions in world space must be preserved exactly
- **Test Method**: Compare PMD vertex positions to glTF mesh accessor data
- **Tolerance**: ±0.001 units
- **Rationale**: PMD stores vertices in world space matching rest pose; glTF must preserve this

#### Bone Rest Position Preservation
- **Requirement**: Bone rest transforms must be maintained in glTF
- **Test Method**: 
  - For flat bones: Check world positions match
  - For hierarchical bones: Check local transforms compute to same world positions
- **Tolerance**: ±0.001 units
- **Rationale**: Bones define the skeleton's rest pose; must be accurate for proper skinning

#### Bone Hierarchy Validation
- **Requirement**: Parent-child relationships must be preserved
- **Test Method**: Parse skeleton XML and verify glTF node hierarchy matches
- **For 5-bone test**: Center bone should be parent of 4 corner bones
- **Rationale**: Hierarchy affects transform propagation and animation

#### Animation Frame Accuracy
- **Requirement**: Animation keyframes must match PSA data
- **Test Method**: Compare first/last frame transforms
- **Tolerance**: ±0.001 for translations, ±0.0001 for quaternions
- **Rationale**: Animations must play back correctly in glTF viewers

## Test Execution

### Building and Running Tests

```bash
# Configure and build
cmake --preset default
cmake --build --preset default

# Run all tests
cd build
ctest --output-on-failure

# Run specific test category
ctest -R integration_  # Integration tests only
ctest -R unit_         # Unit tests only
```

### Test Dependencies

Test execution order:
1. `generate_test_data` - Creates PMD/PSA test files
2. `integration_pmd_cubes` - Validates file loading
3. `integration_cube_*` - Validates glTF conversion

The `generate_test_data` test must pass before other tests run, as they depend on the generated files.

### Expected Output

All tests should pass with output similar to:

```
Test project /path/to/build
    Start 1: unit_filesystem
1/9 Test #1: unit_filesystem ..................   Passed    0.00 sec
    Start 2: unit_animation
2/9 Test #2: unit_animation ...................   Passed    0.00 sec
    ...
    Start 5: integration_pmd_cubes
5/9 Test #5: integration_pmd_cubes ............   Passed    0.00 sec
    ...

100% tests passed, 0 tests failed out of 9
```

## glTF Output Validation

### Manual Validation

Generated glTF files can be validated using:

1. **glTF Validator** (https://github.khronos.org/glTF-Validator/)
   ```bash
   gltf-validator tests/output/cube_4bones.gltf
   ```

2. **Visual Inspection** in glTF viewers:
   - Babylon.js Sandbox (https://sandbox.babylonjs.com/)
   - three.js editor (https://threejs.org/editor/)
   - Blender (import glTF)

### Expected glTF Structure

Each output should contain:

```json
{
  "asset": {"version": "2.0"},
  "scene": 0,
  "scenes": [{"nodes": [0]}],
  "nodes": [
    {"name": "Armature", "children": [...]},
    {"mesh": 0, "skin": 0}
  ],
  "meshes": [{
    "primitives": [{
      "attributes": {
        "POSITION": ...,
        "NORMAL": ...,
        "TEXCOORD_0": ...,
        "JOINTS_0": ...,
        "WEIGHTS_0": ...
      },
      "indices": ...
    }]
  }],
  "skins": [{
    "inverseBindMatrices": ...,
    "joints": [...]
  }],
  "animations": [...]
}
```

## Known Limitations

1. **Coordinate System**: Tests assume glTF right-handed Y-up coordinate system
2. **Texture Coordinates**: Simple UV mapping (0,0) to (1,1)
3. **Normals**: Computed as normalized position vectors (suitable for cubes)
4. **Materials**: Not included in test files
5. **Multiple Animations**: Each PSA file contains one animation

## Future Enhancements

Potential additions to the test suite:

1. **Stress Tests**:
   - Large vertex counts (10,000+ vertices)
   - Complex bone hierarchies (100+ bones)
   - Long animations (1000+ frames)

2. **Edge Cases**:
   - Zero-weight vertices
   - Degenerate faces
   - Non-normalized quaternions
   - Extreme scale/rotation values

3. **Format Variations**:
   - PMD version 1, 2, 3 files
   - Multiple UV sets
   - Prop points with attachments

4. **Visual Regression Tests**:
   - Render glTF and compare images
   - Detect visual artifacts
   - Validate animation playback

5. **Performance Benchmarks**:
   - Conversion time measurements
   - Memory usage profiling
   - Optimization validation
