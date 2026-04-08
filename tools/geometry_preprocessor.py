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

point_cloud = open3d.io.read_point_cloud(filename)
rotation_matrix = open3d.geometry.AxisAlignedBoundingBox.get_rotation_matrix_from_xyz([0, pi, 0])
point_cloud = point_cloud.rotate(rotation_matrix)
point_cloud = point_cloud.scale(3, [0,0,0])
#open3d.visualization.draw_geometries([point_cloud])
points_and_colours = numpy.asarray(numpy.array([[i, j] for i, j in zip(point_cloud.points, point_cloud.colors)]).ravel(), dtype=numpy.float32)
points_and_colours.tofile("./vertex_data")

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
