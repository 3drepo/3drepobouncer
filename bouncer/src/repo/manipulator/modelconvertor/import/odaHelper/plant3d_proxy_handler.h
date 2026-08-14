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

#include "proxy_app_handler.h"

namespace repo {
	namespace manipulator {
		namespace modelconvertor {
			namespace odaHelper {
				/* Plant3D-specific proxy handling: class-name classification and
				extension-dictionary stored properties. No display-name map and no
				geometry capture exist for Plant3D yet - everything else is inherited
				as inert ProxyAppHandler defaults. This is the shape a future Plant3D
				geometry-capture feature, or any other app, would follow. */
				class Plant3DProxyHandler : public ProxyAppHandler
				{
				public:
					bool matches(const std::string& originalClass) const override;
					ProxyAppType appType() const override { return ProxyAppType::Plant3D; }
					std::string appName() const override { return "Plant3D"; }

					void addDictionaryMetadata(
						OdDbDictionaryPtr dict,
						std::unordered_map<std::string, repo::lib::RepoVariant>& metadata) const override;
				};
			}
		}
	}
}
