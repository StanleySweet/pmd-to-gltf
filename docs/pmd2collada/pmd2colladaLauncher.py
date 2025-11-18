import pmd2collada, os, sys
import xml.dom.minidom as xml

toplevel = raw_input('Enter the top-level path to find and convert PMD files: ')
#toplevel = 'C:\\Users\\Ben\\devel\\ps\\binaries\\data\\mods\\'
skeleton = raw_input('Enter path to skeletons.xml (or Enter for none): ')
#skeleton = 'C:\\Users\\Ben\\devel\\ps\\binaries\\data\\mods\\public\\art\\skeletons'

if not toplevel or not os.path.exists(toplevel):
    print 'Valid top-level path required'
else:
    skeletonsDoc = None
    if skeleton:
        # Parse skeletons.xml once, then pass it to each PMDtoCollada instance
        skeletonsPath = os.path.join(skeleton, 'skeletons.xml')
        if not os.path.exists(skeleton) or not os.path.exists(skeletonsPath):
            print 'Invalid skeletons.xml path -- skipping'
        else:
            skeletonsDoc = xml.parse(skeletonsPath)

    pmdfiles = []
    for root, dirs, files in os.walk(toplevel):
        for f in files:
            if f.split('.')[1] == 'pmd':
                pmdfiles.append(os.path.join(root,f))
    for pmd in pmdfiles:
        convert = pmd2collada.PMDtoCollada(pmd, skeletonsDoc)

