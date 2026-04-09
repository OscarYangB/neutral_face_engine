# open3d not compatible with python 3.14 which is stupid and dumb
#
# $ py install 3.12
# $ py -3.12 -m pip install open3d
# $ py -3.12 geometry_preprocessor.py

import open3d
import numpy
from math import pi

print("File name:", end=" ")
filename = input()

mesh = open3d.io.read_triangle_mesh(filename)
rotation_matrix = open3d.geometry.AxisAlignedBoundingBox.get_rotation_matrix_from_xyz([0, pi, 0])
mesh = mesh.rotate(rotation_matrix)
mesh = mesh.scale(3, [0,0,0])
#open3d.visualization.draw_geometries([mesh])d
points_and_colours = numpy.asarray(numpy.array([[i, j] for i, j in zip(mesh.vertices, mesh.vertex_colors)]).ravel(), dtype=numpy.float32)
points_and_colours.tofile("./vertex_data")
indices = numpy.asarray(mesh.triangles, dtype=numpy.int32).ravel()
indices.tofile("./index_data")

#points = numpy.asarray(point_cloud.points, dtype=numpy.float32)
#colours = numpy.asarray(point_cloud.colors, dtype=numpy.float32)

# print("Points")
# for i in range(5):
#     print(point_cloud.points[i])

# print("Colours")
# for i in range(5):
#     print(point_cloud.colors[i])

# print("Test")
# for i in range(20):
#     print(points_and_colours[i])
