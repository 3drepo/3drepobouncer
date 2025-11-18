/**
*  Copyright (C) 2025 3D Repo Ltd
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

#include <memory>
#include <unordered_map>

namespace repo {
	namespace manipulator {
		namespace modelutility {
			namespace clash {

				/*
				* ResourceCache is a simple resource manager for use by the Clash Pipeline
				* implementations. It allows them to cache intermediate data that might be
				* reused across tests during each stage.
				* 
				* Records are keyed by the objects that are themselves used to initialise
				* the intermediate structures (which should be done by the Cache itself
				* in the overridden initialise method). The caller can specify anything they
				* want to hold the intermediate data.
				* 
				* ResourceCache does not use the key's equality operators; do not use this
				* outside the clash pipeline as the behaviour this way may be unexpected.
				*/

				template<typename Key, typename Node>
				class ResourceCache
				{
				private:
					struct Record {
						Node node;
						size_t referenceCount = 0;
					};
					
					struct Deleter {
						ResourceCache& cache;
						Key* key;
						void operator()(Node* ptr) {
							cache.release(key);
						}
					};;

				public:
					using Entry = std::unique_ptr<Node, Deleter>;

					Entry get(Key& key) {

						Record* record = nullptr;
						auto it = map.find(&key);
						if (it == map.end())
						{
							record = &map[&key];
							initialise(key, record->node);
						}
						else
						{
							record = &it->second;
						}

						record->referenceCount++;
						Deleter deleter{ *this, &key };
						return { &record->node, deleter };
					}

					/*
					* Create a new cache entry node if one is requested for a key that does
					* not yet exist. Nodes should be cheap to make (and copy), until their
					* content is initialised downstream.
					*/
					virtual void initialise(Key& key, Node& node) const = 0;

				protected:
					// The map itself uses the pointers to the keying objects, as inbuilt hash
					// and equality operators exist for pointers, but if we try and store
					// references the map will expect operators for a semantic comparision of
					// the actual types to be available.
					// Since the pointer value is stored in the deleter, technically the key
					// could be deleted after a time and the cache would still work. This should
					// not be allowed to happen however as it is always possible another key may
					// be allocated to the same address.

					std::unordered_map<Key*, Record> map;

					void release(Key* key) {
						auto it = map.find(key);
						if (it != map.end()) {
							it->second.referenceCount--;
							if (it->second.referenceCount == 0) {
								map.erase(it);
							}
						}
					}
				};
			}
		}
	}
}