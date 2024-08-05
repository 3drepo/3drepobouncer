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


#include "repo_metadataVariantHelper.h"

using namespace repo::lib;

bool MetadataVariantHelper::TryConvert(aiMetadataEntry& assimpMetaEntry, MetadataVariant& v)
{
	// Dissect the entry object
	switch (assimpMetaEntry.mType)
	{
	case AI_BOOL:
	{
		v = *(static_cast<bool*>(assimpMetaEntry.mData));
		break;
	}
	case AI_INT32:
	{
		v = *(static_cast<int*>(assimpMetaEntry.mData));
		break;
	}
	case AI_UINT64:
	{
		uint64_t value = *(static_cast<uint64_t*>(assimpMetaEntry.mData));
		v = static_cast<long long>(value); // Potentially losing precision here, but mongo does not accept uint64_t
		break;
	}
	case AI_FLOAT:
	{
		float value = *(static_cast<float*>(assimpMetaEntry.mData));
		v = static_cast<double>(value); // Potentially losing precision here, but mongo does not accept float
		break;
	}
	case AI_DOUBLE:
	{
		v = *(static_cast<double*>(assimpMetaEntry.mData));
		break;
	}
	case AI_AIVECTOR3D:
	{
		aiVector3D* vector = (static_cast<aiVector3D*>(assimpMetaEntry.mData));
		RepoVector3D repoVector = { (float)vector->x, (float)vector->y, (float)vector->z };
		v = repoVector.toString(); // not the best way to store a vector, but this appears to be the way it is done at the moment.
		break;
	}
	default:
	{
		// The other cases (AI_AISTRING and FORCE_32BIT) need extra treatment.
		return false;
	}
	}

	return true;

}
