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

#include <cstdlib>
#include <repo/lib/datastructure/repo_matrix.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <gtest/gtest-matchers.h>

#include "../../repo_test_utils.h"
#include "../../repo_test_matchers.h"

using namespace repo::lib;
using namespace testing;

#define RAD(x) ((float)x*(3.14159274101257324219 / 180))

bool checkIsIdentity(const RepoMatrix &mat)
{
	return RepoMatrix().equals(mat);
}

std::vector<double> asVector(const RepoMatrix &mat)
{
	std::vector<double> res;
	res.resize(16);
	memcpy(res.data(), mat.getData(), 16 * sizeof(double));
	return res;
}

TEST(RepoMatrixTest, constructorTest)
{
	std::vector<float> sourceMat1;
	std::vector<std::vector<float>> sourceMat2;
	std::vector<double> sourceMat3;
	RepoMatrix matrix;
	RepoMatrix matrix2(sourceMat1);
	RepoMatrix matrix4(sourceMat3);

	for (int i = 0; i < 16; ++i)
	{
		sourceMat1.push_back((rand()%1000) / 1000.f);
		sourceMat3.push_back((rand() % 1000) / 1000.f);
	}

	RepoMatrix matrix7(sourceMat1), matrix6(sourceMat3);

	std::vector<double> sourceMat64 = {
		0.41611923158633757, 0.41192361684200907, 0.3802399115383849, 1383.5544861408555,
		0.3102479090644362, 0.8211657559760365, 0.42445244930658144, 6159.647077873367,
		0.4608093818203498, 0.18089090705175348, 0.9258387270989096, 7714.581019037681,
		0, 0, 0, 1
	};

	std::vector<float> sourceMat32;
	sourceMat32.resize(16);
	for (auto i = 0; i < 16; i++)
	{
		sourceMat32[i] = (double)sourceMat64[i];
	}

	RepoMatrix doubleFromSingle(sourceMat32.data());
	RepoMatrix doubleFromDouble(sourceMat64.data());

	EXPECT_THAT(doubleFromSingle.isIdentity(), IsFalse());
	EXPECT_THAT(doubleFromDouble.isIdentity(), IsFalse());

	EXPECT_THAT(asVector(doubleFromSingle), ElementsAreArray(sourceMat32));
	EXPECT_THAT(asVector(doubleFromDouble), ElementsAreArray(sourceMat64));

	RepoMatrix colMajor(sourceMat64.data(), false);
	EXPECT_THAT(colMajor, Eq(doubleFromDouble.transpose()));
}

TEST(RepoMatrixTest, determinantTest)
{
	RepoMatrix id;
	EXPECT_EQ(1, id.determinant());
	std::vector<double> matValues = { 2, 0.3f,   0.4f, 1.23f,
		0.45f,    1, 0.488f, 12345,
		0,   0,     3.5f,     0,
		0,   0,       0,     1
	};
	RepoMatrix rand(matValues);

	EXPECT_EQ(6.5274999937415128, rand.determinant());
}

TEST(RepoMatrixTest, equalsTest)
{
	RepoMatrix id, id2;

	EXPECT_TRUE(id.equals(id2));
	EXPECT_TRUE(id2.equals(id));
	EXPECT_TRUE(id.equals(id));

	std::vector<float> ranV, ranV2;
	for (int i = 0; i < 16; ++i)
	{
		ranV.push_back((rand() % 1000) / 1000.f);
		ranV2.push_back((rand() % 1000) / 1000.f);
	}

	RepoMatrix rand1(ranV), rand2(ranV2), rand3(ranV);
	EXPECT_TRUE(rand1.equals(rand1));
	EXPECT_TRUE(rand1.equals(rand3));
	EXPECT_TRUE(rand3.equals(rand1));
	EXPECT_FALSE(id.equals(rand1));
	EXPECT_FALSE(rand2.equals(rand1));
	EXPECT_FALSE(rand1.equals(rand2));

}

TEST(RepoMatrixTest, getDataTest)
{
	std::vector<double> sourceMat1, sourceMat2_;
	std::vector<std::vector<double>> sourceMat2;
	RepoMatrix matrix;
	RepoMatrix matrix2(sourceMat1);
	RepoMatrix matrix3(sourceMat2);
	EXPECT_TRUE(checkIsIdentity(matrix));
	EXPECT_TRUE(checkIsIdentity(matrix2));
	EXPECT_TRUE(checkIsIdentity(matrix3));


	for (int i = 0; i < 16; ++i)
	{
		sourceMat1.push_back((rand() % 1000) / 1000.f);
		if (i % 4 == 0) sourceMat2.push_back(std::vector<double>());
		sourceMat2.back().push_back((rand() % 1000) / 1000.f);
		sourceMat2_.push_back(sourceMat2.back().back());
	}

	RepoMatrix matrix4(sourceMat1), matrix5(sourceMat2);

	EXPECT_THAT(memcmp(sourceMat1.data(), matrix4.getData(), 16), Eq(0));
	EXPECT_THAT(memcmp(sourceMat2_.data(), matrix5.getData(), 16), Eq(0));
}

TEST(RepoMatrixTest, invertTest)
{
	RepoMatrix id;
	EXPECT_TRUE(checkIsIdentity(id.invert()));
	std::vector<double> matValues = { 
		2, 0.3f, 0.4f, 1.23f,
		0.45f, 1, 0.488f, 12345,
		0, 0, 3.5f, 0,
		0, 0, 0, 1
	};
	RepoMatrix rand(matValues);
	auto inverted = rand.invert();

	std::vector<double> expectedRes = {
		0.53619303000471197, -0.16085791539333261, -0.038851014743946609, 1985.1314480935582,
		-0.24128685711020137, 1.0723860600094239, -0.12194561682368632, -13238.309127977491,
		0, 0, 0.28571428571428575, 0,
		0,	0,	0,	1 };

	EXPECT_THAT(RepoMatrix(expectedRes), Eq(inverted));
}

TEST(RepoMatrixTest, isIdentityTest)
{
	RepoMatrix id;
	EXPECT_TRUE(id.isIdentity());
	std::vector<float> idv = {1,0,0,0,
							 0,1,0,0,
							 0,0,1,0,
							 0,0,0,1};
	std::vector<float> higherb, lowerb, higherOver, lowerOver, higherUnder, lowerUnder;

	EXPECT_TRUE(RepoMatrix(idv).isIdentity());

	float eps = 1e-5;
	for (int i = 0; i < idv.size(); ++i)
	{
		higherb.push_back(idv[i] + eps);
		lowerb.push_back(idv[i] - eps);

		higherUnder.push_back(higherb[i] - 1e-8);
		lowerUnder.push_back(lowerb[i] + 1e-8);

		higherOver.push_back(higherb[i] + 1e-8);
		lowerOver.push_back(lowerb[i] - 1e-8);
	}

	EXPECT_TRUE(RepoMatrix(higherb).isIdentity(eps));
	EXPECT_TRUE(RepoMatrix(lowerb).isIdentity(eps));
	EXPECT_FALSE(RepoMatrix(higherOver).isIdentity(eps));
	EXPECT_FALSE(RepoMatrix(lowerOver).isIdentity(eps));
	EXPECT_TRUE(RepoMatrix(higherUnder).isIdentity(eps));
	EXPECT_TRUE(RepoMatrix(lowerUnder).isIdentity(eps));
}

TEST(RepoMatrixTest, toStringTest)
{

	std::vector<float> data;
	std::stringstream ss, ssID;

	for (int i = 0; i < 16; ++i)
	{
		data.push_back((rand() % 1000) / 1000.f);
		ss << " " << data[i];

		ssID << " " << (i % 5 == 0) ? 1.0f : 0.0f;
		if (i % 4 == 3)
		{
			ss << "\n";
			ssID << "\n";
		}
	}

	EXPECT_EQ(ss.str(), RepoMatrix(data).toString());
	EXPECT_EQ(ssID.str(), RepoMatrix().toString());

}

TEST(RepoMatrixTest, transposeTest)
{
	std::vector<float> values;
	values.resize(16);
	for (auto i = 0; i < 16; i++)
	{
		values[i] = rand() / 1000.f;
	}

	auto a = RepoMatrix(values);
	auto b = a.transpose();

	EXPECT_THAT(a.getData()[1], Eq(b.getData()[4]));
	EXPECT_THAT(a.getData()[4], Eq(b.getData()[1]));
	EXPECT_THAT(a.getData()[2], Eq(b.getData()[8]));
	EXPECT_THAT(a.getData()[8], Eq(b.getData()[2]));
	EXPECT_THAT(a.getData()[3], Eq(b.getData()[12]));
	EXPECT_THAT(a.getData()[12], Eq(b.getData()[3]));
	EXPECT_THAT(a.getData()[6], Eq(b.getData()[9]));
	EXPECT_THAT(a.getData()[9], Eq(b.getData()[6]));
	EXPECT_THAT(a.getData()[7], Eq(b.getData()[13]));
	EXPECT_THAT(a.getData()[13], Eq(b.getData()[7]));
	EXPECT_THAT(a.getData()[11], Eq(b.getData()[14]));
	EXPECT_THAT(a.getData()[14], Eq(b.getData()[11]));
	EXPECT_THAT(a.getData()[0], Eq(b.getData()[0]));
	EXPECT_THAT(a.getData()[5], Eq(b.getData()[5]));
	EXPECT_THAT(a.getData()[10], Eq(b.getData()[10]));
	EXPECT_THAT(a.getData()[15], Eq(b.getData()[15]));
}

TEST(RepoMatrixTest, matVecTest)
{
	RepoVector3D sampleVec(3.4653f, 2.543637f, 0.3253252f);
	RepoMatrix id;
	auto newVec = id * sampleVec;

	EXPECT_EQ(sampleVec.x, newVec.x);
	EXPECT_EQ(sampleVec.y, newVec.y);
	EXPECT_EQ(sampleVec.z, newVec.z);
	std::vector<float> matValues = { 2, 0.3f, 0.4f, 1.23f,
		0.45f, 1, 0.488f, 12345,
		0.5f, 0, 3.5f, 0,
		0, 0, 0, 1
	};
	RepoMatrix rand(matValues);

	auto newVec2 = rand * sampleVec;

	EXPECT_EQ(9.05382156372070310f, newVec2.x);
	EXPECT_EQ(12349.26171875000000000f, newVec2.y);
	EXPECT_EQ(2.87128829956054690f, newVec2.z);
}

TEST(RepoMatrixTest, matMatTest)
{
	EXPECT_TRUE(checkIsIdentity(RepoMatrix()*RepoMatrix()));

	std::vector <double> matValues = {
		2, 0.3f, 0.4f, 1.23f,
		0.45f, 1, 0.488f, 12345,
		0.5f, 0, 3.5f, 0,
		0, 4.56f, 0.0001f, 1
	};
	RepoMatrix rand(matValues);

	std::vector<double> matValues2 = {
		3.254f, 13.12456f, 0.0001f, 1.264f,
		0.5f,   0.645f,   10, 321.02f,
		0.7892f, 10.3256f, 1, 0.5f,
		0.5f, 0.6f, 0.7f, 1
	};

	RepoMatrix rand2(matValues2);

	EXPECT_EQ(rand, RepoMatrix()*rand);
	EXPECT_EQ(rand, rand*RepoMatrix());
	auto resultRand = rand * rand2;

	std::vector<double> expectedRes ={
		7.5886799203705788, 31.310860684726237, 4.2612001238533992, 100.26400066936003,
		6174.8494295462251, 7418.5902390082501, 8651.9878978416127, 12666.832789027523,
		4.3892000019550323, 42.701879024505615, 3.5000499999987369, 2.3820000290870667,
		2.7800788913885683, 3.5422324599005215, 46.300099415871955, 1464.8511815334314 
	};

	EXPECT_THAT(RepoMatrix(expectedRes), Eq(resultRand));
}

TEST(RepoMatrixTest, eqOpTest)
{
	RepoMatrix id, id2;

	EXPECT_TRUE(id==id2);
	EXPECT_TRUE(id2==id);
	EXPECT_TRUE(id==id);

	std::vector<double> ranV, ranV2;
	for (int i = 0; i < 16; ++i)
	{
		ranV.push_back((rand() % 1000) / 1000.f);
		ranV2.push_back((rand() % 1000) / 1000.f);
	}

	RepoMatrix rand1(ranV), rand2(ranV2), rand3(ranV);
	EXPECT_TRUE(rand1==rand1);
	EXPECT_TRUE(rand1==rand3);
	EXPECT_TRUE(rand3==rand1);
	EXPECT_FALSE(id==rand1);
	EXPECT_FALSE(rand2==rand1);
	EXPECT_FALSE(rand1==rand2);

}

TEST(RepoMatrixTest, neqOpTest)
{
	RepoMatrix id, id2;

	EXPECT_FALSE(id != id2);
	EXPECT_FALSE(id2 != id);
	EXPECT_FALSE(id != id);

	std::vector<double> ranV, ranV2;
	for (int i = 0; i < 16; ++i)
	{
		ranV.push_back((rand() % 1000) / 1000.f);
		ranV2.push_back((rand() % 1000) / 1000.f);
	}

	RepoMatrix rand1(ranV), rand2(ranV2), rand3(ranV);
	EXPECT_FALSE(rand1 != rand1);
	EXPECT_FALSE(rand1 != rand3);
	EXPECT_FALSE(rand3 != rand1);
	EXPECT_TRUE(id != rand1);
	EXPECT_TRUE(rand2 != rand1);
	EXPECT_TRUE(rand1 != rand2);

}

TEST(RepoMatrixTest, RotateX)
{
	repo::lib::RepoVector3D x(1, 0, 0);
	repo::lib::RepoVector3D y(0, 1, 0);
	repo::lib::RepoVector3D z(0, 0, 1);

	EXPECT_THAT(RepoMatrix::rotationX(RAD(45)) * x, VectorNear(x));
	EXPECT_THAT(RepoMatrix::rotationX(RAD(90)) * y, VectorNear(z));
	EXPECT_THAT(RepoMatrix::rotationX(RAD(45)) * repo::lib::RepoVector3D(1, 1, 1), VectorNear(repo::lib::RepoVector3D(1, 0, 1.41421353816986084)));
}

TEST(RepoMatrixTest, RotateY)
{
	repo::lib::RepoVector3D x(1, 0, 0);
	repo::lib::RepoVector3D y(0, 1, 0);
	repo::lib::RepoVector3D z(0, 0, 1);

	EXPECT_THAT(RepoMatrix::rotationY(RAD(45)) * y, VectorNear(y));
	EXPECT_THAT(RepoMatrix::rotationY(RAD(90)) * z, VectorNear(x));
	EXPECT_THAT(RepoMatrix::rotationY(RAD(45)) * repo::lib::RepoVector3D(1, 1, 1), VectorNear(repo::lib::RepoVector3D(1.41421353816986084, 1, 0)));
}

TEST(RepoMatrixTest, RotateZ)
{
	repo::lib::RepoVector3D x(1, 0, 0);
	repo::lib::RepoVector3D y(0, 1, 0);
	repo::lib::RepoVector3D z(0, 0, 1);

	EXPECT_THAT(RepoMatrix::rotationZ(RAD(45)) * z, VectorNear(z));
	EXPECT_THAT(RepoMatrix::rotationZ(RAD(90)) * x, VectorNear(y));
	EXPECT_THAT(RepoMatrix::rotationZ(RAD(45)) * repo::lib::RepoVector3D(1, 1, 1), VectorNear(repo::lib::RepoVector3D(0, 1.41421353816986084, 1)));
}

TEST(RepoMatrixTest, Translate)
{
	repo::lib::RepoVector3D a(rand(), rand(), rand());
	repo::lib::RepoVector3D b(rand(), rand(), rand());

	repo::lib::RepoVector3D c(a.x + b.x, a.y + b.y, a.z + b.z);

	EXPECT_THAT(RepoMatrix::translate(a) * b, VectorNear(c));
}

TEST(RepoMatrixTest, IsUniformScaling)
{

}

TEST(RepoMatrixTest, AxisAngleRotation)
{
	repo::lib::RepoVector3D x(1, 0, 0);
	repo::lib::RepoVector3D y(0, 1, 0);
	repo::lib::RepoVector3D z(0, 0, 1);
	repo::lib::RepoVector3D xy(1, 1, 0);
	xy.normalize();
	repo::lib::RepoVector3D xz(1, 0, 1);
	xz.normalize();

	EXPECT_THAT(repo::lib::RepoMatrix::rotation(x, RAD(90)) * y, VectorNear(z));
	EXPECT_THAT(repo::lib::RepoMatrix::rotation(x, RAD(90)) * x, VectorNear(x));
	EXPECT_THAT(repo::lib::RepoMatrix::rotation(x, RAD(90)) * xy, VectorNear(xz));
}