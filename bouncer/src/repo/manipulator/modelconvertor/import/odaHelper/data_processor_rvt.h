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
#include <Database/Entities/BmBasePoint.h>
#include <Database/Entities/BmGeoLocation.h>
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

				class DataProcessorRvtContext
				{
				public:
					DataProcessorRvtContext(repo::manipulator::modelutility::RepoSceneBuilder& builder);

					RepoMaterialBuilder materials;
					repo::manipulator::modelutility::RepoSceneBuilder& scene;
					repo::lib::RepoUUID rootNodeSharedId;
					repo_material_t lastUsedMaterial;

					/*
					* Convenience methods to create transformation nodes in the scene for the
					* given Id, or return the existing ones Id, if it exists, allowing the
					* processor to work with its local ids.
					*/
					void createLayer(std::string id, std::string name, std::string parent);
					repo::lib::RepoUUID getSharedId(std::string id);

				private:
					std::unordered_map<std::string, repo::lib::RepoUUID> layers;
				};

				class DataProcessorRvt : public OdGiGeometrySimplifier, public OdGsBaseMaterialView
				{
					//Environment variable name for Revit textures
					const char* RVT_TEXTURES_ENV_VARIABLE = "REPO_RVT_TEXTURES";

				public:
					void initialise(DataProcessorRvtContext* collector, OdBmDatabasePtr pDb, OdBmDBViewPtr view);

					static bool tryConvertMetadataEntry(
						OdTfVariant& metaEntry,
						OdBmLabelUtilsPEPtr labelUtils,
						OdBmParamDefPtr paramDef,
						OdBm::BuiltInParameter::Enum param,
						repo::lib::RepoVariant& v);

				protected:
					void draw(const OdGiDrawable*) override;

					void beginViewVectorization() override;

				private:
					std::string determineTexturePath(const std::string& inputPath);

					/*
					* This method sets the convertTo3DRepoWorldCoorindates functor to translate from
					* model space into Project Coordinates, including the conversion from internal
					* units to project units. For Revit, the Project Coordinate System is defined by
					* the active Survey Point.
					*/
					void establishProjectTranslation(OdBmDatabasePtr pDb);
					void establishWorldOffset(OdBmDBViewPtr view);
					void fillTexture(OdBmMaterialElemPtr materialPtr, repo_material_t& material, bool& missingTexture);
					void fillMaterial(OdBmMaterialElemPtr materialPtr, const OdGiMaterialTraitsData& materialData, repo_material_t& material);

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
					OdBmAUnitsPtr getUnits(OdBmDatabasePtr database);
					OdBmForgeTypeId getLengthUnits(OdBmDatabasePtr database);

					bool ignoreParam(const std::string& param);

					void processParameter(
						OdBmElementPtr element,
						OdBmObjectId paramId,
						std::unordered_map<std::string, repo::lib::RepoVariant> &metadata,
						const OdBm::BuiltInParameter::Enum &buildInEnum
					);

					ModelUnits getProjectUnits(OdBmDatabasePtr pDb);

					/*
					* Gets the bounds of the geometry in model space. This is an expensive operation
					* and should only be called once.
					*/
					OdGeExtents3d getModelBounds(OdBmDBViewPtr view);

					OdBmDatabasePtr database;
					OdBmDBViewPtr view;
					OdBmSampleLabelUtilsPE* labelUtils = nullptr;

					class DrawContext
					{
					public:
						repo::lib::RepoVector3D64 offset;
						std::unordered_map<uint64_t, std::unique_ptr<RepoMeshBuilder>> meshBuilders;

						RepoMeshBuilder* meshBuilder;

						/*
						* Creates a MeshBuilder for the material, and sets it as the active
						* one.
						*/
						void setMaterial(const repo_material_t& material);
					};

					std::unique_ptr<DrawContext> createNewDrawContext();

					struct DrawFrame
					{
						DrawFrame(OdBmElementPtr element)
							:element(element),
							context(nullptr)
						{
						}

						OdBmElementPtr element;
						std::unique_ptr<DrawContext> context;
					};

					DataProcessorRvtContext* importContext;
					DrawContext* drawContext;
					std::stack<DrawFrame*> drawStack;

					OdGeMatrix3d modelToProjectCoordinates;

					repo::lib::RepoVector3D64 convertToRepoWorldCoordinates(OdGePoint3d p);

					/*
					* Controls whether and how we change the draw context during tree traversal.
					* The draw context controls which transformation nodes mesh data ends up
					* under.
					*/
					bool shouldCreateNewLayer(const OdBmElementPtr element);

					repo::lib::RepoVector3D64 offset;

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
				};
			}
		}
	}
}
