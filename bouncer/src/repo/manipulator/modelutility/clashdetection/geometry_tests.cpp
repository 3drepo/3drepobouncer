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

#include "geometry_tests.h"

#include "predicates.h"

#include <cmath>

#include <iostream>

#pragma optimize("", off)

using namespace geometry;

using Vector3 = repo::lib::RepoVector3D64;

static const repo::lib::RepoVector3D64 axes[] = {
    repo::lib::RepoVector3D64(1.0, 0.0, 0.0),
    repo::lib::RepoVector3D64(0.0, 1.0, 0.0),
    repo::lib::RepoVector3D64(0.0, 0.0, 1.0),
};

Vector3 geometry::closestPointTriangle(const Vector3& p, const repo::lib::RepoTriangle& T)
{
    // Ericson, C. (2004). Real-Time Collision Detection.CRC Press.

    auto ab = T.b - T.a;
    auto ac = T.c - T.a;
    auto ap = p - T.a;

    auto d1 = ab.dotProduct(ap);
    auto d2 = ac.dotProduct(ap);

    if (d1 <= 0 && d2 <= 0)
    {
        return T.a;
    }

    auto bp = p - T.b;
    auto d3 = ab.dotProduct(bp);
    auto d4 = ac.dotProduct(bp);

    if (d3 >= 0 && d4 <= d3)
    {
        return T.b;
    }

    auto vc = d1 * d4 - d3 * d2;
    if (vc <= 0 && d1 >= 0 && d3 <= 0)
    {
        return T.a + ab * (d1 / (d1 - d3));
    }

    auto cp = p - T.c;
    auto d5 = ab.dotProduct(cp);
    auto d6 = ac.dotProduct(cp);

    if (d6 >= 0 && d5 <= d6)
    {
        return T.c;
    }

    auto vb = d5 * d2 - d1 * d6;
    if (vb <= 0 && d2 >= 0 && d6 <= 0)
    {
        return T.a + ac * (d2 / (d2 - d6));
    }

    auto va = d3 * d6 - d5 * d4;
    if (va <= 0 && (d4 - d3) >= 0 && (d5 - d6) >= 0)
    {
        return T.b + (T.c - T.b) * (d4 - d3) / ((d4 - d3) + (d5 - d6));
    }

    auto denom = 1 / (va + vb + vc);
    auto v = vb * denom;
    auto w = vc * denom;
    return T.a + ab * v + ac * w;
}

repo::lib::RepoLine geometry::closestPointPointTriangle(const Vector3& p, const repo::lib::RepoTriangle& T)
{
    return { p, closestPointTriangle(p, T) };
}

repo::lib::RepoLine geometry::closestPointLineLine(const repo::lib::RepoLine& A, const repo::lib::RepoLine& B)
{
    // Ericson, C. (2004). Real-Time Collision Detection. CRC Press.

    auto d1 = A.d();
    auto d2 = B.d();
    auto r = A.start - B.start;
    auto a = d1.dotProduct(d1);
    auto e = d2.dotProduct(d2);
    auto f = d2.dotProduct(r);

    double s = 0;
    double t = 0;

    // check for degenerations
    if (a <= DBL_EPSILON && e <= DBL_EPSILON) {
        s = 0;
        t = 0;
    }
    else
    {
        if (a <= DBL_EPSILON) {
            s = 0.0;
            t = std::clamp(f / e, 0.0, 1.0);
        }
        else
        {
            auto c = d1.dotProduct(r);
            if (e <= DBL_EPSILON) {
                t = 0;
                s = std::clamp(-c / a, 0.0, 1.0);
            }
            else
            {
                auto b = d1.dotProduct(d2);
                auto denom = a * e - b * b;

                if (denom != 0) {
                    s = std::clamp((b * f - c * e) / denom, 0.0, 1.0);
                }
                else {
                    s = 0.0;
                }

                t = (b * s + f) / e;

                if (t < 0) {
                    t = 0;
                    s = std::clamp(-c / a, 0.0, 1.0);
                }
                else {
                    if (t > 1)
                    {
                        t = 1;
                        s = std::clamp((b - c) / a, 0.0, 1.0);
                    }
                }
            }
        }
    }

    repo::lib::RepoLine result;
    result.start = A.start + d1 * s;
    result.end = B.start + d2 * t;
    return result;
}

// Returns a line of zero length if B intersects the face of A, otherwise returns
// a line of infinite length.

repo::lib::RepoLine lineFaceIntersection(const repo::lib::RepoLine& B, const repo::lib::RepoTriangle& A)
{
    // Möller, T., & Trumbore, B. (1997). Fast, minimum storage ray-triangle intersection.

    auto dir = B.d();
    auto o = B.start;

    auto e1 = A.b - A.a;
    auto e2 = A.c - A.a;
    auto c2 = dir.crossProduct(e2);
    float det = e1.dotProduct(c2);

    if (det > -DBL_EPSILON && det < DBL_EPSILON) {
        return repo::lib::RepoLine::Max();
    }

    float inv_det = 1.0 / det;
    auto s = o - A.a;
    float u = inv_det * s.dotProduct(c2);

    if ((u < 0 && abs(u) > DBL_EPSILON) || (u > 1 && abs(u - 1) > DBL_EPSILON))
        return repo::lib::RepoLine::Max();

    auto c1 = s.crossProduct(e1);
    float v = inv_det * dir.dotProduct(c1);

    if ((v < 0 && abs(v) > DBL_EPSILON) || (u + v > 1 && abs(u + v - 1) > DBL_EPSILON))
        return repo::lib::RepoLine::Max();

    float t = inv_det * e2.dotProduct(c1);

    if (t > DBL_EPSILON && t < 1.0)
    {
        return { o + dir * t, o + dir * t };
    }
    else
    {
        return repo::lib::RepoLine::Max();
    }
}

repo::lib::RepoLine geometry::closestPointTriangleTriangle(const repo::lib::RepoTriangle& A, const repo::lib::RepoTriangle& B)
{
    // Ericson, C. (2004). Real-Time Collision Detection. CRC Press.

    repo::lib::RepoLine Ea1(A.a, A.b);
    repo::lib::RepoLine Ea2(A.b, A.c);
    repo::lib::RepoLine Ea3(A.c, A.a);
    repo::lib::RepoLine Eb1(B.a, B.b);
    repo::lib::RepoLine Eb2(B.b, B.c);
    repo::lib::RepoLine Eb3(B.c, B.a);

    repo::lib::RepoLine results[21];

    results[0] = closestPointLineLine(Ea1, Eb1);
    results[1] = closestPointLineLine(Ea1, Eb2);
    results[2] = closestPointLineLine(Ea1, Eb3);
    results[3] = closestPointLineLine(Ea2, Eb1);
    results[4] = closestPointLineLine(Ea2, Eb2);
    results[5] = closestPointLineLine(Ea2, Eb3);
    results[6] = closestPointLineLine(Ea3, Eb1);
    results[7] = closestPointLineLine(Ea3, Eb2);
    results[8] = closestPointLineLine(Ea3, Eb3);

    results[9] = closestPointPointTriangle(A.a, B);
    results[10] = closestPointPointTriangle(A.b, B);
    results[11] = closestPointPointTriangle(A.c, B);
    results[12] = closestPointPointTriangle(B.a, A);
    results[13] = closestPointPointTriangle(B.b, A);
    results[14] = closestPointPointTriangle(B.c, A);

	results[15] = lineFaceIntersection(Eb1, A);
	results[16] = lineFaceIntersection(Eb2, A);
	results[17] = lineFaceIntersection(Eb3, A);
	results[18] = lineFaceIntersection(Ea1, B);
	results[19] = lineFaceIntersection(Ea2, B);
	results[20] = lineFaceIntersection(Ea3, B);

    repo::lib::RepoLine min = results[0];
    for (int i = 1; i < 21; i++)
    {
        if (results[i].magnitude() < min.magnitude())
        {
            min = results[i];
        }
    }

    return min;
}

double ulp(double x)
{
    if (x > 0)
        return std::nexttoward(x, std::numeric_limits<double>::infinity()) - x;
    else
        return x - std::nexttoward(x, -std::numeric_limits<double>::infinity());
}

double geometry::orient(const repo::lib::RepoVector3D64& p, const repo::lib::RepoVector3D64& q, const repo::lib::RepoVector3D64& r, const repo::lib::RepoVector3D64& s)
{
	return predicates::orient3d((double*)&p, (double*)&q, (double*)&r, (double*)&s);
}

repo::lib::RepoVector3D64 geometry::minimumSeparatingAxis(const repo::lib::RepoBounds& a, repo::lib::RepoBounds& b)
{
    repo::lib::RepoVector3D64 m;

	auto left = reinterpret_cast<const double*>(&a);
	auto right = reinterpret_cast<const double*>(&b);

    auto d = DBL_MAX;
    for (int i = 0; i < 3; i++) {
        auto d1 = left[i + 3] - right[i];
        if (d1 <= 0) {
            return {};
        }
        auto d2 = right[i + 3] - left[i];
        if (d2 <= 0) {
            return {};
        }
        repo::lib::RepoVector3D64 dv;
        if (d1 < d2) {
            dv = axes[i] * -d1;
        }
        else {
            dv = axes[i] * d2;
        }
        if (dv.norm() < d) {
            d = dv.norm();
            m = dv;
        }
    }

    return m;
}

repo::lib::RepoVector3D64 edgePlaneIntersection(
    const repo::lib::RepoVector3D64& e1,
    const repo::lib::RepoVector3D64& e2,
    const repo::lib::RepoVector3D64& n,
    const repo::lib::RepoVector3D64& p
)
{
	auto l = e2 - e1;
    l.normalize();
	return e1 + l * ((p - e1).dotProduct(n) / l.dotProduct(n));
}

double checkMinMax(
    const repo::lib::RepoVector3D64& p1,
    const repo::lib::RepoVector3D64& q1,
    const repo::lib::RepoVector3D64& r1,
    const repo::lib::RepoVector3D64& p2,
    const repo::lib::RepoVector3D64& q2,
    const repo::lib::RepoVector3D64& r2)
{
    // This snippet performs an early rejection if the triangles are disjoint
    // on L. It works by finding whether the leftmost edge of one triangle is
    // oriented to the left or right of the rightmost edge of the other, and
    // vice versa.

    auto v1 = p2 - p1;
    auto v2 = p1 - q1;
	auto N1 = v1.crossProduct(v2);
	v1 = q2 - q1;
    if (N1.dotProduct(v1) > 0.0) {
        //return 0.0;
    }
    v1 = p2 - p1;
	v2 = r1 - p1;
	N1 = v1.crossProduct(v2);
	v1 = r2 - p1;
    if (N1.dotProduct(v1) > 0.0) {
       // return 0.0;
    }
	
	// If there is an overlap, estimate the minimum translation distance.
    // Triangles can either be shifted along L by k-j or l-i, or moved
    // normal so that they are separated by one or the other's planes.

    auto pr1 = r1 - p1;
    auto pq1 = q1 - p1;
    auto n1 = pq1.crossProduct(pr1);
    n1.normalize();

    auto pr2 = r2 - p2;
    auto pq2 = q2 - p2;
    auto n2 = pq2.crossProduct(pr2);
    n2.normalize();

	// i,j,k,l are the intersections of the edges with their counterpart's
    // plane.

	auto i = edgePlaneIntersection(p1, r1, n2, p2);
	auto j = edgePlaneIntersection(p1, q1, n2, p2);
	auto k = edgePlaneIntersection(p2, q2, n1, p1);
    auto l = edgePlaneIntersection(p2, r2, n1, p1);

    auto o = i;

	auto L = n1.crossProduct(n2);
    
	auto ii = (i - o).dotProduct(L);
	auto jj = (j - o).dotProduct(L);
	auto kk = (k - o).dotProduct(L);
	auto ll = (l - o).dotProduct(L);

    if (kk > jj) {
        return 0;
    }
    if (ll < ii) {
        return 0;
    }
    
    return 1.0;
}

#define COPLANAR 0

double geometry::intersects(const repo::lib::RepoTriangle& A, const repo::lib::RepoTriangle& B)
{
    // Peforms a triangle-triangle intersection and returns the upper bound on
	// the distance either of the triangles must move to resolve the intersection.

    // This implementation is based on Faster Triangle-Triangle Intersection 
    // Tests. Olivier Devillers, Philippe Guigue. Journal of Graphics Tools, 
    // 8(1), 2003.
 
    // A reference implementation is available:
    // https://github.com/erich666/jgt-code/blob/master/Volume_08/Number_1/Guigue2003/tri_tri_intersect.c

    // Our version starts the same way, but after the permutations (when the
    // intersecting edges have been found), individual segment-face intersection
	// tests are performed to approximate the intersection distance.

    // The motivation is to allow upstream thresholding to disambiguate the
    // in-contact from the intersection case under rounding error - not rounding
    // error in the predicates but in the original data. For the same reason we
	// do not consider the coplanar case - as the resolution of this case is
    // trivial.  


    // Check if either triangle sits entirely on one side of other's plane, for
    // early rejection.

    // For the orientation tests we use Shewchuk's adaptive precision predicates.
	// Note that these have the opposite sign to the cross-product version used
	// by Guigue and Devillers.

	const auto& p1 = A.a;
	const auto& q1 = A.b;
	const auto& r1 = A.c;
	const auto& p2 = B.a;
	const auto& q2 = B.b;
	const auto& r2 = B.c;

	auto dp1 = -geometry::orient(p2, q2, r2, p1);
    auto dq1 = -geometry::orient(p2, q2, r2, q1);
    auto dr1 = -geometry::orient(p2, q2, r2, r1);

    if (((dp1 * dq1) > 0.0) && ((dp1 * dr1) > 0.0)) {
        return false;
    }

    auto dp2 = -geometry::orient(p1, q1, r1, p2);
    auto dq2 = -geometry::orient(p1, q1, r1, q2);
    auto dr2 = -geometry::orient(p1, q1, r1, r2);

    if (((dp2 * dq2) > 0.0f) && ((dp2 * dr2) > 0.0f)) {
        return false;
    }

    // Even on modern compilers, Guigue and Devillers' macros are still faster
    // at working out the permutations than function calls, by a factor of 2.

#define INTERSECT(p1,q1,r1,p2,q2,r2) return checkMinMax(p1,q1,r1,p2,q2,r2);

#define PERMUTE2(p1,q1,r1,p2,q2,r2,dp2,dq2,dr2) { \
	if (dp2 > 0.0f) { \
		if (dq2 > 0.0f) INTERSECT(p1,r1,q1,r2,p2,q2) \
		else if (dr2 > 0.0f) INTERSECT(p1,r1,q1,q2,r2,p2)\
		else INTERSECT(p1,q1,r1,p2,q2,r2) }\
	else if (dp2 < 0.0f) { \
		if (dq2 < 0.0f) INTERSECT(p1,q1,r1,r2,p2,q2)\
		else if (dr2 < 0.0f) INTERSECT(p1,q1,r1,q2,r2,p2)\
		else INTERSECT(p1,r1,q1,p2,q2,r2)\
	} else { \
		if (dq2 < 0.0f) { \
			if (dr2 >= 0.0f)  INTERSECT(p1,r1,q1,q2,r2,p2)\
			else INTERSECT(p1,q1,r1,p2,q2,r2)\
		} \
		else if (dq2 > 0.0f) { \
			if (dr2 > 0.0f) INTERSECT(p1,r1,q1,p2,q2,r2)\
			else  INTERSECT(p1,q1,r1,q2,r2,p2)\
		} \
		else  { \
			if (dr2 > 0.0f) INTERSECT(p1,q1,r1,r2,p2,q2)\
			else if (dr2 < 0.0f) INTERSECT(p1,r1,q1,r2,p2,q2)\
			else return COPLANAR;\
		}}}


    // Permute (circular shift) the vertices of the triangles such that it is
	// always p1 on the opposide side of B and p2 on the opposite side of A.
    // We then know that the edges from p1 and p2 are the ones that intersect
    // the segment's line.
        
    if (dp1 > 0.0f) {
        if (dq1 > 0.0f) PERMUTE2(r1, p1, q1, p2, r2, q2, dp2, dr2, dq2)
        else if (dr1 > 0.0f) PERMUTE2(q1, r1, p1, p2, r2, q2, dp2, dr2, dq2)
        else PERMUTE2(p1, q1, r1, p2, q2, r2, dp2, dq2, dr2)
    }
    else if (dp1 < 0.0f) {
        if (dq1 < 0.0f) PERMUTE2(r1, p1, q1, p2, q2, r2, dp2, dq2, dr2)
        else if (dr1 < 0.0f) PERMUTE2(q1, r1, p1, p2, q2, r2, dp2, dq2, dr2)
        else PERMUTE2(p1, q1, r1, p2, r2, q2, dp2, dr2, dq2)
    }
    else {
        if (dq1 < 0.0f) {
            if (dr1 >= 0.0f) PERMUTE2(q1, r1, p1, p2, r2, q2, dp2, dr2, dq2)
            else PERMUTE2(p1, q1, r1, p2, q2, r2, dp2, dq2, dr2)
        }
        else if (dq1 > 0.0f) {
            if (dr1 > 0.0f) PERMUTE2(p1, q1, r1, p2, r2, q2, dp2, dr2, dq2)
            else PERMUTE2(q1, r1, p1, p2, q2, r2, dp2, dq2, dr2)
        }
        else {
            if (dr1 > 0.0f) PERMUTE2(r1, p1, q1, p2, q2, r2, dp2, dq2, dr2)
            else if (dr1 < 0.0f) PERMUTE2(r1, p1, q1, p2, r2, q2, dp2, dr2, dq2)
            else return COPLANAR;
        }
    }

#undef PERMUTE2
#undef INTERSECT
}

// Three-dimensional Triangle-Triangle Overlap Test
int tri_tri_overlap_test_3d(double p1[3], double q1[3], double r1[3],
    double p2[3], double q2[3], double r2[3]);

/* Epsilon coplanarity checks */

#define USE_EPSILON_TEST false /* set to true to use coplanarity robustness checks */
#define EPSILON 1e-16          /* you may also need to adjust this value according to your data */

#define FABS(x) (fabs(x))       /* implement as is fastest on your machine */

/* some 3D macros */

#define CROSS(dest,v1,v2)                       \
               dest[0]=v1[1]*v2[2]-v1[2]*v2[1]; \
               dest[1]=v1[2]*v2[0]-v1[0]*v2[2]; \
               dest[2]=v1[0]*v2[1]-v1[1]*v2[0];

#define DOT(v1,v2) (v1[0]*v2[0]+v1[1]*v2[1]+v1[2]*v2[2])



#define SUB(dest,v1,v2) dest[0]=v1[0]-v2[0]; \
                        dest[1]=v1[1]-v2[1]; \
                        dest[2]=v1[2]-v2[2]; 


#define SCALAR(dest,alpha,v) dest[0] = alpha * v[0]; \
                             dest[1] = alpha * v[1]; \
                             dest[2] = alpha * v[2];

#define CHECK_MIN_MAX(p1,q1,r1,p2,q2,r2) {\
  SUB(v1,p2,q1)\
  SUB(v2,p1,q1)\
  CROSS(N1,v1,v2)\
  SUB(v1,q2,q1)\
  if (DOT(v1,N1) > 0.0f) { \
    return 0; }\
  SUB(v1,p2,p1)\
  SUB(v2,r1,p1)\
  CROSS(N1,v1,v2)\
  SUB(v1,r2,p1) \
  if (DOT(v1,N1) > 0.0f) { \
     return 0; }\
  else return 1; }


#define TRI_TRI_3D(p1,q1,r1,p2,q2,r2,dp2,dq2,dr2) { \
  if (dp2 > 0.0f) { \
     if (dq2 > 0.0f) CHECK_MIN_MAX(p1,r1,q1,r2,p2,q2) \
     else if (dr2 > 0.0f) CHECK_MIN_MAX(p1,r1,q1,q2,r2,p2)\
     else CHECK_MIN_MAX(p1,q1,r1,p2,q2,r2) }\
  else if (dp2 < 0.0f) { \
    if (dq2 < 0.0f) CHECK_MIN_MAX(p1,q1,r1,r2,p2,q2)\
    else if (dr2 < 0.0f) CHECK_MIN_MAX(p1,q1,r1,q2,r2,p2)\
    else CHECK_MIN_MAX(p1,r1,q1,p2,q2,r2)\
  } else { \
    if (dq2 < 0.0f) { \
      if (dr2 >= 0.0f)  CHECK_MIN_MAX(p1,r1,q1,q2,r2,p2)\
      else CHECK_MIN_MAX(p1,q1,r1,p2,q2,r2)\
    } \
    else if (dq2 > 0.0f) { \
      if (dr2 > 0.0f) CHECK_MIN_MAX(p1,r1,q1,p2,q2,r2)\
      else  CHECK_MIN_MAX(p1,q1,r1,q2,r2,p2)\
    } \
    else  { \
      if (dr2 > 0.0f) CHECK_MIN_MAX(p1,q1,r1,r2,p2,q2)\
      else if (dr2 < 0.0f) CHECK_MIN_MAX(p1,r1,q1,r2,p2,q2)\
      else return 0;\
     }}}

// Three-dimensional Triangle-Triangle Overlap Test
// additionaly computes the segment of intersection of the two triangles if it exists. 
// coplanar returns whether the triangles are coplanar, 
// source and target are the endpoints of the line segment of intersection 
int tri_tri_intersection_test_3d(double p1[3], double q1[3], double r1[3],
    double p2[3], double q2[3], double r2[3],
    int* coplanar,
    double source[3], double target[3]);


int coplanar_tri_tri3d(const double* p1, const double* q1, const double* r1,
    const double* p2, const double* q2, const double* r2,
    const double* normal_1);


// Two dimensional Triangle-Triangle Overlap Test
int tri_tri_overlap_test_2d(const double* p1, const double* q1, const double* r1,
    const double* p2, const double* q2, const double* r2);

int tri_tri_overlap_test_3d(double p1[3], double q1[3], double r1[3],
    double p2[3], double q2[3], double r2[3])
{
    double dp1, dq1, dr1, dp2, dq2, dr2;
    double v1[3], v2[3];
    double N1[3], N2[3];

    /* Compute distance signs  of p1, q1 and r1 to the plane of
       triangle(p2,q2,r2) */


    SUB(v1, p2, r2)
        SUB(v2, q2, r2)
        CROSS(N2, v1, v2)

        SUB(v1, p1, r2)
        dp1 = DOT(v1, N2);
    SUB(v1, q1, r2)
        dq1 = DOT(v1, N2);
    SUB(v1, r1, r2)
        dr1 = DOT(v1, N2);

    /* coplanarity robustness check */
#if USE_EPSILON_TEST
    if (FABS(dp1) < EPSILON) dp1 = 0.0;
    if (FABS(dq1) < EPSILON) dq1 = 0.0;
    if (FABS(dr1) < EPSILON) dr1 = 0.0;
#endif

    if (((dp1 * dq1) > 0.0f) && ((dp1 * dr1) > 0.0f))  return 0;

    /* Compute distance signs  of p2, q2 and r2 to the plane of
       triangle(p1,q1,r1) */


    SUB(v1, q1, p1)
        SUB(v2, r1, p1)
        CROSS(N1, v1, v2)

        SUB(v1, p2, r1)
        dp2 = DOT(v1, N1);
    SUB(v1, q2, r1)
        dq2 = DOT(v1, N1);
    SUB(v1, r2, r1)
        dr2 = DOT(v1, N1);

#if USE_EPSILON_TEST
    if (FABS(dp2) < EPSILON) dp2 = 0.0;
    if (FABS(dq2) < EPSILON) dq2 = 0.0;
    if (FABS(dr2) < EPSILON) dr2 = 0.0;
#endif

    if (((dp2 * dq2) > 0.0f) && ((dp2 * dr2) > 0.0f)) return 0;

    /* Permutation in a canonical form of T1's vertices */

    if (dp1 > 0.0f) {
        if (dq1 > 0.0f) TRI_TRI_3D(r1, p1, q1, p2, r2, q2, dp2, dr2, dq2)
        else if (dr1 > 0.0f) TRI_TRI_3D(q1, r1, p1, p2, r2, q2, dp2, dr2, dq2)
        else TRI_TRI_3D(p1, q1, r1, p2, q2, r2, dp2, dq2, dr2)
    }
    else if (dp1 < 0.0f) {
        if (dq1 < 0.0f) TRI_TRI_3D(r1, p1, q1, p2, q2, r2, dp2, dq2, dr2)
        else if (dr1 < 0.0f) TRI_TRI_3D(q1, r1, p1, p2, q2, r2, dp2, dq2, dr2)
        else TRI_TRI_3D(p1, q1, r1, p2, r2, q2, dp2, dr2, dq2)
    }
    else {
        if (dq1 < 0.0f) {
            if (dr1 >= 0.0f) TRI_TRI_3D(q1, r1, p1, p2, r2, q2, dp2, dr2, dq2)
            else TRI_TRI_3D(p1, q1, r1, p2, q2, r2, dp2, dq2, dr2)
        }
        else if (dq1 > 0.0f) {
            if (dr1 > 0.0f) TRI_TRI_3D(p1, q1, r1, p2, r2, q2, dp2, dr2, dq2)
            else TRI_TRI_3D(q1, r1, p1, p2, q2, r2, dp2, dq2, dr2)
        }
        else {
            if (dr1 > 0.0f) TRI_TRI_3D(r1, p1, q1, p2, q2, r2, dp2, dq2, dr2)
            else if (dr1 < 0.0f) TRI_TRI_3D(r1, p1, q1, p2, r2, q2, dp2, dr2, dq2)
            else return 0;
        }
    }    
};

int coplanar_tri_tri3d(const double* p1, const double* q1, const double* r1,
    const double* p2, const double* q2, const double* r2,
    const double* normal_1) {

    double P1[2], Q1[2], R1[2];
    double P2[2], Q2[2], R2[2];

    double n_x, n_y, n_z;

    n_x = ((normal_1[0] < 0) ? -normal_1[0] : normal_1[0]);
    n_y = ((normal_1[1] < 0) ? -normal_1[1] : normal_1[1]);
    n_z = ((normal_1[2] < 0) ? -normal_1[2] : normal_1[2]);


    /* Projection of the triangles in 3D onto 2D such that the area of
       the projection is maximized. */


    if ((n_x > n_z) && (n_x >= n_y)) {
        // Project onto plane YZ

        P1[0] = q1[2]; P1[1] = q1[1];
        Q1[0] = p1[2]; Q1[1] = p1[1];
        R1[0] = r1[2]; R1[1] = r1[1];

        P2[0] = q2[2]; P2[1] = q2[1];
        Q2[0] = p2[2]; Q2[1] = p2[1];
        R2[0] = r2[2]; R2[1] = r2[1];

    }
    else if ((n_y > n_z) && (n_y >= n_x)) {
        // Project onto plane XZ

        P1[0] = q1[0]; P1[1] = q1[2];
        Q1[0] = p1[0]; Q1[1] = p1[2];
        R1[0] = r1[0]; R1[1] = r1[2];

        P2[0] = q2[0]; P2[1] = q2[2];
        Q2[0] = p2[0]; Q2[1] = p2[2];
        R2[0] = r2[0]; R2[1] = r2[2];

    }
    else {
        // Project onto plane XY

        P1[0] = p1[0]; P1[1] = p1[1];
        Q1[0] = q1[0]; Q1[1] = q1[1];
        R1[0] = r1[0]; R1[1] = r1[1];

        P2[0] = p2[0]; P2[1] = p2[1];
        Q2[0] = q2[0]; Q2[1] = q2[1];
        R2[0] = r2[0]; R2[1] = r2[1];
    }

    return tri_tri_overlap_test_2d(P1, Q1, R1, P2, Q2, R2);

};

/*
*
*  Three-dimensional Triangle-Triangle Intersection
*
*/

/*
   This macro is called when the triangles surely intersect
   It constructs the segment of intersection of the two triangles
   if they are not coplanar.
*/

// NOTE: a faster, but possibly less precise, method of computing
// point B is described here: https://github.com/erich666/jgt-code/issues/5

#define CONSTRUCT_INTERSECTION(p1,q1,r1,p2,q2,r2) { \
  SUB(v1,q1,p1) \
  SUB(v2,r2,p1) \
  CROSS(N,v1,v2) \
  SUB(v,p2,p1) \
  if (DOT(v,N) > 0.0f) {\
    SUB(v1,r1,p1) \
    CROSS(N,v1,v2) \
    if (DOT(v,N) <= 0.0f) { \
      SUB(v2,q2,p1) \
      CROSS(N,v1,v2) \
      if (DOT(v,N) > 0.0f) { \
  SUB(v1,p1,p2) \
  SUB(v2,p1,r1) \
  alpha = DOT(v1,N2) / DOT(v2,N2); \
  SCALAR(v1,alpha,v2) \
  SUB(source,p1,v1) \
  SUB(v1,p2,p1) \
  SUB(v2,p2,r2) \
  alpha = DOT(v1,N1) / DOT(v2,N1); \
  SCALAR(v1,alpha,v2) \
  SUB(target,p2,v1) \
  return 1; \
      } else { \
  SUB(v1,p2,p1) \
  SUB(v2,p2,q2) \
  alpha = DOT(v1,N1) / DOT(v2,N1); \
  SCALAR(v1,alpha,v2) \
  SUB(source,p2,v1) \
  SUB(v1,p2,p1) \
  SUB(v2,p2,r2) \
  alpha = DOT(v1,N1) / DOT(v2,N1); \
  SCALAR(v1,alpha,v2) \
  SUB(target,p2,v1) \
  return 1; \
      } \
    } else { \
      return 0; \
    } \
  } else { \
    SUB(v2,q2,p1) \
    CROSS(N,v1,v2) \
    if (DOT(v,N) < 0.0f) { \
      return 0; \
    } else { \
      SUB(v1,r1,p1) \
      CROSS(N,v1,v2) \
      if (DOT(v,N) >= 0.0f) { \
  SUB(v1,p1,p2) \
  SUB(v2,p1,r1) \
  alpha = DOT(v1,N2) / DOT(v2,N2); \
  SCALAR(v1,alpha,v2) \
  SUB(source,p1,v1) \
  SUB(v1,p1,p2) \
  SUB(v2,p1,q1) \
  alpha = DOT(v1,N2) / DOT(v2,N2); \
  SCALAR(v1,alpha,v2) \
  SUB(target,p1,v1) \
  return 1; \
      } else { \
  SUB(v1,p2,p1) \
  SUB(v2,p2,q2) \
  alpha = DOT(v1,N1) / DOT(v2,N1); \
  SCALAR(v1,alpha,v2) \
  SUB(source,p2,v1) \
  SUB(v1,p1,p2) \
  SUB(v2,p1,q1) \
  alpha = DOT(v1,N2) / DOT(v2,N2); \
  SCALAR(v1,alpha,v2) \
  SUB(target,p1,v1) \
  return 1; \
      }}}} 



#define TRI_TRI_INTER_3D(p1,q1,r1,p2,q2,r2,dp2,dq2,dr2) { \
  if (dp2 > 0.0f) { \
     if (dq2 > 0.0f) CONSTRUCT_INTERSECTION(p1,r1,q1,r2,p2,q2) \
     else if (dr2 > 0.0f) CONSTRUCT_INTERSECTION(p1,r1,q1,q2,r2,p2)\
     else CONSTRUCT_INTERSECTION(p1,q1,r1,p2,q2,r2) }\
  else if (dp2 < 0.0f) { \
    if (dq2 < 0.0f) CONSTRUCT_INTERSECTION(p1,q1,r1,r2,p2,q2)\
    else if (dr2 < 0.0f) CONSTRUCT_INTERSECTION(p1,q1,r1,q2,r2,p2)\
    else CONSTRUCT_INTERSECTION(p1,r1,q1,p2,q2,r2)\
  } else { \
    if (dq2 < 0.0f) { \
      if (dr2 >= 0.0f)  CONSTRUCT_INTERSECTION(p1,r1,q1,q2,r2,p2)\
      else CONSTRUCT_INTERSECTION(p1,q1,r1,p2,q2,r2)\
    } \
    else if (dq2 > 0.0f) { \
      if (dr2 > 0.0f) CONSTRUCT_INTERSECTION(p1,r1,q1,p2,q2,r2)\
      else  CONSTRUCT_INTERSECTION(p1,q1,r1,q2,r2,p2)\
    } \
    else  { \
      if (dr2 > 0.0f) CONSTRUCT_INTERSECTION(p1,q1,r1,r2,p2,q2)\
      else if (dr2 < 0.0f) CONSTRUCT_INTERSECTION(p1,r1,q1,r2,p2,q2)\
      else { \
        *coplanar = 1; \
  return coplanar_tri_tri3d(p1,q1,r1,p2,q2,r2,N1);\
     } \
  }} }


/*
   The following version computes the segment of intersection of the
   two triangles if it exists.
   coplanar returns whether the triangles are coplanar
   source and target are the endpoints of the line segment of intersection
*/

int tri_tri_intersection_test_3d(double p1[3], double q1[3], double r1[3],
    double p2[3], double q2[3], double r2[3],
    int* coplanar,
    double source[3], double target[3])

{
    double dp1, dq1, dr1, dp2, dq2, dr2;
    double v1[3], v2[3], v[3];
    double N1[3], N2[3], N[3];
    double alpha;

    // Compute distance signs  of p1, q1 and r1 
    // to the plane of triangle(p2,q2,r2)


    SUB(v1, p2, r2)
        SUB(v2, q2, r2)
        CROSS(N2, v1, v2)

        SUB(v1, p1, r2)
        dp1 = DOT(v1, N2);
    SUB(v1, q1, r2)
        dq1 = DOT(v1, N2);
    SUB(v1, r1, r2)
        dr1 = DOT(v1, N2);

    /* coplanarity robustness check */
#if USE_EPSILON_TEST
    if (FABS(dp1) < EPSILON) dp1 = 0.0;
    if (FABS(dq1) < EPSILON) dq1 = 0.0;
    if (FABS(dr1) < EPSILON) dr1 = 0.0;
#endif

    if (((dp1 * dq1) > 0.0f) && ((dp1 * dr1) > 0.0f))  return 0;

    // Compute distance signs  of p2, q2 and r2 
    // to the plane of triangle(p1,q1,r1)


    SUB(v1, q1, p1)
        SUB(v2, r1, p1)
        CROSS(N1, v1, v2)

        SUB(v1, p2, r1)
        dp2 = DOT(v1, N1);
    SUB(v1, q2, r1)
        dq2 = DOT(v1, N1);
    SUB(v1, r2, r1)
        dr2 = DOT(v1, N1);

#if USE_EPSILON_TEST
    if (FABS(dp2) < EPSILON) dp2 = 0.0;
    if (FABS(dq2) < EPSILON) dq2 = 0.0;
    if (FABS(dr2) < EPSILON) dr2 = 0.0;
#endif

    if (((dp2 * dq2) > 0.0f) && ((dp2 * dr2) > 0.0f)) return 0;

    // Permutation in a canonical form of T1's vertices


    if (dp1 > 0.0f) {
        if (dq1 > 0.0f) TRI_TRI_INTER_3D(r1, p1, q1, p2, r2, q2, dp2, dr2, dq2)
        else if (dr1 > 0.0f) TRI_TRI_INTER_3D(q1, r1, p1, p2, r2, q2, dp2, dr2, dq2)

        else TRI_TRI_INTER_3D(p1, q1, r1, p2, q2, r2, dp2, dq2, dr2)
    }
    else if (dp1 < 0.0f) {
        if (dq1 < 0.0f) TRI_TRI_INTER_3D(r1, p1, q1, p2, q2, r2, dp2, dq2, dr2)
        else if (dr1 < 0.0f) TRI_TRI_INTER_3D(q1, r1, p1, p2, q2, r2, dp2, dq2, dr2)
        else TRI_TRI_INTER_3D(p1, q1, r1, p2, r2, q2, dp2, dr2, dq2)
    }
    else {
        if (dq1 < 0.0f) {
            if (dr1 >= 0.0f) TRI_TRI_INTER_3D(q1, r1, p1, p2, r2, q2, dp2, dr2, dq2)
            else TRI_TRI_INTER_3D(p1, q1, r1, p2, q2, r2, dp2, dq2, dr2)
        }
        else if (dq1 > 0.0f) {
            if (dr1 > 0.0f) TRI_TRI_INTER_3D(p1, q1, r1, p2, r2, q2, dp2, dr2, dq2)
            else TRI_TRI_INTER_3D(q1, r1, p1, p2, q2, r2, dp2, dq2, dr2)
        }
        else {
            if (dr1 > 0.0f) TRI_TRI_INTER_3D(r1, p1, q1, p2, q2, r2, dp2, dq2, dr2)
            else if (dr1 < 0.0f) TRI_TRI_INTER_3D(r1, p1, q1, p2, r2, q2, dp2, dr2, dq2)
            else {
                // triangles are co-planar

                *coplanar = 1;
                return coplanar_tri_tri3d(p1, q1, r1, p2, q2, r2, N1);
            }
        }
    }
};





/*
*
*  Two dimensional Triangle-Triangle Overlap Test
*
*/


/* some 2D macros */

#define ORIENT_2D(a, b, c)  ((a[0]-c[0])*(b[1]-c[1])-(a[1]-c[1])*(b[0]-c[0]))


#define INTERSECTION_TEST_VERTEX(P1, Q1, R1, P2, Q2, R2) {\
  if (ORIENT_2D(R2,P2,Q1) >= 0.0f)\
    if (ORIENT_2D(R2,Q2,Q1) <= 0.0f)\
      if (ORIENT_2D(P1,P2,Q1) > 0.0f) {\
  if (ORIENT_2D(P1,Q2,Q1) <= 0.0f) return 1; \
  else return 0;} else {\
  if (ORIENT_2D(P1,P2,R1) >= 0.0f)\
    if (ORIENT_2D(Q1,R1,P2) >= 0.0f) return 1; \
    else return 0;\
  else return 0;}\
    else \
      if (ORIENT_2D(P1,Q2,Q1) <= 0.0f)\
  if (ORIENT_2D(R2,Q2,R1) <= 0.0f)\
    if (ORIENT_2D(Q1,R1,Q2) >= 0.0f) return 1; \
    else return 0;\
  else return 0;\
      else return 0;\
  else\
    if (ORIENT_2D(R2,P2,R1) >= 0.0f) \
      if (ORIENT_2D(Q1,R1,R2) >= 0.0f)\
  if (ORIENT_2D(P1,P2,R1) >= 0.0f) return 1;\
  else return 0;\
      else \
  if (ORIENT_2D(Q1,R1,Q2) >= 0.0f) {\
    if (ORIENT_2D(R2,R1,Q2) >= 0.0f) return 1; \
    else return 0; }\
  else return 0; \
    else  return 0; \
 };



#define INTERSECTION_TEST_EDGE(P1, Q1, R1, P2, Q2, R2) { \
  if (ORIENT_2D(R2,P2,Q1) >= 0.0f) {\
    if (ORIENT_2D(P1,P2,Q1) >= 0.0f) { \
        if (ORIENT_2D(P1,Q1,R2) >= 0.0f) return 1; \
        else return 0;} else { \
      if (ORIENT_2D(Q1,R1,P2) >= 0.0f){ \
  if (ORIENT_2D(R1,P1,P2) >= 0.0f) return 1; else return 0;} \
      else return 0; } \
  } else {\
    if (ORIENT_2D(R2,P2,R1) >= 0.0f) {\
      if (ORIENT_2D(P1,P2,R1) >= 0.0f) {\
  if (ORIENT_2D(P1,R1,R2) >= 0.0f) return 1;  \
  else {\
    if (ORIENT_2D(Q1,R1,R2) >= 0.0f) return 1; else return 0;}}\
      else  return 0; }\
    else return 0; }}



int ccw_tri_tri_intersection_2d(const double p1[2], const  double q1[2], const double r1[2],
    const double p2[2], const double q2[2], const  double r2[2]) {
    if (ORIENT_2D(p2, q2, p1) >= 0.0f) {
        if (ORIENT_2D(q2, r2, p1) >= 0.0f) {
            if (ORIENT_2D(r2, p2, p1) >= 0.0f) return 1;
            else INTERSECTION_TEST_EDGE(p1, q1, r1, p2, q2, r2)
        }
        else {
            if (ORIENT_2D(r2, p2, p1) >= 0.0f)
                INTERSECTION_TEST_EDGE(p1, q1, r1, r2, p2, q2)
            else INTERSECTION_TEST_VERTEX(p1, q1, r1, p2, q2, r2)
        }
    }
    else {
        if (ORIENT_2D(q2, r2, p1) >= 0.0f) {
            if (ORIENT_2D(r2, p2, p1) >= 0.0f)
                INTERSECTION_TEST_EDGE(p1, q1, r1, q2, r2, p2)
            else  INTERSECTION_TEST_VERTEX(p1, q1, r1, q2, r2, p2)
        }
        else INTERSECTION_TEST_VERTEX(p1, q1, r1, r2, p2, q2)
    }
};


int tri_tri_overlap_test_2d(const double* p1, const double* q1, const double* r1,
    const double* p2, const double* q2, const double* r2) {
    if (ORIENT_2D(p1, q1, r1) < 0.0f)
        if (ORIENT_2D(p2, q2, r2) < 0.0f)
            return ccw_tri_tri_intersection_2d(p1, r1, q1, p2, r2, q2);
        else
            return ccw_tri_tri_intersection_2d(p1, r1, q1, p2, q2, r2);
    else
        if (ORIENT_2D(p2, q2, r2) < 0.0f)
            return ccw_tri_tri_intersection_2d(p1, q1, r1, p2, r2, q2);
        else
            return ccw_tri_tri_intersection_2d(p1, q1, r1, p2, q2, r2);

};