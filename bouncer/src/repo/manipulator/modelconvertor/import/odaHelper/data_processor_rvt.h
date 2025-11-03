/**
*  Copyright (C) 2018 3D Repo Ltd
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
#pragma once
#include <SharedPtr.h>
#include <Gs/GsBaseInclude.h>
#include <Gs/GsBaseMaterialView.h>
#include <Gi/GiGeometrySimplifier.h>
#include <Gs/GsBaseInclude.h>
#include <Gs/GsBaseMaterialView.h>

#include <OdPlatformSettings.h>
#include <BimCommon.h>
#include <Database/BmElement.h>
#include <RxObjectImpl.h>
#include <ColorMapping.h>
#include <toString.h>
#include <Common/BuiltIns/BmBuiltInParameter.h>
#include <Database/Entities/BmMaterialElem.h>
#include <Geometry/Entities/BmMaterial.h>
#include <Database/BmAppearanceAssetHelper.h>
#include <Database/Entities/BmParamElem.h>
#include <Database/Entities/BmDirectShape.h>
#include <Database/Entities/BmDirectShapeCell.h>
#include <Database/Entities/BmDirectShapeDataCell.h>
#include <Database/Entities/BmElementHeader.h>

#include <Essential/Entities/BmLevel.h>

#include <Database/BmElement.h>
#include <Database/PE/BmLabelUtilsPE.h>
#include <Database/Entities/BmAUnits.h>
#include <Database/BmUnitUtils.h>
#include <Database/Managers/BmUnitsTracking.h>
#include <Database/Managers/BmElementTrackingData.h>
#include <Database/Entities/BmUnitsElem.h>
#include <Database/Entities/BmViewport.h>
#include <Database/Entities/BmHiddenElementsViewSettings.h>
#include <Essential/Entities/BmBasePoint.h>
#include <Essential/Entities/BmGeoLocation.h>
#include <Tf/TfVariant.h>

#include <TB_ExLabelUtils/BmSampleLabelUtilsPE.h>

#include "repo/lib/datastructure/repo_structs.h"
#include "geometry_collector.h"
#include "data_processor.h"

#include "repo/lib/datastructure/repo_variant.h"
#include "repo/lib/datastructure/repo_variant_utils.h"

#include <map>
#include <stack>

namespace repo {
	namespace manipulator {
		namespace modelconvertor {
			namespace odaHelper {
				class VectoriseDeviceRvt;

				class DataProcessorRvt : public OdGiGeometrySimplifier, public OdGsBaseMaterialView
				{
					//Environment variable name for Revit textures
					const char* RVT_TEXTURES_ENV_VARIABLE = "REPO_RVT_TEXTURES";

					GeometryCollector* collector;

				public:
					void initialise(GeometryCollector* collector, OdBmDatabasePtr pDb, OdBmDBViewPtr view, const OdGeMatrix3d& modelToWorld);

					static bool tryConvertMetadataEntry(
						const OdTfVariant& metaEntry,
						OdBmLabelUtilsPEPtr labelUtils,
						OdBmParamDefPtr paramDef,
						OdBm::BuiltInParameter::Enum param,
						repo::lib::RepoVariant& v);

					static OdBmAUnitsPtr getUnits(OdBmDatabasePtr database);
					static OdBmForgeTypeId getLengthUnits(OdBmDatabasePtr database);

				protected:
					void draw(const OdGiDrawable*) override;

					void beginViewVectorization() override;

				private:
					std::string determineTexturePath(const std::string& inputPath);

					void fillTexture(OdBmMaterialElemPtr materialPtr, repo::lib::repo_material_t& material, bool& missingTexture);
					void fillMaterial(OdBmMaterialElemPtr materialPtr, const OdGiMaterialTraitsData& materialData, repo::lib::repo_material_t& material);

					void fillMetadataById(
						OdBmObjectId id,
						std::unordered_map<std::string, repo::lib::RepoVariant>& metadata);

					void fillMetadataByElemPtr(
						OdBmElementPtr element,
						std::unordered_map<std::string, repo::lib::RepoVariant>& metadata);

					std::unordered_map<std::string, repo::lib::RepoVariant> fillMetadata(OdBmElementPtr element);
					std::string getLevelName(OdBmElementPtr element, const std::string& name);
					std::string getElementName(OdBmElementPtr element);

					void initLabelUtils();

					bool ignoreParam(const std::string& param);

					void processParameter(
						const OdTfVariant& value,
						OdBmObjectId paramId,
						std::unordered_map<std::string, repo::lib::RepoVariant> &metadata,
						const OdBm::BuiltInParameter::Enum &buildInEnum
					);

					OdBmDatabasePtr database;
					OdBmDBViewPtr view;
					OdBmSampleLabelUtilsPE* labelUtils = nullptr;

					OdGeMatrix3d modelToProjectCoordinates;

					repo::lib::RepoVector3D64 convertToRepoWorldCoordinates(OdGePoint3d p);

					/*
					* Controls whether and how we change the draw context during tree traversal.
					* The draw context controls which transformation nodes mesh data ends up
					* under.
					*/
					bool shouldCreateNewLayer(const OdBmElementPtr element);

					void convertTo3DRepoTriangle(
						const OdInt32* p3Vertices,
						std::vector<repo::lib::RepoVector3D64>& verticesOut,
						repo::lib::RepoVector3D64& normalOut,
						std::vector<repo::lib::RepoVector2D>& uvOut);


					// These functions override the outptus from the ODA visualisation classes
					// and are used to recieve the geometry

					/**
					* This callback is invoked when next triangle should be processed
					* defined in OdGiGeometrySimplifier class
					* @param p3Vertices - input vertices of the triangle
					* @param pNormal - input veritces normal
					*/
					void triangleOut(const OdInt32* p3Vertices,
						const OdGeVector3d* pNormal) final;

					/**
					* This callback is invoked when the next line should be processed.
					* Defined in OdGiGeometrySimplifier
					* @param numPoints Number of points.
					* @param vertexIndexList Pointer to an array of vertex indices.
					*/
					void polylineOut(OdInt32 numPoints, 
						const OdInt32* vertexIndexList) final;

					void polylineOut(OdInt32 numPoints,
						const OdGePoint3d* vertexList) final;

					/**
					* This callback is invoked when next material should be processed
					* @param prevCache - previous material cache
					* @param materialId - database id of the material
					* @param materialData - structure with material data
					*/
					OdGiMaterialItemPtr fillMaterialCache(
						OdGiMaterialItemPtr prevCache,
						OdDbStub* materialId,
						const OdGiMaterialTraitsData& materialData) final;

					std::unordered_map<OdUInt64, repo::lib::repo_material_t> materialCache;
				};
			}
		}
	}
}
