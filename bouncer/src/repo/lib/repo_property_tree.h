/**
*  Copyright (C) 2016 3D Repo Ltd
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

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/lexical_cast.hpp>
//#include <boost/property_tree/json_parser.hpp>
#include "json_parser.h"
#include "../core/model/repo_node_utils.h"

namespace repo{
	namespace lib{
		class PropertyTree
		{
		public:
			PropertyTree();
			PropertyTree(const bool &enableJSONWorkAround);
			~PropertyTree();

			/**
			* Add an array of objects to the tree at a specified level
			* This is used for things like arrays of objects within JSON
			* @param label indicating where the child lives in the tree
			* @param value vector of children to add
			*/
			void addArrayObjects(
				const std::string               &label,
				const std::vector<PropertyTree> &value)
			{
				boost::property_tree::ptree arraytree;
				for (size_t i = 0; i < value.size(); ++i)
				{
					arraytree.push_back(std::make_pair("", value[i].tree));
				}

				tree.add_child(label, arraytree);
			}

			/**
			* Add an attribute to the field
			* This is very xml specific
			* it is to achieve things like <FIELD NAME="Hi">
			* instead of <FIELD>"Hi"</FIELD>
			* The type fo the value can be of any type as long as
			* it is a qualified member of std::to_string()
			* @param label field name to add to
			* @param attribute name of attribute
			* @param value value of attribute
			*/
			template <typename T>
			void addFieldAttribute(
				const std::string &label,
				const std::string &attribute,
				const T &value
				)
			{
				addFieldAttribute(label, attribute, boost::lexical_cast<std::string>(value));
			}

			/**
			* Add an attribute to the field
			* This is very xml specific
			* it is to achieve things like <FIELD NAME="Hi">
			* instead of <FIELD>"Hi"</FIELD>
			* The type of the vector can be of any type as long as
			* it is a qualified member of std::to_string()
			* @param label field name to add to
			* @param attribute name of attribute
			* @param value value of attribute
			*/
			template <typename T>
			void addFieldAttribute(
				const std::string &label,
				const std::string &attribute,
				const std::vector<T> &value,
				const bool        &useDelimiter = true
				)
			{
				std::stringstream ss;
				for (uint32_t i = 0; i < value.size(); ++i)
				{
					ss << value[i];
					if (useDelimiter && i != value.size() - 1)
					{
						ss << ",";
					}
					else
						ss << " ";
				}
				std::string val = ss.str();
				addFieldAttribute(label, attribute, val);
			}

			/**
			* Add a children onto the tree
			* label can denote it's inheritance.
			* for e.g. trunk.branch.leaf would denote the child name is
			* leaf, with parent branch and grandparent trunk.
			* @param label indicating where the child lives in the tree
			* @param value value of the children
			*/
			template <typename T>
			void addToTree(
				const std::string           &label,
				const T                     &value)
			{
				if (label.empty())
					tree.put(label, value);
				else
					tree.add(label, value);
			}

			template <std::size_t N>
			void addToTree(
				const std::string           &label,
				const char(&value)[N])
			{
				addToTree(label, std::string(value));
			}

			/**
			* Add an array of values to the tree at a specified level
			* This is used for things like arrays within JSON
			* @param label indicating where the child lives in the tree
			* @param value vector of children to add
			* @param join merge vector into a string value instead of putting in a vector
			*/
			template <typename T>
			void addToTree(
				const std::string           &label,
				const std::vector<T>        &value,
				const bool                  &join = true)
			{
				if (join)
				{
					boost::property_tree::ptree arrayTree;
					for (const auto &child : value)
					{
						PropertyTree childTree;
						childTree.addToTree("", child);
						arrayTree.push_back(std::make_pair("", childTree.tree));
					}

					tree.add_child(label, arrayTree);
				}
				else
				{
					std::stringstream ss;
					for (size_t i = 0; i < value.size(); ++i)
					{
						ss << boost::lexical_cast<std::string>(value[i]);
						if (i != value.size() - 1)
							ss << " ";
					}

					std::string valueInStr = ss.str();
					addToTree(label, valueInStr);
				}
			}

			/**
			* Disable the JSON workaround for quotes
			* There is currently a work around to allow
			* json parser writes to remove quotes from
			* anything that isn't a string. Calling this
			* function will disable it. You might want to do this
			* BEFORE you add any data into the tree for XML
			* or any non JSON exports
			*/
			void disableJSONWorkaround()
			{
				hackStrings = false;
			}

			/**
			* Merging sub tree into this tree
			* @param label label where the subTree goes
			* @param subTree the sub tree to add
			*/
			void mergeSubTree(
				const std::string   &label,
				const PropertyTree  &subTree)
			{
				tree.add_child(label, subTree.tree);
			}

			/**
			* Write tree data onto a stream
			* @param stream stream to write onto
			*/
			void write_json(
				std::iostream &stream
				) const
			{
				boost::property_tree::write_json(stream, tree, false);
			}

			/**
			* Write tree data onto a stream
			* @param stream stream to write onto
			*/
			void write_xml(
				std::iostream &stream
				) const
			{
				std::stringstream ss;
				boost::property_tree::write_xml(ss, tree);
				std::string stringxml = ss.str();
				//revert the xml_parser's utility where it swaps tabs and newlines with codes
				boost::replace_all(stringxml, "&#9;", "\t");
				boost::replace_all(stringxml, "&#10;", "\n");
				stream << stringxml;
			}

		private:
			bool hackStrings;
			boost::property_tree::ptree tree;

			/**
			* Sanitize the names, adding escape keys where required
			* @param value the word to sanitize
			* @return returns a sanitized version of the word
			*/

			std::string sanitizeStr(const std::string &value);

			/**
			* Locate the first location of a special phrase that needs escaping
			* @param value the word to search through
			* @param start the starting index to search from
			* @return reutnrs the index of the special word, std::string::npos otherwise
			*/
			int findSpecialWords(
				const std::string &value,
				const int         &start);

			int findBackSlash(
				const std::string &value,
				const int         &start);
		};
		// Template specialization
		template <>
		void PropertyTree::addToTree<repo::lib::RepoVector3D>(
			const std::string             &label,
			const repo::lib::RepoVector3D &value);

		template <>
		void PropertyTree::addToTree<std::string>(
			const std::string           &label,
			const std::string           &value);

		template <>
		void PropertyTree::addToTree<repo::lib::RepoUUID>(
			const std::string           &label,
			const repo::lib::RepoUUID              &value);

		template <>
		void PropertyTree::addToTree(
			const std::string                &label,
			const std::vector<PropertyTree>  &value,
			const bool                       &join);

		template <>
		void PropertyTree::addFieldAttribute(
			const std::string &label,
			const std::string &attribute,
			const std::string &value
			);

		template <>
		void PropertyTree::addFieldAttribute(
			const std::string  &label,
			const std::string  &attribute,
			const repo::lib::RepoVector3D &value
			);
	}
}
