import numpy as np
from CustomDialog import *

class AdjustLinesDialog(CustomDialog):
    def __init__(self, parent, title, margin=0.5, resolution=0):
        self.__parent = parent
        self.__title = title
        self.__components = []
        component = {}
        component["type"] = "Entry"
        component["text"] = "Margin around lines"
        component["text-type"] = "float" # float, int or str
        component["default"] = margin
        component["minvalue"] = 1.0e-10
        component["maxvalue"] = np.inf
        self.__components.append(component)
        component = {}
        component["type"] = "Entry"
        component["text"] = "Resolution"
        component["text-type"] = "int" # float, int or str
        component["default"] = resolution
        component["minvalue"] = 0.0
        component["maxvalue"] = np.inf
        self.__components.append(component)
        component = {}
        component["type"] = "Checkbutton"
        component["text"] = "Check derivatives before fitting"
        component["default"] = False
        self.__components.append(component)

    def show(self):
        self.results = None
        CustomDialog.__init__(self, self.__parent, self.__title, self.__components)


