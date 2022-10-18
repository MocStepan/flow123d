import flowpy

class PyFieldAnisotropy(flowpy.PythonFieldBase):
    pass

    def used_fields(self):
        field_list = ["X"]
        return field_list
    
    def __call__(self):
        z = self.X[2]
        diag = np.where( z < 0.25, 0.5+2*z, 1)
        return self.repl(np.eye(3)) * diag


class PyFieldWaterSourceDensity(flowpy.PythonFieldBase):
    pass

    def used_fields(self):
        field_list = ["X"]
        return field_list
    
    def __call__(self):
        XY = self.X[0:2]
        AB = 2*(1-XY**2)
        return AB[0] + AB[1]   # or np.sum(AB, axis=0)
