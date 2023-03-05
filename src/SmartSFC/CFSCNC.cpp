#include "stdafx.h"
#include "CFSCNC.h"
#include "Tree.h"
#include "cgalpackage.h"
#include <limits>

#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/boost/graph/graph_traits_Surface_mesh.h>
#include <CGAL/AABB_halfedge_graph_segment_primitive.h>
#include <CGAL/AABB_face_graph_triangle_primitive.h>
#include <CGAL/AABB_tree.h>
#include <CGAL/AABB_traits.h>
#include <CGAL/Polyhedron_3.h>
#include <CGAL/Polyhedron_items_with_id_3.h>
#include <CGAL/IO/Polyhedron_iostream.h>
//#include "Python.h"
//#include <numpy/arrayobject.h>
#include "kdtree.h"
#include "ToolPathTimeEstimator.hpp"
#include <sysinfoapi.h>

typedef CGAL::Exact_predicates_inexact_constructions_kernel K;
typedef K::Point_3                    Point_3;
typedef K::Ray_3                      Ray_3;
typedef K::Vector_3                   Vector_3;
typedef K::Triangle_3				  Triangle_3;

typedef CGAL::Polyhedron_3<K, CGAL::Polyhedron_items_with_id_3> Polyhedron_3;
typedef CGAL::AABB_face_graph_triangle_primitive<Polyhedron_3> Primitive;
typedef CGAL::AABB_traits<K, Primitive> Traits_poly;
typedef CGAL::AABB_tree<Traits_poly> AABB_Tree;
typedef AABB_Tree::Point_and_primitive_id Point_and_primitive_id;
typedef boost::optional<AABB_Tree::Intersection_and_primitive_id<Ray_3>::Type> Ray_intersection;

using namespace hpcg;

namespace cnc {

	bool output_debug_offset = true;
	bool output_rerun = true;

	CFSCNC::CFSCNC()
	{
		fermat_success = true;
		corrent_direction = true;

		max_scallop = 0.02;
		drill_radius = 2.0;
		chord_error = -1.0;
		
		//0.003125
		shortest_segments_length = 0.1;
		whether_using_heat = false;

		sub_div = 0;
	}

	void CFSCNC::ClearAll()
	{
		std::vector<Node>().swap(contour_nodes);
		std::vector<Edge>().swap(contour_edges);
		Vector3d2().swap(offsets);

		std::vector<string>().swap(offsets_name);
		std::vector<string>().swap(single_path_name);
		std::vector<string>().swap(single_final_path_name);

		Vector3d1().swap(single_path);

		Vector3d2().swap(single_path_parts);
		std::vector<Part>().swap(parts);
		std::vector<int>().swap(merge_edge);
		std::vector<int>().swap(removed_merge_edge);
		std::vector<bool>().swap(used);
	}

	/*
	Main function of CFS
	*****************************************************************************************************************/
	void CFSCNC::OffsetsBasedCFSLinking(std::string path,  bool re_running, bool magic_number)
	{
		input_path = path;

		//generate iso level sets
		/************************************************************************************/
		rerouting_points_nb = 0;
//		toolpath_size = 5.0;
		//SSFC
		Vector3d3 offsetses;
		std::vector<int> offset_graph;
		Vector3d3 offset_graph_sharing_parts_no_par;
		std::vector<int> mst;

		Vector3d2 boundaries;
		LoadBoundary(input_path + "shape.txt", boundaries);

		BuildOffsets(boundaries, offsetses, offsets);

//		OutputOffsets(path + "offsets.obj", offsets);

		if (offsets.size() == 1)
		{
			single_path = offsets[0];
		}


		if (!std::ifstream(input_path + "output.path", std::ios::in) || re_running)
		{
			//build offset graph
			//Assign edge weight
			std::vector<double> parts_length;
			Tree::BuildOffsetGraph_Nopar(input_path,toolpath_size, offsetses, offsets, offset_graph, offset_graph_sharing_parts_no_par, parts_length);

			//debug
			for (int i = 0; i < offset_graph_sharing_parts_no_par.size(); i++)
			{
				//OutputStrips(path + "debug\\sharing_" + IntString(i) + ".obj", offset_graph_sharing_parts_no_par[i]);
			}

			//minimal spanning tree
			Tree::MinimalSpanningTree(offsets, offset_graph, parts_length, mst);
			Output_tree(offsets.size(), offset_graph, "./CFS/1.gml");
			Output_tree(offsets.size(), mst, "./CFS/2.gml");

			//input nodes and edges
			InputDataStructure(offsetses, offset_graph_sharing_parts_no_par, mst, offset_graph);

			DWORD start_time = GetTickCount();

			if (true)
			{
				
				if (false)
					//if (Tree::Degree3Number(offsets.size(), mst)==0)
				{
					GenerateSmoothFermatSpirals(mst, 0);

					GetOneSinglePath();

					if (false)
					if (fermat_success)
					{
						if (GetOneSinglePath_0())
						{
							/*Vector3d1 vecs;
							CGAL_3D_Neareast_Point_Mesh(path + "\\" + IntString(cfs_index) + ".off", single_path, vecs);
							Vector3d1().swap(single_path);
							single_path = vecs;
							Vector3d1().swap(vecs);
							Output_Path(input_path + "\\path\\" + IntString(cfs_index) + ".path");
							Output_Obj_Cur_Normals(input_path + "\\path\\" + IntString(cfs_index) + "_normals_curs.data");*/
						}
						else
						{
							std::cout << "Get one single path errors..." << std::endl;
						}
					}
					else
					{
						std::cout << "Generate Smoothness fermat spirals errors..." << std::endl;
					}
				}
				else
				{
					//generate FS for each group
					GenerateFermatSpirals(mst, 0);
					//get one single path from these fermat spirals

					GetOneSinglePath();

//					if (false)
					if (fermat_success)
					{
						Output_Path(input_path + "output" + ".path");
//						Output_Obj_Cur_Normals(input_path + "path/" + IntString(cfs_index) + "_normals_curs.data");
					}
				}
			}
			else
			{
				if (GenerateFermatSpirals_HybridMethod(mst))
					GetOneSinglePath();
				else
				{
					std::cerr << "Error...407" << std::endl;
				}
			}

			single_paths.push_back(single_path);
			DWORD end_time = GetTickCount();
			std::cout << "[TIME] tool path generation times " << (end_time - start_time) / 1000.0 << std::endl;
		}
		else
		{
			Load_Path(input_path + "output.path");
			single_paths.push_back(single_path);
		}

		Vector3d3().swap(offsetses);
		std::vector<int>().swap(offset_graph);
		std::vector<int>().swap(mst);

		std::cout << "****************************************************************************"<< std::endl;
		std::cout << "Rerouting points: " << rerouting_points_nb << std::endl;

	}

	void CFSCNC::ShowZigzag(std::string  path)
	{
		std::ifstream file(path+"\\nc.txt", std::ios::in);

		int nb = 0;
		file >> nb;
		/*for (int i = 0; i < nb; i++)
		{
			double x, y, z;
			file >> x >> y >> z;
			single_final_path.push_back(Vector3d(x, y, z));
		}*/

		std::string line;
		while (std::getline(file, line))
		{
			int pos_x = line.find_first_of("X");
			int pos_y = line.find_first_of("Y");
			int pos_z = line.find_first_of("Z");
			int pos_a = line.find_first_of("A");

			if (pos_x >= 0 && pos_y >= 0 && pos_z >= 0&&pos_a<0)
			{
				string x = line.substr(pos_x + 1, pos_y-pos_x-1);
				string y = line.substr(pos_y+1, pos_z-pos_y-1);
				string z = line.substr(pos_z+1, line.size()-pos_z-1);
				single_final_path.push_back(Vector3d(StringToNum<float>(x), StringToNum<float>(y), StringToNum<float>(z)));
			}

			if (pos_x >= 0 && pos_y >= 0 && pos_z >= 0 && pos_a>=0)
			{
				string x = line.substr(pos_x + 1, pos_y - pos_x - 1);
				string y = line.substr(pos_y + 1, pos_z - pos_y - 1);
				string z = line.substr(pos_z + 1, pos_a - pos_z - 1);
				single_final_path.push_back(Vector3d(StringToNum<float>(x), StringToNum<float>(y), StringToNum<float>(z)));
			}
		}


		file.clear();
		file.close();
	}

	void CFSCNC::ShowZigzag1(std::string  path)
	{
		std::ifstream file(path + "\\0.2799.NC", std::ios::in);
		double r_x, r_y, r_z;

		std::string line;
		while (std::getline(file, line))
		{
			int pos_x = line.find_first_of("X");
			int pos_y = line.find_first_of("Y");
			int pos_z = line.find_first_of("Z");

			bool b = false;

			if (pos_x >= 0 && pos_y >= 0 && pos_z >= 0 )
			{
				string str_x = line.substr(pos_x + 1, pos_y - pos_x - 1);
				string str_y = line.substr(pos_y + 1, pos_z - pos_y - 1);
				string str_z = line.substr(pos_z + 1, line.size() - pos_z - 1);
				
				r_x = StringToNum<float>(str_x);
				r_y = StringToNum<float>(str_y);
				r_z = StringToNum<float>(str_z);
				b = true;
			}

			if (pos_x >= 0 && pos_y < 0 && pos_z >= 0)
			{
				string str_x = line.substr(pos_x + 1, pos_z - pos_x - 1);
				string str_z = line.substr(pos_z + 1, line.size() - pos_z - 1);
				r_x = StringToNum<float>(str_x);
				r_z = StringToNum<float>(str_z);
				b = true;
			}

			if (pos_x >= 0 && pos_y < 0 && pos_z < 0)
			{
				string str_x = line.substr(pos_x + 1, line.size() - pos_x - 1);
				r_x = StringToNum<float>(str_x);
				b = true;
			}
			
			if (b)
			{
				single_final_path.push_back(Vector3d(r_x, r_y, r_z));
			}
		}

		file.clear();
		file.close();
	}

	double total_path_length = 0.0;
	double total_path_rate = 0.0;

	
	AABB_Tree machining_shape_tree_0;

	double CFSCNC::InterferenceOneRayDistance(Vector3d v, Vector3d n)
	{
		SetVectorLength(n, 5.0);
		Ray_3 ray(Point_3(v[0], v[1], v[2]), Vector_3(n[0], n[1], n[2]));

		Ray_intersection intersection = machining_shape_tree_0.any_intersection(ray);

		if (intersection){
			if (boost::get<Point_3>(&(intersection->first))){
				const Point_3* p = boost::get<Point_3>(&(intersection->first));

				double d0 = (*p)[0];
				double d1 = (*p)[1];
				double d2 = (*p)[2];

				return getLength(Vector3d(d0, d1, d2) - v);
			}
		}
		return -1;
	}

	double CFSCNC::ClosedPoint(Vector3d v)
	{
		Point_3 p(v[0], v[1], v[2]);
		Point_3 closest_point = machining_shape_tree_0.closest_point(p);
		return getLength(Vector3d(closest_point[0], closest_point[1], closest_point[2]) - v);
	}

	void CFSCNC::ShowZigzag3(std::string  path)
	{
		Vector3d1 surface_vertices;
		Vector3d1 surface_vertices_normals;

		std::vector<int> face_id_0;
		std::vector<int> face_id_1;
		std::vector<int> face_id_2;

		CGAL_3D_Read_Triangle_Mesh(scallop_base_mesh, surface_vertices, face_id_0, face_id_1, face_id_2);
		CGAL_3D_Mesh_Normal(surface_vertices, face_id_0, face_id_1, face_id_2, surface_vertices_normals);

		double total_mesh_area = CGAL_3D_Triangle_Mesh_Area(surface_vertices, face_id_0, face_id_1, face_id_2);

		/****************************************************************************************************************/
		/****************************************************************************************************************/

		std::vector<bool> bools;
		CGAL_3D_Triangle_Mesh_Boundary(surface_vertices, face_id_0, face_id_1, face_id_2, bools);

		struct kdtree *kd_tree  = kd_create(3);

		Vector3d1 vertices;
		std::vector<int> face_0_id_0;
		std::vector<int> face_0_id_1;
		std::vector<int> face_0_id_2;

		CGAL_3D_Read_Triangle_Mesh(scallop_simulated_mesh, vertices, face_0_id_0, face_0_id_1, face_0_id_2);

		for (int i = 0; i < vertices.size(); i++)
		{
			void *val = &vertices[i];
			kd_insert3(kd_tree, vertices[i][0], vertices[i][1], vertices[i][2], val);
		}


		Polyhedron_3 polyhedron;
		std::ifstream input(scallop_simulated_mesh, std::ios::in);
		input >> polyhedron;
		input.close();
		CGAL::set_halfedgeds_items_id(polyhedron);
		machining_shape_tree_0.clear();
		machining_shape_tree_0.insert(faces(polyhedron).first, faces(polyhedron).second, polyhedron);

		double max_d = -100000.0;
		double min_d = 1000000.0;

		double total_error = 0.0;

		std::vector<double> height;

		int indexxx = 0;
		for (int i = 0; i < surface_vertices.size(); i++)
		{
			//-2.30594 -3.85774 0.0

			if (i % ((int)(surface_vertices.size() / 100.0)) == 0)
				std::cout << "Vertice Index: " << i << " / " << surface_vertices.size() << std::endl;

			Vector3d v = surface_vertices[i];
			Vector3d n = surface_vertices_normals[i];

			//v[0] = -2.30594;
			//v[1] = -3.85774;
			//v[2] = 0.0;
			double d = InterferenceOneRayDistance(v, n);

			if (d >= 0.0)
			{
				height.push_back(d);
			}
			else
			{
				d = InterferenceOneRayDistance(v, -n);
				if (d >= 0.0)
					height.push_back(-d);
				else
				{
					height.push_back(0.0);
				}
				//height.push_back(-ClosedPoint(surface_vertices[i]));
				/*			double *pos = new double[3];
				pos[0] = v[0];
				pos[1] = v[1];
				pos[2] = v[2];
				struct kdres *r = kd_nearest(kd_tree, pos);
				double position[3];
				*(int*)kd_res_item(r, position);
				double d = Strip::Distance(v, Vector3d(position[0], position[1], position[2]));
				height.push_back(-d);*/
			}

			if (!bools[i])
			{
				max_d = max(max_d, height[height.size() - 1]);
				min_d = min(min_d, height[height.size() - 1]);
				total_error += abs(height[height.size() - 1]);

				if (height[height.size() - 1] > max_scallop)
				{
					indexxx++;
				}
			}
			//break;
		}

		total_error = total_error / surface_vertices.size()*total_mesh_area;

		double over_cut_rate = (double)indexxx / (double)surface_vertices.size();

		std::cout << "Length      :   " << total_path_length << std::endl;
		std::cout << "SharpTurns  :   " << total_path_rate << std::endl;
		std::cout << "max_d       :   " << max_d << std::endl;
		std::cout << "min_d       :   " << min_d << std::endl;
		std::cout << "total_error :   " << total_error << std::endl;
		std::cout << "OverCutRate :   " << over_cut_rate << std::endl;

		ofstream   ofresult(measure_path_output, ios::app);

		ofresult << cls_path << std::endl;
		ofresult << "Length      :   " << total_path_length << std::endl;
		ofresult << "SharpTurns  :   " << total_path_rate << std::endl;
		ofresult << "max_d       :   " << max_d << std::endl;
		ofresult << "min_d       :   " << min_d << std::endl;
		ofresult << "total_error :   " << total_error << std::endl;
		ofresult << "OverCutRate :   " << over_cut_rate << std::endl;
		ofresult << "" << std::endl;

		ofresult.clear();
		ofresult.close();

		max_d = max_scallop;
		min_d = -max_scallop;


		std::ofstream file(scallop_map_path);

		double arv_r, arv_g, arv_b;
		ColorMapping(0.5, arv_r, arv_g, arv_b);

		for (int i = 0; i < surface_vertices.size(); i++)
		{
			double d = (height[i] - min_d) / (max_d - min_d);
			double r, g, b;

			ColorMapping(d, r, g, b);
			//file << "v " << surface_vertices[i][0] << " " << surface_vertices[i][1] << " " << surface_vertices[i][2] << std::endl;

		/*	if (height[i]>0.10)
				file << "v " << surface_vertices[i][0] << " " << surface_vertices[i][1] << " " << surface_vertices[i][2] << " 1.0 0.0 0.0" << std::endl;
			else
			{
				file << "v " << surface_vertices[i][0] << " " << surface_vertices[i][1] << " " << surface_vertices[i][2] << " 0.0 1.0 0.0" << std::endl;
			}*/

			if (!bools[i])
			{
				file << "v " << surface_vertices[i][0] << " " << surface_vertices[i][1] << " " << surface_vertices[i][2] << " " << r << " " << g << " " << b << std::endl;
			}
			else
				file << "v " << surface_vertices[i][0] << " " << surface_vertices[i][1] << " " << surface_vertices[i][2] << " " << arv_r << " " << arv_g << " " << arv_b << std::endl;

		}

		for (int i = 0; i < face_id_0.size(); i++)
		{
			file << "f " << face_id_0[i] + 1 << " " << face_id_1[i] + 1 << " " << face_id_2[i] + 1 << std::endl;
		}

		file.clear();
		file.close();
	}



	double SearchDouble(std::string line, std::string str)
	{
		//r_z = StringToNum<float>(double_line);
		string double_line = "";

		int index = line.find_first_of(str);

		while (true)
		{
			index++;
			string sub_line = line.substr(index,1);
			if (sub_line == "X" || sub_line == "Y" || sub_line == "Z" || sub_line == "I" || sub_line == "J" || sub_line.size() == 0)
				break;
			double_line += sub_line;
		}

		return  StringToNum<double>(double_line);
	}

	void CFSCNC::ShowNCPath(std::string path)
	{
		input_path = path;

		toolpath_size = 2 * sqrt(2.0*drill_radius*max_scallop - max_scallop*max_scallop);

		std::ifstream file(nc_path, std::ios::in);

		if (!file)
		{
			std::cout << "Invilid file path: " << cls_path << std::endl;
		}

		double r_x, r_y, r_z;

		bool b_x = false;
		bool b_y = false;
		bool b_z = false;

		bool g_0 = true;
		  
		Vector3d2 single_final_path_es;

		std::string line;
		while (std::getline(file, line))
		{
			int pos_x = line.find_first_of("X");
			int pos_y = line.find_first_of("Y");
			int pos_z = line.find_first_of("Z");

			int g00_pos = line.find_first_of("P");
			int g01_pos = line.find_first_of("T");

			if (g00_pos >= 0)
				g_0 = true;

			if (g01_pos >= 0)
				g_0 = false;

			if (pos_x >= 0 || pos_y >= 0 || pos_z >= 0)
			{
				if (pos_x >= 0){
					b_x = true;
					r_x = SearchDouble(line, "X");
				}
				if (pos_y >= 0){
					b_y = true;
					r_y = SearchDouble(line, "Y");
				}
				if (pos_z >= 0){
					b_z = true;
					r_z = SearchDouble(line, "Z");
				}

				if (g_0)
				{
					if (b_x&&b_y&&b_z)
					{
						single_final_path.push_back(Vector3d(r_x, r_y, r_z));
					}
				}
				else
				{
					if (single_final_path.size() > 2)
					{
						single_final_path_es.push_back(single_final_path);
						Vector3d1().swap(single_final_path);
					}
				}
			}
		}

		double length = Strip::GetTotalLength(single_final_path);
		std::cout << "Total Length: " << length << std::endl;

		if (single_final_path.size() > 2)
		{
			single_final_path_es.push_back(single_final_path);
		}
	
		file.clear();
		file.close();
		Vector3d1().swap(single_final_path);

		for (int i = 0; i < single_final_path_es.size(); i++)
		{
			for (int j = 0; j < single_final_path_es[i].size(); j++)
			{
				single_final_path.push_back(single_final_path_es[i][j]);
			}
		}
	}


	/*
	Input the data structure
	*****************************************************************************************************************/
	void CFSCNC::InputDataStructure(Vector3d3 &offsetses,
		Vector3d3 &offset_graph_sharing_parts, std::vector<int> &mst, std::vector<int> &offset_graph)
	{
		DWORD start_time = GetTickCount();

		//build the contour_nodes
		int node_id = 0;
		for (int i = 0; i < offsetses.size(); i++)
		{
			for (int j = 0; j < offsetses[i].size(); j++)
			{
				Node node;
				node.points.assign(offsetses[i][j].begin(), offsetses[i][j].end());
				node.id = node_id;
				node.layer_id = i;
				node.has_connected = false;
				node.connecting_label = "bruteforce";
				contour_nodes.push_back(node);
				node_id++;
			}
		}

		//compute node degree along the tree
		std::vector<int> node_degree = Tree::ComputeNodeDegree(contour_nodes.size(), mst);
		for (int i = 0; i < node_degree.size(); i++)
			contour_nodes[i].degree = node_degree[i];

		//build the contour edges
		for (int i = 0; i < mst.size(); i = i + 2)
		{
			Edge edge;
			edge.edge_id = contour_edges.size();
			edge.node_id.push_back(mst[i]);
			edge.node_id.push_back(mst[i + 1]);
			//edge.cutting_parts.push_back(std::vector<double>());
			//edge.cutting_parts.push_back(std::vector<double>());

			for (int j = 0; j < offset_graph.size(); j = j + 2)
			{
				if (offset_graph[j] == mst[i] && offset_graph[j + 1] == mst[i + 1])
				{
					edge.sharing_parts_v.push_back(offset_graph_sharing_parts[j]);
					edge.sharing_parts_v.push_back(offset_graph_sharing_parts[j + 1]);
					break;
				}
				if (offset_graph[j + 1] == mst[i] && offset_graph[j] == mst[i + 1])
				{
					edge.sharing_parts_v.push_back(offset_graph_sharing_parts[j + 1]);
					edge.sharing_parts_v.push_back(offset_graph_sharing_parts[j]);
					break;
				}
			}

			//edges_id edges_index
			contour_nodes[edge.node_id[0]].edges_id.push_back(edge.edge_id);
			contour_nodes[edge.node_id[0]].edges_index.push_back(0);
			contour_nodes[edge.node_id[1]].edges_id.push_back(edge.edge_id);
			contour_nodes[edge.node_id[1]].edges_index.push_back(1);

			contour_edges.push_back(edge);
		}

		DWORD end_time = GetTickCount();
		std::cout << "[TIME] InputDataStructure: " << (end_time - start_time) / 1000.0 << std::endl;
	}
	/*****************************************************************************************************************/

	//smooth fermat spiral
	void CFSCNC::GenerateSmoothFermatSpirals(std::vector<int> &mst, int start_nodel_id)
	{
		std::vector<std::vector<int>> node_sequences;
		Tree::GetAllSequences(contour_nodes, mst, node_sequences);
		SmoothFermatSpiral_2(node_sequences[0]);
	}

	//For a local spirallable iso-contours, generate a fermat spiral with a smoothing pattern
	//The basic idea is to understand to connect the 1-3,2-4,3-5
	//sequence: the input sequence node ids of the spirallable iso-contours
	bool CFSCNC::SmoothFermatSpiral_2(std::vector<int> sequence)
	{
		//print the node id to the CMD console
		std::cout << "contour sequence: ";
		for (int i = 0; i < sequence.size(); i++) std::cout << sequence[i] << " ";
		std::cout << std::endl;

		Vector3d2 delta_ps_save;
		//Save the cutting point during last connecting
		//In deed, this is used to gurrentee the Fermat spiral pattern

		Vector3d2 delta_ps_0;// the forward and backwark point list along source node from source_p_0
		Vector3d2 delta_ps_1;// the forward and backwark point list along target node from source_p_1
		
		Vector3d1 source_cutting_part;//short part between two cutting points of source node
		Vector3d1 target_cutting_part;//short part between two cutting points of target node

		for (int iter = 0; iter < sequence.size() - 1; iter++)
		{
			int index_0 = sequence[iter];//one node id
			int index_1 = sequence[iter + 1];//one node id

			//find the related edge
			int edge_id = -1;
			int source_id = -1;
			int target_id = -1;
			if (!GetEdgeid(index_0, index_1, edge_id, source_id, target_id))
				return false;

			//get the possible connecting parts
			Edge *edge = &contour_edges[edge_id];//edge between the source node and the target node
			Node *source_node = &contour_nodes[edge->node_id[source_id]];//source node
			Node *target_node = &contour_nodes[edge->node_id[target_id]];//target node
			
			//compute source point
			Vector3d source_p_0;
			Vector3d source_p_1;
			if (iter == 0)
			{
				bool goon = true;
				
				/******************************************************************************/
				//try to compute from existed cutting parts of source node 
				/******************************************************************************/
				Vector3d2 existed_source_cutting_parts = GetNodeCuttingParts(source_node->id);//existed cutting parts along the source node
				Vector3d1 possible_cutting_points;
				for (int i = 0; i < existed_source_cutting_parts.size() && goon; i++)
				{
					Vector3d1 &cutting_part = existed_source_cutting_parts[i];
					possible_cutting_points.push_back(cutting_part[0]);
					possible_cutting_points.push_back(cutting_part[cutting_part.size()-1]);
				}
				
				for (int i = 0; i < possible_cutting_points.size(); i++)
				{
					Vector3d possible_source_p_0 = possible_cutting_points[i];
					Vector3d possible_source_p_1 = Circuit::FindNearestPoint(possible_source_p_0, target_node->points);
					//neareast point from source_p_0 to target node points

					//get two delta parts
					delta_ps_0 = Circuit::DeltaDEuclideanDistance(possible_source_p_0, toolpath_size, source_node->points);
					delta_ps_1 = Circuit::DeltaDEuclideanDistance(possible_source_p_1, toolpath_size, target_node->points);
					if (delta_ps_0.size() != 2 || delta_ps_1.size() != 2) return false;//delta_ps_0 and delta_ps_1 must have two elements, forward and backward
					if (CheckValidDirection(source_p_0, delta_ps_0, source_p_1, delta_ps_1)) //delta_ps_0 and delta_ps_1 must have the same directions
						std::reverse(delta_ps_1.begin(), delta_ps_1.end());
					
					source_cutting_part = delta_ps_0[0];
					target_cutting_part = delta_ps_1[1];
					std::reverse(target_cutting_part.begin(), target_cutting_part.end());

					if (CheckCutValid(edge_id, source_id, source_cutting_part) && CheckCutValid(edge_id, target_id, target_cutting_part))
					{
						goon = false;
						source_p_0 = possible_source_p_0;
						break;
					}
				}

				Vector3d2 sharing_parts_0 = edge->sharing_parts_v[source_id];//the possible connecting part between the two nodes' contour
				Vector3d2 sharing_parts_1 = edge->sharing_parts_v[target_id];//the possible connecting part between the two nodes' contour

				if ((index_0 == 1 && index_1 == 3) || (index_0 == 3 && index_1 == 1))
				{
					int upper_index = index_0;
					int lower_index = index_1;

					std::ofstream debug_file("Z:\\Pictures\\CFS\\sharingpart_" + IntString(upper_index) + "_" + IntString(lower_index) + ".obj");
					int debug_index = 1;
					for (int i = 0; i < sharing_parts_0.size(); i++)
					{
						for (int j = 0; j < sharing_parts_0[i].size() - 1; j++)
						{
							CGAL_Export_Path_Segment(debug_file, debug_index, "debug_0_" + IntString(upper_index) + "_" + IntString(lower_index), 1.0, 0.0, 0.0,
								sharing_parts_0[i][j], sharing_parts_0[i][j + 1], 0.1);
						}
					}
					for (int i = 0; i < sharing_parts_1.size(); i++)
					{
						for (int j = 0; j < sharing_parts_1[i].size() - 1; j++)
						{
							CGAL_Export_Path_Segment(debug_file, debug_index, "debug_1_" + IntString(upper_index) + "_" + IntString(lower_index), 1.0, 0.0, 0.0,
								sharing_parts_1[i][j], sharing_parts_1[i][j + 1], 0.1);
						}
					}
					debug_file.clear();
					debug_file.close();
				}


				/******************************************************************************/
				//try to compute cutting from sharing parts directly
				/******************************************************************************/
				if (goon)
				{
					
					//searching for an optimal cutting point
					//only useful for the first node of this sequence
					double max_angle = -1000.0;
					for (int i = 0; i < sharing_parts_0.size() && goon; i++)
					{
						for (int j = 1; j < sharing_parts_0[i].size() - 1; j++)
						{
							j = sharing_parts_0[i].size() / 2;

							if (!(j >= 1 && j < sharing_parts_0[i].size() - 1))
							{
								break;
							}

							Vector3d v0 = sharing_parts_0[i][j - 1];
							Vector3d v1 = sharing_parts_0[i][j];
							Vector3d v2 = sharing_parts_0[i][j + 1];
							double angle = getAngleBetween(v0 - v1, v2 - v1) / MM_PI*180.0;

							delta_ps_0 = Circuit::DeltaDEuclideanDistance(sharing_parts_0[i][j], toolpath_size, source_node->points);
							angle += Strip::GetTotalAngles(delta_ps_0[0]) / MM_PI*180.0;
							angle += Strip::GetTotalAngles(delta_ps_0[1]) / MM_PI*180.0;

							source_p_1 = Circuit::FindNearestPoint(sharing_parts_0[i][j], target_node->points);//neareast point from source_p_0 to target node points
							delta_ps_1 = Circuit::DeltaDEuclideanDistance(source_p_1, toolpath_size, target_node->points);
							angle += Strip::GetTotalAngles(delta_ps_1[0]) / MM_PI*180.0;
							angle += Strip::GetTotalAngles(delta_ps_1[1]) / MM_PI*180.0;

							if (delta_ps_0.size() != 2 || delta_ps_1.size() != 2) continue;//delta_ps_0 and delta_ps_1 must have two elements, forward and backward
							if (CheckValidDirection(sharing_parts_0[i][j], delta_ps_0, source_p_1, delta_ps_1)) //delta_ps_0 and delta_ps_1 must have the same directions
								std::reverse(delta_ps_1.begin(), delta_ps_1.end());

							if (CheckCutValid(edge_id, source_id, delta_ps_0[0]) && CheckCutValid(edge_id, target_id, delta_ps_1[1]))
							{
								if (angle > max_angle)
								{
									max_angle = angle;
									source_p_0 = sharing_parts_0[i][j];
									goon = false;
								}
							}
							break;
						}
					}

					if (goon)
					{
						std::cerr << "error.....1880" << std::endl;
						return false;
					}

					std::cout << source_p_0[0] << " " << source_p_0[1] << " " << source_p_0[2] << std::endl;
				}

				if (index_0 == 2 && index_1 == 1)
				{
					//source_p_0 = Circuit::FindNearestPoint(Vector3d(-14.189, 44.909, 1.144), source_node->points);
				}

				if (index_0 == 1 && index_1 == 2)
				{
					//source_p_0 = Circuit::FindNearestPoint(Vector3d(-14.189, 44.909, 1.144), source_node->points);
				}
			}
			else
			{
				source_p_0 = delta_ps_save[0][0];
			}

			source_p_1 = Circuit::FindNearestPoint(source_p_0, target_node->points);//neareast point from source_p_0 to target node points

			//get two delta parts
			if (iter == 0)
				delta_ps_0 = Circuit::DeltaDEuclideanDistance(source_p_0, toolpath_size, source_node->points);
			else
				delta_ps_0 = delta_ps_save;

			delta_ps_1 = Circuit::DeltaDEuclideanDistance(source_p_1, toolpath_size, target_node->points);
			if (delta_ps_0.size() != 2 || delta_ps_1.size() != 2) return false;//delta_ps_0 and delta_ps_1 must have two elements, forward and backward
			if (CheckValidDirection(source_p_0, delta_ps_0, source_p_1, delta_ps_1)) //delta_ps_0 and delta_ps_1 must have the same directions
				std::reverse(delta_ps_1.begin(), delta_ps_1.end());

			delta_ps_save = delta_ps_1;

			//two cutting parts

			source_cutting_part = delta_ps_0[0];
			target_cutting_part = delta_ps_1[1];
			std::reverse(target_cutting_part.begin(), target_cutting_part.end());

			//insert into cutting parts
			if (source_id == 0)
			{
				edge->cutting_parts_v.push_back(source_cutting_part);
				edge->cutting_parts_v.push_back(target_cutting_part);

				Vector3d1 segment_0, segment_1;
				segment_0.push_back(source_cutting_part[0]);
				segment_0.push_back(target_cutting_part[0]);
				segment_1.push_back(source_cutting_part[source_cutting_part.size() - 1]);
				segment_1.push_back(target_cutting_part[target_cutting_part.size() - 1]);

				edge->connecting_segments.push_back(segment_0);
				edge->connecting_segments.push_back(segment_1);

				if (MyGetUserName() == "debug")
				{
					//Today I meet one bug.
					//From this bug, I learn the two cutting parts must be the same;
					/////////////////////////////////////////////////////////////////////////////////////
					std::ofstream debug_file("Z:\\Pictures\\CFS\\debug_" + IntString(index_0) + "_" + IntString(index_1) + ".obj");
					int debug_index = 1;
					CGAL_Export_Path_Segment(debug_file, debug_index, "debug_" + IntString(index_0) + "_" + IntString(index_1), 1.0, 0.0, 0.0,
						segment_0[0], segment_0[1], 0.1);
					CGAL_Export_Path_Segment(debug_file, debug_index, "debug_" + IntString(index_0) + "_" + IntString(index_1), 1.0, 0.0, 0.0,
						segment_1[0], segment_1[1], 0.1);
					debug_file.clear();
					debug_file.close();
					/////////////////////////////////////////////////////////////////////////////////////
				}
			}
			else
			{
				edge->cutting_parts_v.push_back(target_cutting_part);
				edge->cutting_parts_v.push_back(source_cutting_part);

				Vector3d1 segment_0, segment_1;
				segment_0.push_back(source_cutting_part[0]);
				segment_0.push_back(target_cutting_part[0]);
				segment_1.push_back(source_cutting_part[source_cutting_part.size() - 1]);
				segment_1.push_back(target_cutting_part[target_cutting_part.size() - 1]);

				edge->connecting_segments.push_back(segment_1);
				edge->connecting_segments.push_back(segment_0);

				if (MyGetUserName() == "debug")
				{
					//Today I meet one bug.
					//From this bug, I learn the two cutting parts must be the same;
					/////////////////////////////////////////////////////////////////////////////////////
					std::ofstream debug_file("Z:\\Pictures\\CFS\\debug_" + IntString(index_0) + "_" + IntString(index_1) + ".obj");
					int debug_index = 1;
					CGAL_Export_Path_Segment(debug_file, debug_index, "debug_" + IntString(index_0) + "_" + IntString(index_1), 1.0, 0.0, 0.0,
						segment_0[0], segment_0[1], 0.1);
					CGAL_Export_Path_Segment(debug_file, debug_index, "debug_" + IntString(index_0) + "_" + IntString(index_1), 1.0, 0.0, 0.0,
						segment_1[0], segment_1[1], 0.1);
					debug_file.clear();
					debug_file.close();
					/////////////////////////////////////////////////////////////////////////////////////
				}

			}
		}
		//select one point along the outside iso-contour

		return true;
	}

	//For a local spirallable iso-contours, generate a fermat spiral with a smoothing pattern
	//sequence: the input sequence node ids of the spirallable iso-contours
	void CFSCNC::SmoothFermatSpiral(std::vector<int> sequence)
	{
		//print the node id to the CMD console
		std::cout << "contour sequence: ";
		for (int i = 0; i < sequence.size(); i++)
			std::cout << sequence[i]<<" ";
		std::cout << std::endl;

		Vector3d1 last_cutting_part;
		//Save the cutting part during last connecting
		//In deed, this is used to gurrentee the Fermat spiral pattern

		//set the inner nodes'connecting_label' of this sequence to be "spiral" 
		for (int iter = 0; iter < sequence.size() - 1; iter++)
			contour_nodes[sequence[iter]].connecting_label = "spiral";

		for (int iter = 0; iter < sequence.size() - 1; iter++)
		{
			int index_0 = sequence[iter];//one node id
			int index_1 = sequence[iter + 1];//one node id

			rerouting_points_nb = rerouting_points_nb + 4;//record the rerouting points for the paper result section

			//find the related edge
			int edge_id = -1;
			int source_id = -1;
			int target_id = -1;
			if (!GetEdgeid(index_0, index_1, edge_id, source_id, target_id))
				return;

			Edge *edge = &contour_edges[edge_id];//edge between the source node and the target node
			Node *node_0 = &contour_nodes[edge->node_id[source_id]];//source node
			Node *node_1 = &contour_nodes[edge->node_id[target_id]];//target node
			Vector3d2 sharing_parts_0 = edge->sharing_parts_v[source_id];//the possible connecting part between the two nodes' contour
			Vector3d2 sharing_parts_1 = edge->sharing_parts_v[target_id];//the possible connecting part between the two nodes' contour

			/******************************************************************************/
			//Get the cutting points of source/target nodes
			/******************************************************************************/
			Vector3d1 cutting_part_0;//short part between two cutting points of source node
			Vector3d1 cutting_part_1;//short part between two cutting points of target node
			Vector3d1 cutting_part_2;//long part between two cutting points of source node
			Vector3d1 cutting_part_3;//long part between two cutting points of target node

			bool goon = true;
			//get the cutting points from the connecting parts
			for (int i = 0; i < sharing_parts_0.size() && goon; i++)
			{
				//searching for an optimal cutting point
				//only usefull for the first node of this sequence
				int index = -1;
				double min_angle = MAXDOUBLE;
				for (int j = 1; j < sharing_parts_0[i].size() - 1 && goon; j++)
				{

					Vector3d v0 = sharing_parts_0[i][j - 1];
					Vector3d v1 = sharing_parts_0[i][j];
					Vector3d v2 = sharing_parts_0[i][j + 1];
					double angle = getAngleBetween(v0 - v1, v2 - v1) / MM_PI*180.0;
					if (angle < min_angle)
					{
						min_angle = angle;
						index = j;
					}
				}

				std::cout << "Index: " << index << std::endl;

				int j = index;//apply the optimal cutting point

				Vector3d source_p_0 = sharing_parts_0[i][j];

				if (iter > 0) source_p_0 = last_cutting_part[0];

				Vector3d2 delta_ps_0;// the forward and backwark point list along source node from source_p_0
				delta_ps_0 = Circuit::DeltaDEuclideanDistance(source_p_0, toolpath_size, node_0->points);
				Vector3d source_p_1 = Circuit::FindNearestPoint(source_p_0, node_1->points);//neareast point from source_p_0 to target node points
				Vector3d2 delta_ps_1;// the forward and backwark point list along target node from source_p_1
				delta_ps_1 = Circuit::DeltaDEuclideanDistance(source_p_1, toolpath_size, node_1->points);


				if (delta_ps_0.size() != 2 || delta_ps_1.size() != 2) return;//delta_ps_0 and delta_ps_1 must have two elements, forward and backward
				if (CheckValidDirection(source_p_0, delta_ps_0, source_p_1, delta_ps_1)) //delta_ps_0 and delta_ps_1 must have the same directions
					std::reverse(delta_ps_1.begin(), delta_ps_1.end());

				goon = false;

				if (iter == 0 || isAlmostZero(getLength(last_cutting_part[last_cutting_part.size() - 1], delta_ps_0[0][delta_ps_0[0].size() - 1])))
				{
					cutting_part_0 = delta_ps_0[0];//short part between two cutting points of source node
					cutting_part_1 = delta_ps_1[0];//short part between two cutting points of target node
				}
				else
				{
					cutting_part_0 = delta_ps_0[1];//short part between two cutting points of source node
					cutting_part_1 = delta_ps_1[1];//short part between two cutting points of target node
				}

				last_cutting_part = cutting_part_1;
				//Save the cutting part during last connecting
				//In deed, this is used to gurrentee the Fermat spiral pattern

				//get cutting_part_2
				/*************************************************************************************************/
				double part_0_par_0 = Circuit::FindNearestPointPar(cutting_part_0[0], node_0->points);
				double part_0_par_1 = Circuit::FindNearestPointPar(cutting_part_0[cutting_part_0.size() - 1], node_0->points);
				Vector3d1 vec_0 = Circuit::SelectOnePartOffset(node_0->points, part_0_par_0, part_0_par_1);
				if (isAlmostZero(abs(Strip::GetTotalLength(vec_0) - Strip::GetTotalLength(cutting_part_0))))
					cutting_part_2 = Circuit::SelectOnePartOffset(node_0->points, part_0_par_1, part_0_par_0);
				else
				{
					std::reverse(vec_0.begin(), vec_0.end());
					cutting_part_2 = vec_0;
				}
				/*************************************************************************************************/

				//get cutting_part_3
				/*************************************************************************************************/
				double part_1_par_0 = Circuit::FindNearestPointPar(cutting_part_1[0], node_1->points);
				double part_1_par_1 = Circuit::FindNearestPointPar(cutting_part_1[cutting_part_1.size() - 1], node_1->points);
				Vector3d1 vec_1 = Circuit::SelectOnePartOffset(node_1->points, part_1_par_0, part_1_par_1);
				if (isAlmostZero(abs(Strip::GetTotalLength(vec_1) - Strip::GetTotalLength(cutting_part_1))))
					cutting_part_3 = Circuit::SelectOnePartOffset(node_1->points, part_1_par_1, part_1_par_0);
				else
				{
					std::reverse(vec_1.begin(), vec_1.end());
					cutting_part_3 = vec_1;
				}
				/*************************************************************************************************/
			}
			
			/******************************************************************************/
			//Get the cutting points of source/target nodes
			/******************************************************************************/
			if (!goon)
			{
				//spiral interpolation
				Vector3d1 vector_0;
				Vector3d1 vector_1;
				Strip::Interpolation(cutting_part_0, cutting_part_1, vector_0);
				Strip::Interpolation(cutting_part_2, cutting_part_3, vector_1);

				//input the first part
				if (iter == 0)
					single_path_parts.push_back(cutting_part_2);

				single_path_parts.push_back(vector_0);
				single_path_parts.push_back(vector_1);

				//input the final part
				if (iter == sequence.size() - 2)
					single_path_parts.push_back(cutting_part_3);

				//add the cutting_parts and segments into the edges 
				if (source_id == 0)
				{
					edge->cutting_parts_v.push_back(cutting_part_0);
					edge->cutting_parts_v.push_back(cutting_part_1);

					Vector3d1 segment_0, segment_1;
					segment_0.push_back(cutting_part_0[0]);
					segment_0.push_back(cutting_part_1[0]);
					segment_1.push_back(cutting_part_0[cutting_part_0.size() - 1]);
					segment_1.push_back(cutting_part_1[cutting_part_1.size() - 1]);

					edge->connecting_segments.push_back(segment_0);
					edge->connecting_segments.push_back(segment_1);
				}
				else
				{
					edge->cutting_parts_v.push_back(cutting_part_0);
					edge->cutting_parts_v.push_back(cutting_part_1);

					Vector3d1 segment_0, segment_1;
					segment_0.push_back(cutting_part_1[0]);
					segment_0.push_back(cutting_part_0[0]);
					segment_1.push_back(cutting_part_1[cutting_part_1.size() - 1]);
					segment_1.push_back(cutting_part_0[cutting_part_0.size() - 1]);

					edge->connecting_segments.push_back(segment_1);
					edge->connecting_segments.push_back(segment_0);
				}
			}
			else
			{
				fermat_success = false;
				break;
			}
		}
	}

	/***************************************************************************************************************** /
	/*Generate fermat spirals
	//MinimalSpanningTree() must be called before calling this function.
	//The solution is based on a depth-first searching.
	//The whole process starts from a leaf node.
	//mst: input tree
	//start_node_id: the first node to be searched
	*****************************************************************************************************************/
	void CFSCNC::GenerateFermatSpirals(std::vector<int> &mst, int start_node_id)
	{
		////std::vector<std::vector<int>> node_sequences;
		////std::vector<int> node_degree = Tree::ComputeNodeDegree(contour_nodes.size(), mst);
		////
		////Tree::GetNodeSequence(contour_nodes, mst, start_node_id, node_sequences);

		////for (int i = 0; i < node_sequences.size(); i++)
		////{
		////	FermatSpiral(node_sequences[i]);
		////	if (node_degree[node_sequences[i][node_sequences[i].size() - 1]]>2)
		////		GenerateFermatSpirals(mst, node_sequences[i][node_sequences[i].size() - 1]);
		////}

		std::vector<std::vector<int>> node_sequences;
		Tree::GetAllSequences(contour_nodes, mst, node_sequences);

		for (int i = 0; i < node_sequences.size(); i++)
		{
			FermatSpiral(node_sequences[i]);
		}
	}

	//Generate fermat spiral with hybrid methods, smoothing Fermat spiral method and bruteforce method
	bool CFSCNC::GenerateFermatSpirals_HybridMethod(std::vector<int> &mst)
	{
		std::vector<std::vector<int>> node_sequences;
		Tree::GetAllSequences(contour_nodes, mst, node_sequences);

		for (int i = 0; i < node_sequences.size(); i++)
		{
			int degree_0 = contour_nodes[node_sequences[i][0]].degree;
			int degree_1 = contour_nodes[node_sequences[i][node_sequences[i].size() - 1]].degree;

			if (degree_0 == 1 && degree_1 > 2)
				std::reverse(node_sequences[i].begin(), node_sequences[i].end());
			
			if (!SmoothFermatSpiral_2(node_sequences[i]))
				return false;

		}
		return true;
	}
	
	//For a local spirallable iso-contours, generate a Fermat spiral with a brute-force pattern
	//sequence: the input sequence node ids of the spirallable iso-contours
	void CFSCNC::FermatSpiral(std::vector<int> sequence)
	{
		//print the node id to the CMD console
		std::cout << "contour sequence: ";
		for (int i = 0; i < sequence.size(); i++)
			std::cout << sequence[i] << " ";
		std::cout << std::endl;

		//Fermat spiral generation by bruteforce method
		//The basic process is connecting two nodes following the sequence order
		for (int i = 0; i < sequence.size() - 1; i++)
		{
			int index_0 = sequence[i];
			int index_1 = sequence[i + 1];

			if (!ConnectTwoContours(index_0, index_1))
			{
				int edge_id = -1;
				if (GetEdgeid(index_0, index_1, edge_id))
				{
					std::cout << "False edge id: " << edge_id << std::endl;
				}
					std::cout << "False connecting: " << index_0 << " " << index_1 << std::endl;
				fermat_success = false;
			}
		}

		//Fermat spiral generation by smoothing fermat spiral method
		//SmoothFermatSpiral(sequence);
	}


	//connect two neighbour iso-contours
	bool CFSCNC::ConnectTwoContours(int index_0, int index_1)
	{
		rerouting_points_nb = rerouting_points_nb+4; //record the rerouting points for the paper result section

		//find the related edge
		int edge_id = -1;
		int source_id = -1;
		int target_id = -1;
		if (!GetEdgeid(index_0, index_1, edge_id, source_id, target_id))
			return false;

		//find the cutting point
		Edge *edge = &contour_edges[edge_id];//edge between the source node and the target node
		Node *source_node = &contour_nodes[edge->node_id[source_id]];//source node
		Node *target_node = &contour_nodes[edge->node_id[target_id]];//target node
		Vector3d2 parts_0 = edge->sharing_parts_v[source_id];//the possible connecting part between the two nodes' contour
		Vector3d2 parts_1 = edge->sharing_parts_v[target_id];//the possible connecting part between the two nodes' contour

		Vector3d1 source_cutting_part;//short part between two cutting points of source node
		Vector3d1 target_cutting_part;//short part between two cutting points of target node

		bool goon = true;//check whether the computing process should be continuous

		/******************************************************************************/
		//try to compute from existed cutting parts of source node 
		/******************************************************************************/
		Vector3d2 existed_source_cutting_parts = GetNodeCuttingParts(source_node->id);//existed cutting parts along the source node
		Vector3d2 existed_target_cutting_parts = GetNodeCuttingParts(target_node->id);//exist cutting parts along the target node
		for (int i = 0; i < existed_source_cutting_parts.size() && goon; i++)
		{
			Vector3d1 &cutting_part = existed_source_cutting_parts[i];//one existed cutting part

			Vector3d source_p_0;//cutting point of source node
			Vector3d source_p_1;//cutting point of target node
			/*****************************************************/
			double par_0;//parameter of the first point of cutting_part along node points
			double par_1;//parameter of the final point of cutting_part along node points
			par_0 = Circuit::FindNearestPointPar(cutting_part[0], source_node->points);
			par_1 = Circuit::FindNearestPointPar(cutting_part[cutting_part.size() - 1], source_node->points);
			double length_0 = Strip::GetTotalLength(Circuit::SelectOnePartOffset(source_node->points, par_0, par_1));//length from par_0 to par_1
			double length_1 = Strip::GetTotalLength(Circuit::SelectOnePartOffset(source_node->points, par_1, par_0));//length from par_1 to par_0
			length_0 > length_1 ? source_p_0 = cutting_part[0] : source_p_0 = cutting_part[cutting_part.size() - 1];
			source_p_1 = Circuit::FindNearestPoint(source_p_0, target_node->points);//neareast point from source_p_0 to target node points
			/*****************************************************/

			Vector3d2 delta_ps_0; //the forward and backwark point list along source node from source_p_0
			Vector3d2 delta_ps_1;//the forward and backward point list along target node from source_p_1
			/*****************************************************/
			delta_ps_0 = Circuit::DeltaDEuclideanDistance(source_p_0, toolpath_size, source_node->points);
			delta_ps_1 = Circuit::DeltaDEuclideanDistance(source_p_1, toolpath_size, target_node->points);
			if (delta_ps_0.size() != 2 || delta_ps_1.size() != 2) return false;//delta_ps_0 and delta_ps_1 must have two elements, forward and backward
			if (CheckValidDirection(source_p_0, delta_ps_0, source_p_1, delta_ps_1))
				std::reverse(delta_ps_1.begin(), delta_ps_1.end());//delta_ps_0 and delta_ps_1 must have the same directions
			/*****************************************************/

			//get source_cutting_part and target_cutting_part
			/*****************************************************/
			if (CheckCutValid(edge_id, source_id, delta_ps_0[0]) && CheckCutValid(edge_id, target_id, delta_ps_1[0]))
			{
				goon = false;
				source_cutting_part = delta_ps_0[0];
				target_cutting_part = delta_ps_1[0];
				break;
			}

			if (CheckCutValid(edge_id, source_id, delta_ps_0[1]) && CheckCutValid(edge_id, target_id, delta_ps_1[1]))
			{
				goon = false;
				source_cutting_part = delta_ps_0[1];
				target_cutting_part = delta_ps_1[1];
				break;
			}
			/*****************************************************/
		}

		/******************************************************************************/
		//try to compute cutting parts by brute-force connecting 
		/******************************************************************************/
		for (int i = 0; i < parts_0.size()&&goon; i++)
		{
			for (int j = 1; j < parts_0[i].size()-1 && goon; j++)
			{
				Vector3d v0 = parts_0[i][j - 1];
				Vector3d v1 = parts_0[i][j];
				Vector3d v2 = parts_0[i][j + 1];
				double angle = getAngleBetween(v0-v1,v2-v1);
				if (angle < MM_PI *2.0/ 3.0) continue; //try to break at a flat curve

				Vector3d source_p_0 = parts_0[i][j];//cutting point of source node
				Vector3d source_p_1 = Circuit::FindNearestPoint(source_p_0, target_node->points);//cutting point of target node

				Vector3d2 delta_ps_0; //the forward and backwark point list along source node from source_p_0
				Vector3d2 delta_ps_1;//the forward and backward point list along target node from source_p_1
				/*****************************************************/
				delta_ps_0 = Circuit::DeltaDEuclideanDistance(source_p_0, toolpath_size, source_node->points);
				delta_ps_1 = Circuit::DeltaDEuclideanDistance(source_p_1, toolpath_size, target_node->points);
				if (delta_ps_0.size() != 2 || delta_ps_1.size() != 2) return false;
				if (CheckValidDirection(source_p_0, delta_ps_0, source_p_1, delta_ps_1)) std::reverse(delta_ps_1.begin(), delta_ps_1.end());
				/*****************************************************/

				if (CheckCutValid(edge_id, source_id, delta_ps_0[0]) && CheckCutValid(edge_id, target_id, delta_ps_1[0]))
				{
					goon = false;
					source_cutting_part = delta_ps_0[0];
					target_cutting_part = delta_ps_1[0];
					break;
				}

				if (CheckCutValid(edge_id, source_id, delta_ps_0[1]) && CheckCutValid(edge_id, target_id, delta_ps_1[1]))
				{
					goon = false;
					source_cutting_part = delta_ps_0[1];
					target_cutting_part = delta_ps_1[1];
					break;
				}
			}
		}

		if (!goon)
		{
			if (source_id == 0)
			{
				edge->cutting_parts_v.push_back(source_cutting_part);
				edge->cutting_parts_v.push_back(target_cutting_part);

				Vector3d1 segment_0, segment_1;
				segment_0.push_back(source_cutting_part[0]);
				segment_0.push_back(target_cutting_part[0]);
				segment_1.push_back(source_cutting_part[source_cutting_part.size() - 1]);
				segment_1.push_back(target_cutting_part[target_cutting_part.size() - 1]);

				edge->connecting_segments.push_back(segment_0);
				edge->connecting_segments.push_back(segment_1);
			}
			else
			{
				edge->cutting_parts_v.push_back(target_cutting_part);
				edge->cutting_parts_v.push_back(source_cutting_part);

				Vector3d1 segment_0, segment_1;
				segment_0.push_back(source_cutting_part[0]);
				segment_0.push_back(target_cutting_part[0]);
				segment_1.push_back(source_cutting_part[source_cutting_part.size() - 1]);
				segment_1.push_back(target_cutting_part[target_cutting_part.size() - 1]);

				edge->connecting_segments.push_back(segment_1);
				edge->connecting_segments.push_back(segment_0);
			}

			//geodesice computing 
			//CGAL_Shortest_Geodesic_Path(input_path, cutting_point_0, cutting_point_2, segment_0);
			//CGAL_Shortest_Geodesic_Path(input_path, cutting_point_1, cutting_point_3, segment_1);
		}
		/////////////////////////////////////////////////////////////////////////////////////


		return !goon;
	}
	/*****************************************************************************************************************/

	/*
	//Straight forward method to get the single path
	****************************************************************************************************/
	void CFSCNC::GetOneSinglePath()
	{
		//Get all cutting parts from node's points
		//Then push all of these cutting parts into "parts"
		for (int i = 0; i < contour_nodes.size(); i++)
		{
			Node *node = &contour_nodes[i];
		
			if (node->connecting_label == "bruteforce")
			{
				node->cutting_parts = GetNodeCuttingPartsPar(node->id);//get all of the cutting points' parameters
				node->GetCuttingPartsOrders();//compute the order of the cutting points' parameters
				int parts_number = node->cutting_parts.size();
				for (int j = 0; j < parts_number; j++)
				{
					int index_0 = node->GetIndex(j);
					int index_1 = node->GetIndex((j + 1) % parts_number);
					single_path_parts.push_back(Circuit::SelectOnePartOffset(node->points, node->cutting_parts[index_1][1], node->cutting_parts[index_0][0]));
				}
			}
		}

		//Get all connecting edge
		//Then push all of these connecting edges into "parts"
		for (int i = 0; i < contour_edges.size(); i++)
		{
			if (contour_nodes[contour_edges[i].node_id[0]].connecting_label == "bruteforce" &&
				contour_nodes[contour_edges[i].node_id[1]].connecting_label == "bruteforce")
			{
				Edge &edge = contour_edges[i];
				for (int j = 0; j < contour_edges[i].connecting_segments.size(); j++)
					single_path_parts.push_back(contour_edges[i].connecting_segments[j]);
			}
		}

//		OutputStrips(input_path + "single_path_parts.obj", single_path_parts);

		//add all parts
		for (int i = 0; i < single_path_parts.size(); i++)
		{
			Part part;
			part.points.assign(single_path_parts[i].begin(), single_path_parts[i].end());

			bool goon=true;
			if (part.points.size() == 1)
			{
				goon = false;
			}
			else
			{
				if (Math::GetLength(part.points.back() - part.points.front())<0.1)
					goon = false;
			}

			if (goon)
			parts.push_back(part);
		}

		//construct the edges
		for (int i = 0; i < parts.size(); i++)
		{
			if (parts[i].points.size() == 1)
			{
				Vector3d p0 = parts[i].points[0];
				for (int j = 0; j < parts.size(); j++)
				{
					if (i == j)
						continue;
					if (parts[j].points.size() == 1)
					{
						Vector3d p1 = parts[j].points[0];
						
						if (CGAL_3D_Distance_Point_Point(p0, p1)<0.1)
						//if (isAlmostZero(CGAL_3D_Distance_Point_Point(p0, p1)))
						{
							merge_edge.push_back(i);
							merge_edge.push_back(0);
							merge_edge.push_back(j);
							merge_edge.push_back(0);
						}
					}
					else
					{
						Vector3d p1 = parts[j].points[0];
						Vector3d p2 = parts[j].points[parts[j].points.size() - 1];

						if (CGAL_3D_Distance_Point_Point(p0, p1)<0.1)
						//if (isAlmostZero(CGAL_3D_Distance_Point_Point(p0, p1)))
						{
							merge_edge.push_back(i);
							merge_edge.push_back(0);
							merge_edge.push_back(j);
							merge_edge.push_back(0);
						}

						if (CGAL_3D_Distance_Point_Point(p0, p2)<0.1)
						//if (isAlmostZero(CGAL_3D_Distance_Point_Point(p0, p2)))
						{
							merge_edge.push_back(i);
							merge_edge.push_back(0);
							merge_edge.push_back(j);
							merge_edge.push_back(1);
						}
					}
				}
			}
			else
			{
				Vector3d p0 = parts[i].points[0];
				Vector3d p1 = parts[i].points[parts[i].points.size() - 1];

				for (int j = 0; j < parts.size(); j++)
				{
					if (i == j)continue;

					if (parts[j].points.size() == 1)
					{
						Vector3d p3 = parts[j].points[0];

						if (CGAL_3D_Distance_Point_Point(p0, p3)<0.1)
						//if (isAlmostZero(CGAL_3D_Distance_Point_Point(p0, p3)))
						{
							merge_edge.push_back(i);
							merge_edge.push_back(0);
							merge_edge.push_back(j);
							merge_edge.push_back(0);
						}

						if (CGAL_3D_Distance_Point_Point(p1, p3)<0.1)
						//if (isAlmostZero(CGAL_3D_Distance_Point_Point(p1, p3)))
						{
							merge_edge.push_back(i);
							merge_edge.push_back(1);
							merge_edge.push_back(j);
							merge_edge.push_back(0);
						}
					}
					else
					{
						Vector3d p2 = parts[j].points[0];
						Vector3d p3 = parts[j].points[parts[j].points.size() - 1];

						double d0 = CGAL_3D_Distance_Point_Point(p0, p2);
						double d1 = CGAL_3D_Distance_Point_Point(p1, p2);
						double d2 = CGAL_3D_Distance_Point_Point(p0, p3);
						double d3 = CGAL_3D_Distance_Point_Point(p1, p3);

						if ((i == 55 && j == 63) || (i == 63 && j == 55))
						{
							int dsad = 0;
						}

						if (CGAL_3D_Distance_Point_Point(p0, p2)<0.1)
						//if (isAlmostZero(CGAL_3D_Distance_Point_Point(p0, p2)))
						{
							merge_edge.push_back(i);
							merge_edge.push_back(0);
							merge_edge.push_back(j);
							merge_edge.push_back(0);
						}
						if (CGAL_3D_Distance_Point_Point(p1, p2)<0.1)
						//if (isAlmostZero(CGAL_3D_Distance_Point_Point(p1, p2)))
						{
							merge_edge.push_back(i);
							merge_edge.push_back(1);
							merge_edge.push_back(j);
							merge_edge.push_back(0);

						}

						if (CGAL_3D_Distance_Point_Point(p0, p3)<0.1)
						//if (isAlmostZero(CGAL_3D_Distance_Point_Point(p0, p3)))
						{
							merge_edge.push_back(i);
							merge_edge.push_back(0);
							merge_edge.push_back(j);
							merge_edge.push_back(1);

						}

						if (CGAL_3D_Distance_Point_Point(p1, p3)<0.1)
						//if (isAlmostZero(CGAL_3D_Distance_Point_Point(p1, p3)))
						{
							merge_edge.push_back(i);
							merge_edge.push_back(1);
							merge_edge.push_back(j);
							merge_edge.push_back(1);
						}
					}
				}
			}
		}

		//remove repeated edges
		for (int i = 0; i < merge_edge.size(); i = i + 4)
		{
			int a = merge_edge[i];
			int a1 = merge_edge[i+1];
			int a2 = merge_edge[i+2];
			int a3 = merge_edge[i+3];

			bool b = true;
			for (int j = 0; j < removed_merge_edge.size(); j = j + 4)
			{

				int a11 = removed_merge_edge[j];
				int a111 = removed_merge_edge[j + 1];
				int a2111 = removed_merge_edge[j + 2];
				int a3111 = removed_merge_edge[j + 3];

				if (merge_edge[i] == removed_merge_edge[j] &&
					merge_edge[i + 1] == removed_merge_edge[j + 1] &&
					merge_edge[i + 2] == removed_merge_edge[j + 2] &&
					merge_edge[i + 3] == removed_merge_edge[j + 3])
				{
					b = false;
					break;
				}

				if (merge_edge[i+2] == removed_merge_edge[j] &&
					merge_edge[i + 3] == removed_merge_edge[j + 1] &&
					merge_edge[i + 0] == removed_merge_edge[j + 2] &&
					merge_edge[i + 1] == removed_merge_edge[j + 3])
				{
					b = false;
					break;
				}
			}

			if (b)
			{
				removed_merge_edge.push_back(merge_edge[i]);
				removed_merge_edge.push_back(merge_edge[i + 1]);
				removed_merge_edge.push_back(merge_edge[i + 2]);
				removed_merge_edge.push_back(merge_edge[i + 3]);
			}
		}
	
		for (int i = 0; i < removed_merge_edge.size(); i = i + 4)
		{
			used.push_back(false);
		}


		//output the gml
		/**************************************************/
		std::vector<int> mst;
		for (int i = 0; i < removed_merge_edge.size(); i = i + 4)
		{
			int a = removed_merge_edge[i];
			int a1 = removed_merge_edge[i + 1];
			int a2 = removed_merge_edge[i + 2];
			int a3 = removed_merge_edge[i + 3];
			mst.push_back(a);
			mst.push_back(a2);
		}

		Output_tree(parts.size(), mst, "Z:\\Pictures\\CFS\\mst.gml");
		/**************************************************/

		//single_path.assign(parts[0].points.begin(), parts[0].points.end());
		ConnectAllParts(0,0);

	}

	/****************************************************************************************************/
	bool CFSCNC::GetOneSinglePath_0()
	{
		//construct the edges
		for (int i = 0; i < single_path_parts.size(); i++)
		{
			Part part;
			part.points.assign(single_path_parts[i].begin(), single_path_parts[i].end());
			parts.push_back(part);
		}

		for (int i = 0; i < parts.size(); i++)
		{
			if (parts[i].points.size() == 1)
			{
				Vector3d p0 = parts[i].points[0];
				for (int j = 0; j < parts.size(); j++)
				{
					if (i == j)
						continue;

					if (parts[j].points.size() == 1)
					{
						Vector3d p1 = parts[j].points[0];
						if (isAlmostZero(CGAL_3D_Distance_Point_Point(p0, p1)))
						{
							merge_edge.push_back(i);
							merge_edge.push_back(0);
							merge_edge.push_back(j);
							merge_edge.push_back(0);
						}
					}
					else
					{
						Vector3d p1 = parts[j].points[0];
						Vector3d p2 = parts[j].points[parts[j].points.size() - 1];

						if (isAlmostZero(CGAL_3D_Distance_Point_Point(p0, p1)))
						{
							merge_edge.push_back(i);
							merge_edge.push_back(0);
							merge_edge.push_back(j);
							merge_edge.push_back(0);
						}
						if (isAlmostZero(CGAL_3D_Distance_Point_Point(p0, p2)))
						{
							merge_edge.push_back(i);
							merge_edge.push_back(0);
							merge_edge.push_back(j);
							merge_edge.push_back(1);
						}
					}
				}
			}
			else
			{
				Vector3d p0 = parts[i].points[0];
				Vector3d p1 = parts[i].points[parts[i].points.size() - 1];

				for (int j = 0; j < parts.size(); j++)
				{
					if (i == j)continue;

					if (parts[j].points.size() == 1)
					{
						Vector3d p3 = parts[j].points[0];
						if (isAlmostZero(CGAL_3D_Distance_Point_Point(p0, p3)))
						{
							merge_edge.push_back(i);
							merge_edge.push_back(0);
							merge_edge.push_back(j);
							merge_edge.push_back(0);
						}
						if (isAlmostZero(CGAL_3D_Distance_Point_Point(p1, p3)))
						{
							merge_edge.push_back(i);
							merge_edge.push_back(1);
							merge_edge.push_back(j);
							merge_edge.push_back(0);
						}
					}
					else
					{
						Vector3d p2 = parts[j].points[0];
						Vector3d p3 = parts[j].points[parts[j].points.size() - 1];

						if (isAlmostZero(CGAL_3D_Distance_Point_Point(p0, p2)))
						{
							merge_edge.push_back(i);
							merge_edge.push_back(0);
							merge_edge.push_back(j);
							merge_edge.push_back(0);
						}
						if (isAlmostZero(CGAL_3D_Distance_Point_Point(p1, p2)))
						{
							merge_edge.push_back(i);
							merge_edge.push_back(1);
							merge_edge.push_back(j);
							merge_edge.push_back(0);

						}
						if (isAlmostZero(CGAL_3D_Distance_Point_Point(p0, p3)))
						{
							merge_edge.push_back(i);
							merge_edge.push_back(0);
							merge_edge.push_back(j);
							merge_edge.push_back(1);

						}
						if (isAlmostZero(CGAL_3D_Distance_Point_Point(p1, p3)))
						{
							merge_edge.push_back(i);
							merge_edge.push_back(1);
							merge_edge.push_back(j);
							merge_edge.push_back(1);
						}
					}
				}
			}
		}

		for (int i = 0; i < merge_edge.size(); i = i + 4)
		{
			int a = merge_edge[i];
			int a1 = merge_edge[i + 1];
			int a2 = merge_edge[i + 2];
			int a3 = merge_edge[i + 3];

			bool b = true;
			for (int j = 0; j < removed_merge_edge.size(); j = j + 4)
			{
				if (merge_edge[i] == removed_merge_edge[j] &&
					merge_edge[i + 1] == removed_merge_edge[j + 1] &&
					merge_edge[i + 2] == removed_merge_edge[j + 2] &&
					merge_edge[i + 3] == removed_merge_edge[j + 3])
				{
					b = false;
					break;
				}

				if (merge_edge[i + 2] == removed_merge_edge[j] &&
					merge_edge[i + 3] == removed_merge_edge[j + 1] &&
					merge_edge[i + 0] == removed_merge_edge[j + 2] &&
					merge_edge[i + 1] == removed_merge_edge[j + 3])
				{
					b = false;
					break;
				}
			}

			if (b)
			{
				removed_merge_edge.push_back(merge_edge[i]);
				removed_merge_edge.push_back(merge_edge[i + 1]);
				removed_merge_edge.push_back(merge_edge[i + 2]);
				removed_merge_edge.push_back(merge_edge[i + 3]);
			}
		}

		for (int i = 0; i < removed_merge_edge.size(); i = i + 4)
		{
			used.push_back(false);
		}

		//single_path.assign(parts[0].points.begin(), parts[0].points.end());
		return ConnectAllParts(0, 0);
	}

	bool CFSCNC::ConnectAllParts(int start_part, int start_part_end)
	{
		int next_part = -10;
		int next_part_end = -1;

		for (int iter_nb = 0; iter_nb < parts.size(); iter_nb++)
		{
			next_part = -10;
			next_part_end = -1;

			for (int i = 0; i < removed_merge_edge.size(); i = i + 4)
			{
				if (used[i / 4])
					continue;

				int part_0 = removed_merge_edge[i];
				int part_end_0 = removed_merge_edge[i + 1];
				int part_1 = removed_merge_edge[i + 2];
				int part_end_1 = removed_merge_edge[i + 3];

				if (part_0 == start_part&&part_end_0 == start_part_end)
				{
					next_part = part_1;
					next_part_end = part_end_1;
					used[i / 4] = true;
					break;
				}
				if (part_1 == start_part&&part_end_1 == start_part_end)
				{
					next_part = part_0;
					next_part_end = part_end_0;
					used[i / 4] = true;
					break;
				}
			}

			if (next_part != -10)
			{
				//if (next_part == 2)
				//{
				//	Vector3d1 vector_1 = parts[next_part].points;
				//	double total_length = Strip::GetTotalLength(vector_1);
				//	Vector3d1 vecs;
				//	bool b = false;
				//	for (int i = 0; i < vector_1.size(); i++)
				//	{
				//		if (b || Strip::GetTotalLength(vecs) / total_length>0.5)
				//		{
				//			if (!b)
				//				Vector3d1().swap(vecs);
				//			vecs.push_back(vector_1[i]);
				//			b = true;
				//		}
				//		else
				//		{
				//			vecs.push_back(vector_1[i]);
				//		}
				//	}
				//	Vector3d1().swap(parts[next_part].points);
				//	parts[next_part].points = vecs;
				//}

				if (next_part_end == 0)
				{
					for (int i = 0; i < parts[next_part].points.size(); i++)
					{
						single_path.push_back(parts[next_part].points[i]);
					}
				}
				else
				{
					for (int i = parts[next_part].points.size() - 1; i >= 0; i--)
					{
						single_path.push_back(parts[next_part].points[i]);
					}
				}

				if (iter_nb == parts.size() - 1)
				{
					for (int i = 0; i < parts[next_part].points.size(); i++)
						single_path_fixed_label.push_back(true);
				}
				else
				{
					if (iter_nb == 0)
					{
						for (int i = 0; i < parts[next_part].points.size(); i++)
						{
							if (i<parts[next_part].points.size()/4)
								single_path_fixed_label.push_back(true);
							else
								single_path_fixed_label.push_back(false);
						}
					}
					else
					{
						for (int i = 0; i < parts[next_part].points.size(); i++)
							single_path_fixed_label.push_back(false);
					}
				}

				start_part = next_part;

				if (parts[next_part].points.size() == 1)
					start_part_end = next_part_end;
				else
					start_part_end = 1 - next_part_end;
			}
			else
			{
				return false;
			}
		}

		return true;
	}
	/*****************************************************************************************************/

	/*
	Smart method to get the single path
	****************************************************************************************************
	void CFSCNC::GetOneSinglePath()
	{
		std::vector<Part> parts;

		for (int i = 0; i < contour_nodes.size(); i++)
		{
			contour_nodes[i].cutting_parts = GetNodeCuttingPartsPar(contour_nodes[i].id);
			contour_nodes[i].GetCuttingPartsOrders();
			
			int parts_number = contour_nodes[i].cutting_parts.size();
			
			for (int j = 0; j < parts_number; j++)
			{
				int index_0 = contour_nodes[i].GetIndex(j);
				int index_1 = contour_nodes[i].GetIndex((j+1)%parts_number);

				Part part;
				part.node_id = contour_nodes[i].id;
				//here here
				part.points = Circuit::SelectOnePartOffset(contour_nodes[i].points, contour_nodes[i].cutting_parts[index_1][0], contour_nodes[i].cutting_parts[index_0][1]);

				part.connecting_edges_id.push_back(contour_nodes[i].edges_id[index_1]);
				part.connecting_edges_id.push_back(contour_nodes[i].edges_id[index_0]);

				part.connecting_edges_index.push_back(contour_nodes[i].edges_index[index_1]);
				part.connecting_edges_index.push_back(contour_nodes[i].edges_index[index_0]);
				parts.push_back(part);
			}
		}

		for (int i = 0; i < parts.size(); i++)
		{
			Part *part = &parts[i];
			//working for the first end point
			int edge_id = part->connecting_edges_id[0];
			int edge_index =1- part->connecting_edges_index[0];

			//hard to think....
			//working for the second end point
		}
	}
	*/

	void CFSCNC::UnifiedContourDirection(double toolpath_size, Vector3d3 &offsetses, Vector3d2 &offsets)
	{
		DWORD start_time = GetTickCount();
		Tree::UnifiedContourDirection(toolpath_size, offsetses, offsets);
		DWORD end_time = GetTickCount();
		std::cout << "[TIME] UnifiedContourDirection: " << (end_time - start_time) / 1000.0 << std::endl;
	}

	bool CFSCNC::CheckValidDirection(Vector3d source_p_0, Vector3d2 delta_ps_0, Vector3d source_p_1, Vector3d2 delta_ps_1)
	{
		//double angle = getAngleBetween(delta_ps_0[0][delta_ps_0[0].size() - 1] - source_p_0, delta_ps_1[0][delta_ps_1[0].size() - 1] - source_p_1);

		double angle_0 = getAngleBetween(delta_ps_0[0][delta_ps_0[0].size() - 1] - source_p_0, delta_ps_1[0][delta_ps_1[0].size() - 1] - source_p_1);
		double angle_1 = getAngleBetween(delta_ps_0[0][delta_ps_0[0].size() - 1] - source_p_0, delta_ps_1[1][delta_ps_1[1].size() - 1] - source_p_1);

		if (angle_0 < angle_1)
			return false;
		else
		{
			return true;
		}

		/*
		if (angle < 3.141592653 / 2.0)
			return false;
		else
			return true;
		*/
	}



	bool CFSCNC::CheckCutValid(int edge_index, int index, Vector3d1 part)
	{
		Node *node = &contour_nodes[contour_edges[edge_index].node_id[index]];

		bool b = true;

		//insert a middle point
		if (part.size() == 2)
		{
			Vector3d p0 = part[0];
			Vector3d p1 = part[1];
			Vector3d p = p0 + p1;

			p[0] = p[0] / 2.0;
			p[1] = p[1] / 2.0;
			p[2] = p[2] / 2.0;

			part.clear();
			Vector3d1().swap(part);
			part.push_back(p0);
			part.push_back(p);
			part.push_back(p1);
		}


		//checking whether the part is inside the connecting parts
		for (int i = 0; i < part.size(); i++)
		{
			if (!isAlmostZero(Strip::Distance(part[i], contour_edges[edge_index].sharing_parts_v[index])))
			{
				b = false;
				break;
			}
		}

		//checking whether intersection with other cutting parts
		Vector3d2 cutting_parts = GetNodeCuttingParts(node->id);

		if (cutting_parts.size() > 0)
		{
			for (int i = 0; i < part.size(); i++)
			{
				if (isAlmostZero(Strip::Distance(part[i], cutting_parts)))
				{ 
					double min_d = 100000.0;
					for (int j = 0; j < cutting_parts.size(); j++)
					{
						double d_0 = Strip::Distance(part[i], cutting_parts[j][0]);
						double d_1 = Strip::Distance(part[i], cutting_parts[j][cutting_parts[j].size()-1]);
						min_d = min(min_d, d_0);
						min_d = min(min_d, d_1);
					}
					if (!isAlmostZero(min_d))
					{
						b = false;
						break;
					}
				}
			}
		}
		
		return b;
	}
	bool CFSCNC::CheckCutValid(int edge_index, int index, double source, double target)
	{
		Node *node = &contour_nodes[contour_edges[edge_index].node_id[index]];

		Vector3d1 part = Circuit::SelectOnePartOffset(node->points, source, target);
		bool b = true;

		//checking whether the part is inside the connecting parts
		for (int i = 0; i < part.size(); i++)
		{
			if (!isAlmostZero(Strip::Distance(part[i], contour_edges[edge_index].sharing_parts_v[index])))
			{
				b = false;
				break;
			}
		}
		//checking whether itersection with other cutting parts
		Vector3d2 cutting_parts = GetNodeCuttingParts(node->id);

		if (cutting_parts.size()>0)
			for (int i = 0; i < part.size(); i++)
			{
				if (isAlmostZero(Strip::Distance(part[i], cutting_parts)))
				{
					b = false;
					break;
				}
			}

		return b;
	}

	//For a node, get all of the cutting parts' parameters along the node points
	//Each element of the output of this function has two doubles.
	//We must adjust the directions of each two parameter pairs.
	std::vector<std::vector<double>> CFSCNC::GetNodeCuttingPartsPar(int node_id)
	{
		Node *node = &contour_nodes[node_id];
		//existed cutting parts along the node
		Vector3d2 cutting_parts = GetNodeCuttingParts(node_id);
		std::vector<std::vector<double>> cutting_parts_par;

		//////////////////////////////////////////////////////////////////////////////////////////////
		for (int i = 0; i < cutting_parts.size(); i++)
		{
			Vector3d p0 = cutting_parts[i][0];
			Vector3d p1 = cutting_parts[i][cutting_parts[i].size() - 1];

			double par_0 = Circuit::FindNearestPointPar(p0, node->points);
			double par_1 = Circuit::FindNearestPointPar(p1, node->points);

			double length0 = Strip::GetTotalLength(cutting_parts[i]);
			double length1 = Strip::GetTotalLength(Circuit::SelectOnePartOffset(node->points, par_0, par_1));

			if (isAlmostZero(abs(length0 - length1)))
			{
				std::vector<double> part_par;
				part_par.push_back(par_0);
				part_par.push_back(par_1);
				cutting_parts_par.push_back(part_par);
			}
			else
			{
				std::vector<double> part_par;
				part_par.push_back(par_1);
				part_par.push_back(par_0);
				cutting_parts_par.push_back(part_par);
			}
		}
		//////////////////////////////////////////////////////////////////////////////////////////////
		return 	cutting_parts_par;
	}


	//void CGAL_2D_Polygon_Offsets(std::vector<std::vector<double>> xs, std::vector<std::vector<double>> ys, double d,
	//	std::vector<std::vector<double>> &offsets_xs, std::vector<std::vector<double>> &offsets_ys);


	void CFSCNC::BuildOffsets(Vector3d2 &input_boundaries,
		Vector3d3 &offsetses, Vector3d2 &offsets)
	{
		//void CGAL_2D_Polygon_Offsets(Vector2d2 boundaries, double d, Vector2d2 &offsets);

		//build data structs from 3d to 2d
		Vector2d2 boundaries_2d;
		for (int i = 0; i < input_boundaries.size(); i++)
		{
			Vector2d1 one_2d_boundary;
			for (int j = 0; j < input_boundaries[i].size(); j++)
				one_2d_boundary.push_back(Vector2d(input_boundaries[i][j][0], input_boundaries[i][j][1]));

			//bool CGAL_2D_Polygon_Is_Clockwise_Oriented(Vector2d1 &ps);
			bool b = CGAL_2D_Polygon_Is_Clockwise_Oriented(one_2d_boundary);

			if (i == 0)
			{
				if (b)
					std::reverse(one_2d_boundary.begin(), one_2d_boundary.end());
			}
			else
			{
				if (!b)
					std::reverse(one_2d_boundary.begin(), one_2d_boundary.end());
			}
			boundaries_2d.push_back(one_2d_boundary);
			Vector2d1().swap(one_2d_boundary);
		}

		double d = 0.0;
		Vector2d2 offsets_2d;

		auto OFFSETS = [&](Vector2d2 boundaries, double d, Vector2d2 &offsets){
			std::vector<std::vector<double>> xs, ys;
			for (auto &boundary : boundaries)
			{
				std::vector<double> x, y;
				for (auto &p : boundary)
				{
					x.emplace_back(p[0]);
					y.emplace_back(p[1]);
				}
				xs.emplace_back(x);
				ys.emplace_back(y);
			}

			std::vector<std::vector<double>> offsets_xs, offsets_ys;
			CGAL_2D_Polygon_Offsets(xs, ys, d, offsets_xs, offsets_ys);

			for (int i = 0; i < offsets_xs.size(); i++)
			{
				Vector2d1 offset;
				for (int j = 0; j < offsets_xs[i].size(); j++)
					offset.emplace_back(Vector2d(offsets_xs[i][j], offsets_ys[i][j]));
				offsets.emplace_back(offset);
			}
		};

		//compute offsets with CGALPACKAGE
		//OFFSETS(boundaries_2d, d, offsets_2d);

		offsets_2d = boundaries_2d;

		bool goon = true;

		while (offsets_2d.size() != 0)
		{
			Vector3d2 one_layer_3d_boundaries;
			for (int i = 0; i < offsets_2d.size(); i++)
			{
				//if (CGAL_2D_Polygon_Is_Clockwise_Oriented(offsets_2d[i]))
					//std::reverse(offsets_2d[i].begin(), offsets_2d[i].end());
				Vector3d1 one_3d_boundary = Math::Vector2d3d(offsets_2d[i]);
				if (goon)
				{
					if (one_3d_boundary.size() > 3){
						Vector3d1 output_points;
						Circuit::KeepingOriginalSampling(one_3d_boundary, toolpath_size / 2.0, output_points);
						one_3d_boundary = output_points;
						offsets.push_back(one_3d_boundary);
						one_layer_3d_boundaries.push_back(one_3d_boundary);
					}
					goon = false;
				}
				else
				{
					if (one_3d_boundary.size() > 3 && Circuit::GetTotalLength(one_3d_boundary) > toolpath_size*4.0){
						Vector3d1 output_points;
						Circuit::KeepingOriginalSampling(one_3d_boundary, toolpath_size / 2.0, output_points);
						one_3d_boundary = output_points;
						offsets.push_back(one_3d_boundary);
						one_layer_3d_boundaries.push_back(one_3d_boundary);
					}
				}



				Vector3d1().swap(one_3d_boundary);
			}

			if (!one_layer_3d_boundaries.empty())
			offsetses.push_back(one_layer_3d_boundaries);

			Vector3d2().swap(one_layer_3d_boundaries);
			Vector2d2().swap(offsets_2d);

			d = d + toolpath_size;
			OFFSETS(boundaries_2d, d, offsets_2d);

			//if (offsetses.size() == 2)break;
		}
	}



	void CFSCNC::LoadBoundary(std::string path, Vector3d2 &input_boundaries)
	{
		std::ifstream file(path, std::ios::in);

		if (!file)
		{
			std::cout << "input boundary error..." << std::endl;
			return;
		}

		int boundary_nb;
		file >> boundary_nb;


		for (int i = 0; i < boundary_nb; i++)
		{
			int point_nb;
			file >> point_nb;

			Vector3d1 one_boundary;
			for (int j = 0; j < point_nb; j++)
			{
				double x, y;
				file >> x >> y;
				one_boundary.push_back(Vector3d(x, y, 0.0));
			}
			input_boundaries.push_back(one_boundary);
		}


		//scale
		Vector3d boundary_center(0.0, 0.0, 0.0);
		int nb = 0;
        double max_x = std::numeric_limits<double>::min();
        double min_x = std::numeric_limits<double>::max();
        double max_y = std::numeric_limits<double>::min();
        double min_y = std::numeric_limits<double>::max();

		for (int i = 0; i < input_boundaries.size(); i++)
		{
			for (int j = 0; j < input_boundaries[i].size(); j++)
			{
				boundary_center[0] += input_boundaries[i][j][0];
				boundary_center[1] += input_boundaries[i][j][1];
				nb++;
				max_x = max(max_x, (double)input_boundaries[i][j][0]);
				max_y = max(max_y, (double)input_boundaries[i][j][1]);
				min_x = min(min_x, (double)input_boundaries[i][j][0]);
				min_y = min(min_y, (double)input_boundaries[i][j][1]);
			}
		}

		boundary_center[0] = boundary_center[0] / nb;
		boundary_center[1] = boundary_center[1] / nb;

		double scale = max_y - min_y > max_x - min_x ? max_y - min_y : max_x - min_x;

        for (auto& boundary : input_boundaries) {
            for (auto& point : boundary) {
                point[0] = (point[0] - boundary_center[0]) / scale * 100;
                point[1] = (point[1] - boundary_center[1]) / scale * 100;
            }
        }

		//insert more points;
		for (int i = 0; i < input_boundaries.size(); i++)
		{
			Vector3d1 one_boundary;

			for (int j = 0; j < input_boundaries[i].size(); j++)
			{
				Vector3d v0 = input_boundaries[i][j];
				Vector3d v1 = input_boundaries[i][(j + 1) % input_boundaries[i].size()];

				double length = CGAL_3D_Distance_Point_Point(v0, v1);

				if (length > toolpath_size)
				{
					int insert_nb = (int)(length / toolpath_size) * 5;

					for (int k = 0; k < insert_nb; k++)
					{
						float d = (float)(k + 1) / (float)(insert_nb + 1);

						one_boundary.push_back((float)(1.0 - d) * v0 + d * v1);
					}
				}

				one_boundary.push_back(v1);

			}

			Vector3d1().swap(input_boundaries[i]);

			input_boundaries[i] = one_boundary;
		}

	}
}

