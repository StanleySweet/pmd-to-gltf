#!/usr/bin/env python3
"""
Detailed PMD analysis tool
Shows the same information as pmd2collada but with Python 3
"""

import struct
import sys
import os

class Vector3D:
    def __init__(self, vals=[0.0, 0.0, 0.0]):
        self.x = vals[0]
        self.y = vals[1] 
        self.z = vals[2]
    
    def __str__(self):
        return f"({self.x:.3f}, {self.y:.3f}, {self.z:.3f})"

class Quaternion:
    def __init__(self, vals=[0.0, 0.0, 0.0, 1.0]):
        self.x = vals[0]
        self.y = vals[1]
        self.z = vals[2]
        self.w = vals[3]
    
    def __str__(self):
        return f"({self.x:.3f}, {self.y:.3f}, {self.z:.3f}, {self.w:.3f})"

def read_vector3d(f):
    return Vector3D(struct.unpack('<fff', f.read(12)))

def read_quaternion(f):
    return Quaternion(struct.unpack('<ffff', f.read(16)))

def analyze_pmd(filepath):
    """Detailed PMD analysis similar to pmd2collada"""
    with open(filepath, 'rb') as f:
        # Read header
        magic = f.read(4).decode('ascii')
        version = struct.unpack('<L', f.read(4))[0]
        data_size = struct.unpack('<L', f.read(4))[0]
        
        print(f"=== PMD Analysis: {os.path.basename(filepath)} ===")
        print(f"Magic: {magic}")
        print(f"Version: {version}")
        print(f"Data size: {data_size} bytes")
        print()
        
        # Read vertices
        num_vertices = struct.unpack('<L', f.read(4))[0]
        num_texcoords = 1
        if version >= 4:
            num_texcoords = struct.unpack('<L', f.read(4))[0]
            
        print(f"Vertices: {num_vertices}")
        print(f"UV sets per vertex: {num_texcoords}")
        
        # Read first few vertices for sampling
        vertices_sample = min(3, num_vertices)
        print(f"\nFirst {vertices_sample} vertices:")
        
        for i in range(vertices_sample):
            pos = read_vector3d(f)
            normal = read_vector3d(f)
            
            # Read UV coordinates
            uvs = []
            for j in range(num_texcoords):
                u, v = struct.unpack('<ff', f.read(8))
                uvs.append((u, v))
            
            # Read bone weights
            bones = struct.unpack('<BBBB', f.read(4))
            weights = struct.unpack('<ffff', f.read(16))
            
            print(f"  Vertex {i}: pos={pos}, normal={normal}")
            print(f"    UVs: {uvs}")
            print(f"    Bones: {bones}, Weights: {[f'{w:.3f}' for w in weights]}")
        
        # Skip remaining vertices
        vertex_size = 3*4 + 3*4 + num_texcoords*2*4 + 4 + 4*4
        remaining_vertices = num_vertices - vertices_sample
        f.seek(remaining_vertices * vertex_size, 1)
        
        # Read faces
        num_faces = struct.unpack('<L', f.read(4))[0]
        print(f"\nFaces: {num_faces}")
        
        # Read first few faces
        faces_sample = min(3, num_faces)
        print(f"First {faces_sample} faces:")
        for i in range(faces_sample):
            face = struct.unpack('<HHH', f.read(6))
            print(f"  Face {i}: vertices {face}")
        
        # Skip remaining faces
        f.seek((num_faces - faces_sample) * 6, 1)
        
        # Read bones
        num_bones = struct.unpack('<L', f.read(4))[0]
        print(f"\nBones: {num_bones}")
        
        # Read first few bones
        bones_sample = min(5, num_bones)
        print(f"First {bones_sample} bones:")
        for i in range(bones_sample):
            translation = read_vector3d(f)
            rotation = read_quaternion(f)
            print(f"  Bone {i}: T={translation}, R={rotation}")
        
        # Skip remaining bones
        f.seek((num_bones - bones_sample) * 28, 1)
        
        # Read prop points
        num_props = 0
        if version >= 2:
            num_props = struct.unpack('<L', f.read(4))[0]
            print(f"\nProp points: {num_props}")
            
            for i in range(min(num_props, 3)):  # Show first 3
                name_len = struct.unpack('<L', f.read(4))[0]
                name = f.read(name_len).decode('ascii')
                translation = read_vector3d(f)
                rotation = read_quaternion(f)
                bone = struct.unpack('<B', f.read(1))[0]
                
                print(f"  Prop {i}: '{name}' on bone {bone}")
                print(f"    T={translation}, R={rotation}")
        
        print(f"\n=== Summary ===")
        print(f"Valid PMDv{version}: Verts={num_vertices}, Faces={num_faces}, Bones={num_bones}, Props={num_props}")
        
        # Coordinate system info
        print(f"\nCoordinate System: World space (no transformation needed)")
        print(f"Format: PMD version {version} with {num_texcoords} UV set(s)")

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python3 analyze_pmd.py <pmd_file>")
        sys.exit(1)
    
    pmd_file = sys.argv[1]
    if not os.path.exists(pmd_file):
        print(f"File not found: {pmd_file}")
        sys.exit(1)
        
    try:
        analyze_pmd(pmd_file)
    except Exception as e:
        print(f"Error analyzing PMD file: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)