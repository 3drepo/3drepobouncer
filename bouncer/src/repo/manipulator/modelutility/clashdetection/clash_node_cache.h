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
#include <stdexcept>

namespace repo {
	namespace manipulator {
		namespace modelutility {
			namespace clash {

				/*
				* ResourceCache is a simple resource manager for use by the Clash Pipeline
				* implementations. It allows them to cache intermediate data that might be
				* reused across tests during each stage.
				* 
				* Callers first create a Record for a given Key, and use this object to
				* request references to the Node stored in it. Once all desired references
				* are acquired, finalise() is called on the cache that will drop all records.
				* From that moment on, all nodes are held by the references and once those all
				* went out of scope, they will be removed too.
				* 
				* Records are keyed by the objects that are themselves used to initialise
				* the intermediate structures (which should be done by the Cache itself
				* in the overridden initialise method). The caller can specify anything they
				* want to hold the intermediate data.
				* 
				* ResourceCache does not use the key's equality operators - do not use this
				* class outside the clash pipeline as this behaviour may be unexpected!
				*/

				template<typename Key, typename Node>
				class ResourceCache
				{
				public:

					using Entry = std::shared_ptr<Node>;

					struct Record
					{
						// Node must be default-constructible
						Entry node;

						Record()
						{
							node = std::make_shared<Node>();
						}

						Entry getReference()
						{
							return node;
						}
					};

					/*
					* Gets (or creates) a record associated with the given key. Once a record
					* is no longer needed, it should be destroyed by calling destroy. If
					* finalise() is called, this happens automatically.
					*/
					Record* get(const Key& key) {

						if (!map.contains(&key))
						{
							auto newRecord = std::make_unique<Record>();
							initialise(key, newRecord->node.get());
							map[&key] = std::move(newRecord);
						}

						return map[&key].get();
					}

					void release(const Key& key) {
						auto it = map.find(&key);
						if (it != map.end()) {
							map.erase(it);
						}
					}

					/*
					* "Finalises" the cache by releasing all records.
					* After this, the Nodes are only held by all references acquired through getReference().
					* Nodes that don't have any references pointing at them will be removed now.
					* Nodes with references will be removed once all of the references go out of scope.
					*/
					void finalise() {
						map.clear();
					}

					/*
					* Create a new cache entry node if one is requested for a key that does
					* not yet exist. Nodes should be cheap to make (and copy), until their
					* content is initialised downstream.
					*/
					virtual void initialise(const Key& key, Node* node) const = 0;

				protected:
					// The map itself uses the pointers to the keying objects, as inbuilt hash
					// and equality operators exist for pointers, but if we try and store
					// references the map will expect operators for a semantic comparision of
					// the actual types to be available.
					//
					// As std::map is an associative container, references remain valid until
					// the associated entry is removed, even if other entries are emplaced or
					// erased.

					std::unordered_map<const Key*, std::unique_ptr<Record>> map;

					~ResourceCache() noexcept(false) {
						if(map.size()) {
							throw std::runtime_error("ResourceCache destroyed while still holding records");
						}
					}
				};
			}
		}
	}
}