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
				* request references to the Node stored in it. These references are counted,
				* and once they all go out of scope, the Record is removed from the cache.
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
					struct Record
					{
						// Node must be default-constructible
						Node node;

						size_t referenceCount = 0;
						ResourceCache* cache = nullptr;
						const Key* key = nullptr;

						/*
						* Takes a counted reference to the Record. Once all references go out of
						* scope, the Record will be destroyed. If no reference is ever taken,
						* the record will only be destroyed with the cache itself.
						*/
						std::unique_ptr<Node, Record&> getReference() {
							referenceCount++;
							return { &node, *this };
						}

						/*
						* Record is its own deleter for the purposes of the unique_ptrs. We can pass
						* a reference to the pointers, which is very lightweight.
						*/
						void operator()(Node* ptr) {
							referenceCount--;
							if (!referenceCount) {
								cache->release(*key);
							}
						}
					};

					using Entry = std::unique_ptr<Node, Record&>;

					/*
					* Gets (or creates) a record associated with the given key. Once a record
					* is no longer needed, it should be destroyed by calling destroy. If the
					* record has references taken from it, this will be done automatically.
					*/
					Record* get(const Key& key) {

						Record* record = nullptr;
						auto it = map.find(&key);
						if (it == map.end())
						{
							record = &map[&key];
							record->key = &key;
							record->cache = this;
							initialise(key, record->node);
						}
						else
						{
							record = &it->second;
						}
						return record;
					}

					void release(const Key& key) {
						auto it = map.find(&key);
						if (it != map.end()) {
							map.erase(it);
						}
					}

					/*
					* Create a new cache entry node if one is requested for a key that does
					* not yet exist. Nodes should be cheap to make (and copy), until their
					* content is initialised downstream.
					*/
					virtual void initialise(const Key& key, Node& node) const = 0;

				protected:
					// The map itself uses the pointers to the keying objects, as inbuilt hash
					// and equality operators exist for pointers, but if we try and store
					// references the map will expect operators for a semantic comparision of
					// the actual types to be available.
					//
					// As std::map is an associative container, references remain valid until
					// the associated entry is removed, even if other entries are emplaced or
					// erased.

					std::unordered_map<const Key*, Record> map;

					~ResourceCache() {
						if(map.size()) {
							throw std::runtime_error("ResourceCache destroyed while still holding records");
						}
					}
				};
			}
		}
	}
}