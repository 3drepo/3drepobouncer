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

#include <string>
#include <sstream>
#include <iostream>
#include <boost/locale/encoding_utf.hpp>
#include "helper_functions.h"

using namespace boost::locale::conv;

const double repo::manipulator::modelconvertor::odaHelper::DOUBLE_TOLERANCE = 1E-6;

std::string repo::manipulator::modelconvertor::odaHelper::convertToStdString(const OdString &value)
{
	std::wstring wstr((wchar_t*)value.c_str());
	std::string str = utf_to_utf<char>(wstr.c_str(), wstr.c_str() + wstr.size());
	return str;
}

void repo::manipulator::modelconvertor::odaHelper::forEachBmDBView(OdBmDatabasePtr database, std::function<void(OdBmDBViewPtr viewPtr)> func)
{
	OdDbBaseDatabasePEPtr pDbPE(database);
	OdRxIteratorPtr layouts = pDbPE->layouts(database);
	for (; !layouts->done(); layouts->next())
	{
		OdBmDBDrawingPtr pDBDrawing = layouts->object();
		if (pDBDrawing->getBaseViewType() != OdBm::ViewType::ThreeD)
			continue;

		OdBmViewportPtr pViewport = pDBDrawing->getBaseViewportId().safeOpenObject();
		if (pViewport.isNull()) 
			continue;

		OdBmDBViewPtr pDBView = pViewport->getDbViewId().safeOpenObject();
		if (pDBView.isNull()) 
			continue;

		func(pDBView);
	}
}

int repo::manipulator::modelconvertor::odaHelper::compare(double d1, double d2)
{
	float diff = d1 - d2;
	if (diff > DOUBLE_TOLERANCE)
		return 1;
	if (diff < -DOUBLE_TOLERANCE)
		return -1;

	return 0;
}

repo::lib::RepoVector3D64 repo::manipulator::modelconvertor::odaHelper::calcNormal(repo::lib::RepoVector3D64 p1, repo::lib::RepoVector3D64 p2, repo::lib::RepoVector3D64 p3)
{
	repo::lib::RepoVector3D64 vecA = p2 - p1;
	repo::lib::RepoVector3D64 vecB = p3 - p2;
	repo::lib::RepoVector3D64 vecC = vecA.crossProduct(vecB);
	vecC.normalize();
	return vecC;
}
