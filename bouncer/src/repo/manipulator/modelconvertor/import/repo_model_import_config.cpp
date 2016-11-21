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

#include "repo_model_import_config.h"
#include "repo_model_import_config_default_values.h"
#include "../../../lib/repo_log.h"

#include <fstream>
#include <boost/program_options.hpp>

using namespace repo::manipulator::modelconvertor;

const std::string ModelImportConfig::CALCULATE_TANGENT_SPACE = "calculateTangentSpace";
const std::string ModelImportConfig::CALCULATE_TANGENT_SPACE_MAX_SMOOTHING_ANGLE = "calculateTangentSpaceMaxSmoothingAngle";
const std::string ModelImportConfig::CONVERT_TO_UV_COORDINATES = "convertToUVCoordinates";
const std::string ModelImportConfig::DEGENERATES_TO_POINTS_LINES = "degeneratesToPoinstLines";
const std::string ModelImportConfig::DEBONE = "debone";
const std::string ModelImportConfig::DEBONE_THRESHOLD = "deboneThreshold";
const std::string ModelImportConfig::DEBONE_ONLY_IF_ALL = "deboneOnlyIfAll";
const std::string ModelImportConfig::FIND_INSTANCES = "findInstances";
const std::string ModelImportConfig::FIND_INAVLID_DATA = "findInvalidData";
const std::string ModelImportConfig::FIND_INAVLID_DATA_ANIMATION_ACCURACY = "findInvalidDataAnimationAccuracy";
const std::string ModelImportConfig::FIX_INFACING_NORMALS = "fixInfacingNormals";
const std::string ModelImportConfig::FLIP_UV_COORDINATES = "flipUVCoordinates";
const std::string ModelImportConfig::FLIP_WINDING_ORDER = "flipWindingOrder";
const std::string ModelImportConfig::GENERATE_NORMALS = "generateNormals";
const std::string ModelImportConfig::GENERATE_NORMALS_FLAT = "generateNormalsFlat";
const std::string ModelImportConfig::GENERATE_NORMALS_SMOOTH = "generateNormalsSmooth";
const std::string ModelImportConfig::GENERATE_NORMALS_SMOOTH_CREASE_ANGLE = "generateNormalsSmoothCreaseAngle";
const std::string ModelImportConfig::IFC_SKIP_SPACE_REPRESENTATIONS = "skipIFCSpaceRepresentations";
const std::string ModelImportConfig::IMPROVE_CACHE_LOCALITY = "improveCacheLocality";
const std::string ModelImportConfig::IMPROVE_CACHE_LOCALITY_VERTEX_CACHE_SIZE = "improveCacheLocalityVertexCacheSize";
const std::string ModelImportConfig::JOIN_IDENTICAL_VERTICES = "joinIdenticalVertices";
const std::string ModelImportConfig::LIMIT_BONE_WEIGHTS = "limitBoneWeights";
const std::string ModelImportConfig::LIMIT_BONE_WEIGHTS_MAX_WEIGHTS = "limitBoneWeightsMaxWeight";
const std::string ModelImportConfig::MAKE_LEFT_HANDED = "makeLeftHanded";
const std::string ModelImportConfig::OPTIMIZE_MESHES = "optimizeMeshes";
const std::string ModelImportConfig::PRE_TRANSFORM_UV_COORDINATES = "preTransformUVCoordinates";
const std::string ModelImportConfig::PRE_TRANSFORM_VERTICES = "preTransformVertices";
const std::string ModelImportConfig::PRE_TRANSFORM_VERTICES_NORMALIZE = "preTransformVerticesNormalize";
const std::string ModelImportConfig::REMOVE_COMPONENTS = "removeComponents";
const std::string ModelImportConfig::REMOVE_COMPONENTS_ANIMATIONS = "removeComponentsAnimations";
const std::string ModelImportConfig::REMOVE_COMPONENTS_BI_TANGENTS = "removeComponentsBiTangents";
const std::string ModelImportConfig::REMOVE_COMPONENTS_BONE_WEIGHTS = "removeComponentsBoneWeights";
const std::string ModelImportConfig::REMOVE_COMPONENTS_CAMERAS = "removeComponentsCameras";
const std::string ModelImportConfig::REMOVE_COMPONENTS_COLORS = "removeComponentsColors";
const std::string ModelImportConfig::REMOVE_COMPONENTS_LIGHTS = "removeComponentsLights";
const std::string ModelImportConfig::REMOVE_COMPONENTS_MATERIALS = "removeComponentsMaterials";
const std::string ModelImportConfig::REMOVE_COMPONENTS_MESHES = "removeComponentsMeshes";
const std::string ModelImportConfig::REMOVE_COMPONENTS_NORMALS = "removeComponentsNormals";
const std::string ModelImportConfig::REMOVE_COMPONENTS_TEXTURES = "removeComponentsTextures";
const std::string ModelImportConfig::REMOVE_COMPONENTS_TEXTURE_COORDINATES = "removeComponentsTextureCoordinates";
const std::string ModelImportConfig::REMOVE_REDUNDANT_MATERIALS = "removeRedundantMaterials";
const std::string ModelImportConfig::REMOVE_REDUNDANT_MATERIALS_SKIP = "removeRedundantMaterialsSkip";
const std::string ModelImportConfig::REMOVE_REDUNDANT_NODES = "removeRedundantNodes";
const std::string ModelImportConfig::REMOVE_REDUNDANT_NODES_SKIP = "removeRedundantNodesSkip";
const std::string ModelImportConfig::SORT_AND_REMOVE = "sortAndRemove";
const std::string ModelImportConfig::SORT_AND_REMOVE_POINTS = "sortAndRemovePoints";
const std::string ModelImportConfig::SORT_AND_REMOVE_LINES = "sortAndRemoveLines";
const std::string ModelImportConfig::SORT_AND_REMOVE_TRIANGLES = "sortAndRemoveTriangles";
const std::string ModelImportConfig::SORT_AND_REMOVE_POLYGONS = "sortAndRemovePolygons";
const std::string ModelImportConfig::SPLIT_BY_BONE_COUNT = "splitByBoneCount";
const std::string ModelImportConfig::SPLIT_BY_BONE_COUNT_MAX_BONES = "splitByBoneCountMaxBones";
const std::string ModelImportConfig::SPLIT_LARGE_MESHES = "splitLargeMeshes";
const std::string ModelImportConfig::SPLIT_LARGE_MESHES_TRIANGLE_LIMIT = "splitLargeMeshesTriangleLimit";
const std::string ModelImportConfig::SPLIT_LARGE_MESHES_VERTEX_LIMIT = "splitLargeMeshesVertexLimit";
const std::string ModelImportConfig::TRIANGULATE = "triangulate";
const std::string ModelImportConfig::VALIDATE_DATA_STRUCTURES = "validateDataStructures";

//IFCOpenShell settings
const std::string ModelImportConfig::USE_IFC_OPEN_SHELL = "useIfcOpenShell";

ModelImportConfig::ModelImportConfig(
	const std::string &configFile)
{
	reset();

	//read config file if any
	std::ifstream conf;
	conf.open(configFile);
	if (conf.is_open())
	{
		repoInfo << "Reading config file " << configFile << "for  file import";

		readConfig(conf);
	}
	else
		repoTrace << "config File not found! using default settings for file import.";
}

ModelImportConfig::~ModelImportConfig()
{
}

void ModelImportConfig::readConfig(
	std::ifstream &conf)
{
	std::string name;
	boost::program_options::options_description desc("Options");
	desc.add_options()
		(CALCULATE_TANGENT_SPACE.c_str(), boost::program_options::value<bool>(&boolSettings[CALCULATE_TANGENT_SPACE]), "CALCULATE_TANGENT_SPACE")
		(CONVERT_TO_UV_COORDINATES.c_str(), boost::program_options::value<bool>(&boolSettings[CONVERT_TO_UV_COORDINATES]), "CONVERT_TO_UV_COORDINATES")
		(DEGENERATES_TO_POINTS_LINES.c_str(), boost::program_options::value<bool>(&boolSettings[DEGENERATES_TO_POINTS_LINES]), "DEGENERATES_TO_POINTS_LINES")
		(DEBONE.c_str(), boost::program_options::value<bool>(&boolSettings[DEBONE]), "DEBONE")
		(DEBONE_ONLY_IF_ALL.c_str(), boost::program_options::value<bool>(&boolSettings[DEBONE_ONLY_IF_ALL]), "DEBONE_ONLY_IF_ALL")
		(FIND_INSTANCES.c_str(), boost::program_options::value<bool>(&boolSettings[FIND_INSTANCES]), "FIND_INSTANCES")
		(FIND_INAVLID_DATA.c_str(), boost::program_options::value<bool>(&boolSettings[FIND_INAVLID_DATA]), "FIND_INAVLID_DATA")
		(FIND_INAVLID_DATA_ANIMATION_ACCURACY.c_str(), boost::program_options::value<bool>(&boolSettings[FIND_INAVLID_DATA_ANIMATION_ACCURACY]), "FIND_INAVLID_DATA_ANIMATION_ACCURACY")
		(FIX_INFACING_NORMALS.c_str(), boost::program_options::value<bool>(&boolSettings[FIX_INFACING_NORMALS]), "FIX_INFACING_NORMALS")
		(FLIP_UV_COORDINATES.c_str(), boost::program_options::value<bool>(&boolSettings[FLIP_UV_COORDINATES]), "FLIP_UV_COORDINATES")
		(FLIP_WINDING_ORDER.c_str(), boost::program_options::value<bool>(&boolSettings[FLIP_WINDING_ORDER]), "FLIP_WINDING_ORDER")
		(GENERATE_NORMALS.c_str(), boost::program_options::value<bool>(&boolSettings[GENERATE_NORMALS]), "GENERATE_NORMALS")
		(GENERATE_NORMALS_FLAT.c_str(), boost::program_options::value<bool>(&boolSettings[GENERATE_NORMALS_FLAT]), "GENERATE_NORMALS_FLAT")
		(GENERATE_NORMALS_SMOOTH.c_str(), boost::program_options::value<bool>(&boolSettings[FIX_INFACING_NORMALS]), "FIX_INFACING_NORMALS")
		(IMPROVE_CACHE_LOCALITY.c_str(), boost::program_options::value<bool>(&boolSettings[IMPROVE_CACHE_LOCALITY]), "IMPROVE_CACHE_LOCALITY")
		(JOIN_IDENTICAL_VERTICES.c_str(), boost::program_options::value<bool>(&boolSettings[JOIN_IDENTICAL_VERTICES]), "JOIN_IDENTICAL_VERTICES")
		(LIMIT_BONE_WEIGHTS.c_str(), boost::program_options::value<bool>(&boolSettings[LIMIT_BONE_WEIGHTS]), "LIMIT_BONE_WEIGHTS")
		(MAKE_LEFT_HANDED.c_str(), boost::program_options::value<bool>(&boolSettings[MAKE_LEFT_HANDED]), "MAKE_LEFT_HANDED")
		(OPTIMIZE_MESHES.c_str(), boost::program_options::value<bool>(&boolSettings[OPTIMIZE_MESHES]), "OPTIMIZE_MESHES")
		(PRE_TRANSFORM_UV_COORDINATES.c_str(), boost::program_options::value<bool>(&boolSettings[PRE_TRANSFORM_UV_COORDINATES]), "PRE_TRANSFORM_UV_COORDINATES")
		(PRE_TRANSFORM_VERTICES.c_str(), boost::program_options::value<bool>(&boolSettings[PRE_TRANSFORM_VERTICES]), "PRE_TRANSFORM_VERTICES")
		(PRE_TRANSFORM_VERTICES_NORMALIZE.c_str(), boost::program_options::value<bool>(&boolSettings[PRE_TRANSFORM_VERTICES_NORMALIZE]), "PRE_TRANSFORM_VERTICES_NORMALIZE")
		(REMOVE_COMPONENTS.c_str(), boost::program_options::value<bool>(&boolSettings[REMOVE_COMPONENTS]), "REMOVE_COMPONENTS")
		(REMOVE_COMPONENTS_ANIMATIONS.c_str(), boost::program_options::value<bool>(&boolSettings[REMOVE_COMPONENTS_ANIMATIONS]), "REMOVE_COMPONENTS_ANIMATIONS")
		(REMOVE_COMPONENTS_BI_TANGENTS.c_str(), boost::program_options::value<bool>(&boolSettings[REMOVE_COMPONENTS_BI_TANGENTS]), "REMOVE_COMPONENTS_BI_TANGENTS")
		(REMOVE_COMPONENTS_BONE_WEIGHTS.c_str(), boost::program_options::value<bool>(&boolSettings[REMOVE_COMPONENTS_BONE_WEIGHTS]), "REMOVE_COMPONENTS_BONE_WEIGHTS")
		(REMOVE_COMPONENTS_CAMERAS.c_str(), boost::program_options::value<bool>(&boolSettings[REMOVE_COMPONENTS_CAMERAS]), "REMOVE_COMPONENTS_CAMERAS")
		(REMOVE_COMPONENTS_COLORS.c_str(), boost::program_options::value<bool>(&boolSettings[REMOVE_COMPONENTS_COLORS]), "REMOVE_COMPONENTS_COLORS")
		(REMOVE_COMPONENTS_LIGHTS.c_str(), boost::program_options::value<bool>(&boolSettings[REMOVE_COMPONENTS_LIGHTS]), "REMOVE_COMPONENTS_LIGHTS")
		(REMOVE_COMPONENTS_MATERIALS.c_str(), boost::program_options::value<bool>(&boolSettings[REMOVE_COMPONENTS_MATERIALS]), "REMOVE_COMPONENTS_MATERIALS")
		(REMOVE_COMPONENTS_MESHES.c_str(), boost::program_options::value<bool>(&boolSettings[REMOVE_COMPONENTS_MESHES]), "REMOVE_COMPONENTS_MESHES")
		(REMOVE_COMPONENTS_NORMALS.c_str(), boost::program_options::value<bool>(&boolSettings[REMOVE_COMPONENTS_NORMALS]), "REMOVE_COMPONENTS_NORMALS")
		(REMOVE_COMPONENTS_TEXTURES.c_str(), boost::program_options::value<bool>(&boolSettings[REMOVE_COMPONENTS_TEXTURES]), "REMOVE_COMPONENTS_TEXTURES")
		(REMOVE_COMPONENTS_TEXTURE_COORDINATES.c_str(), boost::program_options::value<bool>(&boolSettings[REMOVE_COMPONENTS_TEXTURE_COORDINATES]), "REMOVE_COMPONENTS_TEXTURE_COORDINATES")
		(REMOVE_REDUNDANT_MATERIALS.c_str(), boost::program_options::value<bool>(&boolSettings[REMOVE_REDUNDANT_MATERIALS]), "REMOVE_REDUNDANT_MATERIALS")
		(REMOVE_REDUNDANT_NODES.c_str(), boost::program_options::value<bool>(&boolSettings[REMOVE_REDUNDANT_NODES]), "REMOVE_REDUNDANT_NODES")
		(SORT_AND_REMOVE.c_str(), boost::program_options::value<bool>(&boolSettings[SORT_AND_REMOVE]), "SORT_AND_REMOVE")
		(SORT_AND_REMOVE_POINTS.c_str(), boost::program_options::value<bool>(&boolSettings[SORT_AND_REMOVE_POINTS]), "SORT_AND_REMOVE_POINTS")
		(SORT_AND_REMOVE_LINES.c_str(), boost::program_options::value<bool>(&boolSettings[SORT_AND_REMOVE_LINES]), "SORT_AND_REMOVE_LINES")
		(SORT_AND_REMOVE_POLYGONS.c_str(), boost::program_options::value<bool>(&boolSettings[SORT_AND_REMOVE_POLYGONS]), "SORT_AND_REMOVE_POLYGONS")
		(SPLIT_BY_BONE_COUNT.c_str(), boost::program_options::value<bool>(&boolSettings[SPLIT_BY_BONE_COUNT]), "SPLIT_BY_BONE_COUNT")
		(SPLIT_LARGE_MESHES.c_str(), boost::program_options::value<bool>(&boolSettings[SPLIT_LARGE_MESHES]), "SPLIT_LARGE_MESHES")
		(TRIANGULATE.c_str(), boost::program_options::value<bool>(&boolSettings[TRIANGULATE]), "TRIANGULATE")
		(VALIDATE_DATA_STRUCTURES.c_str(), boost::program_options::value<bool>(&boolSettings[VALIDATE_DATA_STRUCTURES]), "VALIDATE_DATA_STRUCTURES")
		(USE_IFC_OPEN_SHELL.c_str(), boost::program_options::value<bool>(&boolSettings[USE_IFC_OPEN_SHELL]), "USE_IFC_OPEN_SHELL")

		(LIMIT_BONE_WEIGHTS_MAX_WEIGHTS.c_str(), boost::program_options::value<int32_t>(&intSettings[LIMIT_BONE_WEIGHTS_MAX_WEIGHTS]), "LIMIT_BONE_WEIGHTS_MAX_WEIGHTS")
		(IMPROVE_CACHE_LOCALITY_VERTEX_CACHE_SIZE.c_str(), boost::program_options::value<int32_t>(&intSettings[IMPROVE_CACHE_LOCALITY_VERTEX_CACHE_SIZE]), "IMPROVE_CACHE_LOCALITY_VERTEX_CACHE_SIZE")
		(SPLIT_LARGE_MESHES_TRIANGLE_LIMIT.c_str(), boost::program_options::value<int32_t>(&intSettings[SPLIT_LARGE_MESHES_TRIANGLE_LIMIT]), "SPLIT_LARGE_MESHES_TRIANGLE_LIMIT")
		(SPLIT_LARGE_MESHES_VERTEX_LIMIT.c_str(), boost::program_options::value<int32_t>(&intSettings[SPLIT_LARGE_MESHES_VERTEX_LIMIT]), "SPLIT_LARGE_MESHES_VERTEX_LIMIT")
		(SPLIT_BY_BONE_COUNT_MAX_BONES.c_str(), boost::program_options::value<int32_t>(&intSettings[SPLIT_BY_BONE_COUNT_MAX_BONES]), "SPLIT_BY_BONE_COUNT_MAX_BONES")

		(CALCULATE_TANGENT_SPACE_MAX_SMOOTHING_ANGLE.c_str(), boost::program_options::value<float>(&floatSettings[CALCULATE_TANGENT_SPACE_MAX_SMOOTHING_ANGLE]), "CALCULATE_TANGENT_SPACE_MAX_SMOOTHING_ANGLE")
		(DEBONE_THRESHOLD.c_str(), boost::program_options::value<float>(&floatSettings[DEBONE_THRESHOLD]), "DEBONE_THRESHOLD")
		(GENERATE_NORMALS_SMOOTH_CREASE_ANGLE.c_str(), boost::program_options::value<float>(&floatSettings[GENERATE_NORMALS_SMOOTH_CREASE_ANGLE]), "GENERATE_NORMALS_SMOOTH_CREASE_ANGLE")

		(REMOVE_REDUNDANT_MATERIALS_SKIP.c_str(), boost::program_options::value<std::string>(&stringSettings[REMOVE_REDUNDANT_MATERIALS_SKIP]), "REMOVE_REDUNDANT_MATERIALS_SKIP")
		(REMOVE_REDUNDANT_NODES_SKIP.c_str(), boost::program_options::value<std::string>(&stringSettings[REMOVE_REDUNDANT_NODES_SKIP]), "REMOVE_REDUNDANT_NODES_SKIP")
		;

	boost::program_options::variables_map vm;
	try{
		boost::program_options::store(boost::program_options::parse_config_file(conf, desc), vm);
		boost::program_options::notify(vm);
	}
	catch (boost::program_options::error &e)
	{
		repoError << "Error whilst reading file import configurations: " << e.what();
	}

	conf.close();
}

void ModelImportConfig::reset()
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
	boolSettings[CALCULATE_TANGENT_SPACE] = repoDefaultCalculateTangentSpace;
	boolSettings[CONVERT_TO_UV_COORDINATES] = repoDefaultConvertToUVCoordinates;
	boolSettings[DEGENERATES_TO_POINTS_LINES] = repoDefaultDegeneratesToPointsLines;
	boolSettings[DEBONE] = repoDefaultDebone;
	boolSettings[DEBONE_ONLY_IF_ALL] = repoDefaultDeboneOnlyIfAll;
	boolSettings[FIND_INSTANCES] = repoDefaultFindInstances;
	boolSettings[FIND_INAVLID_DATA] = repoDefaultFindInvalidData;
	boolSettings[FIND_INAVLID_DATA_ANIMATION_ACCURACY] = repoDefaultFindInvalidDataAnimationAccuracy;
	boolSettings[FIX_INFACING_NORMALS] = repoDefaultFixInfacingNormals;
	boolSettings[FLIP_UV_COORDINATES] = repoDefaultFlipUVCoordinates;
	boolSettings[FLIP_WINDING_ORDER] = repoDefaultFlipWindingOrder;
	boolSettings[GENERATE_NORMALS] = repoDefaultGenerateNormals;
	boolSettings[GENERATE_NORMALS_FLAT] = repoDefaultGenerateNormalsFlat;
	boolSettings[GENERATE_NORMALS_SMOOTH] = repoDefaultGenerateNormalsSmooth;
	boolSettings[IFC_SKIP_SPACE_REPRESENTATIONS] = repoDefaultIfcSkipSpaceRepresentation;
	boolSettings[IMPROVE_CACHE_LOCALITY] = repoDefaultImproveCacheLocality;
	boolSettings[JOIN_IDENTICAL_VERTICES] = repoDefaultJoinIdenticalVertices;
	boolSettings[LIMIT_BONE_WEIGHTS] = repoDefaultLimitBoneWeights;
	boolSettings[MAKE_LEFT_HANDED] = repoDefaultMakeLeftHanded;
	boolSettings[OPTIMIZE_MESHES] = repoDefaultOptimizeMeshes;
	boolSettings[PRE_TRANSFORM_UV_COORDINATES] = repoDefaultPreTransformUVCoordinates;
	boolSettings[PRE_TRANSFORM_VERTICES] = repoDefaultPreTransformVertices;
	boolSettings[PRE_TRANSFORM_VERTICES_NORMALIZE] = repoDefaultPreTransformVerticesNormalize;
	boolSettings[REMOVE_COMPONENTS] = repoDefaultRemoveComponents;
	boolSettings[REMOVE_COMPONENTS_ANIMATIONS] = repoDefaultRemoveComponentsAnimations;
	boolSettings[REMOVE_COMPONENTS_BI_TANGENTS] = repoDefaultRemoveComponentsBiTangents;
	boolSettings[REMOVE_COMPONENTS_BONE_WEIGHTS] = repoDefaultRemoveComponentsBoneWeights;
	boolSettings[REMOVE_COMPONENTS_CAMERAS] = repoDefaultRemoveComponentsCameras;
	boolSettings[REMOVE_COMPONENTS_COLORS] = repoDefaultRemoveComponentsColors;
	boolSettings[REMOVE_COMPONENTS_LIGHTS] = repoDefaultRemoveComponentsLights;
	boolSettings[REMOVE_COMPONENTS_MATERIALS] = repoDefaultRemoveComponentsMaterials;
	boolSettings[REMOVE_COMPONENTS_MESHES] = repoDefaultRemoveComponentsMeshes;
	boolSettings[REMOVE_COMPONENTS_NORMALS] = repoDefaultRemoveComponentsNormals;
	boolSettings[REMOVE_COMPONENTS_TEXTURES] = repoDefaultRemoveComponentsTextures;
	boolSettings[REMOVE_COMPONENTS_TEXTURE_COORDINATES] = repoDefaultRemoveComponentsTextureCoordinates;
	boolSettings[REMOVE_REDUNDANT_MATERIALS] = repoDefaultRemoveRedundantMaterials;
	boolSettings[REMOVE_REDUNDANT_NODES] = repoDefaultRemoveRedundantNodes;
	boolSettings[SORT_AND_REMOVE] = repoDefaultSortAndRemove;
	boolSettings[SORT_AND_REMOVE_POINTS] = repoDefaultSortAndRemovePoints;
	boolSettings[SORT_AND_REMOVE_LINES] = repoDefaultSortAndRemoveLines;
	boolSettings[SORT_AND_REMOVE_TRIANGLES] = repoDefaultSortAndRemoveTriangles;
	boolSettings[SORT_AND_REMOVE_POLYGONS] = repoDefaultSortAndRemovePolygons;
	boolSettings[SPLIT_BY_BONE_COUNT] = repoDefaultSplitByBoneCount;
	boolSettings[SPLIT_LARGE_MESHES] = repoDefaultSplitLargeMeshes;
	boolSettings[TRIANGULATE] = repoDefaultTriangulate;
	boolSettings[VALIDATE_DATA_STRUCTURES] = repoDefaultValidateDataStructures;
	boolSettings[USE_IFC_OPEN_SHELL] = repoDefaultUseIfcOpenShell;

	/**
	* ----------------- int fields ----------------------
	*/
	intSettings[LIMIT_BONE_WEIGHTS_MAX_WEIGHTS] = repoDefaultLimitBoneWeightsMaxWeight;
	intSettings[IMPROVE_CACHE_LOCALITY_VERTEX_CACHE_SIZE] = repoDefaultImproveCacheLocalityVertexCacheSize;
	intSettings[SPLIT_LARGE_MESHES_TRIANGLE_LIMIT] = repoDefaultSplitLargeMeshesTriangleLimit;
	intSettings[SPLIT_LARGE_MESHES_VERTEX_LIMIT] = repoDefaultSplitLargeMeshesVertexLimit;
	intSettings[SPLIT_BY_BONE_COUNT_MAX_BONES] = repoDefaultSplitByBoneCountMaxBones;

	/**
	* ----------------- float fields ----------------------
	*/
	floatSettings[CALCULATE_TANGENT_SPACE_MAX_SMOOTHING_ANGLE] = repoDefaultCalculateTangentSpaceMaxSmoothingAngle;
	floatSettings[DEBONE_THRESHOLD] = repoDefaultDeboneThreshold;
	floatSettings[GENERATE_NORMALS_SMOOTH_CREASE_ANGLE] = repoDefaultGenerateNormalsSmoothCreaseAngle;

	/**
	* ----------------- string fields ----------------------
	*/
	stringSettings[REMOVE_REDUNDANT_MATERIALS_SKIP] = repoDefaultRemoveRedundantMaterialsSkip;
	stringSettings[REMOVE_REDUNDANT_NODES_SKIP] = repoDefaultRemoveRedundantNodesSkip;
}