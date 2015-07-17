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
* Model Convertor Config
* used to track all settings required for Model Convertor (mostly ASSIMP's)
*/

#include "repo_model_convertor_config.h"
#include "repo_model_convertor_config_default_values.h"



using namespace repo::manipulator::modelconvertor;

const std::string ModelConvertorConfig::CALCULATE_TANGENT_SPACE = "calculateTangentSpace";
const std::string ModelConvertorConfig::CALCULATE_TANGENT_SPACE_MAX_SMOOTHING_ANGLE = "calculateTangentSpaceMaxSmoothingAngle";
const std::string ModelConvertorConfig::CONVERT_TO_UV_COORDINATES = "convertToUVCoordinates";
const std::string ModelConvertorConfig::DEGENERATES_TO_POINTS_LINES = "degeneratesToPoinstLines";
const std::string ModelConvertorConfig::DEBONE = "debone";
const std::string ModelConvertorConfig::DEBONE_THRESHOLD = "deboneThreshold";
const std::string ModelConvertorConfig::DEBONE_ONLY_IF_ALL = "deboneOnlyIfAll";
const std::string ModelConvertorConfig::FIND_INSTANCES = "findInstances";
const std::string ModelConvertorConfig::FIND_INAVLID_DATA = "findInvalidData";
const std::string ModelConvertorConfig::FIND_INAVLID_DATA_ANIMATION_ACCURACY = "findInvalidDataAnimationAccuracy";
const std::string ModelConvertorConfig::FIX_INFACING_NORMALS = "fixInfacingNormals";
const std::string ModelConvertorConfig::FLIP_UV_COORDINATES = "flipUVCoordinates";
const std::string ModelConvertorConfig::FLIP_WINDING_ORDER = "flipWindingOrder";
const std::string ModelConvertorConfig::GENERATE_NORMALS = "generateNormals";
const std::string ModelConvertorConfig::GENERATE_NORMALS_FLAT = "generateNormalsFlat";
const std::string ModelConvertorConfig::GENERATE_NORMALS_SMOOTH = "generateNormalsSmooth";
const std::string ModelConvertorConfig::GENERATE_NORMALS_SMOOTH_CREASE_ANGLE = "generateNormalsSmoothCreaseAngle";
const std::string ModelConvertorConfig::IMPROVE_CACHE_LOCALITY = "improveCacheLocality";
const std::string ModelConvertorConfig::IMPROVE_CACHE_LOCALITY_VERTEX_CACHE_SIZE = "improveCacheLocalityVertexCacheSize";
const std::string ModelConvertorConfig::JOIN_IDENTICAL_VERTICES = "joinIdenticalVertices";
const std::string ModelConvertorConfig::LIMIT_BONE_WEIGHTS = "limitBoneWeights";
const std::string ModelConvertorConfig::LIMIT_BONE_WEIGHTS_MAX_WEIGHTS = "limitBoneWeightsMaxWeight";
const std::string ModelConvertorConfig::MAKE_LEFT_HANDED = "makeLeftHanded";
const std::string ModelConvertorConfig::OPTIMIZE_MESHES = "optimizeMeshes";
const std::string ModelConvertorConfig::PRE_TRANSFORM_UV_COORDINATES = "preTransformUVCoordinates";
const std::string ModelConvertorConfig::PRE_TRANSFORM_VERTICES = "preTransformVertices";
const std::string ModelConvertorConfig::PRE_TRANSFORM_VERTICES_NORMALIZE = "preTransformVerticesNormalize";
const std::string ModelConvertorConfig::REMOVE_COMPONENTS = "removeComponents";
const std::string ModelConvertorConfig::REMOVE_COMPONENTS_ANIMATIONS = "removeComponentsAnimations";
const std::string ModelConvertorConfig::REMOVE_COMPONENTS_BI_TANGENTS = "removeComponentsBiTangents";
const std::string ModelConvertorConfig::REMOVE_COMPONENTS_BONE_WEIGHTS = "removeComponentsBoneWeights";
const std::string ModelConvertorConfig::REMOVE_COMPONENTS_CAMERAS = "removeComponentsCameras";
const std::string ModelConvertorConfig::REMOVE_COMPONENTS_COLORS = "removeComponentsColors";
const std::string ModelConvertorConfig::REMOVE_COMPONENTS_LIGHTS = "removeComponentsLights";
const std::string ModelConvertorConfig::REMOVE_COMPONENTS_MATERIALS = "removeComponentsMaterials";
const std::string ModelConvertorConfig::REMOVE_COMPONENTS_MESHES = "removeComponentsMeshes";
const std::string ModelConvertorConfig::REMOVE_COMPONENTS_NORMALS = "removeComponentsNormals";
const std::string ModelConvertorConfig::REMOVE_COMPONENTS_TEXTURES = "removeComponentsTextures";
const std::string ModelConvertorConfig::REMOVE_COMPONENTS_TEXTURE_COORDINATES = "removeComponentsTextureCoordinates";
const std::string ModelConvertorConfig::REMOVE_REDUNDANT_MATERIALS = "removeRedundantMaterials";
const std::string ModelConvertorConfig::REMOVE_REDUNDANT_MATERIALS_SKIP = "removeRedundantMaterialsSkip";
const std::string ModelConvertorConfig::REMOVE_REDUNDANT_NODES = "removeRedundantNodes";
const std::string ModelConvertorConfig::REMOVE_REDUNDANT_NODES_SKIP = "removeRedundantNodesSkip";
const std::string ModelConvertorConfig::SORT_AND_REMOVE = "sortAndRemove";
const std::string ModelConvertorConfig::SORT_AND_REMOVE_POINTS = "sortAndRemovePoints";
const std::string ModelConvertorConfig::SORT_AND_REMOVE_LINES = "sortAndRemoveLines";
const std::string ModelConvertorConfig::SORT_AND_REMOVE_TRIANGLES = "sortAndRemoveTriangles";
const std::string ModelConvertorConfig::SORT_AND_REMOVE_POLYGONS = "sortAndRemovePolygons";
const std::string ModelConvertorConfig::SPLIT_BY_BONE_COUNT = "splitByBoneCount";
const std::string ModelConvertorConfig::SPLIT_BY_BONE_COUNT_MAX_BONES = "splitByBoneCountMaxBones";
const std::string ModelConvertorConfig::SPLIT_LARGE_MESHES = "splitLargeMeshes";
const std::string ModelConvertorConfig::SPLIT_LARGE_MESHES_TRIANGLE_LIMIT = "splitLargeMeshesTriangleLimit";
const std::string ModelConvertorConfig::SPLIT_LARGE_MESHES_VERTEX_LIMIT = "splitLargeMeshesVertexLimit";
const std::string ModelConvertorConfig::TRIANGULATE = "triangulate";
const std::string ModelConvertorConfig::VALIDATE_DATA_STRUCTURES = "validateDataStructures";

ModelConvertorConfig::ModelConvertorConfig()
{
	//call reset() to initialise all fields with defaults
	reset();
}


ModelConvertorConfig::~ModelConvertorConfig()
{
}

void ModelConvertorConfig::reset()
{
	//clear everything
	boolSettings.clear();
	intSettings.clear();
	floatSettings.clear();
	stringSettings.clear();

	//add in default values

	/**
	* ----------------- Bool fields ----------------------
	*/
	boolSettings[CALCULATE_TANGENT_SPACE]                      = repoDefaultCalculateTangentSpace;
	boolSettings[CONVERT_TO_UV_COORDINATES]                    = repoDefaultConvertToUVCoordinates;
	boolSettings[DEGENERATES_TO_POINTS_LINES]                  = repoDefaultDegeneratesToPointsLines;
	boolSettings[DEBONE]                                       = repoDefaultDebone;
	boolSettings[DEBONE_ONLY_IF_ALL]                           = repoDefaultDeboneOnlyIfAll;
	boolSettings[FIND_INSTANCES]                               = repoDefaultFindInstances;
	boolSettings[FIND_INAVLID_DATA]                            = repoDefaultFindInvalidData;
	boolSettings[FIND_INAVLID_DATA_ANIMATION_ACCURACY]         = repoDefaultFindInvalidDataAnimationAccuracy;
	boolSettings[FIX_INFACING_NORMALS]                         = repoDefaultFixInfacingNormals;
	boolSettings[FLIP_UV_COORDINATES]                          = repoDefaultFlipUVCoordinates;
	boolSettings[FLIP_WINDING_ORDER]                           = repoDefaultFlipWindingOrder;
	boolSettings[GENERATE_NORMALS]                             = repoDefaultGenerateNormals;
	boolSettings[GENERATE_NORMALS_FLAT]                        = repoDefaultGenerateNormalsFlat;
	boolSettings[GENERATE_NORMALS_SMOOTH]                      = repoDefaultGenerateNormalsSmooth;
	boolSettings[IMPROVE_CACHE_LOCALITY]                       = repoDefaultImproveCacheLocality;
	boolSettings[JOIN_IDENTICAL_VERTICES]                      = repoDefaultJoinIdenticalVertices;
	boolSettings[LIMIT_BONE_WEIGHTS]                           = repoDefaultLimitBoneWeights;
	boolSettings[MAKE_LEFT_HANDED]                             = repoDefaultMakeLeftHanded;
	boolSettings[OPTIMIZE_MESHES]                              = repoDefaultOptimizeMeshes;
	boolSettings[PRE_TRANSFORM_UV_COORDINATES]                 = repoDefaultPreTransformUVCoordinates;
	boolSettings[PRE_TRANSFORM_VERTICES]                       = repoDefaultPreTransformVertices;
	boolSettings[PRE_TRANSFORM_VERTICES_NORMALIZE]             = repoDefaultPreTransformVerticesNormalize;
	boolSettings[REMOVE_COMPONENTS]                            = repoDefaultRemoveComponents;
	boolSettings[REMOVE_COMPONENTS_ANIMATIONS]                 = repoDefaultRemoveComponentsAnimations;
	boolSettings[REMOVE_COMPONENTS_BI_TANGENTS]                = repoDefaultRemoveComponentsBiTangents;
	boolSettings[REMOVE_COMPONENTS_BONE_WEIGHTS]               = repoDefaultRemoveComponentsBoneWeights;
	boolSettings[REMOVE_COMPONENTS_CAMERAS]                    = repoDefaultRemoveComponentsCameras;
	boolSettings[REMOVE_COMPONENTS_COLORS]                     = repoDefaultRemoveComponentsColors;
	boolSettings[REMOVE_COMPONENTS_LIGHTS]                     = repoDefaultRemoveComponentsLights;
	boolSettings[REMOVE_COMPONENTS_MATERIALS]                  = repoDefaultRemoveComponentsMaterials;
	boolSettings[REMOVE_COMPONENTS_MESHES]                     = repoDefaultRemoveComponentsMeshes;
	boolSettings[REMOVE_COMPONENTS_NORMALS]                    = repoDefaultRemoveComponentsNormals;
	boolSettings[REMOVE_COMPONENTS_TEXTURES]                   = repoDefaultRemoveComponentsTextures;
	boolSettings[REMOVE_COMPONENTS_TEXTURE_COORDINATES]        = repoDefaultRemoveComponentsTextureCoordinates;
	boolSettings[REMOVE_REDUNDANT_MATERIALS]                   = repoDefaultRemoveRedundantMaterials;
	boolSettings[REMOVE_REDUNDANT_NODES]                       = repoDefaultRemoveRedundantNodes;
	boolSettings[SORT_AND_REMOVE]                              = repoDefaultSortAndRemove;
	boolSettings[SORT_AND_REMOVE_POINTS]                       = repoDefaultSortAndRemovePoints;
	boolSettings[SORT_AND_REMOVE_LINES]                        = repoDefaultSortAndRemoveLines;
	boolSettings[SORT_AND_REMOVE_TRIANGLES]                    = repoDefaultSortAndRemoveTriangles;
	boolSettings[SORT_AND_REMOVE_POLYGONS]                     = repoDefaultSortAndRemovePolygons;
	boolSettings[SPLIT_BY_BONE_COUNT]                          = repoDefaultSplitByBoneCount;
	boolSettings[SPLIT_LARGE_MESHES]                           = repoDefaultSplitLargeMeshes;
	boolSettings[TRIANGULATE]                                  = repoDefaultTriangulate;
	boolSettings[VALIDATE_DATA_STRUCTURES]                     = repoDefaultValidateDataStructures;


	/**
	* ----------------- int fields ----------------------
	*/
	intSettings[LIMIT_BONE_WEIGHTS_MAX_WEIGHTS]           = repoDefaultLimitBoneWeightsMaxWeight;
	intSettings[IMPROVE_CACHE_LOCALITY_VERTEX_CACHE_SIZE] = repoDefaultImproveCacheLocalityVertexCacheSize;
	intSettings[SPLIT_LARGE_MESHES_TRIANGLE_LIMIT]        = repoDefaultSplitLargeMeshesTriangleLimit;
	intSettings[SPLIT_LARGE_MESHES_VERTEX_LIMIT]          = repoDefaultSplitLargeMeshesVertexLimit;
	intSettings[SPLIT_BY_BONE_COUNT_MAX_BONES]            = repoDefaultSplitByBoneCountMaxBones;


	/**
	* ----------------- float fields ----------------------
	*/
	floatSettings[CALCULATE_TANGENT_SPACE_MAX_SMOOTHING_ANGLE]  = repoDefaultCalculateTangentSpaceMaxSmoothingAngle;
	floatSettings[DEBONE_THRESHOLD]                             = repoDefaultDeboneThreshold;
	floatSettings[GENERATE_NORMALS_SMOOTH_CREASE_ANGLE]         = repoDefaultGenerateNormalsSmoothCreaseAngle;


	/**
	* ----------------- string fields ----------------------
	*/
	stringSettings[REMOVE_REDUNDANT_MATERIALS_SKIP] = repoDefaultRemoveRedundantMaterialsSkip;
	stringSettings[REMOVE_REDUNDANT_NODES_SKIP]     = repoDefaultRemoveRedundantNodesSkip;

}