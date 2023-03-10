#pragma once

#include "stdafx.h"

#ifdef MYDLL_EXPORTS
#define MYDLL_API __declspec(dllexport)
#else
#define MYDLL_API __declspec(dllimport)
#endif

#include <vector>
#include <glm/glm.hpp>
#include <iostream>
#include <fstream>
#include <cassert>

#include <string>
#include <algorithm>
#include <list>

#include "math.hpp"


typedef glm::highp_vec2 Vector2d;
typedef std::vector<Vector2d> Vector2d1;
typedef std::vector<std::vector<Vector2d>> Vector2d2;
typedef std::vector<std::vector<std::vector<Vector2d>>> Vector2d3;

typedef glm::highp_vec3 Vector3d;
typedef std::vector<Vector3d> Vector3d1;
typedef std::vector<std::vector<Vector3d>> Vector3d2;
typedef std::vector<std::vector<std::vector<Vector3d>>> Vector3d3;

typedef glm::highp_ivec2 Vector2i;
typedef glm::highp_ivec3 Vector3i;

struct Index
{
	int x;
	int y;
	Index(){}
	Index(int x0, int y0)
	{
		x = x0;
		y = y0;
	}
};

struct Edge
{
	int source;
	int end;
	Edge(){}
	Edge(int s, int e)
	{
		source = s;
		end = e;
	}
};

struct EndEdge
{
	Index source;
	Index end;
	EndEdge(int source_x, int source_y, int end_x, int end_y)
	{
		source.x = source_x;
		source.y = source_y;
		end.x = end_x;
		end.y = end_y;
	}
};

//many kinds of distance computing in 2D
/***************************************************************************************************/
double CGAL_2D_Distance_Point_Segment(Vector2d v, Vector2d s_0, Vector2d s_1);
double CGAL_2D_Distance_Point_Line(double, double, double, double, double, double);
double CGAL_2D_Distance_Segment_Segment(double, double, double, double, double, double, double, double);
double CGAL_2D_Distance_Point_Point(Vector2d p_0, Vector2d p_1);
void CGAL_2D_Projection_Point_Segment(double p_x, double p_y,double s_s_x, double s_s_y,double s_e_x, double s_e_y, double &o_x, double &o_y);
/***************************************************************************************************/
double CGAL_2D_Distance_Point_Polygon(Vector2d p, Vector2d1 py); 
double CGAL_2D_Distance_Point_Polygon(Vector2d p, Vector2d2 pys);

//many kinds of polygon computing in 2D
/***************************************************************************************************/
bool CGAL_2D_Location_Point_Polygon(Vector2d p, Vector2d1 py);

bool CGAL_2D_Detect_Polygon_Inside(Vector2d1 outside_py, Vector2d1 inside_py);

bool CGAL_2D_Detect_Polygon_Inside(Vector2d2 outside_pys, Vector2d1 inside_py);
bool CGAL_2D_Detect_Polygon_Inside(Vector2d2 outside_pys, Vector2d2 inside_pys);

//Check the location relationship between a point "p" and a 2d polygon "py"
//p: a 2d point
//py: a 2d polygon
//return true: p is in py
//return false: p is outside of py

void CGAL_2d_Polygon_Boundingbox(Vector2d1 &ps, Vector2d &min_corner, Vector2d &max_corner);

bool CGAL_2D_Polygon_Is_Clockwise_Oriented(Vector2d1 &ps);
double CGAL_2D_Polygon_Area(Vector2d1 py);
void CGAL_2D_Polygon_Dart_Sampling(Vector2d1 &py,double d, Vector2d1 &sampling_points); //search for cgal sampling method

Vector2d CGAL_2D_Polygon_Inside_Point(Vector2d1 poly);

bool CGAL_2D_Polygon_Inside_Point(Vector2d2 polys, Vector2d &inner_vec);

void CGAL_2D_Polygon_One_Offsets(std::vector<std::vector<double>> xs, std::vector<std::vector<double>> ys, double d,
	std::vector<std::vector<double>> &offsets_xs, std::vector<std::vector<double>> &offsets_ys);
void CGAL_2D_Polygon_Offsets(std::vector<std::vector<double>> xs, std::vector<std::vector<double>> ys, double d,
	std::vector<std::vector<double>> &offsets_xs, std::vector<std::vector<double>> &offsets_ys);
Vector2d CGAL_2D_Nearest_Point_Polygon(Vector2d v, Vector2d1 poly);
void CGAL_2D_Nearest_Point_Polygon(Vector2d v, Vector2d1 poly, Vector2d &p, double &min_d);
Vector2d CGAL_2D_Nearest_Point_Polygon(Vector2d v, Vector2d2 polys);

double CGAL_2D_Two_Polygons_Intersection(Vector2d1 poly_0, Vector2d1 poly_1);
double CGAL_2D_Two_Polygons_Intersection(Vector2d1 poly_0, Vector2d1 poly_1, Vector2d2 &inter_polygons);

bool CGAL_2D_Polygon_Simple(Vector2d1 poly);

void CGAL_2D_Polygon_Triangulation(Vector2d1 &p, std::string output_file, std::string path);
void CGAL_2D_Polygon_Triangulation(Vector2d1 &p, Vector2d1 &inside_p,std::vector<double> heights, std::string output_file, std::string path);
std::vector<std::vector<int>> CGAL_2D_Polygon_Triangulation(const Vector2d2 &polys);
std::vector<std::vector<int>> CGAL_2D_Polygon_Triangulation(const Vector2d1 &poly);
/***************************************************************************************************/

//many kinds of intersection in 2D
/***************************************************************************************************/
bool CGAL_2D_Intersection_Segment_Line(Vector2d s_s, Vector2d s_e, Vector2d l_s, Vector2d l_e, Vector2d &inter);
bool CGAL_2D_Intersection_Segment_Segment(Vector2d s_0_s, Vector2d s_0_e, Vector2d s_1_s, Vector2d s_1_e, Vector2d &inter);
bool CGAL_2D_Intersection_Line_Line(double, double, double, double, double, double, double, double, double&, double&);
bool CGAL_2D_Intersection_Segment_Polygon(Vector2d s_s, Vector2d s_e, Vector2d1 &p);
/***************************************************************************************************/
//compute 2d Convex Hulls
/***************************************************************************************************/
void CGAL_2D_Convex_Hulls(Vector2d1& vec, Vector2d1& hull_points);
void CGAL_2D_OBB_Box(Vector2d1& vec,
	Vector2d &center, Vector2d &axis_0, Vector2d &axis_1, double &entent_0, double &entent_1);
/***************************************************************************************************/

//many kinds of distance computing in 3D
/***************************************************************************************************/
//double CGAL_3D_Distance_Point_Segment(Vector3d p, Vector3d s_s, Vector3d s_e);
 double CGAL_3D_Distance_Point_Segment(Vector3d p, Vector3d s_s, Vector3d s_e);

double CGAL_3D_Distance_Point_Line(Vector3d p, Vector3d l_s, Vector3d l_e);

Vector3d CGAL_3D_Projection_Point_Segment(Vector3d p, Vector3d s_s, Vector3d s_e);
Vector3d CGAL_3D_Projection_Point_Line(Vector3d p, Vector3d l_s, Vector3d l_e);
double CGAL_3D_Distance_Segment_Segment(Vector3d s_0_s, Vector3d s_0_e, Vector3d s_1_s, Vector3d s_1_e);
double CGAL_3D_Distance_Point_Point(double, double, double, double, double, double);
double CGAL_3D_Distance_Point_Point(Vector3d v0,Vector3d v1);

double CGAL_3D_Distance_Point_Triangle(Vector3d p, Vector3d t_0, Vector3d t_1, Vector3d t_2);
double CGAL_3D_Distance_Point_Triangles(Vector3d p, Vector3d1& vecs, std::vector<int>& face_id_0, std::vector<int>& face_id_1, std::vector<int>& face_id_2);
Vector3d CGAL_3D_Nearest_Point_Triangles(Vector3d p, Vector3d1& vecs, std::vector<int>& face_id_0, std::vector<int>& face_id_1, std::vector<int>& face_id_2);
void CGAL_3D_Distance_Point_Mesh(std::string path, Vector3d1 &query_points, std::vector<double>& distances);
void CGAL_3D_Neareast_Point_Mesh(std::string path, Vector3d1 &ves, Vector3d1 &ners);
double CGAL_3D_Distance_Point_Plane(Vector3d v, Vector3d plane_p, Vector3d plane_n);
void  CGAL_3D_Mesh_Near_Triangles(Vector3d1 &vecs, std::vector<int> &face_id_0, std::vector<int> &face_id_1, std::vector<int> &face_id_2, 
	Vector3d1 &points, double d, std::vector<std::vector<int>> &triangles);

void CGAL_3D_Points_inside_Triangles(Vector3d1 &vecs, std::vector<int> &face_id_0, std::vector<int> &face_id_1, std::vector<int> &face_id_2, 
	Vector3d1 &points, std::vector<bool> &insides);
void CGAL_3D_Points_inside_Triangles(std::string path, Vector3d1 &points, std::vector<bool> &insides);
/***************************************************************************************************/

void CGAL_3D_Plane_3D_to_2D_Points(Vector3d &plane_p, Vector3d &plane_n,
	Vector3d1 &points_3d,
	Vector2d1 &points_2d);

//many kinds of intersection in 3D
/***************************************************************************************************/

bool CGAL_3D_Intersection_Segment_Line(Vector3d s_s, Vector3d s_e, Vector3d l_s, Vector3d l_e, Vector3d& inter);
bool CGAL_3D_Intersection_Segment_Segment(Vector3d s_0_s, Vector3d s_0_e, Vector3d s_1_s, Vector3d s_1_e, Vector3d &iter);

//bool CGAL_3D_Intersection_Line_Line(double, double, double, double, double, double, double, double, double&, double&);
bool CGAL_3D_Intersection_Segment_Plane(Vector3d s_s, Vector3d s_e, Vector3d plane_p, Vector3d plane_n, Vector3d& inter);

bool CGAL_3D_Intersection_Line_Plane(Vector3d l_s, Vector3d l_e, Vector3d plane_p, Vector3d plane_n, Vector3d& inter);

bool CGAL_3D_Intersection_Sphere_Ray(double, double, double, double,
	double, double, double, double, double, double, std::vector<double>&, std::vector<double>&, std::vector<double>&);

bool CGAL_3D_Intersection_Ray_Triangle(Vector3d p, Vector3d n, Vector3d p0, Vector3d p1, Vector3d p2);

bool CGAL_3D_Intersection_Ray_Mesh(Vector3d p, Vector3d n,std::string path);
void CGAL_3D_Intersection_Ray_Mesh(Vector3d1 ps, Vector3d1 ns, std::string path, Vector3d1 &inters);
/***************************************************************************************************/

//subdivision the input mesh
/***************************************************************************************************/
void CGAL_Mesh_Subdivision(std::string in_path, std::string sub, int step,std::string out_path);

void CGAL_Mesh_Loop_Subdivision_One_Step(Vector3d1 &vecs, std::vector<int> &face_id_0, std::vector<int> &face_id_1, std::vector<int> &face_id_2);
void CGAL_Mesh_Laplace_Smooth(std::string in_path, std::string out_path, int laplace_nb = 0);
void CGAL_Mesh_Laplace_Smooth(Vector3d1 &vecs, std::vector<int> &face_id_0, std::vector<int> &face_id_1, std::vector<int> &face_id_2, int laplace_nb = 0);
void CGAL_Mesh_Laplace_Smooth_by_Curvature(Vector3d1 &vecs, std::vector<int> &face_id_0, std::vector<int> &face_id_1, std::vector<int> &face_id_2, double low_curvature);
void CGAL_Mesh_Loop_Subdivision_Own_Version(std::string in_path,int step, std::string out_path, int laplace_nb=0);
/***************************************************************************************************/

//IO mesh
/***************************************************************************************************/
void CGAL_Output_Obj(std::string path, Vector3d1 &vecs);
void CGAL_Output_Obj(std::string path, Vector3d1 &vecs, std::vector<int> &face_id_0, std::vector<int> &face_id_1, std::vector<int> &face_id_2);
void CGAL_Output_Obj(std::string path, Vector3d1 &vecs, std::vector<std::vector<int>> &face_ids);
void CGAL_Output_Obj(std::string path, Vector3d1 &vecs, std::vector<std::vector<int>> &face_ids, 
	std::vector<int> &triangles_lables, int index);
void CGAL_Output_Obj(std::string path, Vector3d1 &vecs, Vector3d1 &colors, std::vector<int> &face_id_0, std::vector<int> &face_id_1, std::vector<int> &face_id_2);
void CGAL_Output_Obj(std::string path, Vector3d1 &vecs, Vector3d1 &colors, std::vector<std::vector<int>> &face_ids);

void CGAL_Output_Off(std::string path, Vector3d1 &vecs, std::vector<int> &face_id_0, std::vector<int> &face_id_1, std::vector<int> &face_id_2);
void CGAL_Load_Obj(std::string path, std::vector<double> &coords, std::vector<int> &tris);
void CGAL_Rotation_Obj(std::string path, double angle, Vector3d axis);

void CGAL_Export_Path_Segment(std::ofstream &export_file_output, int &export_index,
	std::string s_name, double r, double g, double b, Vector3d start, Vector3d end, double radius);
void CGAL_Export_Path_Point(std::ofstream &export_file_output, int &export_index,
	std::string s_name, double r, double g, double b, Vector3d point, double radius);
/***************************************************************************************************/

//slice the input mesh
/***************************************************************************************************/
void CGAL_Slicer_Mesh(std::string path, Vector3d plane_normal, std::vector<double> plane_d,
	Vector3d3& offsetses, Vector3d2& offsets);
/***************************************************************************************************/

//shortest geodesic path
/***************************************************************************************************/
void CGAL_Shortest_Geodesic_Path(std::string, std::vector<double> &, std::vector<double> &, std::vector<double> &);
void CGAL_Shortest_Geodesic_Path(std::string path, Vector3d source, Vector3d target, Vector3d1 &output);
void CGAL_Shortest_Geodesic_Path(std::string, std::vector<double>, std::vector<double>, std::vector<double>, std::vector<double>, std::vector<double>, std::vector<double>, 
	std::vector<std::vector<double>> &, std::vector<std::vector<double>> &, std::vector<std::vector<double>> &);

//shortest geodesic distance
/***************************************************************************************************/
double CGAL_Geodesic_Distance(std::string path, Vector3d source, Vector3d target);
//project points onto mesh
/***************************************************************************************************/
Vector3d1 CGAL_Project_Points_Onto_Surface(Vector3d1& vecs, std::vector<int>& face_id_0, std::vector<int>& face_id_1, std::vector<int>& face_id_2, Vector3d1& points);
Vector3d1 CGAL_Project_Points_Onto_Surface(std::string path, Vector3d1& points);

void CGAL_3D_Read_Triangle_Mesh(std::string path, Vector3d1& vecs, std::vector<int>& face_id_0, std::vector<int>& face_id_1, std::vector<int>& face_id_2);
void CGAL_3D_Read_Triangle_Mesh(std::string path, Vector3d1& vecs, std::vector<std::vector<int>>& face_ids);

void CGAL_3D_Triangle_Mesh_Boundary(Vector3d1 &vecs, std::vector<int> &face_id_0, std::vector<int> &face_id_1, std::vector<int> &face_id_2, std::vector<bool> &bools);
void CGAL_3D_Triangle_Mesh_Boundary(std::string path, std::vector<bool> &bools);
void CGAL_3D_Triangle_Mesh_Boundary(Vector3d1 &vecs, std::vector<int> &face_id_0, std::vector<int> &face_id_1, std::vector<int> &face_id_2, 
	Vector3d2 &boundaries, Vector3d &inside=Vector3d(0.0,0.0,0.0));

void CGAL_3D_Triangle_Mesh_Boundary(Vector3d1& vecs, std::vector<std::vector<int>>& face_ids, Vector3d2 &boundaries);
//Get the mesh boundaries
//vecs, face_id_0/1/2: points of mesh
//boundaries: output boundaries
//inside: return a inside vector among the input surface
void CGAL_3D_Triangle_Mesh_Boundary(std::string path, Vector3d2 &boundaries, Vector3d &inside = Vector3d(0.0, 0.0, 0.0));
//Get the mesh boundaries
//path: input obj/off path (Note that input model should have open boundaries.)
//boundaries: output boundaries
//inside: return a inside vector among the input surface

void CGAL_3D_Triangel_Mesh_Most_Inside_Point(Vector3d1 &vecs, std::vector<int> &face_id_0, std::vector<int> &face_id_1, std::vector<int> &face_id_2, Vector3d &inside);

double CGAL_3D_One_Triangle_Area(Vector3d v0, Vector3d v1, Vector3d v2);
double CGAL_3D_Triangle_Mesh_Area(Vector3d1 &vecs, std::vector<int> &face_id_0, std::vector<int> &face_id_1, std::vector<int> &face_id_2);

void CGAL_3D_Triangle_Mesh_Vecs_Faces(Vector3d1 &vecs, std::vector<int> &face_id_0, std::vector<int> &face_id_1, std::vector<int> &face_id_2, std::vector<std::vector<int>> &surface_vectices_to_face);

void CGAL_3D_Triangle_Mesh_Vecs_Neighbors(Vector3d1 &vecs, std::vector<int> &face_id_0, std::vector<int> &face_id_1, std::vector<int> &face_id_2, std::vector<std::vector<int>> &neighs);

void CGAL_3D_Triangle_Mesh_Vecs_Neighbor_Edges(Vector3d1 &vecs, std::vector<int> &face_id_0, std::vector<int> &face_id_1, std::vector<int> &face_id_2, std::vector<std::vector<std::vector<int>>> &surface_vectices_to_neighbor_edges);

//compute 3d Convex Hulls
/***************************************************************************************************/
void CGAL_3D_Convex_Hulls(Vector3d1 &vec, Vector3d1 &hull_points);

void CGAL_3D_Convex_Hulls(Vector3d1 &vec, Vector3d1 &hull_points,
	std::vector<int>& hulls_surface_0, std::vector<int>& hulls_surface_1, std::vector<int>& hulls_surface_2);

void CGAL_3D_Convex_Hulls(Vector3d1 &vec, Vector3d1 &hull_points,
	Vector3d1 &plane_p, Vector3d1 &plane_n);

void CGAL_3D_Convex_Hulls(Vector3d1 &vec, Vector3d1 &hull_points,
	std::vector<int>& hulls_surface_0, std::vector<int>& hulls_surface_1, std::vector<int>& hulls_surface_2,
	Vector3d1 &plane_p, Vector3d1 &plane_n);


Vector3d CGAL_3D_Projection_Point_Plane(Vector3d p, Vector3d plane_p, Vector3d plane_n);
Vector3d CGAL_3D_Projection_Point_Plane(Vector3d p, Vector3d plane_p_0, Vector3d plane_p_1, Vector3d plane_p_2);

Vector2d CGAL_3D_Projection_3D_Point_Plane_2D(Vector3d p, Vector3d plane_p, Vector3d plane_n);
Vector2d CGAL_3D_Projection_3D_Point_Plane_2D(Vector3d p, Vector3d plane_p_0, Vector3d plane_p_1, Vector3d plane_p_2);

/***************************************************************************************************/

//3d plane related
/***************************************************************************************************/
Vector3d CGAL_3D_Plane_Base_1(Vector3d plane_p, Vector3d plane_n);

//3D mesh curvature
/***************************************************************************************************/
void CGAL_3D_Mesh_Curvature(Vector3d1& vecs, std::vector<int>& face_id_0, std::vector<int>& face_id_1,
	std::vector<int>& face_id_2, std::vector<double> &max_curs, std::vector<double> &min_curs);
void CGAL_3D_Mesh_Curvature(Vector3d1& vecs, std::vector<std::vector<int>>& face_ids, std::vector<double> &max_curs, std::vector<double> &min_curs);

void CGAL_3D_Mesh_Curvature(Vector3d1& vecs, std::vector<int>& face_id_0, std::vector<int>& face_id_1,
	std::vector<int>& face_id_2, std::vector<double> &max_curs, std::vector<double> &min_curs,
	Vector3d1 &max_curs_directions, Vector3d1 &min_curs_directions);
void CGAL_3D_Mesh_Curvature(Vector3d1& vecs, std::vector<std::vector<int>>& face_ids, std::vector<double> &max_curs, std::vector<double> &min_curs,
	Vector3d1 &max_curs_directions, Vector3d1 &min_curs_directions);

void CGAL_3D_Mesh_Curvature(Vector3d1& vecs, std::vector<int>& face_id_0, std::vector<int>& face_id_1,
	std::vector<int>& face_id_2, std::vector<double> &max_curs, std::vector<double> &min_curs,
	Vector3d1 &max_curs_directions, Vector3d1 &min_curs_directions, Vector3d1 &normals);
void CGAL_3D_Mesh_Curvature(Vector3d1& vecs, std::vector<std::vector<int>>& face_ids, std::vector<double> &max_curs, std::vector<double> &min_curs,
	Vector3d1 &max_curs_directions, Vector3d1 &min_curs_directions, Vector3d1 &normals);

//void CGAL_3D_Mesh_Curvature(std::vector<double>& xs, std::vector<double>& ys, std::vector<double>& zs,
//	std::vector<int>& face_id_0, std::vector<int>& face_id_1, std::vector<int>& face_id_2, std::vector<double> &max_curs, std::vector<double> &min_curs,
//	std::vector<double> &max_curs_directions_x, std::vector<double> &max_curs_directions_y, std::vector<double> &max_curs_directions_z,
//	std::vector<double> &min_curs_directions_x, std::vector<double> &min_curs_directions_y, std::vector<double> &min_curs_directions_z,
//	std::vector<double> &normal_x, std::vector<double> &normal_y, std::vector<double> &normal_z);

void CGAL_Mesh_Field_Query(std::string path, Vector3d1 &gradients, Vector3d1& input_points, Vector3d1 &points_gradients);
void CGAL_Mesh_Field_Query(std::string path, std::vector<double> &gradient_values, Vector3d1& input_points, std::vector<double> &points_gradient_values);
void CGAL_Mesh_Field_Query(std::string path, std::vector<double> &gradient_values, Vector3d2& input_point_es, std::vector<std::vector<double>> &points_gradient_value_es);


void CGAL_Curvature_Mesh(std::string path, Vector3d1& input_points, std::vector<double> &max_curs, std::vector<double> &min_curs,
	Vector3d1 &max_curs_directions, Vector3d1 &min_curs_directions);

//normal computation
/***************************************************************************************************/
Vector3d CGAL_Face_Normal(Vector3d source, Vector3d tri_0, Vector3d tri_1, Vector3d tri_2, Vector3d normal_0, Vector3d normal_1, Vector3d normal_2);
void CGAL_Normal_Mesh(std::string path, Vector3d1 &mesh_points, Vector3d1 &mesh_normals);
void CGAL_Normal_Mesh(std::string path, Vector3d2 &mesh_pointses, Vector3d2 &mesh_normalses);

void CGAL_3D_Mesh_Normal(Vector3d1 &ps, std::vector<std::vector<int>> &face_ids, Vector3d1 &normals);
void CGAL_3D_Mesh_Normal(Vector3d1 &ps, std::vector<int>& face_id_0, std::vector<int>& face_id_1, std::vector<int>& face_id_2, Vector3d1 &normals);
/***************************************************************************************************/

Vector3d CGAL_3D_Mesh_Center(Vector3d1 &ps);
void CGAL_3D_Mesh_Boundingbox(Vector3d1 &ps, Vector3d &min_corner, Vector3d &max_corner);
void CGAL_Barycentric(Vector3d p, Vector3d a, Vector3d b, Vector3d c, double &u, double &v, double &w);

/***************************************************************************************************/
void CGAL_Image_Grid_Decomposition(std::vector<std::vector<int>> &image, std::vector<std::vector<double>> &boundary_xs, std::vector<std::vector<double>> &boundary_ys);
void CGAL_Image_Grid_Decomposition_Conservative(std::vector<std::vector<int>> &image, std::vector<std::vector<double>> &boundary_xs, std::vector<std::vector<double>> &boundary_ys);
void CGAL_Image_Grid_Decomposition(std::vector<std::vector<int>> &image, Vector2d2 &boundaries);
void CGAL_Image_Grid_Decomposition_Conservative(std::vector<std::vector<int>> &image, Vector2d2 &boundaries);
/***************************************************************************************************/
void CGAL_Surface_Decomposition(std::string path, std::vector<double> &face_sdf, int &regions_nb, std::vector<int> &face_segments);

/***************************************************************************************************/
void CGAL_Intergral_Curvature(Vector2d1 &input_points, int sampling_points_nb, double radius, double thresholder, 
	Vector2d1 &output_points, std::vector<double> &output_rates);

/***************************************************************************************************/
void CGAL_3D_Connecting_Segments(Vector3d2 &segments, Vector3d2 &lines);


/***************************************************************************************************/
void CGAL_BSplineCurveFit(Vector3d1& samples, Vector3d1& output);
