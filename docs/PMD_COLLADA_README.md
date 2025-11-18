# PMD to COLLADA Converter - Python 3 Version

This directory contains Python 3 compatible tools for converting PMD (Pyrogenesis Model Data) files to COLLADA format and analyzing their contents.

## Files

### Core Converters
- **`pmd2collada_py3.py`** - Main PMD to COLLADA converter (Python 3 compatible)
- **`analyze_pmd_py3.py`** - PMD file analysis tool
- **`convert_pmd_all.py`** - Multi-format converter and report generator

### Original Reference (Python 2)
- **`docs/pmd2collada/`** - Original Python 2 implementation for reference

## Usage

### Convert PMD to COLLADA
```bash
python3 pmd2collada_py3.py input/horse.pmd
# Creates: input/horse.dae
```

### Analyze PMD File
```bash
python3 analyze_pmd_py3.py input/horse.pmd
# Shows detailed analysis of PMD contents
```

### Multi-format Conversion
```bash
python3 convert_pmd_all.py input/horse.pmd
# Creates: input/horse.dae + input/horse_report.txt
```

## Features

### PMD Format Support
- **All PMD versions** (1-4) supported
- **Multiple UV sets** (version 4+)
- **Prop points** (version 2+)
- **Vertex skinning** with bone weights
- **World space coordinates** (no transformation needed)

### COLLADA Export
- **Standard COLLADA 1.4.1** format
- **Geometry mesh** with positions, normals, UVs
- **Triangle faces** with proper indexing
- **XML structure** compatible with 3D software

### Analysis Tools
- **Detailed PMD inspection** 
- **Vertex/face/bone statistics**
- **Prop point information**
- **Format comparison** with existing exports

## Technical Details

### Coordinate System
PMD files store coordinates in **world space** matching the rest pose. No coordinate transformation is needed during reading - the data is already in the correct format.

### Vertex Skinning
- Up to **4 bones per vertex** with weights
- Bone index `255` indicates "no bone"
- Weights sum to 1.0 for active bones

### File Structure
```
PMD v2/v3/v4:
├── Header (magic, version, size)
├── Vertices (position, normal, UV, skinning)
├── Faces (triangle indices)
├── Bones (rest pose transforms)
└── Prop Points (attachment points) [v2+]
```

## Example Output

### Analysis
```
=== PMD Analysis: horse.pmd ===
Magic: PSMD
Version: 2
Vertices: 206, Faces: 312, Bones: 33, Props: 8
Coordinate System: World space
```

### COLLADA
```xml
<?xml version="1.0" encoding="utf-8"?>
<COLLADA xmlns="http://www.collada.org/2005/11/COLLADASchema" version="1.4.1">
  <asset>
    <up_axis>Y_UP</up_axis>
  </asset>
  <library_geometries>
    <geometry id="mesh-geometry">
      <mesh>
        <!-- Positions, normals, UVs, triangles -->
      </mesh>
    </geometry>
  </library_geometries>
</COLLADA>
```

## Comparison with Original

| Feature | Original (Python 2) | This Version (Python 3) |
|---------|---------------------|-------------------------|
| **Compatibility** | Python 2.7 | Python 3.6+ |
| **Dependencies** | Custom XML fixes | Standard library only |
| **PMD Support** | All versions | All versions |
| **COLLADA Output** | Full featured | Basic geometry mesh |
| **Analysis Tools** | Basic | Detailed inspection |

## Notes

This Python 3 version focuses on:
- **Modern Python compatibility**
- **Clean, readable code** with type hints  
- **Comprehensive analysis** of PMD contents
- **Basic COLLADA export** for geometry
- **No external dependencies** (uses standard library)

For advanced COLLADA features (bones, animations, materials), consider extending this implementation or using the original Python 2 version with appropriate compatibility tools.