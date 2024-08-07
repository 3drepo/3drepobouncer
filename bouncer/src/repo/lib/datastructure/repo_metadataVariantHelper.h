/**
*  Copyright (C) 2024 3D Repo Ltd
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


#include <string>
#include "boost/variant.hpp"
#include "assimp/scene.h"

#include "repo_vector.h"
#include "repo_metadataVariant.h"
#include <ctime>
#include <iostream>

// For ODA
#include "repo/manipulator/modelconvertor/import/odaHelper/helper_functions.h"

// For NWG
#include <NwProperty.h>
#include <NwColor.h>

// For RVT
#include <Database/Entities/BmParamElem.h>
#include <TB_ExLabelUtils/BmSampleLabelUtilsPE.h>
#include <Base/BmSpecTypeId.h>
#include <Tf/TfVariant.h>


namespace repo {
	namespace lib {


		class MetadataVariantHelper  
		{		
		public:		
			static bool TryConvert(aiMetadataEntry& assimpMetaEntry, MetadataVariant& v);

			static bool TryConvert(OdNwPropertyPtr& metaProperty, MetadataVariant& v);

			static bool TryConvert(
				OdTfVariant& metaEntry,
				OdBmLabelUtilsPEPtr labelUtils,
				OdBmParamDefPtr paramDef,
				OdBmDatabase* database,
				OdBm::BuiltInParameter::Enum param,
				MetadataVariant& v);
		};

		class DuplicationVisitor : public boost::static_visitor<bool> {
		public:

			template <typename T1, typename T2>
			bool operator()(const T1&, const T2&) const {
				return false; // Different types so it can't be equal
			}

			template <typename T>
			bool operator()(const T& lhs, const T& rhs) const {
				return lhs == rhs;
			}

			// Special case for tm, because it does not have an == operator
			bool operator()(const tm lhs, const tm rhs) const {
				tm lhsCpy = lhs; // Copy because mktime can alter the struct
				tm rhsCpy = rhs; // Copy because mktime can alter the struct

				time_t lhsTime = mktime(&lhsCpy);
				time_t rhsTime = mktime(&rhsCpy);

				return lhsTime == rhsTime;
			}
		};

		class StringConversionVisitor : public boost::static_visitor<std::string> {
		public:

			std::string operator()(bool& b) const {
				return std::to_string(b);
			}

			std::string operator()(int& i) const {
				return std::to_string(i);
			}

			std::string operator()(long long& ll) const {
				return std::to_string(ll);
			}

			std::string operator()(double& d) const {
				return std::to_string(d);
			}

			std::string operator()(std::string& s) const {
				return s;
			}

			std::string operator()(const tm& t) const {
				char buffer[80];

				strftime(buffer, sizeof(buffer), "%d-%m-%Y %H-%M-%S", &t);
				return std::string(buffer);
			}
		};
	}



}