#!/usr/bin/env python3
"""
PMD to COLLADA converter - Python 3 compatible version
Converts 3D models in the PMD format to the COLLADA XML-based format

Based on the original pmd2collada.py but adapted for Python 3 compatibility
"""

import struct
import sys
import os
import datetime
import xml.dom.minidom as xml
from typing import List, Tuple, Optional, BinaryIO

# Vector and math utilities
class Vector3:
    def __init__(self, x: float = 0.0, y: float = 0.0, z: float = 0.0):
        self.x = x
        self.y = y
        self.z = z
    
    def __str__(self):
        return f"{self.x} {self.y} {self.z}"

class Quaternion:
    def __init__(self, x: float = 0.0, y: float = 0.0, z: float = 0.0, w: float = 1.0):
        self.x = x
        self.y = y
        self.z = z
        self.w = w
    
    def __str__(self):
        return f"{self.x} {self.y} {self.z} {self.w}"

class TexCoord:
    def __init__(self, u: float = 0.0, v: float = 0.0):
        self.u = u
        self.v = v
    
    def __str__(self):
        return f"{self.u} {self.v}"

class VertexBlend:
    def __init__(self):
        self.bones = [255, 255, 255, 255]  # 255 = no bone
        self.weights = [0.0, 0.0, 0.0, 0.0]

class Vertex:
    def __init__(self):
        self.position = Vector3()
        self.normal = Vector3()
        self.texcoord = TexCoord()
        self.blend = VertexBlend()

class Face:
    def __init__(self):
        self.vertices = [0, 0, 0]

class BoneState:
    def __init__(self):
        self.translation = Vector3()
        self.rotation = Quaternion()

class PropPoint:
    def __init__(self):
        self.name = ""
        self.translation = Vector3()
        self.rotation = Quaternion()
        self.bone = 0

class PMD:
    """Main class containing all the elements of the PMD format"""
    
    def __init__(self):
        self.magic = ''
        self.version = None
        self.datasize = None
        self.numVertices = 0
        self.vertices: List[Vertex] = []
        self.numFaces = 0
        self.faces: List[Face] = []
        self.numBones = 0
        self.bones: List[BoneState] = []
        self.numPropPoints = 0
        self.propPoints: List[PropPoint] = []
        self.numTexCoords = 1  # Default for versions < 4

def read_float(f: BinaryIO) -> float:
    """Read a 32-bit float from binary file"""
    return struct.unpack('<f', f.read(4))[0]

def read_u32(f: BinaryIO) -> int:
    """Read a 32-bit unsigned integer from binary file"""
    return struct.unpack('<I', f.read(4))[0]

def read_u16(f: BinaryIO) -> int:
    """Read a 16-bit unsigned integer from binary file"""
    return struct.unpack('<H', f.read(2))[0]

def read_u8(f: BinaryIO) -> int:
    """Read an 8-bit unsigned integer from binary file"""
    return struct.unpack('<B', f.read(1))[0]

def read_vector3(f: BinaryIO) -> Vector3:
    """Read a 3D vector from binary file"""
    x = read_float(f)
    y = read_float(f)
    z = read_float(f)
    return Vector3(x, y, z)

def read_quaternion(f: BinaryIO) -> Quaternion:
    """Read a quaternion from binary file"""
    x = read_float(f)
    y = read_float(f)
    z = read_float(f)
    w = read_float(f)
    return Quaternion(x, y, z, w)

def read_texcoord(f: BinaryIO) -> TexCoord:
    """Read texture coordinates from binary file"""
    u = read_float(f)
    v = read_float(f)
    return TexCoord(u, v)

def load_pmd(filename: str) -> Optional[PMD]:
    """Load a PMD file and return a PMD object"""
    
    try:
        with open(filename, 'rb') as f:
            pmd = PMD()
            
            # Read header
            pmd.magic = f.read(4).decode('ascii')
            if pmd.magic != 'PSMD':
                print(f"Error: Invalid PMD magic number: {pmd.magic}")
                return None
            
            pmd.version = read_u32(f)
            pmd.datasize = read_u32(f)
            
            print(f"Loading PMD v{pmd.version}, data size: {pmd.datasize}")
            
            # Read vertices
            pmd.numVertices = read_u32(f)
            
            # Read number of texture coordinates per vertex (version 4+)
            if pmd.version >= 4:
                pmd.numTexCoords = read_u32(f)
            
            print(f"Reading {pmd.numVertices} vertices with {pmd.numTexCoords} UV sets")
            
            for i in range(pmd.numVertices):
                vertex = Vertex()
                vertex.position = read_vector3(f)
                vertex.normal = read_vector3(f)
                
                # Read texture coordinates (multiple sets for version 4+)
                if pmd.numTexCoords > 0:
                    vertex.texcoord = read_texcoord(f)
                    # Skip additional UV sets for now
                    for j in range(1, pmd.numTexCoords):
                        read_texcoord(f)  # Skip
                
                # Read vertex blend data
                for j in range(4):
                    vertex.blend.bones[j] = read_u8(f)
                for j in range(4):
                    vertex.blend.weights[j] = read_float(f)
                
                pmd.vertices.append(vertex)
            
            # Read faces
            pmd.numFaces = read_u32(f)
            print(f"Reading {pmd.numFaces} faces")
            
            for i in range(pmd.numFaces):
                face = Face()
                face.vertices[0] = read_u16(f)
                face.vertices[1] = read_u16(f)
                face.vertices[2] = read_u16(f)
                pmd.faces.append(face)
            
            # Read bones
            pmd.numBones = read_u32(f)
            print(f"Reading {pmd.numBones} bones")
            
            for i in range(pmd.numBones):
                bone = BoneState()
                bone.translation = read_vector3(f)
                bone.rotation = read_quaternion(f)
                pmd.bones.append(bone)
            
            # Read prop points (version 2+)
            if pmd.version >= 2:
                pmd.numPropPoints = read_u32(f)
                print(f"Reading {pmd.numPropPoints} prop points")
                
                for i in range(pmd.numPropPoints):
                    prop = PropPoint()
                    name_len = read_u32(f)
                    prop.name = f.read(name_len).decode('ascii')
                    prop.translation = read_vector3(f)
                    prop.rotation = read_quaternion(f)
                    prop.bone = read_u8(f)
                    pmd.propPoints.append(prop)
            
            print(f"Successfully loaded PMD: {pmd.numVertices} vertices, {pmd.numFaces} faces, {pmd.numBones} bones, {pmd.numPropPoints} props")
            return pmd
            
    except Exception as e:
        print(f"Error loading PMD file {filename}: {e}")
        return None

def create_collada_document(pmd: PMD, filename: str) -> xml.Document:
    """Create a COLLADA XML document from PMD data"""
    
    # Create XML document
    doc = xml.getDOMImplementation().createDocument(None, "COLLADA", None)
    root = doc.documentElement
    root.setAttribute("xmlns", "http://www.collada.org/2005/11/COLLADASchema")
    root.setAttribute("version", "1.4.1")
    
    # Asset information
    asset = doc.createElement("asset")
    root.appendChild(asset)
    
    created = doc.createElement("created")
    created.appendChild(doc.createTextNode(datetime.datetime.now().isoformat()))
    asset.appendChild(created)
    
    modified = doc.createElement("modified")
    modified.appendChild(doc.createTextNode(datetime.datetime.now().isoformat()))
    asset.appendChild(modified)
    
    # Up axis
    up_axis = doc.createElement("up_axis")
    up_axis.appendChild(doc.createTextNode("Y_UP"))
    asset.appendChild(up_axis)
    
    # Library geometries
    lib_geometries = doc.createElement("library_geometries")
    root.appendChild(lib_geometries)
    
    geometry = doc.createElement("geometry")
    geometry.setAttribute("id", "mesh-geometry")
    geometry.setAttribute("name", "mesh-geometry")
    lib_geometries.appendChild(geometry)
    
    mesh = doc.createElement("mesh")
    geometry.appendChild(mesh)
    
    # Positions source
    positions_source = doc.createElement("source")
    positions_source.setAttribute("id", "mesh-geometry-positions")
    mesh.appendChild(positions_source)
    
    positions_array = doc.createElement("float_array")
    positions_array.setAttribute("id", "mesh-geometry-positions-array")
    positions_array.setAttribute("count", str(pmd.numVertices * 3))
    positions_source.appendChild(positions_array)
    
    # Build positions data
    positions_data = []
    for vertex in pmd.vertices:
        positions_data.extend([vertex.position.x, vertex.position.y, vertex.position.z])
    
    positions_array.appendChild(doc.createTextNode(" ".join(map(str, positions_data))))
    
    # Positions accessor
    positions_technique = doc.createElement("technique_common")
    positions_source.appendChild(positions_technique)
    
    positions_accessor = doc.createElement("accessor")
    positions_accessor.setAttribute("source", "#mesh-geometry-positions-array")
    positions_accessor.setAttribute("count", str(pmd.numVertices))
    positions_accessor.setAttribute("stride", "3")
    positions_technique.appendChild(positions_accessor)
    
    for component in ["X", "Y", "Z"]:
        param = doc.createElement("param")
        param.setAttribute("name", component)
        param.setAttribute("type", "float")
        positions_accessor.appendChild(param)
    
    # Normals source
    normals_source = doc.createElement("source")
    normals_source.setAttribute("id", "mesh-geometry-normals")
    mesh.appendChild(normals_source)
    
    normals_array = doc.createElement("float_array")
    normals_array.setAttribute("id", "mesh-geometry-normals-array")
    normals_array.setAttribute("count", str(pmd.numVertices * 3))
    normals_source.appendChild(normals_array)
    
    # Build normals data
    normals_data = []
    for vertex in pmd.vertices:
        normals_data.extend([vertex.normal.x, vertex.normal.y, vertex.normal.z])
    
    normals_array.appendChild(doc.createTextNode(" ".join(map(str, normals_data))))
    
    # Normals accessor
    normals_technique = doc.createElement("technique_common")
    normals_source.appendChild(normals_technique)
    
    normals_accessor = doc.createElement("accessor")
    normals_accessor.setAttribute("source", "#mesh-geometry-normals-array")
    normals_accessor.setAttribute("count", str(pmd.numVertices))
    normals_accessor.setAttribute("stride", "3")
    normals_technique.appendChild(normals_accessor)
    
    for component in ["X", "Y", "Z"]:
        param = doc.createElement("param")
        param.setAttribute("name", component)
        param.setAttribute("type", "float")
        normals_accessor.appendChild(param)
    
    # UV coordinates source
    if pmd.numTexCoords > 0:
        uvs_source = doc.createElement("source")
        uvs_source.setAttribute("id", "mesh-geometry-map")
        mesh.appendChild(uvs_source)
        
        uvs_array = doc.createElement("float_array")
        uvs_array.setAttribute("id", "mesh-geometry-map-array")
        uvs_array.setAttribute("count", str(pmd.numVertices * 2))
        uvs_source.appendChild(uvs_array)
        
        # Build UV data
        uvs_data = []
        for vertex in pmd.vertices:
            uvs_data.extend([vertex.texcoord.u, vertex.texcoord.v])
        
        uvs_array.appendChild(doc.createTextNode(" ".join(map(str, uvs_data))))
        
        # UVs accessor
        uvs_technique = doc.createElement("technique_common")
        uvs_source.appendChild(uvs_technique)
        
        uvs_accessor = doc.createElement("accessor")
        uvs_accessor.setAttribute("source", "#mesh-geometry-map-array")
        uvs_accessor.setAttribute("count", str(pmd.numVertices))
        uvs_accessor.setAttribute("stride", "2")
        uvs_technique.appendChild(uvs_accessor)
        
        for component in ["S", "T"]:
            param = doc.createElement("param")
            param.setAttribute("name", component)
            param.setAttribute("type", "float")
            uvs_accessor.appendChild(param)
    
    # Vertices
    vertices = doc.createElement("vertices")
    vertices.setAttribute("id", "mesh-geometry-vertices")
    mesh.appendChild(vertices)
    
    input_pos = doc.createElement("input")
    input_pos.setAttribute("semantic", "POSITION")
    input_pos.setAttribute("source", "#mesh-geometry-positions")
    vertices.appendChild(input_pos)
    
    # Triangles
    triangles = doc.createElement("triangles")
    triangles.setAttribute("count", str(pmd.numFaces))
    mesh.appendChild(triangles)
    
    # Triangle inputs
    input_vertex = doc.createElement("input")
    input_vertex.setAttribute("semantic", "VERTEX")
    input_vertex.setAttribute("source", "#mesh-geometry-vertices")
    input_vertex.setAttribute("offset", "0")
    triangles.appendChild(input_vertex)
    
    input_normal = doc.createElement("input")
    input_normal.setAttribute("semantic", "NORMAL")
    input_normal.setAttribute("source", "#mesh-geometry-normals")
    input_normal.setAttribute("offset", "1")
    triangles.appendChild(input_normal)
    
    if pmd.numTexCoords > 0:
        input_texcoord = doc.createElement("input")
        input_texcoord.setAttribute("semantic", "TEXCOORD")
        input_texcoord.setAttribute("source", "#mesh-geometry-map")
        input_texcoord.setAttribute("offset", "2")
        triangles.appendChild(input_texcoord)
    
    # Triangle indices
    indices_data = []
    for face in pmd.faces:
        for vertex_idx in face.vertices:
            indices_data.append(str(vertex_idx))  # vertex
            indices_data.append(str(vertex_idx))  # normal
            if pmd.numTexCoords > 0:
                indices_data.append(str(vertex_idx))  # texcoord
    
    p_element = doc.createElement("p")
    p_element.appendChild(doc.createTextNode(" ".join(indices_data)))
    triangles.appendChild(p_element)
    
    # Scene
    lib_visual_scenes = doc.createElement("library_visual_scenes")
    root.appendChild(lib_visual_scenes)
    
    visual_scene = doc.createElement("visual_scene")
    visual_scene.setAttribute("id", "Scene")
    visual_scene.setAttribute("name", "Scene")
    lib_visual_scenes.appendChild(visual_scene)
    
    # Geometry node
    node = doc.createElement("node")
    node.setAttribute("id", "mesh")
    node.setAttribute("name", "mesh")
    node.setAttribute("type", "NODE")
    visual_scene.appendChild(node)
    
    instance_geometry = doc.createElement("instance_geometry")
    instance_geometry.setAttribute("url", "#mesh-geometry")
    node.appendChild(instance_geometry)
    
    # Scene reference
    scene = doc.createElement("scene")
    root.appendChild(scene)
    
    instance_visual_scene = doc.createElement("instance_visual_scene")
    instance_visual_scene.setAttribute("url", "#Scene")
    scene.appendChild(instance_visual_scene)
    
    return doc

def convert_pmd_to_collada(pmd_filename: str, output_filename: Optional[str] = None) -> bool:
    """Convert a PMD file to COLLADA format"""
    
    if not output_filename:
        base_name = os.path.splitext(pmd_filename)[0]
        output_filename = base_name + ".dae"
    
    print(f"Converting {pmd_filename} to {output_filename}")
    
    # Load PMD
    pmd = load_pmd(pmd_filename)
    if not pmd:
        return False
    
    # Create COLLADA document
    doc = create_collada_document(pmd, pmd_filename)
    
    # Write to file
    try:
        with open(output_filename, 'w', encoding='utf-8') as f:
            doc.writexml(f, indent="", addindent="  ", newl="\n", encoding="utf-8")
        
        print(f"Successfully converted to {output_filename}")
        return True
        
    except Exception as e:
        print(f"Error writing COLLADA file: {e}")
        return False

def main():
    """Main function for command line usage"""
    if len(sys.argv) != 2:
        print("Usage: python3 pmd2collada_py3.py <pmd_file>")
        sys.exit(1)
    
    pmd_filename = sys.argv[1]
    if not os.path.exists(pmd_filename):
        print(f"Error: File {pmd_filename} not found")
        sys.exit(1)
    
    success = convert_pmd_to_collada(pmd_filename)
    sys.exit(0 if success else 1)

if __name__ == "__main__":
    main()