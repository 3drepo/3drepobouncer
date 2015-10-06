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
* General Model Convertor Config
* used to track all settings required for Model Convertor
*/

#pragma once

#include <string>
#include <map>
#include <stdint.h>

#include "../../../repo_bouncer_global.h"


namespace repo{
	namespace manipulator{
		namespace modelconvertor{

			class REPO_API_EXPORT ModelImportConfig
			{
			public:
				ModelImportConfig(const std::string &configFile = std::string());
				~ModelImportConfig();



				/**
				* ---------------- Getters -------------------------
				*/
				/*!
				* Returns true if calculate tangent space is checked in settings, false
				* otherwise. Defaults to false.
				*/
				virtual bool getCalculateTangentSpace() const
				{
					return boolSettings.at(CALCULATE_TANGENT_SPACE);
				}

				/*!
				* Returns the maximum smoothing angle for calc tangent space. Defaults to
				* 45.
				*/
				virtual float getCalculateTangentSpaceMaxSmoothingAngle() const
				{
					return floatSettings.at(CALCULATE_TANGENT_SPACE_MAX_SMOOTHING_ANGLE);
				}

				/*!
				* Returns true if convert to UV coords is checked in settings, false
				* otherwise. Defaults to false.
				*/
				virtual bool getConvertToUVCoordinates()  const
				{
					return boolSettings.at(CONVERT_TO_UV_COORDINATES);
				}

				/*!
				* Returns true if degenerates to points/lines is checked in settings, false
				* otherwise. Defaults to false.
				*/
				virtual bool getDegeneratesToPointsLines() const
				{
					return boolSettings.at(DEGENERATES_TO_POINTS_LINES);
				}

				/*!
				* Returns true if debone is checked in settings, false otherwise. Defaults
				* to false.
				*/
				virtual bool getDebone() const
				{
					return boolSettings.at(DEBONE);
				}

				/*!
				* Returns the debone threshold set in settings. Defaults to Assimp's
				* AI_DEBONE_THRESHOLD = 1.0f
				*/
				virtual float getDeboneThreshold() const
				{
					return floatSettings.at(DEBONE_THRESHOLD);
				}

				/*!
				* Returns true if debone only if all is checked in settings, false
				* otherwise. Defaults to false.
				*/
				virtual bool getDeboneOnlyIfAll() const
				{
					return boolSettings.at(DEBONE_ONLY_IF_ALL);
				}

				/*!
				* Returns true if find instances is checked in settings, false otherwise.
				* Defaults to false.
				*/
				virtual bool getFindInstances() const
				{
					return boolSettings.at(FIND_INSTANCES);
				}

				/*!
				* Returns true if find invalid data is checked in settings, false
				* otherwise. Defaults to false.
				*/
				virtual bool getFindInvalidData() const
				{
					return boolSettings.at(FIND_INAVLID_DATA);
				}

				/*!
				* Returns the animation accuracy for find invalid data set in settings.
				* Defaults to 0.0f == all comparisons are exact.
				* See http://assimp.sourceforge.net/lib_html/config_8h.html#ad223c5e7e63d2937685cc704a181b950
				*/
				virtual float getFindInvalidDataAnimationAccuracy() const
				{
					return floatSettings.at(FIND_INAVLID_DATA_ANIMATION_ACCURACY);
				}

				/*!
				* Returns true if fix infacing normals is checked in settings, false
				* otherwise. Defaults to false.
				*/
				virtual bool getFixInfacingNormals() const
				{
					return boolSettings.at(FIX_INFACING_NORMALS);
				}

				/*!
				* Returns true if flip UV coordinates is checked in settings, false
				* otherwise. Defaults to false.
				*/
				virtual bool getFlipUVCoordinates() const
				{
					return boolSettings.at(FLIP_UV_COORDINATES);
				}

				/*!
				* Returns true if flip winding order is checked in settings, false
				* otherwise. Defaults to false.
				*/
				virtual bool getFlipWindingOrder() const
				{
					return boolSettings.at(FLIP_WINDING_ORDER);
				}

				/*!
				* Returns true if generate normals is checked in settings, false otherwise.
				* Defaults to false.
				*/
				virtual bool getGenerateNormals() const
				{
					return boolSettings.at(GENERATE_NORMALS);
				}

				/*!
				* Returns true if generate flat normals is checked in settings, false
				* oterwise. Defaults to true.
				*/
				virtual bool getGenerateNormalsFlat() const
				{
					return boolSettings.at(GENERATE_NORMALS_FLAT);
				}

				/*!
				* Returns true if generate smooth normals is checked in settings, false
				* otherwise. Defaults to false.
				*/
				virtual bool getGenerateNormalsSmooth() const
				{
					return boolSettings.at(GENERATE_NORMALS_SMOOTH);
				}

				/*!
				* Returns the crease angle for generate smooth normals set in settings.
				* Defaults to 175.0f.
				*/
				virtual float getGenerateNormalsSmoothCreaseAngle() const
				{
					return floatSettings.at(GENERATE_NORMALS_SMOOTH_CREASE_ANGLE);
				}

				/*!
				* Returns true if improve cache locality is checked in settings, false
				* otherwise. Defaults to false.
				*/
				virtual bool getImproveCacheLocality() const
				{
					return boolSettings.at(IMPROVE_CACHE_LOCALITY);
				}

				/*!
				* Returns the vertex cache size for improve cache locality set in settings.
				* Defaults to PP_ICL_PTCACHE_SIZE = 12.
				*/
				virtual int getImproveCacheLocalityVertexCacheSize() const
				{
					return intSettings.at(IMPROVE_CACHE_LOCALITY_VERTEX_CACHE_SIZE);
				}

				/*!
				* Returns true if join identical vertices is checked in settings, false
				* otherwise. Defaults to false.
				*/
				virtual bool getJoinIdenticalVertices() const
				{
					return boolSettings.at(JOIN_IDENTICAL_VERTICES);
				}

				/*!
				* Returns true if limit bone weights is checked in settings, false
				* otherwise. Defaults to false.
				*/
				virtual bool getLimitBoneWeights() const
				{
					return boolSettings.at(LIMIT_BONE_WEIGHTS);
				}

				/*!
				* Returns the max weight for limit bone weights set in settings. Defaults
				* to AI_LMW_MAX_WEIGHTS = 4.
				*/
				virtual int getLimitBoneWeightsMaxWeight() const
				{

					return intSettings.at(LIMIT_BONE_WEIGHTS_MAX_WEIGHTS);
				}

				/*!
				* Returns true if make left handed is checked in settings, false otherwise.
				* Defaults to false.
				*/
				virtual bool getMakeLeftHanded() const
				{
					return boolSettings.at(MAKE_LEFT_HANDED);
				}

				/*!
				* Returns true if optimize meshes is checked in settings, false otherwise.
				* Defaults to false.
				*/
				virtual bool getOptimizeMeshes() const
				{
					return boolSettings.at(OPTIMIZE_MESHES);
				}

				/*!
				* Returns true if pre-transform UV coordinates is checked in settings,
				* false otherwise. Defaults to false.
				*/
				virtual bool getPreTransformUVCoordinates() const
				{
					return boolSettings.at(PRE_TRANSFORM_UV_COORDINATES);
				}

				/*!
				* Returns true if pre-transform vertices is checked in settings, false
				* otherwise. Defaults to false.
				*/
				virtual bool getPreTransformVertices() const
				{
					return boolSettings.at(PRE_TRANSFORM_VERTICES);
				}

				/*!
				* Returns true if normalize pre-transform vertices is checked in settings,
				* false otherwise. Defaults to false.
				*/
				virtual bool getPreTransformVerticesNormalize() const
				{
					return boolSettings.at(PRE_TRANSFORM_VERTICES_NORMALIZE);
				}

				/*!
				* Returns true if remove components is checked in settings, false otherwise.
				* Defaults to false.
				*/
				virtual bool getRemoveComponents() const
				{
					return boolSettings.at(REMOVE_COMPONENTS);
				}

				/*!
				* Returns true if remove components animations is checked in settings,
				* false otherwise. Defaults to false.
				*/
				virtual bool getRemoveComponentsAnimations() const
				{
					return boolSettings.at(REMOVE_COMPONENTS_ANIMATIONS);
				}

				/*!
				* Returns true if remove components bi/tangents is checked in settings,
				* false otherwise. Defaults to false.
				*/
				virtual bool getRemoveComponentsBiTangents() const
				{
					return boolSettings.at(REMOVE_COMPONENTS_BI_TANGENTS);
				}

				/*!
				* Returns true if remove components bone weights is checked in settings,
				* false otherwise. Defaults to false.
				*/
				virtual bool getRemoveComponentsBoneWeights() const
				{
					return boolSettings.at(REMOVE_COMPONENTS_BONE_WEIGHTS);
				}

				/*!
				* Returns true if remove components cameras is checked in settings, false
				* otherwise. Defaults to false.
				*/
				virtual bool getRemoveComponentsCameras() const
				{
					return boolSettings.at(REMOVE_COMPONENTS_CAMERAS);
				}

				/*!
				* Returns true if remove components colors is checked in settings, false
				* otherwise. Defaults to false.
				*/
				virtual bool getRemoveComponentsColors() const
				{
					return boolSettings.at(REMOVE_COMPONENTS_COLORS);
				}

				/*!
				* Returns true if remove components lights is checked in settings, false
				* otherwise. Defaults to false.
				*/
				virtual bool getRemoveComponentsLights() const
				{
					return boolSettings.at(REMOVE_COMPONENTS_LIGHTS);
				}

				/*!
				* Returns true if remove components materials is checked in settings, false
				* otherwise. Defaults to false.
				*/
				virtual bool getRemoveComponentsMaterials() const
				{
					return boolSettings.at(REMOVE_COMPONENTS_MATERIALS);
				}

				/*!
				* Returns true if remove components meshes is checked in settings, false
				* otherwise. Defaults to false.
				*/
				virtual bool getRemoveComponentsMeshes() const
				{
					return boolSettings.at(REMOVE_COMPONENTS_MESHES);
				}

				/*!
				* Returns true if remove components normals is checked in settings, false
				* otherwise. Defaults to false.
				*/
				virtual bool getRemoveComponentsNormals() const
				{
					return boolSettings.at(REMOVE_COMPONENTS_NORMALS);
				}

				/*!
				* Returns true if remove components textures is checked in settings, false
				* otherwise. Defaults to false.
				*/
				virtual bool getRemoveComponentsTextures() const
				{
					return boolSettings.at(REMOVE_COMPONENTS_TEXTURES);
				}

				/*!
				* Returns true if remove components texture coordinates is checked in
				* settings, false otherwise. Defaults to false.
				*/
				virtual bool getRemoveComponentsTextureCoordinates() const
				{
					return boolSettings.at(REMOVE_COMPONENTS_TEXTURE_COORDINATES);
				}

				/*!
				* Returns true if remove redundant materials is checked in settings, false
				* otherwise. Defaults to false.
				*/
				virtual bool getRemoveRedundantMaterials() const
				{
					return boolSettings.at(REMOVE_REDUNDANT_MATERIALS);
				}

				/*!
				* Returns a space-separated string of materials names to be skipped set in
				* settings. Defaults to empty string.
				*/
				virtual std::string getRemoveRedundantMaterialsSkip() const
				{
					return stringSettings.at(REMOVE_REDUNDANT_MATERIALS_SKIP);
				}

				/*!
				* Returns true if remove redundant nodes is checked in settings, false
				* otherwise. Defaults to false.
				*/
				virtual bool getRemoveRedundantNodes() const
				{
					return boolSettings.at(REMOVE_REDUNDANT_NODES);
				}

				/*!
				* Returns a space-separated string of node names to be skipped set in
				* settings. Defaults to empty string.
				*/
				virtual std::string getRemoveRedundantNodesSkip() const
				{
					return stringSettings.at(REMOVE_REDUNDANT_NODES_SKIP);
				}

				/*!
				* Returns ture if sort and remove is checked in settings, false otherwise.
				* Defaults to false.
				*/
				virtual bool getSortAndRemove() const
				{
					return boolSettings.at(SORT_AND_REMOVE);
				}

				/*!
				* Returns true if sort and remove lines is checked in settings, false
				* otherwise. Defaults to true.
				*/
				virtual bool getSortAndRemoveLines() const
				{
					return boolSettings.at(SORT_AND_REMOVE_LINES);
				}

				/*!
				* Returns true if sort and remove points is checked in settings, false
				* otherwise. Defaults to true.
				*/
				virtual bool getSortAndRemovePoints() const
				{
					return boolSettings.at(SORT_AND_REMOVE_POINTS);
				}

				/*!
				* Returns true if sort and remove points is checked in settings, false
				* otherwise. Defaults to false.
				*/
				virtual bool getSortAndRemovePolygons() const
				{
					return boolSettings.at(SORT_AND_REMOVE_POLYGONS);
				}

				/*!
				* Returns true if sort and remove triangles is checked in settings, false
				* otherwise. Defaults to false.
				*/
				virtual bool getSortAndRemoveTriangles() const
				{
					return boolSettings.at(SORT_AND_REMOVE_TRIANGLES);
				}

				/*!
				* Returns true if split by bone count is checked in settings, false
				* otherwise. Defaults to false.
				*/
				virtual bool getSplitByBoneCount() const
				{
					return boolSettings.at(SPLIT_BY_BONE_COUNT);
				}

				/*!
				* Returns a split by bones count max bones set in settings. Defaults to
				* AI_SBBC_DEFAULT_MAX_BONES = 60.
				*/
				virtual int getSplitByBoneCountMaxBones() const
				{
					return boolSettings.at(SPLIT_BY_BONE_COUNT_MAX_BONES);
				}

				/*!
				* Returns true if split large meshes is checked in settings, false
				* otherwise. Defaults to false.
				*/
				virtual bool getSplitLargeMeshes() const
				{
					return boolSettings.at(SPLIT_LARGE_MESHES);
				}

				/*!
				* Returns a split large meshes triangle limit set in settings. Defaults to
				* AI_SLM_DEFAULT_MAX_TRIANGLES = 1,000,000.
				*/
				virtual int getSplitLargeMeshesTriangleLimit() const
				{
					return intSettings.at(SPLIT_LARGE_MESHES_TRIANGLE_LIMIT);
				}

				/*!
				* Returns a split large meshes triangle limit set in settings. Defaults to
				* AI_SLM_DEFAULT_MAX_VERTICES = 1,000,000.
				*/
				virtual int getSplitLargeMeshesVertexLimit() const
				{
					return intSettings.at(SPLIT_LARGE_MESHES_VERTEX_LIMIT);
				}

				/*!
				* Returns true if triangulate is checked in settings, false otherwise.
				* Defaults to true.
				*/
				virtual bool getTriangulate() const
				{
					return boolSettings.at(TRIANGULATE);
				}

				/*!
				* Returns true if validate data structures is checked in settings, false
				* otherwise. Defaults to false.
				*/
				virtual bool getValidateDataStructures() const
				{
					return boolSettings.at(VALIDATE_DATA_STRUCTURES);
				}

				/**
				* ---------------- Setters -------------------------
				*/

				//! Sets the calculate tangent space to settings.
				virtual void setCalculateTangentSpace(bool on)
				{
					setValue(CALCULATE_TANGENT_SPACE, on);
				}

				virtual void setCalculateTangentSpaceMaxSmoothingAngle(float angle)
				{
					setValue(CALCULATE_TANGENT_SPACE_MAX_SMOOTHING_ANGLE, angle);
				}

				//! Sets the convert to UV coordinates to settings.
				virtual void setConvertToUVCoordinates(bool on)
				{
					setValue(CONVERT_TO_UV_COORDINATES, on);
				}

				//! Sets the degenerates to points and lines to settings.
				virtual void setDegeneratesToPointsLines(bool on)
				{
					setValue(DEGENERATES_TO_POINTS_LINES, on);
				}

				//! Sets the debone to settings.
				virtual void setDebone(bool on) { setValue(DEBONE, on); }

				//! Sets the debone threshold to settings.
				virtual void setDeboneThreshold(float t)
				{
					setValue(DEBONE_THRESHOLD, t);
				}

				virtual void setDeboneOnlyIfAll(bool on)
				{
					setValue(DEBONE_ONLY_IF_ALL, on);
				}

				virtual void setFindInstances(bool on)
				{
					setValue(FIND_INSTANCES, on);
				}

				virtual void setFindInvalidData(bool on)
				{
					setValue(FIND_INAVLID_DATA, on);
				}

				virtual void setFindInvalidDataAnimationAccuracy(float f)
				{
					setValue(FIND_INAVLID_DATA_ANIMATION_ACCURACY, f);
				}

				virtual void setFixInfacingNormals(bool on)
				{
					setValue(FIX_INFACING_NORMALS, on);
				}

				virtual void setFlipUVCoordinates(bool on)
				{
					setValue(FLIP_UV_COORDINATES, on);
				}

				virtual void setFlipWindingOrder(bool on)
				{
					setValue(FLIP_WINDING_ORDER, on);
				}

				virtual void setGenerateNormals(bool on)
				{
					setValue(GENERATE_NORMALS, on);
				}

				virtual void setGenerateNormalsFlat(bool on)
				{
					setValue(GENERATE_NORMALS_FLAT, on);
				}

				virtual void setGenerateNormalsSmooth(bool on)
				{
					setValue(GENERATE_NORMALS_SMOOTH, on);
				}

				virtual void setGenerateNormalsSmoothCreaseAngle(float f)
				{
					setValue(GENERATE_NORMALS_SMOOTH_CREASE_ANGLE, f);
				}

				virtual void setImproveCacheLocality(bool on)
				{
					setValue(IMPROVE_CACHE_LOCALITY, on);
				}

				virtual void setImproveCacheLocalityCacheSize(int vertexCount)
				{
					setValue(IMPROVE_CACHE_LOCALITY_VERTEX_CACHE_SIZE, vertexCount);
				}

				virtual void setJoinIdenticalVertices(bool on)
				{
					setValue(JOIN_IDENTICAL_VERTICES, on);
				}

				virtual void setLimitBoneWeights(bool on)
				{
					setValue(LIMIT_BONE_WEIGHTS, on);
				}

				virtual void setLimitBoneWeightsMaxWeights(int i)
				{
					setValue(LIMIT_BONE_WEIGHTS_MAX_WEIGHTS, i);
				}

				virtual void setMakeLeftHanded(bool on)
				{
					setValue(MAKE_LEFT_HANDED, on);
				}

				virtual void setOptimizeMeshes(bool on)
				{
					setValue(OPTIMIZE_MESHES, on);
				}

				virtual void setPreTransformUVCoordinates(bool on)
				{
					setValue(PRE_TRANSFORM_UV_COORDINATES, on);
				}

				virtual void setPreTransformVertices(bool on)
				{
					setValue(PRE_TRANSFORM_VERTICES, on);
				}

				virtual void setPreTransforVerticesNormalize(bool on)
				{
					setValue(PRE_TRANSFORM_VERTICES_NORMALIZE, on);
				}

				virtual void setRemoveComponents(bool on)
				{
					setValue(REMOVE_COMPONENTS, on);
				}

				virtual void setRemoveComponentsAnimations(bool on)
				{
					setValue(REMOVE_COMPONENTS_ANIMATIONS, on);
				}

				virtual void setRemoveComponentsBiTangents(bool on)
				{
					setValue(REMOVE_COMPONENTS_BI_TANGENTS, on);
				}

				virtual void setRemoveComponentsBoneWeights(bool on)
				{
					setValue(REMOVE_COMPONENTS_BONE_WEIGHTS, on);
				}

				virtual void setRemoveComponentsCameras(bool on)
				{
					setValue(REMOVE_COMPONENTS_CAMERAS, on);
				}

				virtual void setRemoveComponentsColors(bool on)
				{
					setValue(REMOVE_COMPONENTS_COLORS, on);
				}

				virtual void setRemoveComponentsLights(bool on)
				{
					setValue(REMOVE_COMPONENTS_LIGHTS, on);
				}

				virtual void setRemoveComponentsMaterials(bool on)
				{
					setValue(REMOVE_COMPONENTS_MATERIALS, on);
				}

				virtual void setRemoveComponentsMeshes(bool on)
				{
					setValue(REMOVE_COMPONENTS_MESHES, on);
				}

				virtual void setRemoveComponentsNormals(bool on)
				{
					setValue(REMOVE_COMPONENTS_NORMALS, on);
				}

				virtual void setRemoveComponentsTextures(bool on)
				{
					setValue(REMOVE_COMPONENTS_TEXTURES, on);
				}

				virtual void setRemoveComponentsTextureCoordinates(bool on)
				{
					setValue(REMOVE_COMPONENTS_TEXTURE_COORDINATES, on);
				}

				virtual void setRemoveRedundantMaterials(bool on)
				{
					setValue(REMOVE_REDUNDANT_MATERIALS, on);
				}

				virtual void setRemoveRedundantMaterialsSkip(const std::string& skip)
				{
					setValue(REMOVE_REDUNDANT_MATERIALS_SKIP, skip);
				}

				virtual void setRemoveRedundantNodes(bool on)
				{
					setValue(REMOVE_REDUNDANT_NODES, on);
				}

				virtual void setRemoveRedundantNodesSkip(const std::string& skip)
				{
					setValue(REMOVE_REDUNDANT_NODES_SKIP, skip);
				}

				virtual void setSortAndRemove(bool on)
				{
					setValue(SORT_AND_REMOVE, on);
				}

				virtual void setSortAndRemoveLines(bool on)
				{
					setValue(SORT_AND_REMOVE_LINES, on);
				}

				virtual void setSortAndRemovePoints(bool on)
				{
					setValue(SORT_AND_REMOVE_POINTS, on);
				}

				virtual void setSortAndRemovePolygons(bool on)
				{
					setValue(SORT_AND_REMOVE_POLYGONS, on);
				}

				virtual void setSortAndRemoveTriangles(bool on)
				{
					setValue(SORT_AND_REMOVE_TRIANGLES, on);
				}

				virtual void setSplitByBoneCount(bool on)
				{
					setValue(SPLIT_BY_BONE_COUNT, on);
				}

				virtual void setSplitByBoneCountMaxBones(int max)
				{
					setValue(SPLIT_BY_BONE_COUNT_MAX_BONES, max);
				}

				virtual void setSplitLargeMeshes(bool on)
				{
					setValue(SPLIT_LARGE_MESHES, on);
				}

				virtual void setSplitLargeMeshesTriangleLimit(int limit)
				{
					setValue(SPLIT_LARGE_MESHES_TRIANGLE_LIMIT, limit);
				}

				virtual void setSplitLargeMeshesVertexLimit(int limit)
				{
					setValue(SPLIT_LARGE_MESHES_VERTEX_LIMIT, limit);
				}

				virtual void setTriangulate(bool on)
				{
					setValue(TRIANGULATE, on);
				}

				virtual void setValidateDataStructures(bool on)
				{
					setValue(VALIDATE_DATA_STRUCTURES, on);
				}


			protected:
				/**
				* Resets all the settings to default
				* Also used at initialisation.
				*/
				void reset();

				/**
				* Read configuration from a file stream
				* @param conf file stream to read from
				*/
				void readConfig(
					std::ifstream &conf);

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
		}//namespace modelconvertor
	}//namespace manipulator
}//namespace repo
