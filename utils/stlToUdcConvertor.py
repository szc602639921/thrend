import meshio
import numpy
import sys

DEFAULT_TEMPERATURE = 298.15

if len(sys.argv) != 3:
    print("Usage: python stlToUdcConvertor.py <inputStlFile> <outputUdcFile>")
    sys.exit(1) # Exit the script if the arguments are incorrect

inputStlFile = sys.argv[1]
outputUdcFile = sys.argv[2]

mesh = meshio.read(inputStlFile, file_format="stl")

numPoints = len(mesh.points)
verticesTemperature = numpy.full(numPoints, DEFAULT_TEMPERATURE, dtype=numpy.float64)
mesh.point_data = {'T': verticesTemperature}

mesh.write(outputUdcFile, file_format="avsucd")
