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
* Model import configuration default values
*/

#include <string>
#include <vector>

static const bool        repoDefaultCalculateTangentSpace = false;
static const bool        repoDefaultConvertToUVCoordinates = false;
static const bool        repoDefaultDegeneratesToPointsLines = false;
static const bool        repoDefaultDebone = false;
static const bool        repoDefaultDeboneOnlyIfAll = false;
static const bool        repoDefaultFindInstances = false;
static const bool        repoDefaultFindInvalidData = false;
static const bool        repoDefaultFindInvalidDataAnimationAccuracy = 0.0f;
static const bool        repoDefaultFixInfacingNormals = false;
static const bool        repoDefaultFlipUVCoordinates = false;
static const bool        repoDefaultFlipWindingOrder = false;
static const bool        repoDefaultGenerateNormals = true;
static const bool        repoDefaultGenerateNormalsFlat = true;
static const bool        repoDefaultGenerateNormalsSmooth = false;
static const bool        repoDefaultIfcSkipSpaceRepresentation = false;
static const bool        repoDefaultImproveCacheLocality = false;
static const bool        repoDefaultJoinIdenticalVertices = false;
static const bool        repoDefaultLimitBoneWeights = false;
static const bool        repoDefaultMakeLeftHanded = false;
static const bool        repoDefaultOptimizeMeshes = false;
static const bool        repoDefaultPreTransformUVCoordinates = false;
static const bool        repoDefaultPreTransformVertices = false;
static const bool        repoDefaultPreTransformVerticesNormalize = false;
static const bool        repoDefaultRemoveComponents = false;
static const bool        repoDefaultRemoveComponentsAnimations = false;
static const bool        repoDefaultRemoveComponentsBiTangents = false;
static const bool        repoDefaultRemoveComponentsBoneWeights = false;
static const bool        repoDefaultRemoveComponentsCameras = false;
static const bool        repoDefaultRemoveComponentsColors = false;
static const bool        repoDefaultRemoveComponentsLights = false;
static const bool        repoDefaultRemoveComponentsMaterials = false;
static const bool        repoDefaultRemoveComponentsMeshes = false;
static const bool        repoDefaultRemoveComponentsNormals = false;
static const bool        repoDefaultRemoveComponentsTextures = false;
static const bool        repoDefaultRemoveComponentsTextureCoordinates = false;
static const bool        repoDefaultRemoveRedundantMaterials = true;
static const bool        repoDefaultRemoveRedundantNodes = false;
static const bool        repoDefaultSortAndRemove = false;
static const bool        repoDefaultSortAndRemoveLines = true;
static const bool        repoDefaultSortAndRemovePoints = true;
static const bool        repoDefaultSortAndRemovePolygons = false;
static const bool        repoDefaultSortAndRemoveTriangles = false;
static const bool        repoDefaultSplitByBoneCount = false;
static const bool        repoDefaultSplitLargeMeshes = false;
static const bool        repoDefaultTriangulate = true;
static const bool        repoDefaultValidateDataStructures = false;
static const int         repoDefaultImproveCacheLocalityVertexCacheSize = 12;
static const int         repoDefaultLimitBoneWeightsMaxWeight = 4;
static const int         repoDefaultSplitByBoneCountMaxBones = 60;
static const int         repoDefaultSplitLargeMeshesTriangleLimit = 1000000;
static const int         repoDefaultSplitLargeMeshesVertexLimit = 1000000;
static const float       repoDefaultCalculateTangentSpaceMaxSmoothingAngle = 45.0f;
static const float       repoDefaultDeboneThreshold = 1.0f;
static const float       repoDefaultGenerateNormalsSmoothCreaseAngle = 175.0f;
static const std::string repoDefaultRemoveRedundantMaterialsSkip = "";
static const std::string repoDefaultRemoveRedundantNodesSkip = "";

//IfcOpenShell settings
static const bool		repoDefaultUseIfcOpenShell = true;
static const bool		repoDefaultIOSUseFilter = true;
static const bool		repoDefaultIsExclusion = true;
static const std::vector<std::string> repoDefaultIfcOpenShellFilterList = { "IfcOpeningElement", "IfcMember" };
static const bool       repoDefaultIOSWieldVertices = false;
static const bool       repoDefaultIOSUseWorldCoords = true;
static const bool       repoDefaultIOSConvertBackUnits = false;
static const bool       repoDefaultIOSUseBrepData = false;
static const bool       repoDefaultIOSSewShells = false;
static const bool       repoDefaultIOSFasterBooleans = false;
static const bool       repoDefaultIOSDisableOpeningSubtractions = false;
static const bool       repoDefaultIOSDisableTriangulate = false;
static const bool       repoDefaultIOSApplyDefaultMaterials = true;
static const bool       repoDefaultIOSIncludesCurves = false;
static const bool       repoDefaultIOSExcludesSolidsAndSurfaces = false;
static const bool       repoDefaultIOSNoNormals = false;
static const bool       repoDefaultIOSUseElementGuids = false;
static const bool       repoDefaultIOSUseElementNames = false;
static const bool       repoDefaultIOSUseMatNames = true;
static const bool       repoDefaultIOSCentreModel = false;
static const bool       repoDefaultIOSGenerateUVs = true;
static const bool       repoDefaultIOSApplyLayerSets = false;