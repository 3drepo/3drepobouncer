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

#include "repo_property_tree.h"

using namespace repo::lib;

static const std::string XML_ATTR_TAG = "<xmlattr>";

PropertyTree::PropertyTree() :
hackStrings(true)
{
}

PropertyTree::PropertyTree(const bool &enableJSONWorkAround) :
hackStrings(enableJSONWorkAround)
{}

PropertyTree::~PropertyTree()
{
}

//see http://stackoverflow.com/questions/2855741/why-boost-property-tree-write-json-saves-everything-as-string-is-it-possible-to
struct stringTranslator
{
	typedef std::string internal_type;
	typedef std::string external_type;

	boost::optional<std::string> get_value(const std::string &v) { return  v.substr(1, v.size() - 2); }
	boost::optional<std::string> put_value(const std::string &v) { return '"' + v + '"'; }
};

template <>
void PropertyTree::addFieldAttribute(
	const std::string &label,
	const std::string &attribute,
	const std::string &value
	)
{
	std::string actualLabel = XML_ATTR_TAG;

	if (!label.empty())
		actualLabel = label + "." + actualLabel;

	addToTree(actualLabel + "." + attribute, value);
}

template <>
void PropertyTree::addFieldAttribute(
	const std::string  &label,
	const std::string  &attribute,
	const repo::lib::RepoVector3D &value
	)
{
	std::stringstream ss;
	ss << value.x << "," << value.y << "," << value.z;

	addFieldAttribute(label, attribute, ss.str());
}

template <>
void PropertyTree::addFieldAttribute(
	const std::string  &label,
	const std::string  &attribute,
	const repo::lib::RepoVector3D64 &value
)
{
	std::stringstream ss;
	ss << value.x << "," << value.y << "," << value.z;

	addFieldAttribute(label, attribute, ss.str());
}

template <>
void PropertyTree::addToTree<std::string>(
	const std::string           &label,
	const std::string           &value)
{
	const std::string labelSanitized = sanitizeStr(label);
	const std::string valueSanitized = sanitizeStr(value);
	if (hackStrings)
	{
		if (label.empty())
			tree.put(label, valueSanitized, stringTranslator());
		else
			tree.add(label, valueSanitized, stringTranslator());
	}
	else
	{
		if (label.empty())
			tree.put(label, valueSanitized);
		else
			tree.add(label, valueSanitized);
	}
}

PropertyTree::PropertyTree(const repo::lib::RepoVector3D& vector)
{
	addToTree("x", vector.x);
	addToTree("y", vector.y);
	addToTree("z", vector.z);
}

template <>
void PropertyTree::addToTree<repo::lib::RepoUUID>(
	const std::string          &label,
	const repo::lib::RepoUUID             &value)
{
	addToTree(label, value.toString());
}

template <>
void PropertyTree::addToTree<repo::lib::RepoVector3D>(
	const std::string  &label,
	const repo::lib::RepoVector3D &value
	)
{
	std::stringstream ss;
	ss << value.x << " " << value.y << " " << value.z;

	addToTree(label, ss.str());
}

template <>
void PropertyTree::addToTree<repo::lib::RepoVector3D64>(
	const std::string  &label,
	const repo::lib::RepoVector3D64 &value
	)
{
	std::stringstream ss;
	ss << value.x << " " << value.y << " " << value.z;

	addToTree(label, ss.str());
}

template <>
void PropertyTree::addToTree<repo_color4d_t>(
	const std::string& label,
	const repo_color4d_t& value)
{
	PropertyTree tree;
	tree.addToTree("r", value.r);
	tree.addToTree("g", value.g);
	tree.addToTree("b", value.b);
	tree.addToTree("a", value.a);
	mergeSubTree(label, tree);
}

template <>
void PropertyTree::addToTree<PropertyTree>(
	const std::string               &label,
	const std::vector<PropertyTree> &value,
	const bool                      &join)
{
	//join boolean is irrelevant here.
	if (value.size())
	{
		boost::property_tree::ptree arrayTree;
		for (const auto &child : value)
		{
			arrayTree.push_back(std::make_pair("", child.tree));
		}

		tree.add_child(label, arrayTree);
	}
}

std::string PropertyTree::sanitizeStr(
	const std::string &value)
{
	int startIndex = 0;
	std::string word = value;
	while ((startIndex = findBackSlash(word, startIndex)) != std::string::npos)
	{
		char newWord[2];
		newWord[1] = word[startIndex];
		newWord[0] = '\\';
		word.erase(word.begin() + startIndex);
		word.insert(startIndex, newWord, 2);

		startIndex += 2; //skip the newly added "\" and also the first character of the special word
	}
	startIndex = 0;
	while ((startIndex = findSpecialWords(word, startIndex)) != std::string::npos)
	{
		char newWord[2];
		newWord[1] = word[startIndex];
		newWord[0] = '\\';
		word.erase(word.begin() + startIndex);
		word.insert(startIndex, newWord, 2);

		startIndex += 2; //skip the newly added "\" and also the first character of the special word
	}

	return word;
}

int PropertyTree::findBackSlash(
	const std::string &value,
	const int         &start)
{
	return  value.find("\\", start);
}

int PropertyTree::findSpecialWords(
	const std::string &value,
	const int         &start)
{
	size_t ret = std::string::npos;

	auto firstCr = value.find("\n", start);
	auto firstDoubleQuote = value.find("\"", start);
	auto _firstDoubleQuote = value.find("\\\"", start);
	while (_firstDoubleQuote != std::string::npos &&  firstDoubleQuote > _firstDoubleQuote)
	{
		firstDoubleQuote = value.find("\"", _firstDoubleQuote + 2);
		_firstDoubleQuote = value.find("\\\"", _firstDoubleQuote + 2);
	}

	ret = firstDoubleQuote > firstCr ? firstCr : firstDoubleQuote;

	return ret;
}