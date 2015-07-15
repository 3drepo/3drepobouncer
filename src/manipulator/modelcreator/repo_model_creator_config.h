/**
*  Copyright (C) 2015 3D Repo Ltd
*
*  This program is free software: you can redistribute it and/or modify
*  it under the terms of the GNU Affero General Public License as
*  published by the Free Software Foundation, either version 3 of the
*  License, or (at your option) any later version.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU Affero General Public License for more details.
*
*  You should have received a copy of the GNU Affero General Public License
*  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
/**
* General Model Creator Config
* used to track all settings required for Model Creator
*/

#pragma once

#include <map>
#include <stdint.h>


namespace repo{
	namespace manipulator{
		namespace modelcreator{

			class ModelCreatorConfig
			{
			public:
				ModelCreatorConfig();
				~ModelCreatorConfig();

				/**
				* ---------------- Getters -------------------------
				*/
				/*!
				* Returns true if calculate tangent space is checked in settings, false
				* otherwise. Defaults to false.
				*/
				bool getCalculateTangentSpace()
				{
					return boolSettings[CALCULATE_TANGENT_SPACE];
				}

				/*!
				* Returns the maximum smoothing angle for calc tangent space. Defaults to
				* 45.
				*/
				float getCalculateTangentSpaceMaxSmoothingAngle()
				{
					return floatSettings[CALCULATE_TANGENT_SPACE_MAX_SMOOTHING_ANGLE];
				}

				/*!
				* Returns true if convert to UV coords is checked in settings, false
				* otherwise. Defaults to false.
				*/
				bool getConvertToUVCoordinates()
				{
					return boolSettings[CONVERT_TO_UV_COORDINATES];
				}

				/*!
				* Returns true if degenerates to points/lines is checked in settings, false
				* otherwise. Defaults to false.
				*/
				bool getDegeneratesToPointsLines()
				{
					return boolSettings[DEGENERATES_TO_POINTS_LINES];
				}

				/*!
				* Returns true if debone is checked in settings, false otherwise. Defaults
				* to false.
				*/
				bool getDebone()
				{
					return boolSettings[DEBONE];
				}

				/*!
				* Returns the debone threshold set in settings. Defaults to Assimp's
				* AI_DEBONE_THRESHOLD = 1.0f
				*/
				float getDeboneThreshold()
				{
					return floatSettings[DEBONE_THRESHOLD];
				}

				/*!
				* Returns true if debone only if all is checked in settings, false
				* otherwise. Defaults to false.
				*/
				bool getDeboneOnlyIfAll()
				{
					return boolSettings[DEBONE_ONLY_IF_ALL];
				}

				/*!
				* Returns true if find instances is checked in settings, false otherwise.
				* Defaults to false.
				*/
				bool getFindInstances()
				{
					return boolSettings[FIND_INSTANCES];
				}

				/*!
				* Returns true if find invalid data is checked in settings, false
				* otherwise. Defaults to false.
				*/
				bool getFindInvalidData()
				{
					return boolSettings[FIND_INAVLID_DATA];
				}

				/*!
				* Returns the animation accuracy for find invalid data set in settings.
				* Defaults to 0.0f == all comparisons are exact.
				* See http://assimp.sourceforge.net/lib_html/config_8h.html#ad223c5e7e63d2937685cc704a181b950
				*/
				float getFindInvalidDataAnimationAccuracy()
				{
					return floatSettings[FIND_INAVLID_DATA_ANIMATION_ACCURACY];
				}

				/*!
				* Returns true if fix infacing normals is checked in settings, false
				* otherwise. Defaults to false.
				*/
				bool getFixInfacingNormals()
				{
					return boolSettings[FIX_INFACING_NORMALS];
				}

				/*!
				* Returns true if flip UV coordinates is checked in settings, false
				* otherwise. Defaults to false.
				*/
				bool getFlipUVCoordinates()
				{
					return boolSettings[FLIP_UV_COORDINATES];
				}

				/*!
				* Returns true if flip winding order is checked in settings, false
				* otherwise. Defaults to false.
				*/
				bool getFlipWindingOrder()
				{
					return boolSettings[FLIP_WINDING_ORDER];
				}

				/*!
				* Returns true if generate normals is checked in settings, false otherwise.
				* Defaults to false.
				*/
				bool getGenerateNormals()
				{
					return boolSettings[GENERATE_NORMALS];
				}

				/*!
				* Returns true if generate flat normals is checked in settings, false
				* oterwise. Defaults to true.
				*/
				bool getGenerateNormalsFlat()
				{
					return boolSettings[GENERATE_NORMALS_FLAT];
				}

				/*!
				* Returns true if generate smooth normals is checked in settings, false
				* otherwise. Defaults to false.
				*/
				bool getGenerateNormalsSmooth()
				{
					return boolSettings[GENERATE_NORMALS_SMOOTH];
				}

				/*!
				* Returns the crease angle for generate smooth normals set in settings.
				* Defaults to 175.0f.
				*/
				float getGenerateNormalsSmoothCreaseAngle()
				{
					return floatSettings[GENERATE_NORMALS_SMOOTH_CREASE_ANGLE];
				}

				/*!
				* Returns true if improve cache locality is checked in settings, false
				* otherwise. Defaults to false.
				*/
				bool getImproveCacheLocality()
				{
					return boolSettings[IMPROVE_CACHE_LOCALITY];
				}

				/*!
				* Returns the vertex cache size for improve cache locality set in settings.
				* Defaults to PP_ICL_PTCACHE_SIZE = 12.
				*/
				int getImproveCacheLocalityVertexCacheSize()
				{
					return intSettings[IMPROVE_CACHE_LOCALITY_VERTEX_CACHE_SIZE];
				}

				/*!
				* Returns true if join identical vertices is checked in settings, false
				* otherwise. Defaults to false.
				*/
				bool getJoinIdenticalVertices()
				{
					return boolSettings[JOIN_IDENTICAL_VERTICES];
				}

				/*!
				* Returns true if limit bone weights is checked in settings, false
				* otherwise. Defaults to false.
				*/
				bool getLimitBoneWeights()
				{
					return boolSettings[LIMIT_BONE_WEIGHTS];
				}

				/*!
				* Returns the max weight for limit bone weights set in settings. Defaults
				* to AI_LMW_MAX_WEIGHTS = 4.
				*/
				int getLimitBoneWeightsMaxWeight()
				{

					return intSettings[LIMIT_BONE_WEIGHTS_MAX_WEIGHTS];
				}

				/*!
				* Returns true if make left handed is checked in settings, false otherwise.
				* Defaults to false.
				*/
				bool getMakeLeftHanded()
				{
					return boolSettings[MAKE_LEFT_HANDED];
				}

				/*!
				* Returns true if optimize meshes is checked in settings, false otherwise.
				* Defaults to false.
				*/
				bool getOptimizeMeshes()
				{
					return boolSettings[OPTIMIZE_MESHES];
				}

				/*!
				* Returns true if pre-transform UV coordinates is checked in settings,
				* false otherwise. Defaults to false.
				*/
				bool getPreTransformUVCoordinates()
				{
					return boolSettings[PRE_TRANSFORM_UV_COORDINATES];
				}

				/*!
				* Returns true if pre-transform vertices is checked in settings, false
				* otherwise. Defaults to false.
				*/
				bool getPreTransformVertices()
				{
					return boolSettings[PRE_TRANSFORM_VERTICES];
				}

				/*!
				* Returns true if normalize pre-transform vertices is checked in settings,
				* false otherwise. Defaults to false.
				*/
				bool getPreTransformVerticesNormalize()
				{
					return boolSettings[PRE_TRANSFORM_VERTICES_NORMALIZE];
				}

				/*!
				* Returns true if remove components is checked in settings, false otherwise.
				* Defaults to false.
				*/
				bool getRemoveComponents()
				{
					return boolSettings[REMOVE_COMPONENTS];
				}

				/*!
				* Returns true if remove components animations is checked in settings,
				* false otherwise. Defaults to false.
				*/
				bool getRemoveComponentsAnimations()
				{
					return boolSettings[REMOVE_COMPONENTS_ANIMATIONS];
				}

				/*!
				* Returns true if remove components bi/tangents is checked in settings,
				* false otherwise. Defaults to false.
				*/
				bool getRemoveComponentsBiTangents()
				{
					return boolSettings[REMOVE_COMPONENTS_BI_TANGENTS];
				}

				/*!
				* Returns true if remove components bone weights is checked in settings,
				* false otherwise. Defaults to false.
				*/
				bool getRemoveComponentsBoneWeights()
				{
					return boolSettings[REMOVE_COMPONENTS_BONE_WEIGHTS];
				}

				/*!
				* Returns true if remove components cameras is checked in settings, false
				* otherwise. Defaults to false.
				*/
				bool getRemoveComponentsCameras()
				{
					return boolSettings[REMOVE_COMPONENTS_CAMERAS];
				}

				/*!
				* Returns true if remove components colors is checked in settings, false
				* otherwise. Defaults to false.
				*/
				bool getRemoveComponentsColors()
				{
					return boolSettings[REMOVE_COMPONENTS_COLORS];
				}

				/*!
				* Returns true if remove components lights is checked in settings, false
				* otherwise. Defaults to false.
				*/
				bool getRemoveComponentsLights()
				{
					return boolSettings[REMOVE_COMPONENTS_LIGHTS];
				}

				/*!
				* Returns true if remove components materials is checked in settings, false
				* otherwise. Defaults to false.
				*/
				bool getRemoveComponentsMaterials()
				{
					return boolSettings[REMOVE_COMPONENTS_MATERIALS];
				}

				/*!
				* Returns true if remove components meshes is checked in settings, false
				* otherwise. Defaults to false.
				*/
				bool getRemoveComponentsMeshes()
				{
					return boolSettings[REMOVE_COMPONENTS_MESHES];
				}

				/*!
				* Returns true if remove components normals is checked in settings, false
				* otherwise. Defaults to false.
				*/
				bool getRemoveComponentsNormals()
				{
					return boolSettings[REMOVE_COMPONENTS_NORMALS];
				}

				/*!
				* Returns true if remove components textures is checked in settings, false
				* otherwise. Defaults to false.
				*/
				bool getRemoveComponentsTextures()
				{
					return boolSettings[REMOVE_COMPONENTS_TEXTURES];
				}

				/*!
				* Returns true if remove components texture coordinates is checked in
				* settings, false otherwise. Defaults to false.
				*/
				bool getRemoveComponentsTextureCoordinates()
				{
					return boolSettings[REMOVE_COMPONENTS_TEXTURE_COORDINATES];
				}

				/*!
				* Returns true if remove redundant materials is checked in settings, false
				* otherwise. Defaults to false.
				*/
				bool getRemoveRedundantMaterials()
				{
					return boolSettings[REMOVE_REDUNDANT_MATERIALS];
				}

				/*!
				* Returns a space-separated string of materials names to be skipped set in
				* settings. Defaults to empty string.
				*/
				std::string getRemoveRedundantMaterialsSkip()
				{
					return stringSettings[REMOVE_REDUNDANT_MATERIALS_SKIP];
				}

				/*!
				* Returns true if remove redundant nodes is checked in settings, false
				* otherwise. Defaults to false.
				*/
				bool getRemoveRedundantNodes()
				{
					return boolSettings[REMOVE_REDUNDANT_NODES];
				}

				/*!
				* Returns a space-separated string of node names to be skipped set in
				* settings. Defaults to empty string.
				*/
				std::string getRemoveRedundantNodesSkip()
				{
					return stringSettings[REMOVE_REDUNDANT_NODES_SKIP];
				}

				/*!
				* Returns ture if sort and remove is checked in settings, false otherwise.
				* Defaults to false.
				*/
				bool getSortAndRemove()
				{
					return boolSettings[SORT_AND_REMOVE];
				}

				/*!
				* Returns true if sort and remove lines is checked in settings, false
				* otherwise. Defaults to true.
				*/
				bool getSortAndRemoveLines()
				{
					return boolSettings[SORT_AND_REMOVE_LINES];
				}

				/*!
				* Returns true if sort and remove points is checked in settings, false
				* otherwise. Defaults to true.
				*/
				bool getSortAndRemovePoints()
				{
					return boolSettings[SORT_AND_REMOVE_POINTS];
				}

				/*!
				* Returns true if sort and remove points is checked in settings, false
				* otherwise. Defaults to false.
				*/
				bool getSortAndRemovePolygons()
				{
					return boolSettings[SORT_AND_REMOVE_POLYGONS];
				}

				/*!
				* Returns true if sort and remove triangles is checked in settings, false
				* otherwise. Defaults to false.
				*/
				bool getSortAndRemoveTriangles()
				{
					return boolSettings[SORT_AND_REMOVE_TRIANGLES];
				}

				/*!
				* Returns true if split by bone count is checked in settings, false
				* otherwise. Defaults to false.
				*/
				bool getSplitByBoneCount()
				{
					return boolSettings[SPLIT_BY_BONE_COUNT];
				}

				/*!
				* Returns a split by bones count max bones set in settings. Defaults to
				* AI_SBBC_DEFAULT_MAX_BONES = 60.
				*/
				int getSplitByBoneCountMaxBones()
				{
					return boolSettings[SPLIT_BY_BONE_COUNT_MAX_BONES];
				}

				/*!
				* Returns true if split large meshes is checked in settings, false
				* otherwise. Defaults to false.
				*/
				bool getSplitLargeMeshes()
				{
					return boolSettings[SPLIT_LARGE_MESHES];
				}

				/*!
				* Returns a split large meshes triangle limit set in settings. Defaults to
				* AI_SLM_DEFAULT_MAX_TRIANGLES = 1,000,000.
				*/
				int getSplitLargeMeshesTriangleLimit()
				{
					return intSettings[SPLIT_LARGE_MESHES_TRIANGLE_LIMIT];
				}

				/*!
				* Returns a split large meshes triangle limit set in settings. Defaults to
				* AI_SLM_DEFAULT_MAX_VERTICES = 1,000,000.
				*/
				int getSplitLargeMeshesVertexLimit()
				{
					return intSettings[SPLIT_LARGE_MESHES_VERTEX_LIMIT];
				}

				/*!
				* Returns true if triangulate is checked in settings, false otherwise.
				* Defaults to true.
				*/
				bool getTriangulate()
				{
					return boolSettings[TRIANGULATE];
				}

				/*!
				* Returns true if validate data structures is checked in settings, false
				* otherwise. Defaults to false.
				*/
				bool getValidateDataStructures()
				{
					return boolSettings[VALIDATE_DATA_STRUCTURES];
				}

				/**
				* ---------------- Setters -------------------------
				*/

				//! Sets the calculate tangent space to settings.
				void setCalculateTangentSpace(bool on)
				{
					setValue(CALCULATE_TANGENT_SPACE, on);
				}

				void setCalculateTangentSpaceMaxSmoothingAngle(float angle)
				{
					setValue(CALCULATE_TANGENT_SPACE_MAX_SMOOTHING_ANGLE, angle);
				}

				//! Sets the convert to UV coordinates to settings.
				void setConvertToUVCoordinates(bool on)
				{
					setValue(CONVERT_TO_UV_COORDINATES, on);
				}

				//! Sets the degenerates to points and lines to settings.
				void setDegeneratesToPointsLines(bool on)
				{
					setValue(DEGENERATES_TO_POINTS_LINES, on);
				}

				//! Sets the debone to settings.
				void setDebone(bool on) { setValue(DEBONE, on); }

				//! Sets the debone threshold to settings.
				void setDeboneThreshold(float t)
				{
					setValue(DEBONE_THRESHOLD, t);
				}

				void setDeboneOnlyIfAll(bool on)
				{
					setValue(DEBONE_ONLY_IF_ALL, on);
				}

				void setFindInstances(bool on)
				{
					setValue(FIND_INSTANCES, on);
				}

				void setFindInvalidData(bool on)
				{
					setValue(FIND_INAVLID_DATA, on);
				}

				void setFindInvalidDataAnimationAccuracy(float f)
				{
					setValue(FIND_INAVLID_DATA_ANIMATION_ACCURACY, f);
				}

				void setFixInfacingNormals(bool on)
				{
					setValue(FIX_INFACING_NORMALS, on);
				}

				void setFlipUVCoordinates(bool on)
				{
					setValue(FLIP_UV_COORDINATES, on);
				}

				void setFlipWindingOrder(bool on)
				{
					setValue(FLIP_WINDING_ORDER, on);
				}

				void setGenerateNormals(bool on)
				{
					setValue(GENERATE_NORMALS, on);
				}

				void setGenerateNormalsFlat(bool on)
				{
					setValue(GENERATE_NORMALS_FLAT, on);
				}

				void setGenerateNormalsSmooth(bool on)
				{
					setValue(GENERATE_NORMALS_SMOOTH, on);
				}

				void setGenerateNormalsSmoothCreaseAngle(float f)
				{
					setValue(GENERATE_NORMALS_SMOOTH_CREASE_ANGLE, f);
				}

				void setImproveCacheLocality(bool on)
				{
					setValue(IMPROVE_CACHE_LOCALITY, on);
				}

				void setImproveCacheLocalityCacheSize(int vertexCount)
				{
					setValue(IMPROVE_CACHE_LOCALITY_VERTEX_CACHE_SIZE, vertexCount);
				}

				void setJoinIdenticalVertices(bool on)
				{
					setValue(JOIN_IDENTICAL_VERTICES, on);
				}

				void setLimitBoneWeights(bool on)
				{
					setValue(LIMIT_BONE_WEIGHTS, on);
				}

				void setLimitBoneWeightsMaxWeights(int i)
				{
					setValue(LIMIT_BONE_WEIGHTS_MAX_WEIGHTS, i);
				}

				void setMakeLeftHanded(bool on)
				{
					setValue(MAKE_LEFT_HANDED, on);
				}

				void setOptimizeMeshes(bool on)
				{
					setValue(OPTIMIZE_MESHES, on);
				}

				void setPreTransformUVCoordinates(bool on)
				{
					setValue(PRE_TRANSFORM_UV_COORDINATES, on);
				}

				void setPreTransformVertices(bool on)
				{
					setValue(PRE_TRANSFORM_VERTICES, on);
				}

				void setPreTransforVerticesNormalize(bool on)
				{
					setValue(PRE_TRANSFORM_VERTICES_NORMALIZE, on);
				}

				void setRemoveComponents(bool on)
				{
					setValue(REMOVE_COMPONENTS, on);
				}

				void setRemoveComponentsAnimations(bool on)
				{
					setValue(REMOVE_COMPONENTS_ANIMATIONS, on);
				}

				void setRemoveComponentsBiTangents(bool on)
				{
					setValue(REMOVE_COMPONENTS_BI_TANGENTS, on);
				}

				void setRemoveComponentsBoneWeights(bool on)
				{
					setValue(REMOVE_COMPONENTS_BONE_WEIGHTS, on);
				}

				void setRemoveComponentsCameras(bool on)
				{
					setValue(REMOVE_COMPONENTS_CAMERAS, on);
				}

				void setRemoveComponentsColors(bool on)
				{
					setValue(REMOVE_COMPONENTS_COLORS, on);
				}

				void setRemoveComponentsLights(bool on)
				{
					setValue(REMOVE_COMPONENTS_LIGHTS, on);
				}

				void setRemoveComponentsMaterials(bool on)
				{
					setValue(REMOVE_COMPONENTS_MATERIALS, on);
				}

				void setRemoveComponentsMeshes(bool on)
				{
					setValue(REMOVE_COMPONENTS_MESHES, on);
				}

				void setRemoveComponentsNormals(bool on)
				{
					setValue(REMOVE_COMPONENTS_NORMALS, on);
				}

				void setRemoveComponentsTextures(bool on)
				{
					setValue(REMOVE_COMPONENTS_TEXTURES, on);
				}

				void setRemoveComponentsTextureCoordinates(bool on)
				{
					setValue(REMOVE_COMPONENTS_TEXTURE_COORDINATES, on);
				}

				void setRemoveRedundantMaterials(bool on)
				{
					setValue(REMOVE_REDUNDANT_MATERIALS, on);
				}

				void setRemoveRedundantMaterialsSkip(const std::string& skip)
				{
					setValue(REMOVE_REDUNDANT_MATERIALS_SKIP, skip);
				}

				void setRemoveRedundantNodes(bool on)
				{
					setValue(REMOVE_REDUNDANT_NODES, on);
				}

				void setRemoveRedundantNodesSkip(const std::string& skip)
				{
					setValue(REMOVE_REDUNDANT_NODES_SKIP, skip);
				}

				void setSortAndRemove(bool on)
				{
					setValue(SORT_AND_REMOVE, on);
				}

				void setSortAndRemoveLines(bool on)
				{
					setValue(SORT_AND_REMOVE_LINES, on);
				}

				void setSortAndRemovePoints(bool on)
				{
					setValue(SORT_AND_REMOVE_POINTS, on);
				}

				void setSortAndRemovePolygons(bool on)
				{
					setValue(SORT_AND_REMOVE_POLYGONS, on);
				}

				void setSortAndRemoveTriangles(bool on)
				{
					setValue(SORT_AND_REMOVE_TRIANGLES, on);
				}

				void setSplitByBoneCount(bool on)
				{
					setValue(SPLIT_BY_BONE_COUNT, on);
				}

				void setSplitByBoneCountMaxBones(int max)
				{
					setValue(SPLIT_BY_BONE_COUNT_MAX_BONES, max);
				}

				void setSplitLargeMeshes(bool on)
				{
					setValue(SPLIT_LARGE_MESHES, on);
				}

				void setSplitLargeMeshesTriangleLimit(int limit)
				{
					setValue(SPLIT_LARGE_MESHES_TRIANGLE_LIMIT, limit);
				}

				void setSplitLargeMeshesVertexLimit(int limit)
				{
					setValue(SPLIT_LARGE_MESHES_VERTEX_LIMIT, limit);
				}

				void setTriangulate(bool on)
				{
					setValue(TRIANGULATE, on);
				}

				void setValidateDataStructures(bool on)
				{
					setValue(VALIDATE_DATA_STRUCTURES, on);
				}


			protected:
				/**
				* Resets all the settings to default
				* Also used at initialisation.
				*/
				void reset();

				void setValue(std::string label, bool value)
				{
					boolSettings[label] = value;
				}

				void setValue(std::string label, int32_t value)
				{
					intSettings[label] = value;
				}

				void setValue(std::string label, float value)
				{
					floatSettings[label] = value;
				}

				void setValue(std::string label, std::string value)
				{
					stringSettings[label] = value;
				}


				std::map<std::string, bool> boolSettings;
				std::map<std::string, int32_t> intSettings;
				std::map<std::string, float> floatSettings;
				std::map<std::string, std::string> stringSettings;


				static const std::string CALCULATE_TANGENT_SPACE;
				static const std::string CALCULATE_TANGENT_SPACE_MAX_SMOOTHING_ANGLE;
				static const std::string CONVERT_TO_UV_COORDINATES;
				static const std::string DEGENERATES_TO_POINTS_LINES;
				static const std::string DEBONE;
				static const std::string DEBONE_THRESHOLD;
				static const std::string DEBONE_ONLY_IF_ALL;
				static const std::string FIND_INSTANCES;
				static const std::string FIND_INAVLID_DATA;
				static const std::string FIND_INAVLID_DATA_ANIMATION_ACCURACY;
				static const std::string FIX_INFACING_NORMALS;
				static const std::string FLIP_UV_COORDINATES;
				static const std::string FLIP_WINDING_ORDER;
				static const std::string GENERATE_NORMALS;
				static const std::string GENERATE_NORMALS_FLAT;
				static const std::string GENERATE_NORMALS_SMOOTH;
				static const std::string GENERATE_NORMALS_SMOOTH_CREASE_ANGLE;
				static const std::string IMPROVE_CACHE_LOCALITY;
				static const std::string IMPROVE_CACHE_LOCALITY_VERTEX_CACHE_SIZE;
				static const std::string JOIN_IDENTICAL_VERTICES;
				static const std::string LIMIT_BONE_WEIGHTS;
				static const std::string LIMIT_BONE_WEIGHTS_MAX_WEIGHTS;
				static const std::string MAKE_LEFT_HANDED;
				static const std::string OPTIMIZE_MESHES;
				static const std::string PRE_TRANSFORM_UV_COORDINATES;
				static const std::string PRE_TRANSFORM_VERTICES;
				static const std::string PRE_TRANSFORM_VERTICES_NORMALIZE;
				static const std::string REMOVE_COMPONENTS;
				static const std::string REMOVE_COMPONENTS_ANIMATIONS;
				static const std::string REMOVE_COMPONENTS_BI_TANGENTS;
				static const std::string REMOVE_COMPONENTS_BONE_WEIGHTS;
				static const std::string REMOVE_COMPONENTS_CAMERAS;
				static const std::string REMOVE_COMPONENTS_COLORS;
				static const std::string REMOVE_COMPONENTS_LIGHTS;
				static const std::string REMOVE_COMPONENTS_MATERIALS;
				static const std::string REMOVE_COMPONENTS_MESHES;
				static const std::string REMOVE_COMPONENTS_NORMALS;
				static const std::string REMOVE_COMPONENTS_TEXTURES;
				static const std::string REMOVE_COMPONENTS_TEXTURE_COORDINATES;
				static const std::string REMOVE_REDUNDANT_MATERIALS;
				static const std::string REMOVE_REDUNDANT_MATERIALS_SKIP;
				static const std::string REMOVE_REDUNDANT_NODES;
				static const std::string REMOVE_REDUNDANT_NODES_SKIP;
				static const std::string SORT_AND_REMOVE;
				static const std::string SORT_AND_REMOVE_POINTS;
				static const std::string SORT_AND_REMOVE_LINES;
				static const std::string SORT_AND_REMOVE_TRIANGLES;
				static const std::string SORT_AND_REMOVE_POLYGONS;
				static const std::string SPLIT_BY_BONE_COUNT;
				static const std::string SPLIT_BY_BONE_COUNT_MAX_BONES;
				static const std::string SPLIT_LARGE_MESHES;
				static const std::string SPLIT_LARGE_MESHES_TRIANGLE_LIMIT;
				static const std::string SPLIT_LARGE_MESHES_VERTEX_LIMIT;
				static const std::string TRIANGULATE;
				static const std::string VALIDATE_DATA_STRUCTURES;

			};
		}//namespace modelcreator
	}//namespace manipulator
}//namespace repo