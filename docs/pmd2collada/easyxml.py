## my xml stuff, to be named later

def createElement(document,element_name,attributes={},text='',parent=None):
    '''
        Wrapper method for xml.dom.minidom.Document.createElement().
        Accepts various parameters to add attributes, text, and append to a parent node
        
        input:
        <xml.dom.minidom.Document> document - an xml document
        <string> element - the element name
        <dict> attributes - a mapping of attribute names and their values
        <string> text - text data attached to the element
        <xml.dom.minidom.Element> parent - the parent node of the created element
        output:
        <xml.dom.minidom.Element> element - the xml element
    '''
    element = document.createElement(element_name)
    if attributes:
        for k,v in attributes.iteritems():
            element.setAttribute(k,v)
    if text:
        textNode = document.createTextNode(text)
        element.appendChild(textNode)
    if parent:
        parent.appendChild(element)
    return element
