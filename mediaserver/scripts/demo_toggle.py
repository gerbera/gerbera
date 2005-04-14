#!/usr/bin/env python
import sys
from xml.dom.minidom import *

class Item:
    def __init__(self, input):
        self.xml = parseString(input)

    def __getNodeData(self, data):
        try:
            title = self.xml.getElementsByTagName(data)[0]
            if (len(title.childNodes) > 0):
                if (title.childNodes[0].nodeType == title.childNodes[0].TEXT_NODE):
                    return title.childNodes[0].data

            return ''
        except:
            return ''
            
    def __getNode(self, data):
        try:
            title = self.xml.getElementsByTagName(data)[0]
            if (len(title.childNodes) > 0):
                if (title.childNodes[0].nodeType == title.childNodes[0].TEXT_NODE):
                    return title.childNodes[0]
            else:
                text = self.xml.createTextNode("")
                title.appendChild(text)
                return title.childNodes[0]
                                
        except:
            return None


    def getTitle(self):
        return self.__getNodeData("dc:title")

    def setTitle(self, data):
        node = self.__getNode("dc:title")
        node.data = data 

    def getClass(self):
        return self.__getNodeData("upnp:class")

    def setClass(self, data):
        node = self.__getNode("upnp:class")
        node.data = data 

    def getAction(self):
        return self.__getNodeData("action")

    def setAction(self, data):
        node = self.__getNode("action")
        node.data = data 

    def getState(self):
        return self.__getNodeData("state")

    def setState(self, data):
        node = self.__getNode("state")
        node.data = data 

    def getLocation(self):
        return self.__getNodeData("location")

    def setLocation(self, data):
        node = self.__getNode("location")
        node.data = data 

    def getMimeType(self):
        return self.__getNodeData("mime-type")

    def setMimeType(self, data):
        node = self.__getNode("mime-type")
        node.data = data 

    def render(self):
        return self.xml.toxml()

    def getDescription(self):
        return self.__getNodeData("dc:description")
        
    def setDescription(self, data):
        node = self.__getNode("dc:description")
        node.data = data 

item = Item(sys.stdin.read())

if (item.getState() == "1"):
    item.setState("0")
    item.setTitle("Turn ON")
    item.setDescription("Your toggle item is turned OFF, press PLAY to turn it ON.")
    item.setLocation("/tmp/path/to/picture_1.jpeg")
    
else:    
    item.setState("1")
    item.setTitle("Turn OFF")
    item.setDescription("Your toggle item is turned ON, press PLAY to turn it OFF.")
    item.setLocation("/tmp/path/to/picture_2.jpeg")
    # here you could run any program you want.. just make sure that it stays
    # silent on stdout
    # os.system("/run/something/useful")

sys.stdout.write(item.render())

