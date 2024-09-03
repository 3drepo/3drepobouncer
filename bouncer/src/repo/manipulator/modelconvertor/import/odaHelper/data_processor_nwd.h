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

#include "../../../../core/model/bson/repo_node_mesh.h"
#include "geometry_collector.h"

#include <vector>
#include <string>

#include <OdaCommon.h>
#include <NwDatabase.h>
#include <NwDataProperty.h>
#include <NwVariant.h>
#include <NwName.h>

#include <repo/lib/datastructure/repo_metadataVariant.h>

#include <boost/filesystem.hpp>

namespace repo {
	namespace manipulator {
		namespace modelconvertor {
			namespace odaHelper {
				class DataProcessorNwd
				{
				public:
					DataProcessorNwd(GeometryCollector* collector) {
						this->collector = collector;
					}

					void process(OdNwDatabasePtr pNwDb);

				private:
					GeometryCollector* collector;
				};

				class NwdFilePathRemovalVisitor : public boost::static_visitor<> {
				public:
					NwdFilePathRemovalVisitor() {}

					// Do nothing for most cases
					void operator()(bool& b) const {}
					void operator()(int& i) const {}
					void operator()(long long& ll) const {}
					void operator()(double& d) const {}
					void operator()(tm& t) const {}

					// In the string case, we remove the file path
					void operator()(std::string& s) const {
						s = boost::filesystem::path(s).filename().string();
					}
				};

				bool TryConvertMetadataProperty(OdNwDataPropertyPtr& metaProperty, repo::lib::MetadataVariant& v);
			}
		}
	}
}