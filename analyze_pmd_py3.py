#!/usr/bin/env python3
"""
PMD Analysis and Comparison Tool - Python 3
Analyzes PMD files and compares them with COLLADA/glTF exports
"""

import sys
import os
from pmd2collada_py3 import load_pmd

def analyze_pmd_file(filename: str):
    """Analyze a PMD file and print detailed information"""
    
    print(f"=== PMD Analysis: {os.path.basename(filename)} ===")
    
    pmd = load_pmd(filename)
    if not pmd:
        return False
    
    print(f"Magic: {pmd.magic}")
    print(f"Version: {pmd.version}")
    print(f"Data size: {pmd.datasize} bytes")
    print()
    
    print(f"Vertices: {pmd.numVertices}")
    print(f"UV sets per vertex: {pmd.numTexCoords}")
    print()
    
    if pmd.vertices:
        print("First 3 vertices:")
        for i in range(min(3, len(pmd.vertices))):
            v = pmd.vertices[i]
            print(f"  Vertex {i}: pos=({v.position.x:.3f}, {v.position.y:.3f}, {v.position.z:.3f}), " +
                  f"normal=({v.normal.x:.3f}, {v.normal.y:.3f}, {v.normal.z:.3f})")
            print(f"    UVs: [({v.texcoord.u}, {v.texcoord.v})]")
            active_bones = [b for b in v.blend.bones if b != 255]
            active_weights = [f"{w:.3f}" for i, w in enumerate(v.blend.weights) if v.blend.bones[i] != 255]
            print(f"    Bones: {tuple(v.blend.bones)}, Weights: {active_weights}")
    
    print(f"\nFaces: {pmd.numFaces}")
    if pmd.faces:
        print("First 3 faces:")
        for i in range(min(3, len(pmd.faces))):
            f = pmd.faces[i]
            print(f"  Face {i}: vertices {tuple(f.vertices)}")
    
    print(f"\nBones: {pmd.numBones}")
    if pmd.bones:
        print("First 5 bones:")
        for i in range(min(5, len(pmd.bones))):
            b = pmd.bones[i]
            print(f"  Bone {i}: T=({b.translation.x:.3f}, {b.translation.y:.3f}, {b.translation.z:.3f}), " +
                  f"R=({b.rotation.x:.3f}, {b.rotation.y:.3f}, {b.rotation.z:.3f}, {b.rotation.w:.3f})")
    
    print(f"\nProp points: {pmd.numPropPoints}")
    if pmd.propPoints:
        for i, p in enumerate(pmd.propPoints):
            print(f"  Prop {i}: '{p.name}' on bone {p.bone}")
            print(f"    T=({p.translation.x:.3f}, {p.translation.y:.3f}, {p.translation.z:.3f}), " +
                  f"R=({p.rotation.x:.3f}, {p.rotation.y:.3f}, {p.rotation.z:.3f}, {p.rotation.w:.3f})")
    
    print(f"\n=== Summary ===")
    print(f"Valid PMDv{pmd.version}: Verts={pmd.numVertices}, Faces={pmd.numFaces}, " +
          f"Bones={pmd.numBones}, Props={pmd.numPropPoints}")
    print(f"Coordinate System: World space (no transformation needed)")
    print(f"Format: PMD version {pmd.version} with {pmd.numTexCoords} UV set(s)")
    
    return True

def compare_pmd_formats(pmd_file: str):
    """Compare PMD with different export formats"""
    
    base_name = os.path.splitext(pmd_file)[0]
    dae_file = base_name + ".dae"
    gltf_file = base_name + ".gltf"
    
    print(f"\n=== Format Comparison ===")
    
    # PMD analysis
    print("PMD (source):")
    pmd = load_pmd(pmd_file)
    if pmd:
        print(f"  {pmd.numVertices} vertices, {pmd.numFaces} faces, {pmd.numBones} bones, {pmd.numPropPoints} props")
    
    # COLLADA check
    if os.path.exists(dae_file):
        size = os.path.getsize(dae_file)
        print(f"COLLADA: {dae_file} ({size:,} bytes)")
    else:
        print("COLLADA: Not found")
    
    # glTF check
    if os.path.exists(gltf_file):
        size = os.path.getsize(gltf_file)
        print(f"glTF: {gltf_file} ({size:,} bytes)")
    else:
        print("glTF: Not found")

def main():
    """Main function for command line usage"""
    if len(sys.argv) != 2:
        print("Usage: python3 analyze_pmd_py3.py <pmd_file>")
        print("  Analyzes a PMD file and compares with exported formats")
        sys.exit(1)
    
    pmd_filename = sys.argv[1]
    if not os.path.exists(pmd_filename):
        print(f"Error: File {pmd_filename} not found")
        sys.exit(1)
    
    success = analyze_pmd_file(pmd_filename)
    if success:
        compare_pmd_formats(pmd_filename)
    
    sys.exit(0 if success else 1)

if __name__ == "__main__":
    main()