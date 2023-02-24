#include "stdafx.h"
#include "CFSCNC.h"
#include <Circuit.h>

namespace cnc {



	void CFSCNC::Output_Path(std::string path)
	{
		//Vector3d1 normals;
		//CGAL_Normal_Mesh(input_path + "_"+ "_one_region.off", single_path, normals);

		//samplng
		double gap = toolpath_size;

		Vector3d1 output_points;
		Strip::UniformSampling(single_path, gap*0.1, output_points, single_path_fixed_label);	
		single_path = output_points;


		//output
		std::ofstream file(path);
		file.precision(8);

		file << max_scallop << std::endl;
		file << drill_radius << std::endl;
		file << toolpath_size << std::endl;
		file << single_path.size() << std::endl;

		//fix fixed_label bug

		if (single_path_fixed_label.size() != 0)
		{
			double d = 0.0;

			int index = 0;
			while (d < gap*2.0)
			{

				d = d + CGAL_3D_Distance_Point_Point(single_path[index], single_path[index+1]);
				single_path_fixed_label[index] = 1;
				index++;
			}
		}


		for (int i = 0; i < single_path.size(); i++)
		{
			if (single_path_fixed_label.size() != 0)
			{
				file << single_path[i][0] << " " << single_path[i][1] << " " << single_path[i][2] << " " 
					 << " " << single_path_fixed_label[i] << std::endl;
			}
			else
				file << single_path[i][0] << " " << single_path[i][1] << " " << single_path[i][2] << std::endl;
		}

		file.clear();
		file.close();
	}


	void CFSCNC::Output_Boundary(std::string in_path, std::string out_path)
	{
		std::cout << "Output boundary source for heat geodesic computing" << std::endl;
		Vector3d1 heat_surface_vertices;
		std::vector<int> heat_face_id_0;
		std::vector<int> heat_face_id_1;
		std::vector<int> heat_face_id_2;

		CGAL_3D_Read_Triangle_Mesh(in_path, heat_surface_vertices, heat_face_id_0, heat_face_id_1, heat_face_id_2);
		std::vector<bool> heat_vertices_boundary;
		CGAL_3D_Triangle_Mesh_Boundary(heat_surface_vertices, heat_face_id_0, heat_face_id_1, heat_face_id_2, heat_vertices_boundary);

		int nb = 0;
		for (int i = 0; i < heat_vertices_boundary.size(); i++){
			if (heat_vertices_boundary[i]){
				nb++;
			}
		}

		std::ofstream file(out_path);
		file << nb << std::endl;

		for (int i = 0; i < heat_vertices_boundary.size(); i++){
			if (heat_vertices_boundary[i]){
				file << "1 " << i + 1 << " " << std::endl;;
			}
		}

		file.clear();
		file.close();

		Vector3d1().swap(heat_surface_vertices);
		std::vector<int>().swap(heat_face_id_0);
		std::vector<int>().swap(heat_face_id_1);
		std::vector<int>().swap(heat_face_id_2);
		std::vector<bool>().swap(heat_vertices_boundary);
	}



	void CFSCNC::Output_Obj_Cur_Normals(std::string path)
	{
		Vector3d1 surface_vertices;
		std::vector<std::vector<int>> surface_faces;
		Vector3d1 surface_vertices_normal;
		std::vector<double> surface_vectices_min_curs;
		std::vector<double> surface_vectices_max_curs;
		Vector3d1 max_curvature_directions;
		Vector3d1 min_curvature_directions;

		CGAL_3D_Read_Triangle_Mesh(input_path + "\\path\\" + IntString(cfs_index) + "_split.obj", surface_vertices, surface_faces);
		//CGAL_3D_Mesh_Curvature(surface_vertices, surface_faces, surface_vectices_max_curs, surface_vectices_min_curs, max_curvature_directions, min_curvature_directions, surface_vertices_normal);

		CGAL_Curvature_Mesh(input_path + "\\" + IntString(cfs_index) + "_full.off",
			surface_vertices, surface_vectices_max_curs, surface_vectices_min_curs, max_curvature_directions, min_curvature_directions);
		CGAL_Normal_Mesh(input_path + "\\" + IntString(cfs_index) + "_full.off",
			surface_vertices, surface_vertices_normal);

		std::ofstream file(path);
		file.precision(8);

		file << surface_vertices.size() << std::endl;

		file << "normal_x normal_y normal_z max_cur max_cur_x max_cur_y max_cur_z min_cur min_cur_x min_cur_y min_cur_z" << std::endl;;
		for (int i = 0; i < surface_vertices.size(); i++)
		{
			file << surface_vertices_normal[i][0] << " " << surface_vertices_normal[i][1] << " " << surface_vertices_normal[i][2] << " "
				<< surface_vectices_max_curs[i] << " " << max_curvature_directions[i][0] << " " << max_curvature_directions[i][1] << " " << max_curvature_directions[i][2] << " "
				<< surface_vectices_min_curs[i] << " " << min_curvature_directions[i][0] << " " << min_curvature_directions[i][1] << " " << min_curvature_directions[i][2] << std::endl;
		}

		file.clear();
		file.close();

	}

	void SmoothTheNormals(Vector3d1 &normal)
	{
		Vector3d1 new_normals;
		new_normals.push_back(normal[0]);

		for (int i = 1; i < normal.size() - 1; i++)
		{
			Vector3d n0 = normal[i - 1];
			Vector3d n1 = normal[i];
			Vector3d n2 = normal[i + 1];

			Vector3d n(0.0, 0.0, 0.0);
			n = n + n0;
			n = n + n1;
			n = n + n2;
			n[0] = n[0] / 3.0;
			n[1] = n[1] / 3.0;
			n[2] = n[2] / 3.0;

			new_normals.push_back(n);
		}

		new_normals.push_back(normal[normal.size() - 1]);

		normal = new_normals;
	}

	void CFSCNC::Output_Path2(std::string path)
	{
		///////////////////////////////////////////////////////////////////////////////
		std::cout << "Path: " << input_path + "0.off" << std::endl;

		Vector3d1().swap(single_final_path_normal);
		Vector3d1().swap(single_final_path_RMDF_normal);

		//single_final_path_normal
		//Vector3d1 normals;
		CGAL_Normal_Mesh(input_path + "0.off", single_final_path, single_final_path_normal);

		//single_final_path_normal = normals;
		for (int i = 0; i < 200; i++)
			SmoothTheNormals(single_final_path_normal);

		//single_final_path_RMDF_normal
		for (int i = 0; i < single_final_path.size() - 1; i++)
		{
			Vector3d n = getCrossproduct(single_final_path_normal[i], single_final_path[i + 1] - single_final_path[i]);
			//	inline Vector3d RotationAxis(Vector3d p, double angle, Vector3d n)
			single_final_path_RMDF_normal.push_back(RotationAxis(single_final_path_normal[i], MM_PI / 10, n));
		}
		single_final_path_RMDF_normal.push_back(single_final_path_normal[single_final_path_normal.size() - 1]);

		for (int i = 0; i < 200; i++)
			SmoothTheNormals(single_final_path_RMDF_normal);

		//cc=>cl
		//single_final_CL_path

		for (int i = 0; i < single_final_path.size(); i++)
		{
			Vector3d surface_normal = single_final_path_normal[i];
			SetVectorLength(surface_normal, 2.0);

			Vector3d drill_orientation = single_final_path_RMDF_normal[i];
			SetVectorLength(drill_orientation, 2.0);

			Vector3d center = single_final_path[i] - surface_normal + drill_orientation;
			single_final_CL_path.push_back(center);
		}

		std::ofstream file(path);
		file.precision(8);
		file << toolpath_size << std::endl;

		file << single_final_path.size() << std::endl;
		for (int i = 0; i < single_final_path.size(); i++)
		{
			file << single_final_path[i][0] << " " << single_final_path[i][1] << " " << single_final_path[i][2] <<
				" " << single_final_path_normal[i][0] << " " << single_final_path_normal[i][1] << " " << single_final_path_normal[i][2] <<
				" " << single_final_path_RMDF_normal[i][0] << " " << single_final_path_RMDF_normal[i][1] << " " << single_final_path_RMDF_normal[i][2] <<
				" " << single_final_CL_path[i][0] << " " << single_final_CL_path[i][1] << " " << single_final_CL_path[i][2] << std::endl;
			//file << single_final_path[i][0] << " " << single_final_path[i][1] << " " << single_final_path[i][2] << std::endl;
		}

		file.clear();
		file.close();

		//getCrossproduct
	}

	void CFSCNC::OutputOffsets(const std::string path, const Vector3d1 &offset)
	{
		std::ofstream debug_file(path);
		int debug_index = 1;

		for (int j = 0; j < offset.size(); j++)
		{
			auto s = offset[j];
			auto e = offset[(j + 1) % offset.size()];
			CGAL_Export_Path_Segment(debug_file, debug_index, "name", 1.0, 0.0, 0.0, s, e, 0.1);
		}

		debug_file.clear();
		debug_file.close();
	}


	void CFSCNC::OutputOffsets1(const std::string path, const Vector3d1 &offset)
	{
		std::ofstream debug_file(path);
		int debug_index = 1;

		for (int j = 0; j < offset.size(); j++)
		{
			auto s = offset[j];
			auto e = offset[(j + 1) % offset.size()];
			CGAL_Export_Path_Segment(debug_file, debug_index, "name", 1.0, 0.0, 0.0, s, e, 0.1);
		}

		debug_file.clear();
		debug_file.close();
	}

	void CFSCNC::OutputOffsets(const std::string path, const Vector3d2 &offsets, bool name_b)
	{
		std::ofstream debug_file(path);
		int debug_index = 1;
		for (int i = 0; i < offsets.size(); i++)
		{
			for (int j = 0; j < offsets[i].size(); j++)
			{
				auto s = offsets[i][j];
				auto e = offsets[i][(j + 1) % offsets[i].size()];
				if (name_b)
					CGAL_Export_Path_Segment(debug_file, debug_index, "name_" + std::to_string(i), 1.0, 0.0, 0.0, s, e, 0.02);
				else
					CGAL_Export_Path_Segment(debug_file, debug_index, "name_" + std::to_string(i) + "_" + std::to_string(j), 1.0, 0.0, 0.0, s, e, 0.02);

			}
		}

		debug_file.clear();
		debug_file.close();
	}

	void CFSCNC::OutputStrips(const std::string path, const Vector3d2 &offsets, bool name_b)
	{
		std::ofstream debug_file(path);
		int debug_index = 1;
		for (int i = 0; i < offsets.size(); i++)
		{
			for (int j = 0; j < offsets[i].size()-1; j++)
			{
				auto s = offsets[i][j];
				auto e = offsets[i][(j + 1) % offsets[i].size()];
				if (name_b)
					CGAL_Export_Path_Segment(debug_file, debug_index, "name_"+std::to_string(i), 1.0, 0.0, 0.0, s, e, 0.1);
				else
					CGAL_Export_Path_Segment(debug_file, debug_index, "name_" + std::to_string(i) + "_" + std::to_string(j), 1.0, 0.0, 0.0, s, e, 0.1);

			}
		}

		debug_file.clear();
		debug_file.close();
	}



	void CFSCNC::OutputStrip(const std::string path, const Vector3d1 &offsets, bool name_b)
	{
		std::ofstream debug_file(path);
		int debug_index = 1;
		for (int j = 0; j < offsets.size() - 1; j++)
		{
			auto s = offsets[j];
			auto e = offsets[(j + 1) % offsets.size()];
			if (name_b)
				CGAL_Export_Path_Segment(debug_file, debug_index, "name", 1.0, 0.0, 0.0, s, e, 0.1);
			else
				CGAL_Export_Path_Segment(debug_file, debug_index, "name_" + IntString(j), 1.0, 0.0, 0.0, s, e, 0.1);
		}

		debug_file.clear();
		debug_file.close();
	}


	void CFSCNC::Output_Path1(std::string path)
	{
		//output most hightest path
		///////////////////////////////////////////////////////////////////////////////

		int index = -1;
		double max_y = -10000000.0;
		for (int i = 0; i < single_final_path.size(); i++)
		{
			if (single_final_path[i][1]>max_y)
			{
				max_y = single_final_path[i][1];
				index = i;
			}
		}
		Vector3d1 new_points;
		for (int i = index; i < single_final_path.size(); i++)
		{
			new_points.push_back(single_final_path[i]);
		}
		for (int i = 0; i < index; i++)
		{
			new_points.push_back(single_final_path[i]);
		}

		Vector3d1().swap(single_final_path);
		single_final_path = new_points;
		///////////////////////////////////////////////////////////////////////////////

		std::cout << "Path: " << input_path + "0.off" << std::endl;

		Vector3d1 normals;
		CGAL_Normal_Mesh(input_path + "0.off", single_final_path, normals);

		single_final_path_normal = normals;

		for (int i = 0; i < 1; i++)
			SmoothTheNormals(single_final_path_normal);

		std::ofstream file(path);

		file.precision(8);

		file << toolpath_size << std::endl;

		file << single_final_path.size() << std::endl;
		for (int i = 0; i < single_final_path.size(); i++)
		{
			file << single_final_path[i][0] << " " << single_final_path[i][1] << " " << single_final_path[i][2] << " " << single_final_path_normal[i][0] << " " << single_final_path_normal[i][1] << " " << single_final_path_normal[i][2] << std::endl;
			//file << single_final_path[i][0] << " " << single_final_path[i][1] << " " << single_final_path[i][2] << std::endl;
		}

		file.clear();
		file.close();
	}

	void CFSCNC::Load_Zigzag_Path(std::string path)
	{
		//zigzag_final_path
		int nb;
		std::ifstream file(path, std::ios::in);

		file >> nb;
		for (int i = 0; i < nb; i++)
		{
			double x, y, z;
			file >> x >> y >> z;
			zigzag_final_path.push_back(Vector3d(x, y, z));
		}


		file.clear();
		file.close();
	}

	
	void CFSCNC::Load_Path(std::string path)
	{
		std::ifstream file(path, std::ios::in);

		int nb = 0;
		file >> max_scallop >> drill_radius;
		file >> toolpath_size >> nb;

		Vector3d1 normals;
		for (int i = 0; i < nb; i++)
		{
			double x, y, z;
			bool b;
			double n_x, n_y, n_z;
			file >> x >> y >> z >>n_x>>n_y>>n_z>> b;
			single_path.push_back(Vector3d(x, y, z));
			normals.push_back(Vector3d(n_x, n_y, n_z));
			single_path_fixed_label.push_back(b);
		}

		file.clear();
		file.close();
	}

	bool CFSCNC::Load_Final_Path(std::string path)
	{
		std::ifstream file(path, std::ios::in);

		if (file)
		{
			int nb = 0;
			file >> max_scallop >> drill_radius;
			file >> nb;
			for (int i = 0; i < nb; i++)
			{
				double x, y, z;
				bool b;
				file >> x >> y >> z>>b;
				single_final_path.push_back(Vector3d(x, y, z));

				single_path_fixed_label.push_back(b);
			}
			return true;
		}
		else
		{
			file.clear();
			file.close();

			std::cout << "Load final path error: ???" << std::endl;

			return false;
		}
	}

	void CFSCNC::Output_tree(int nodes_nb, std::vector<int> &edges, std::string path)
	{
		std::ofstream file(path);

		file << "Mark Newman on Sat Jul 22 05:32:16 2006" << std::endl;
		file << "graph" << std::endl;
		file << "[" << std::endl;
		file << "  directed 0" << std::endl;

		for (int i = 0; i < nodes_nb; i++)
		{
			file << "node" << std::endl;
			file << "[" << std::endl;
			file << "id " << i << std::endl;
			file << "label " << i << std::endl;

			file << "]" << std::endl;
		}

		for (int i = 0; i < edges.size(); i = i + 2)
		{
			file << "edge" << std::endl;
			file << "[" << std::endl;

			file << "source " << edges[i] << std::endl;
			file << "target " << edges[i + 1] << std::endl;

			file << "]" << std::endl;
		}

		file << "]" << std::endl;

		file.clear();
		file.close();
	}



	void CFSCNC::OutputOffsetsFiles(std::vector<string> &offsets_files, std::string path)
	{
		std::ofstream output_file(path);
		output_file << offsets_files.size() << std::endl;
		for (int i = 0; i < offsets_files.size(); i++)
			output_file << offsets_files[i] << std::endl;
		output_file.clear();
		output_file.close();
	}

	void CFSCNC::LoadOffsetsFiles(std::vector<string> &offsets_files, std::string path)
	{
		std::ifstream file(path, std::ios::in);
		int nb;
		file >> nb;

		for (int i = 0; i < nb; i++)
		{
			std::string offset_path;
			file >> offset_path;
			offsets_files.push_back(offset_path);
		}
		file.clear();
		file.close();
	}

	void CFSCNC::LoadContour(std::string path, Vector3d2 &offsets, std::vector<double> &dists)
	{
		std::ifstream file(path, std::ios::in);

		if (!file){
			std::cout << "input contour error..." << std::endl;
			return;
		}

		file >> max_scallop;
		file >> drill_radius;
		file >> toolpath_size;
		int contour_number;
		file >> contour_number;

		for (int i = 0; i < contour_number; i++){
			int point_number;
			double distance;
			file >> point_number;
			file >> distance;
			Vector3d1 one_path;
			for (int j = 0; j < point_number; j++){
				double x, y, z;
				file >> x >> y >> z;
				one_path.push_back(Vector3d(x, y, z));
			}

			if (one_path.size() > 3 && Circuit::GetTotalLength(one_path)>toolpath_size*4.0){
				dists.push_back(distance);
				offsets.push_back(one_path);
			}
		}
	}


	//generate data
	Vector2d1 GenerateConcavityData(int nb, int period, double max_r, double min_r)
	{
		Vector2d1 cnc_path;

		for (int i = 0; i < nb; i++)
		{
			double angle = (double)i / (double)nb*MM_PI*2.0;

			double r = (cos(angle*period) + 1.0) / 2.0*(max_r - min_r) + min_r;

			double x, y;
			x = r*cos(angle);
			y = r*sin(angle);
			cnc_path.push_back(Vector2d(x, y));
		}
		return cnc_path;
	}

	void Laplace_Mesh_Vertex_Values(Vector3d1 &vecs, std::vector<int> &face_id_0, std::vector<int> &face_id_1, std::vector<int> &face_id_2, std::vector<double> &values, int iterations)
	{
		std::vector<std::vector<int>> neighs;
		CGAL_3D_Triangle_Mesh_Vecs_Neighbors(vecs, face_id_0, face_id_1, face_id_2, neighs);

		std::vector<double> iter_values(vecs.size(), 0.0);

		for (int i = 0; i < iterations; i++)
		{
			for (int j = 0; j < vecs.size(); j++)
			{
				for (int k = 0; k < neighs[j].size(); k++)
				{
					iter_values[j] += values[neighs[j][k]];
				}
				iter_values[j] = iter_values[j] / (double)neighs[j].size();

				//iter_values[j] = 0.5*iter_values[j] + 0.5*values[i];
			}

			std::vector<double>().swap(values);
			values = iter_values;
			for (int j = 0; j < vecs.size(); j++)
				iter_values[j] = 0.0;
		}
	}


}
