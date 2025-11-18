#    PMD to COLLADA converter, pmd2collada.py v3.0
#
#    Converts 3D models in the PMD format to the COLLADA XML-based format
#
#    This program is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program.  If not, see <http://www.gnu.org/licenses/>.

# Usage: Import pmd2collada into your own script that lists 
# the PMD files you want to convert. Feed a new instance of PMDtoCollada the full
# path and watch it go. PMDtoCollada will convert the PMD files to DAE and 
# put them in the same directory as the source file.

import struct, sys, os, datetime, easyxml
import xml.dom.minidom as xml
from vector_defs import *
from pmd2collada_defs import *

# global constants
TAB = '\t'
NL = '\n'
PROP_PREFIX = 'prop_'

# WARNING: Using Python's XML pretty print feature forces tags to be on separate lines
#   from their data and indents nested nodes, which casues big problems with text,
#   because text whitespace cannot be easily ignored. The resulting document will fail
#   in subtle ways when parsed by FCollada.
# However, this bug is fixed in Python's 2.7 Hg. As a workaround we can copy the latest
#   implementation of Element.writexml() and replace the buggy one:
def fixed_writexml(self, writer, indent="", addindent="", newl=""):
    # indent = current indentation
    # addindent = indentation to add to higher levels
    # newl = newline string
    writer.write(indent+"<" + self.tagName)
    attrs = self._get_attributes()
    a_names = attrs.keys()
    a_names.sort()
    for a_name in a_names:
        writer.write(" %s=\"" % a_name)
        xml._write_data(writer, attrs[a_name].value)
        writer.write("\"")
    if self.childNodes:
        writer.write(">")
        if (len(self.childNodes) == 1 and
            self.childNodes[0].nodeType == xml.Node.TEXT_NODE):
            self.childNodes[0].writexml(writer, '', '', '')
        else:
            writer.write(newl)
            for node in self.childNodes:
                node.writexml(writer, indent+addindent, addindent, newl)
            writer.write(indent)
        writer.write("</%s>%s" % (self.tagName, newl))
    else:
        writer.write("/>%s"%(newl))

xml.Element.writexml = fixed_writexml

class PMD:
    '''
        The main class containing all the elements of the PMD format
    '''
    def __init__(self):
        self.magic = ''
        self.version = None
        self.datasize = None
        self.dataleft = None
        self.numVertices = 0
        self.vertices = []
        self.numFaces = 0
        self.faces = []
        self.numBones = 0
        self.bones = []
        self.numPropPoints = 0
        self.propPoints = []

    def write(self):
        text = self.magic + '\n' + \
               str(self.version) + '\n' + \
               str(self.datasize) + '\n'
        for vert in self.vertices:
            text += ('{0}: '.format(self.vertices.index(vert)) + vert.write())
        for face in self.faces:
            text += ('{0}: '.format(self.faces.index(face)) + face.write())
        for bone in self.bones:
            text += ('{0}: '.format(self.bones.index(bone)) + bone.write())
        for pp in self.propPoints:
            text += ('{0}: '.format(self.propPoints.index(pp)) + pp.write())
        return text
    
    def writeXML(self):
        pass

class Vertex:
    '''
        Represents a single vertex in the PMD format.
    '''
    def __init__(self):
        self.position = Vector3D() #  inst. of Vector3D
        self.normal = Vector3D() # inst of Vector3D
        self.uv = Vector2D() # inst of Vector2D
        self.blends = VertexBlend() # inst. of VertexBlend

    def write(self):
        text = self.position.write() + '\n' + \
               self.normal.write() + '\n' + \
               '{0} {1}'.format(self.u,self.v) + '\n' + \
               self.blends.write() + '\n'
        return text               

class VertexBlend:
    '''
        Contains bone weighting for a given vertex
    '''
    def __init__(self,bones=[],weights=[]):
        self.bones = bones # 4B
        self.weights = weights #4f

    def write(self):
        return '{0} {1}'.format(str(self.bones),str(self.weights))

class Face:
    '''
        Stores the vertex ids that define a face
    '''
    def __init__(self,verts=[0,0,0]):
        self.vertices = verts # 3B

    def write(self):
        return '{0} {1} {2}'.format(self.vertices[0],self.vertices[1],self.vertices[2])

class Bone:
    '''
        Represents a single bone of a given skeleton
    '''
    def __init__(self,trans=[0.0,0.0,0.0],rot=[0.0,0.0,0.0,0.0]):
        self.translation = Vector3D(trans) # inst of Vector3D
        self.rotation = Quaternion(rot) # inst of Quaternion

    def write(self):
        text = self.translation.write() + '\n' + \
               self.rotation.write() + '\n'
        return text

class PropPoint:
    '''
        Represents an attachment point to a rigged mesh
    '''
    def __init__(self):
        self.nameLength = 0
        self.name = ''
        self.translation = Vector3D()
        self.rotation = Quaternion()
        self.bone = 0 # 1B

    def write(self):
        text = self.name + '\n' + \
               self.translation.write() + '\n' + \
               self.rotation.write() + '\n' + \
               str(self.bone) + '\n'
        return text


class PMDtoCollada:
    '''
        Converts a PMD model to the XML-based COLLADA format

        @filename: full path to source PMD file, also used for DAE output
        @skeletonsDoc: parsed skeletons.xml document (optional)
    '''
    def __init__(self,filename,skeletonsDoc=None):
        self.pmd = PMD()
        self.filename = os.path.split(filename)[1]
        self.pathname = os.path.split(filename)[0]
        self.filerootname = self.filename.split('.')[0]
        self.fullpath = filename
        self.skeletonsDoc = skeletonsDoc
        self.main()
       
    def extractPMDData(self,datafile):
        '''
            Extracts binary PMD data and builds an instance of class PMD
            
            @datafile: the binary data file, open for reading
        '''
        metadata = struct.unpack(header_format,
                                 datafile.read(header_length))
        self.pmd.magic = metadata[0]
        self.pmd.version = metadata[1]
        self.pmd.datasize = metadata[2]
        # Vertices
        self.pmd.numVertices = struct.unpack(vertex_header,
                                        datafile.read(vheader_length))[0]
        for i in range(self.pmd.numVertices):
            vert = Vertex()
            raw = struct.unpack(vertex_format,datafile.read(vertex_length))
            vert.position = Vector3D([round(x,6) for x in raw[0:3]])
            vert.normal = Vector3D([round(x,6) for x in raw[3:6]])
            vert.uv = Vector2D([round(x,6) for x in raw[6:8]])
            vert.blends = VertexBlend(raw[8:12],raw[12:16])
            # Convert game coordinates to right-handed Z_UP coordinates
            # (basically undo what gets done in PMDConverter.cpp)
            vert.position = Vector3D([vert.position.x, vert.position.z, vert.position.y])
            vert.normal = Vector3D([vert.normal.x, vert.normal.z, vert.normal.y])
            self.pmd.vertices.append(vert)

        # Faces
        self.pmd.numFaces = struct.unpack(face_header,datafile.read(fheader_length))[0]
        for i in range(self.pmd.numFaces):
            raw = struct.unpack(face_format,datafile.read(face_length))
            face = Face(raw[0:3])
            self.pmd.faces.append(face)

        # Bones
        self.pmd.numBones = struct.unpack(bone_header,datafile.read(bheader_length))[0]
        for i in range(self.pmd.numBones):
            raw = struct.unpack(bone_format,datafile.read(bone_length))
            bone = Bone(raw[0:3],raw[3:7])
            # Convert game coordinates to right-handed Z_UP coordinates
            # (basically undo what gets done in PMDConverter.cpp)
            bone.translation = Vector3D([bone.translation.x, bone.translation.z, bone.translation.y])
            bone.rotation = Quaternion([bone.rotation.x, bone.rotation.z, bone.rotation.y, -bone.rotation.w])
            self.pmd.bones.append(bone)

        # PropPoints - version 1 doesn't have them
        if self.pmd.version >= 2:
            self.pmd.numPropPoints = struct.unpack(numPropPoint_header,datafile.read(pheader_length))[0]
            for i in range(self.pmd.numPropPoints):
                propPoint = PropPoint()
                propPoint.nameLength = struct.unpack(propPointName_header,
                                              datafile.read(pName_header_length))[0]
                # Get name one char at a time
                for j in range(propPoint.nameLength):
                    propPoint.name += struct.unpack(propPointName_format,
                                            datafile.read(pName_length))[0]
                raw = struct.unpack(propPoint_format,datafile.read(propPoint_length))
                propPoint.translation = Vector3D(raw[0:3])
                propPoint.rotation = Quaternion(raw[3:7])
                propPoint.bone = raw[7]
                # Convert game coordinates to right-handed Z_UP coordinates
                # (basically undo what gets done in PMDConverter.cpp)
                propPoint.translation = Vector3D([propPoint.translation.x, propPoint.translation.z, propPoint.translation.y])
                propPoint.rotation = Quaternion([propPoint.rotation.x, propPoint.rotation.z, propPoint.rotation.y, -propPoint.rotation.w])
                self.pmd.propPoints.append(propPoint)
                #print 'Prop {0}, bone={1}'.format(propPoint.name, propPoint.bone)

        # Debug info
        print 'Valid PMDv{0}: Verts={1}, Faces={2}, Bones={3}, Props={4}'.format(self.pmd.version, self.pmd.numVertices, self.pmd.numFaces,
            self.pmd.numBones, self.pmd.numPropPoints)

    def outputCollada(self):
        '''
            Converts internal representation of the PMD to COLLADA document.
        '''
        
        # Generate useful bone data
        if self.pmd.numBones > 0:
            # First thing to do is check if we have skeletons.xml
            modelSkeleton = None
            if self.skeletonsDoc:
                # Now, prompt for the skeleton name, so we can search skeletons.xml and rebuild the bone hierarchy
                #   and offer the choice to list all skeletons with matching bone counts
                while True:
                    skeletonName = raw_input('Enter skeleton name (? for choices, or Enter for none): ')
                    if skeletonName == '?':
                        print 'Possible skeletons:'
                        skeletons = self.skeletonsDoc.getElementsByTagName('skeleton')
                        for skeleton in skeletons:
                            numSkeletonBones = len(skeleton.getElementsByTagName('bone'))
                            if numSkeletonBones == self.pmd.numBones:
                                print skeleton.getAttribute('title')
                    else:
                        # User typed something, fall out of the loop
                        break
                if skeletonName:
                    # Search in skeletons.xml for that skeleton - case insensitive for convenience
                    skeletons = self.skeletonsDoc.getElementsByTagName('skeleton')
                    for skeleton in skeletons:
                        if skeleton.getAttribute('title').lower() == skeletonName.lower():
                            bones = skeleton.getElementsByTagName('bone')
                            # Bone counts must match!
                            if len(bones) != self.pmd.numBones:
                                print 'num skeleton bones ({0}) != num model bones ({1})'.format(len(bones), self.pmd.numBones)
                            else:
                                modelSkeleton = skeleton
                            break
                    if not modelSkeleton:
                        print 'Error! Matching skeleton not found'

            # Bone and bind pose data:
            #   Inverse bind poses - flattened 4x4 transform matrices
            #   Bone names - matching skeletons.xml or numbered
            #   Bone parents - bone ID of each bone's parent, to reconstruct bone hierarchy
            #       or -1 if parent is not a bone
            #   Root node name - the root node's name (possibly a bone)
            #   Root node bone - the root node's bone ID or -1 if not a bone
            boneNames = []
            bindPoses = []
            invBindPoses = []
            boneParents = []
            for bone in self.pmd.bones:
                restPose = convertQuat2Matrix44(bone.rotation, bone.translation)
                bindPoses.append(restPose)
                invBindPoses.append(restPose.inverse())

            # Use this function to recursively find parent IDs
            def boneHierarchyHelper(node, parent, num):
                arr = []
                # Preorder traversal: self, child, sibling
                if node.nodeType == node.ELEMENT_NODE and node.nodeName == 'bone':
                    arr = [parent]
                for child in node.childNodes:
                    if child.nodeType == node.ELEMENT_NODE and child.nodeName == 'bone':
                        arr.extend(boneHierarchyHelper(child, num, num+len(arr)))
                return arr
                
            # Use this function to get value of a text node i.e. <tag>text</tag>
            def getNodeText(nodelist):
                rc = []
                for node in nodelist:
                    if node.nodeType == node.TEXT_NODE:
                        rc.append(node.data)
                return ''.join(rc)
                
            # We have to find which node is really the root, because only its children will be
            #   included in the inverse bind pose array, meaning either we create a new node
            #   with identity transform, or one of the existing bones becomes the root
            rootNodeName = None
            rootNodeBone = 0
            if modelSkeleton:
                bones = modelSkeleton.getElementsByTagName('bone')
                boneNames = [boneNode.getAttribute('name') for boneNode in bones]    
                # Find which node is root
                idElement = modelSkeleton.getElementsByTagName('identifier')[0]
                rootElement = idElement.getElementsByTagName('root')[0]
                rootNodeName = getNodeText(rootElement.childNodes)
                if boneNames.count(rootNodeName) == 1:
                    # One of the bones is the root
                    rootNodeBone = boneNames.index(rootNodeName)
                else:
                    # We need to create a new root node
                    rootNodeBone = -1
                boneParents = boneHierarchyHelper(bones[0], -1, 0)
                # Some bones might have no target, in which case they inherit the transform from
                # their parent, but we can't be exact about this as the original transform was lost
                for i, boneNode in enumerate(bones):
                    if not boneNode.getElementsByTagName('target') and boneParents[i] >= 0:
                        invBindPoses[i] = invBindPoses[boneParents[i]]
            else:
                boneNames = ['bone_{0}'.format(i) for i in range(self.pmd.numBones)]
                rootNodeName = boneNames[0]
                boneParents = [0]*self.pmd.numBones
                boneParents[0] = -1
                rootNodeBone = -1
                rootNodeName = 'RootNode'
            
            assert boneParents[0] == -1
            # Only add children of the root bone to the joints list,
            #   otherwise we won't match the expected bone count
            # TODO: Why do prop points sometimes end up here?
            jointNames = []
            jointPoses = []
            for i, bone in enumerate(self.pmd.bones):
                if i > rootNodeBone:
                    jointNames.append(boneNames[i])
                    jointPoses.append(invBindPoses[i])
            
            # Vertex weights - not necessarily equal to number of vertices
            # (a vertex may have 0 or more than 1 influence)
            #
            #   vertex weights - actual blend value 0.0 - 1.0
            #   number of bones influencing each vertex
            #   weight indices - preformatted string containing bone and weight array index
            weights = []
            boneCounts = []
            weightIndices = []
            for vert in self.pmd.vertices:
                count = 0
                for i in range(4):
                    if vert.blends.bones[i] != 255:
                        weightIndices.append('{0} {1}'.format(vert.blends.bones[i], len(weights)))
                        weights.append(vert.blends.weights[i])
                        count += 1
                boneCounts.append(count)
                
            # Versions <=2 store the vertices relative to the bind-pose:
            #   oldv = IBM * (BSM * v)
            # where IBM = inverse bind-pose matrix and BSM = bind-shape matrix.
            # So we need to apply the inverse of the IBM, or the bind pose,
            # to all vertices:
            #   newv => IBM^-1 * oldv => IBM^-1 * IBM * (BSM * v) => BSM * v
            # in order to have an acceptable COLLADA model.
            if (self.pmd.version <= 2):
                for i in range(self.pmd.numVertices):
                    # They only have one influence per vertex
                    vert = self.pmd.vertices[i]
                    if vert.blends.bones[0] != 255:
                        bindPose = bindPoses[vert.blends.bones[0]]
                        vert.position = transformVector(bindPose, vert.position)
                        vert.normal = rotateVector(bindPose, vert.normal)
                        self.pmd.vertices[i] = vert
                        
        ######################################################################
        # Here comes the messy XML writing
        fileName = "{0}\\{1}.dae".format(self.pathname,self.filerootname)
        try:
            daeout = open(fileName,'w')
        except Exception as error:
            print error.msg
            print 'Output file failed to open, exiting...'
            sys.exit(0)
        now = datetime.datetime.now()
        doc = xml.Document()
        collada = easyxml.createElement(doc,'COLLADA',{'version':'1.4.1','xmlns':'http://www.collada.org/2005/11/COLLADASchema'})
        doc.appendChild(collada)
        asset = easyxml.createElement(doc,'asset',parent=collada)
        contributor = easyxml.createElement(doc,'contributor',parent=asset)
        author = easyxml.createElement(doc,'author',{},
            'PMD to COLLADA Converter',
            parent=contributor)
        authoringTool = easyxml.createElement(doc,'authoring_tool',{},
            'pmd2collada.py, v3.0',
            parent=contributor)
        #comments = easyxml.createElement(doc,'comments',parent=contributor)
        #cp = easyxml.createElement(doc,'copyright',parent=contributor)
        created = easyxml.createElement(doc,'created',text=now.strftime('%Y-%m-%dT%H:%M:%S'),
            parent=asset)
        modified = easyxml.createElement(doc,'modified',text=now.strftime('%Y-%m-%dT%H:%M:%S'),
            parent=asset)
        unit = easyxml.createElement(doc,'unit',{'meter':'0.01','name':'centimeter'},parent=asset)
        upAxis = easyxml.createElement(doc,'up_axis',text='Z_UP',parent=asset)
        libGeo = easyxml.createElement(doc,'library_geometries',parent=collada)
        geoID = '{0}-Geometry'.format(self.filerootname)
        geo = easyxml.createElement(doc,'geometry',{'id':geoID,'name':'{0}-Geometry'.format(self.filerootname)},parent=libGeo)
        mesh = easyxml.createElement(doc,'mesh',parent=geo)
        posSource = easyxml.createElement(doc,'source',{'id':'{0}-Geometry-Position'.format(self.filerootname)},parent=mesh)
        posFloatArray = easyxml.createElement(doc,'float_array',{'count':'{0}'.format(self.pmd.numVertices*3),
            'id':'{0}-Geometry-Position-array'.format(self.filerootname)},
            ' '.join([vert.position.write() for vert in self.pmd.vertices]),parent=posSource)
        posTechCommon = easyxml.createElement(doc,'technique_common',parent=posSource)
        posTechAccessor = easyxml.createElement(doc,'accessor',{'count':'{0}'.format(self.pmd.numVertices),
            'source':'{0}-Geometry-Position-array'.format(self.filerootname),'stride':'3'},parent=posTechCommon)
        posParamX = easyxml.createElement(doc,'param',{'type':'float','name':'X'},parent=posTechAccessor)
        posParamY = easyxml.createElement(doc,'param',{'type':'float','name':'Y'},parent=posTechAccessor)
        posParamZ = easyxml.createElement(doc,'param',{'type':'float','name':'Z'},parent=posTechAccessor)
        normSource = easyxml.createElement(doc,'source',{'id':'{0}-Geometry-Normals'.format(self.filerootname)},parent=mesh)
        normFloatArray = easyxml.createElement(doc,'float_array',{'count':'{0}'.format(self.pmd.numVertices*3),
            'id':'{0}-Geometry-Normal-array'.format(self.filerootname)},
            ' '.join([vert.normal.write() for vert in self.pmd.vertices]),parent=normSource)
        normTechCommon = easyxml.createElement(doc,'technique_common',parent=normSource)
        normTechAccessor = easyxml.createElement(doc,'accessor',{'count':'{0}'.format(self.pmd.numVertices),
            'source':'{0}-Geometry-Normal-array'.format(self.filerootname),'stride':'3'},parent=normTechCommon)
        normParamX = easyxml.createElement(doc,'param',{'type':'float','name':'X'},parent=normTechAccessor)
        normParamY = easyxml.createElement(doc,'param',{'type':'float','name':'Y'},parent=normTechAccessor)
        normParamZ = easyxml.createElement(doc,'param',{'type':'float','name':'Z'},parent=normTechAccessor)
        uvSource = easyxml.createElement(doc,'source',{'id':'{0}-Geometry-UV'.format(self.filerootname)},parent=mesh)
        uvFloatArray = easyxml.createElement(doc,'float_array',{'count':'{0}'.format(self.pmd.numVertices*2),
            'id':'{0}-Geometry-UV-array'.format(self.filerootname)},
            ' '.join([vert.uv.write() for vert in self.pmd.vertices]),parent=uvSource)
        uvTechCommon = easyxml.createElement(doc,'technique_common',parent=uvSource)
        uvTechAccessor = easyxml.createElement(doc,'accessor',{'count':'{0}'.format(self.pmd.numVertices),
            'source':'{0}-Geometry-UV-array'.format(self.filerootname),'stride':'2'},parent=uvTechCommon)
        uvParamX = easyxml.createElement(doc,'param',{'type':'float','name':'U'},parent=uvTechAccessor)
        uvParamY = easyxml.createElement(doc,'param',{'type':'float','name':'V'},parent=uvTechAccessor)
        vertices = easyxml.createElement(doc,'vertices',{'id':'{0}-Geometry-Vertex'.format(self.filerootname)},parent=mesh)
        vertInput = easyxml.createElement(doc,'input',{'semantic':'POSITION',
            'source':'#{0}-Geometry-Position'.format(self.filerootname)},parent=vertices)
        triangles = easyxml.createElement(doc,'triangles',
            {'material':'material0','count':'{0}'.format(self.pmd.numFaces)},parent=mesh)
        triInput1 = easyxml.createElement(doc,'input',{'offset':'0',
            'semantic':'VERTEX','source':'#{0}-Geometry-Vertex'.format(self.filerootname)},parent=triangles)
        triInput2 = easyxml.createElement(doc,'input',{'offset':'0',
            'semantic':'NORMAL','source':'#{0}-Geometry-Normals'.format(self.filerootname)},parent=triangles)
        triInput3 = easyxml.createElement(doc,'input',{'offset':'0',
            'semantic':'TEXCOORD','source':'#{0}-Geometry-UV'.format(self.filerootname)},parent=triangles)
        faceArray = easyxml.createElement(doc,'p',text=' '.join([face.write() for face in self.pmd.faces]),parent=triangles)
        
        ######################################################################
        # If we have bones, we need a skin and controller
        if self.pmd.numBones > 0:
            libControllers = easyxml.createElement(doc,'library_controllers',parent=collada)
            controllerName = format(self.filerootname);
            controller = easyxml.createElement(doc,'controller',{'id':'{0}-skin'.format(controllerName),
                'name':'{0}'.format(controllerName)},parent=libControllers)
            skin = easyxml.createElement(doc,'skin',{'source':'#{0}'.format(geoID)},parent=controller)
            bsm = easyxml.createElement(doc,'bind_shape_matrix',text='1 0 0 0 0 1 0 0 0 0 1 0 0 0 0 1',parent=skin)
            jointSource = easyxml.createElement(doc,'source',{'id':'{0}-skin-joints'.format(controllerName)},parent=skin)
            jointNameArray = easyxml.createElement(doc,'Name_array',{'id':'{0}-skin-joints-array'.format(controllerName),
                'count':'{0}'.format(len(jointNames))},' '.join(jointNames),parent=jointSource)
            jointTechCommon = easyxml.createElement(doc,'technique_common',parent=jointSource)
            jointTechAccessor = easyxml.createElement(doc,'accessor',{'source':'#{0}-skin-joints-array'.format(controllerName),
                'count':'{0}'.format(len(jointNames)),'stride':'1'},parent=jointTechCommon)
            jointParam = easyxml.createElement(doc,'param',{'name':'JOINT','type':'name'},parent=jointTechAccessor)
            invBindPoseSource = easyxml.createElement(doc,'source',{'id':'{0}-skin-bind_poses'.format(controllerName)},parent=skin)
            bindPoseArray = easyxml.createElement(doc,'float_array',{'id':'{0}-skin-bind_poses-array'.format(controllerName),
                'count':'{0}'.format(len(jointNames) * 16)},' '.join([pose.write() for pose in jointPoses]),parent=invBindPoseSource)
            bindPoseTechCommon = easyxml.createElement(doc,'technique_common',parent=invBindPoseSource)
            bindPoseAccessor = easyxml.createElement(doc,'accessor',{'source':'#{0}-skin-bind_poses-array'.format(controllerName),
                'count':'{0}'.format(len(jointNames)),'stride':'16'},parent=bindPoseTechCommon)
            bindPoseParam = easyxml.createElement(doc,'param',{'name':'TRANSFORM','type':'float4x4'},parent=bindPoseAccessor)
            weightSource = easyxml.createElement(doc,'source',{'id':'{0}-skin-weights'.format(controllerName)},parent=skin)
            weightArray = easyxml.createElement(doc,'float_array',{'id':'{0}-skin-weights-array'.format(controllerName),
                'count':'{0}'.format(len(weights))},' '.join(['{0}'.format(weight) for weight in weights]),parent=weightSource)
            weightTechCommon = easyxml.createElement(doc,'technique_common',parent=weightSource)
            weightAccessor = easyxml.createElement(doc,'accessor',{'source':'#{0}-skin-weights-array'.format(controllerName),
                'count':'{0}'.format(len(weights)),'stride':'1'},parent=weightTechCommon)
            weightParam = easyxml.createElement(doc,'param',{'name':'WEIGHT','type':'float'},parent=weightAccessor)
            joints = easyxml.createElement(doc,'joints',parent=skin)
            jointsInputJoint = easyxml.createElement(doc,'input',{'semantic':'JOINT',
                'source':'#{0}-skin-joints'.format(controllerName)},parent=joints)
            jointsInputIBM = easyxml.createElement(doc,'input',{'semantic':'INV_BIND_MATRIX',
                'source':'#{0}-skin-bind_poses'.format(controllerName)},parent=joints)
            vertexWeight = easyxml.createElement(doc,'vertex_weights',{'count':'{0}'.format(self.pmd.numVertices)},parent=skin)
            vertexWeightInputJoint = easyxml.createElement(doc,'input',{'semantic':'JOINT',
                'source':'#{0}-skin-joints'.format(controllerName),'offset':'0'},parent=vertexWeight)
            vertexWeightInputWeight = easyxml.createElement(doc,'input',{'semantic':'WEIGHT',
                'source':'{0}-skin-weights'.format(controllerName),'offset':'1'},parent=vertexWeight)
            # vcount - number of bones associated with an influence in vertex_weights
            vcount = easyxml.createElement(doc,'vcount',text=' '.join(['{0}'.format(count) for count in boneCounts]),parent=vertexWeight)
            # v - list of indices that describe which bones and weights are associated with each vertex
            v = easyxml.createElement(doc,'v',text=' '.join(weightIndices),parent=vertexWeight)
            libVisScenes = easyxml.createElement(doc,'library_visual_scenes',parent=collada)
            visualScene = easyxml.createElement(doc,'visual_scene',{'id':'Scene','name':'Scene'},parent=libVisScenes)
            # Bone node hierarchy
            if rootNodeBone == -1:
                # Create a new root node to be parent of all the bones
                rootNode = easyxml.createElement(doc,'node',{'id':'{0}'.format(rootNodeName),
                    'name':'{0}'.format(rootNodeName),'type':'NODE'},parent=visualScene)
            else:
                # A bone is the root node
                rootNode = visualScene
            boneNodes = []
            for i, bone in enumerate(self.pmd.bones):
                # can't refer to node that hasn't been created yet! (should never happen)
                assert boneParents[i] < len(boneNodes)
                if boneParents[i] == -1:
                    # Root node is parent
                    parentNode = rootNode
                else:
                    # Some other bone is parent
                    parentNode = boneNodes[boneParents[i]]
                # What type of node are we?
                if i <= rootNodeBone:
                    nodeType = 'NODE'
                else:
                    nodeType = 'JOINT'
                boneNode = easyxml.createElement(doc,'node',{'id':'{0}'.format(boneNames[i]),
                    'name':'{0}'.format(boneNames[i]),'sid':'{0}'.format(boneNames[i]),'type':nodeType},parent=parentNode)
                # Convert prop point to 4x4 affine transform matrix
                #   This transforms isn't used by the engine, but maybe some editor
                #   like Blender could use it, so output it anyway
                boneTransform = easyxml.createElement(doc,'matrix',{'sid':'transform'},invBindPoses[i].write(),parent=boneNode)
                boneNodes.append(boneNode)
                
            # Prop point bones
            for prop in self.pmd.propPoints:
                # Skip default prop point, it gets automatically added during conversion
                if prop.name == 'root' or prop.bone == 255:
                    continue
                # Place prop point as child of the correct bone, so it inherits the parent transforms
                propParent = boneNodes[prop.bone]
                # Prepend PROP_PREFIX to the internal prop point name
                propName = PROP_PREFIX+prop.name
                propNode = easyxml.createElement(doc,'node',{'id':propName,'name':propName,'sid':propName,'type':'JOINT'},parent=propParent)
                # Convert prop point to 4x4 affine transform matrix
                #   The engine only uses the local transform of this node when converting,
                #   so we don't have to worry about its parent transform(s)
                mat = convertQuat2Matrix44(prop.rotation, prop.translation)
                propTransform = easyxml.createElement(doc,'matrix',{'sid':'transform'},mat.write(),parent=propNode)
            # This tells the controller where to start searching for joints
            #   (if it's in the wrong place, you'll get "Unknown joint" errors)
            skeletonRoot = boneNames[rootNodeBone+1]
            # Instantiate controller
            #   (uses local coordinate system of it's parent node)
            controller = easyxml.createElement(doc,'node',{'id':'Controller','type':'NODE'},parent=visualScene)
            instController = easyxml.createElement(doc,'instance_controller',{'url':'#{0}-skin'.format(controllerName)},parent=controller)
            instSkeleton = easyxml.createElement(doc,'skeleton',text='#{0}'.format(skeletonRoot),parent=instController)
            # Omit broken bind_materials
        ######################################################################
        # Static model nodes
        else:
            libVisScenes = easyxml.createElement(doc,'library_visual_scenes',parent=collada)
            visualScene = easyxml.createElement(doc,'visual_scene',{'id':'Scene','name':'Scene'},parent=libVisScenes)
            node = easyxml.createElement(doc,'node',{'layer':'L1','id':'{0}'.format(self.filerootname),
                'name':'{0}'.format(self.filerootname)},parent=visualScene)
            instGeo = easyxml.createElement(doc,'instance_geometry',
                {'url':'#{0}-Geometry'.format(self.filerootname)},parent=node)
            # Prop point nodes
            for prop in self.pmd.propPoints:
                # Skip default prop point, it gets automatically added during conversion
                if prop.name == 'root':
                    continue
                # Prepend PROP_PREFIX to the internal prop point name
                propName = PROP_PREFIX+prop.name
                propNode = easyxml.createElement(doc,'node',{'id':propName,'name':propName},parent=node)
                # Convert prop point to 4x4 affine transform matrix
                mat = convertQuat2Matrix44(prop.rotation, prop.translation)
                propTransform = easyxml.createElement(doc,'matrix',{'sid':'transform'},mat.write(),parent=propNode)
        ######################################################################
        scene = easyxml.createElement(doc,'scene',parent=collada)
        instVisScene = easyxml.createElement(doc,'instance_visual_scene',{'url':'#Scene'},parent=scene)
        # Don't use toprettyxml() with default args, we should always specify encoding,
        #   and also use two spaces instead of tabs for indenting.
        # Also see above comments about why Python's XML pretty print is broken.
        daeout.write(doc.toprettyxml('  ','\n','utf-8'))
        daeout.close()

    def main(self):
        try:
            datafile = open('{0}\\{1}'.format(self.pathname,self.filename),'rb')
        except Exception as error:
            print error
            print 'Input file failed to open, exiting...'
            sys.exit(0)
        else:
            print 'Starting data extraction on {0}.pmd...'.format(self.filerootname)
            self.extractPMDData(datafile)
            datafile.close()
            print 'Data extracted, converting and writing {0}.dae...'.format(self.filerootname)
            self.outputCollada()
            print 'Complete!'
