## vector_defs
import math

class Vector2D:
    '''
        A struct of 2 floats, with methods for outputting raw text
    '''
    def __init__(self,vals=[0.0,0.0]):
        self.x = vals[0]
        self.y = vals[1]

    def write(self):
        return '{0} {1}'.format(self.x,self.y)

class Vector3D:
    '''
        A struct of 3 floats, with methods for outputting raw text
    '''
    def __init__(self,vals=[0.0,0.0,0.0]):
        self.x = vals[0]
        self.y = vals[1]
        self.z = vals[2]

    def write(self):
        return '{0:.7f} {1:.7f} {2:.7f}'.format(self.x,self.y,self.z)

class Quaternion:
    '''
        A struct of 4 floats, with methods for outputting raw text
    '''
    def __init__(self,vals=[0.0,0.0,0.0,0.0]):
        self.x = vals[0]
        self.y = vals[1]
        self.z = vals[2]
        self.w = vals[3]

    def write(self):
        return '{0:.7f} {1:.7f} {2:.7f} {3:.7f}'.format(self.x,self.y,self.z,self.w)

class Matrix44:
    '''
        A struct of a 16 floats, representing 4x4 matrix
        With some useful utility methods for outputting flat array and getting inverse matrix
    '''
    def __init__(self,vals=[0.0]*16):
        self.m = vals

    def write(self):
        return ' '.join(['{0:.7f}'.format(f) for f in self.m])

    def inverse(self):
        inv = [0.0]*16
        inv[0] =   self.m[5]*self.m[10]*self.m[15] - self.m[5]*self.m[11]*self.m[14] - self.m[9]*self.m[6]*self.m[15] \
                    + self.m[9]*self.m[7]*self.m[14] + self.m[13]*self.m[6]*self.m[11] - self.m[13]*self.m[7]*self.m[10]
        inv[4] =  -self.m[4]*self.m[10]*self.m[15] + self.m[4]*self.m[11]*self.m[14] + self.m[8]*self.m[6]*self.m[15] \
                    - self.m[8]*self.m[7]*self.m[14] - self.m[12]*self.m[6]*self.m[11] + self.m[12]*self.m[7]*self.m[10]
        inv[8] =   self.m[4]*self.m[9]*self.m[15] - self.m[4]*self.m[11]*self.m[13] - self.m[8]*self.m[5]*self.m[15] \
                    + self.m[8]*self.m[7]*self.m[13] + self.m[12]*self.m[5]*self.m[11] - self.m[12]*self.m[7]*self.m[9]
        inv[12] = -self.m[4]*self.m[9]*self.m[14] + self.m[4]*self.m[10]*self.m[13] + self.m[8]*self.m[5]*self.m[14] \
                    - self.m[8]*self.m[6]*self.m[13] - self.m[12]*self.m[5]*self.m[10] + self.m[12]*self.m[6]*self.m[9]
        inv[1] =  -self.m[1]*self.m[10]*self.m[15] + self.m[1]*self.m[11]*self.m[14] + self.m[9]*self.m[2]*self.m[15] \
                    - self.m[9]*self.m[3]*self.m[14] - self.m[13]*self.m[2]*self.m[11] + self.m[13]*self.m[3]*self.m[10]
        inv[5] =   self.m[0]*self.m[10]*self.m[15] - self.m[0]*self.m[11]*self.m[14] - self.m[8]*self.m[2]*self.m[15] \
                    + self.m[8]*self.m[3]*self.m[14] + self.m[12]*self.m[2]*self.m[11] - self.m[12]*self.m[3]*self.m[10]
        inv[9] =  -self.m[0]*self.m[9]*self.m[15] + self.m[0]*self.m[11]*self.m[13] + self.m[8]*self.m[1]*self.m[15] \
                    - self.m[8]*self.m[3]*self.m[13] - self.m[12]*self.m[1]*self.m[11] + self.m[12]*self.m[3]*self.m[9]
        inv[13] =  self.m[0]*self.m[9]*self.m[14] - self.m[0]*self.m[10]*self.m[13] - self.m[8]*self.m[1]*self.m[14] \
                    + self.m[8]*self.m[2]*self.m[13] + self.m[12]*self.m[1]*self.m[10] - self.m[12]*self.m[2]*self.m[9]
        inv[2] =   self.m[1]*self.m[6]*self.m[15] - self.m[1]*self.m[7]*self.m[14] - self.m[5]*self.m[2]*self.m[15] \
                    + self.m[5]*self.m[3]*self.m[14] + self.m[13]*self.m[2]*self.m[7] - self.m[13]*self.m[3]*self.m[6]
        inv[6] =  -self.m[0]*self.m[6]*self.m[15] + self.m[0]*self.m[7]*self.m[14] + self.m[4]*self.m[2]*self.m[15] \
                    - self.m[4]*self.m[3]*self.m[14] - self.m[12]*self.m[2]*self.m[7] + self.m[12]*self.m[3]*self.m[6]
        inv[10] =  self.m[0]*self.m[5]*self.m[15] - self.m[0]*self.m[7]*self.m[13] - self.m[4]*self.m[1]*self.m[15] \
                    + self.m[4]*self.m[3]*self.m[13] + self.m[12]*self.m[1]*self.m[7] - self.m[12]*self.m[3]*self.m[5]
        inv[14] = -self.m[0]*self.m[5]*self.m[14] + self.m[0]*self.m[6]*self.m[13] + self.m[4]*self.m[1]*self.m[14] \
                    - self.m[4]*self.m[2]*self.m[13] - self.m[12]*self.m[1]*self.m[6] + self.m[12]*self.m[2]*self.m[5]
        inv[3] =  -self.m[1]*self.m[6]*self.m[11] + self.m[1]*self.m[7]*self.m[10] + self.m[5]*self.m[2]*self.m[11] \
                    - self.m[5]*self.m[3]*self.m[10] - self.m[9]*self.m[2]*self.m[7] + self.m[9]*self.m[3]*self.m[6]
        inv[7] =   self.m[0]*self.m[6]*self.m[11] - self.m[0]*self.m[7]*self.m[10] - self.m[4]*self.m[2]*self.m[11] \
                    + self.m[4]*self.m[3]*self.m[10] + self.m[8]*self.m[2]*self.m[7] - self.m[8]*self.m[3]*self.m[6]
        inv[11] = -self.m[0]*self.m[5]*self.m[11] + self.m[0]*self.m[7]*self.m[9] + self.m[4]*self.m[1]*self.m[11] \
                    - self.m[4]*self.m[3]*self.m[9] - self.m[8]*self.m[1]*self.m[7] + self.m[8]*self.m[3]*self.m[5]
        inv[15] =  self.m[0]*self.m[5]*self.m[10] - self.m[0]*self.m[6]*self.m[9] - self.m[4]*self.m[1]*self.m[10] \
                    + self.m[4]*self.m[2]*self.m[9] + self.m[8]*self.m[1]*self.m[6] - self.m[8]*self.m[2]*self.m[5]
        det = self.m[0]*inv[0] + self.m[1]*inv[4] + self.m[2]*inv[8] + self.m[3]*inv[12]
        if (det == 0):
            return 0
        det = 1.0 / det
        for i in range(16):
            inv[i] = inv[i] * det
        return Matrix44(inv)
        
def matrixMult(mat1,mat2):
    '''
        Method to multiply two matrices together
        Returns a Matrix44.
    '''
    result = [0.0]*16
    result[0] = mat1.m[0]*mat2.m[0] + mat1.m[1]*mat2.m[4] + mat1.m[2]*mat2.m[8] + mat1.m[3]*mat2.m[12]
    result[1] = mat1.m[0]*mat2.m[1] + mat1.m[1]*mat2.m[5] + mat1.m[2]*mat2.m[9] + mat1.m[3]*mat2.m[13]
    result[2] = mat1.m[0]*mat2.m[2] + mat1.m[1]*mat2.m[6] + mat1.m[2]*mat2.m[10] + mat1.m[3]*mat2.m[14]
    result[3] = mat1.m[0]*mat2.m[3] + mat1.m[1]*mat2.m[7] + mat1.m[2]*mat2.m[11] + mat1.m[3]*mat2.m[15]
    result[4] = mat1.m[4]*mat2.m[0] + mat1.m[5]*mat2.m[4] + mat1.m[6]*mat2.m[8] + mat1.m[7]*mat2.m[12]
    result[5] = mat1.m[4]*mat2.m[1] + mat1.m[5]*mat2.m[5] + mat1.m[6]*mat2.m[9] + mat1.m[7]*mat2.m[13]
    result[6] = mat1.m[4]*mat2.m[2] + mat1.m[5]*mat2.m[6] + mat1.m[6]*mat2.m[10] + mat1.m[7]*mat2.m[14]
    result[7] = mat1.m[4]*mat2.m[3] + mat1.m[5]*mat2.m[7] + mat1.m[6]*mat2.m[11] + mat1.m[7]*mat2.m[15]
    result[8] = mat1.m[8]*mat2.m[0] + mat1.m[9]*mat2.m[4] + mat1.m[10]*mat2.m[8] + mat1.m[11]*mat2.m[12]
    result[9] = mat1.m[8]*mat2.m[1] + mat1.m[9]*mat2.m[5] + mat1.m[10]*mat2.m[9] + mat1.m[11]*mat2.m[13]
    result[10] = mat1.m[8]*mat2.m[2] + mat1.m[9]*mat2.m[6] + mat1.m[10]*mat2.m[10] + mat1.m[11]*mat2.m[14]
    result[11] = mat1.m[8]*mat2.m[3] + mat1.m[9]*mat2.m[7] + mat1.m[10]*mat2.m[11] + mat1.m[11]*mat2.m[15]
    result[12] = mat1.m[12]*mat2.m[0] + mat1.m[13]*mat2.m[4] + mat1.m[14]*mat2.m[8] + mat1.m[15]*mat2.m[12]
    result[13] = mat1.m[12]*mat2.m[1] + mat1.m[13]*mat2.m[5] + mat1.m[14]*mat2.m[9] + mat1.m[15]*mat2.m[13]
    result[14] = mat1.m[12]*mat2.m[2] + mat1.m[13]*mat2.m[6] + mat1.m[14]*mat2.m[10] + mat1.m[15]*mat2.m[14]
    result[15] = mat1.m[12]*mat2.m[3] + mat1.m[13]*mat2.m[7] + mat1.m[14]*mat2.m[11] + mat1.m[15]*mat2.m[15]
    return Matrix44(result)
    
def transformVector(mat, vec):
    '''
        Method to rotate and translate a Vector3D by the given Matrix44 transformation
        Returns a Vector3D.
    '''
    result = Vector3D()
    result.x = mat.m[0]*vec.x + mat.m[1]*vec.y + mat.m[2]*vec.z + mat.m[3]
    result.y = mat.m[4]*vec.x + mat.m[5]*vec.y + mat.m[6]*vec.z + mat.m[7]
    result.z = mat.m[8]*vec.x + mat.m[9]*vec.y + mat.m[10]*vec.z + mat.m[11]
    return result
    
def rotateVector(mat, vec):
    '''
        Method to rotate a Vector3D by the given Matrix44 transformation
        Returns a Vector3D.
    '''
    result = Vector3D()
    result.x = mat.m[0]*vec.x + mat.m[1]*vec.y + mat.m[2]*vec.z
    result.y = mat.m[4]*vec.x + mat.m[5]*vec.y + mat.m[6]*vec.z
    result.z = mat.m[8]*vec.x + mat.m[9]*vec.y + mat.m[10]*vec.z
    return result

def convertQuat2Matrix44(quat=Quaternion([0.0,0.0,0.0,1.0]), tran=Vector3D([0.0,0.0,0.0])):
    '''
        Method to convert a Quaternion rotation + Vector3D translation into a 4x4 affine matrix
        Returns a Matrix44.
    '''
    mat = [0.0]*16
    mat[0] = 1.0 - 2.0*(quat.y*quat.y + quat.z*quat.z)
    mat[1] = 2.0*(quat.x*quat.y - quat.z*quat.w)
    mat[2] = 2.0*(quat.x*quat.z + quat.y*quat.w)
    mat[3] = tran.x
    mat[4] = 2.0*(quat.x*quat.y + quat.z*quat.w)
    mat[5] = 1.0 - 2.0*(quat.x*quat.x + quat.z*quat.z)
    mat[6] = 2.0*(quat.y*quat.z - quat.x*quat.w)
    mat[7] = tran.y
    mat[8] = 2.0*(quat.x*quat.z - quat.y*quat.w)
    mat[9] = 2.0*(quat.y*quat.z + quat.x*quat.w)
    mat[10] = 1.0 - 2.0*(quat.x*quat.x + quat.y*quat.y)
    mat[11] = tran.z
    mat[12] = 0.0
    mat[13] = 0.0
    mat[14] = 0.0
    mat[15] = 1.0
    return Matrix44(mat)

