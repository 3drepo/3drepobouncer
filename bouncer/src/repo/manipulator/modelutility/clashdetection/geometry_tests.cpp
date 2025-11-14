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

    repo::lib::RepoLine results[15];

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

    repo::lib::RepoVector3D64 p;
	auto i = intersects(A, B, &p);

    // For the distance test, we do not care to disambiguate the contact from
    // the coplanar case, as both mean the distance between the triangles is
    // infinitesimal.

    if (i > COPLANAR) {
		return repo::lib::RepoLine { p, p };
    }

    repo::lib::RepoLine min = results[0];
    for (int i = 1; i < 15; i++)
    {
        if (results[i].magnitude() < min.magnitude())
        {
            min = results[i];

            if (i > 11) {
				std::swap(min.start, min.end);
            }
        }
    }

    return min;
}

double geometry::ulp(double x)
{
    if (x > 0)
        return std::nexttoward(x, std::numeric_limits<double>::infinity()) - x;
    else
        return x - std::nexttoward(x, -std::numeric_limits<double>::infinity());
}

double geometry::coplanarityThreshold(const repo::lib::RepoTriangle& a, const repo::lib::RepoTriangle& b)
{
    repo::lib::RepoVector3D64 deltas[] = {
        a.a - b.a,
        a.a - b.b,
        a.a - b.c,
        a.b - b.a,
        a.b - b.b,
        a.b - b.c,
        a.c - b.a,
        a.c - b.b,
        a.c - b.c,
        a.a - a.b,
        a.a - a.c,
        a.b - a.c,
        b.a - b.b,
        b.a - b.c,
        b.b - b.c
    };
    auto dd = reinterpret_cast<double*>(&deltas);
    auto d = abs(dd[0]);
    for (int i = 1; i < 15; ++i) {
        auto ddd = std::abs(dd[i]);
        if (ddd > d) {
            d = ddd;
        }
    }
    return d * 1e-9;
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

static inline repo::lib::RepoVector3D64 edgePlaneIntersection(
    const repo::lib::RepoVector3D64& ep,
    const repo::lib::RepoVector3D64& ed,
    const repo::lib::RepoVector3D64& n,
    const repo::lib::RepoVector3D64& p
)
{
	return ep + ed * ((p - ep).dotProduct(n) / ed.dotProduct(n));
}

double checkMinMax(
    const repo::lib::RepoVector3D64& p1,
    const repo::lib::RepoVector3D64& q1,
    const repo::lib::RepoVector3D64& r1,
    const repo::lib::RepoVector3D64& p2,
    const repo::lib::RepoVector3D64& q2,
    const repo::lib::RepoVector3D64& r2,
    repo::lib::RepoVector3D64* ip)
{
    // This snippet performs an early rejection if the triangles are disjoint
    // on L. It works by finding whether the leftmost edge of one triangle is
    // oriented to the left or right of the rightmost edge of the other, and
    // vice versa.

	auto N1 = (p2 - p1).crossProduct(p1 - q1);
    if (N1.dotProduct(q2 - q1) > 0.0) {
        return 0.0;
    }
    N1 = (p2 - p1).crossProduct(r1 - p1);
    if (N1.dotProduct(r2 - p1) > 0.0) {
        return 0.0;
    }
	
	// If there is an overlap, estimate the minimum translation distance.
    // Triangles can either be shifted along L by k-j or l-i, or moved
    // normal so that they are separated by one or the other's planes.

    auto pr1 = r1 - p1;
    auto pq1 = q1 - p1;
    pr1.normalize();
	pq1.normalize();
    auto n1 = pq1.crossProduct(pr1);
    n1.normalize();

    auto pr2 = r2 - p2;
    auto pq2 = q2 - p2;
    pr2.normalize();
    pq2.normalize();
    auto n2 = pq2.crossProduct(pr2);
    n2.normalize();
  
	// i,j,k,l are the intersections of the edges with their counterpart's
    // plane.

	auto i = edgePlaneIntersection(p1, pr1, n2, p2);
	auto j = edgePlaneIntersection(p1, pq1, n2, p2);
	auto k = edgePlaneIntersection(p2, pq2, n1, p1);
    auto l = edgePlaneIntersection(p2, pr2, n1, p1);

    // This next snippet projects the points onto L, to get (possibly negative)
    // scalars representing their relative locations to i.

	auto L = n1.crossProduct(n2);
    L.normalize();
    
    auto ii = 0.0;
	auto jj = (j - i).dotProduct(L);
	auto kk = (k - i).dotProduct(L);
	auto ll = (l - i).dotProduct(L);

    if (kk > jj) {
        return 0;
    }
    if (ll < ii) {
        return 0;
    }

    auto d = jj - kk;
	auto d2 = ll - ii;
    if(d2 < d) {
        d = d2;
	}

    if (ip) {
		*ip = i + L * std::max((kk + jj) * 0.5, (ii + ll) * 0.5);
	}

    // This next snippet works out the minimum distance to move normal to the
	// planes, in case they are smaller than the shift along L.

	d2 = (p2 - p1).dotProduct(n1);
    if(d2 < d) {
        d = d2; // p2 is on the positive half-space so this will always be positive (and vice-versa for q & r below)
	}

    d2 = -std::min((q2 - p1).dotProduct(n1), (r2 - p1).dotProduct(n1));
    if (d2 < d) {
        d = d2;
    }

    d2 = (p1 - p2).dotProduct(n2);
    if (d2 < d) {
        d = d2;
    }

    d2 = -std::min((q1 - p2).dotProduct(n2), (r1 - p2).dotProduct(n2));
    if (d2 < d) {
        d = d2;
    }

    return d;
}

double geometry::intersects(const repo::lib::RepoTriangle& A, const repo::lib::RepoTriangle& B, repo::lib::RepoVector3D64* ip)
{
    // Performs a triangle-triangle intersection and returns the upper bound on
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
        return 0.0;
    }

    auto dp2 = -geometry::orient(p1, q1, r1, p2);
    auto dq2 = -geometry::orient(p1, q1, r1, q2);
    auto dr2 = -geometry::orient(p1, q1, r1, r2);

    if (((dp2 * dq2) > 0.0f) && ((dp2 * dr2) > 0.0f)) {
        return 0.0;
    }

    // Even on modern compilers, Guigue and Devillers' macros are still faster
    // at working out the permutations than function calls, by a factor of 2.

#define INTERSECT(p1,q1,r1,p2,q2,r2) return checkMinMax(p1,q1,r1,p2,q2,r2,ip);

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
			else return geometry::COPLANAR;\
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
            else return geometry::COPLANAR;
        }
    }

#undef PERMUTE2
#undef INTERSECT
}
