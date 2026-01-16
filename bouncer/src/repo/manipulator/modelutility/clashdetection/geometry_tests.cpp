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
#include <unordered_map>

using namespace geometry;

static const repo::lib::RepoVector3D64 axes[] = {
    repo::lib::RepoVector3D64(1.0, 0.0, 0.0),
    repo::lib::RepoVector3D64(0.0, 1.0, 0.0),
    repo::lib::RepoVector3D64(0.0, 0.0, 1.0),
};

repo::lib::RepoVector3D64 geometry::closestPoint(const repo::lib::RepoVector3D64& p, const repo::lib::RepoTriangle& T)
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

repo::lib::RepoLine geometry::closestPoints(const repo::lib::RepoVector3D64& p, const repo::lib::RepoTriangle& T)
{
    return { p, closestPoint(p, T) };
}

repo::lib::RepoLine geometry::closestPoints(const repo::lib::RepoLine& A, const repo::lib::RepoLine& B)
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

FaceFaceResult geometry::closestPoints(
	const repo::lib::RepoTriangle& A, 
	const repo::lib::RepoTriangle& B)
{
    // Ericson, C. (2004). Real-Time Collision Detection. CRC Press.

    repo::lib::RepoVector3D64 p;
    auto i = intersects(A, B, &p);

    // For the distance test, we do not care to disambiguate the contact from
    // the coplanar case, as both mean the distance between the triangles is
    // infinitesimal.

    if (i > COPLANAR) {
        return FaceFaceResult{ repo::lib::RepoLine{ p, p }, true, i };
    }

    repo::lib::RepoLine Ea1(A.a, A.b);
    repo::lib::RepoLine Ea2(A.b, A.c);
    repo::lib::RepoLine Ea3(A.c, A.a);
    repo::lib::RepoLine Eb1(B.a, B.b);
    repo::lib::RepoLine Eb2(B.b, B.c);
    repo::lib::RepoLine Eb3(B.c, B.a);

    repo::lib::RepoLine results[15];

    results[0] = closestPoints(Ea1, Eb1);
    results[1] = closestPoints(Ea1, Eb2);
    results[2] = closestPoints(Ea1, Eb3);
    results[3] = closestPoints(Ea2, Eb1);
    results[4] = closestPoints(Ea2, Eb2);
    results[5] = closestPoints(Ea2, Eb3);
    results[6] = closestPoints(Ea3, Eb1);
    results[7] = closestPoints(Ea3, Eb2);
    results[8] = closestPoints(Ea3, Eb3);

    results[9] =  closestPoints(A.a, B);
    results[10] = closestPoints(A.b, B);
    results[11] = closestPoints(A.c, B);

    results[12] = closestPoints(B.a, A);
    results[13] = closestPoints(B.b, A);
    results[14] = closestPoints(B.c, A);

    repo::lib::RepoLine min = results[0];
    for (int i = 1; i < 15; i++)
    {
        if (results[i].magnitude() < min.magnitude())
        {
            min = results[i];

            // This ensures the method always returns the point on A in start,
            // and on B in end
            if (i > 11) {
				std::swap(min.start, min.end);
            }
        }
    }

    return { min, false, COPLANAR };
}

double geometry::ulp(double x)
{
    if (x > 0)
        return std::nexttoward(x, std::numeric_limits<double>::infinity()) - x;
    else
        return x - std::nexttoward(x, -std::numeric_limits<double>::infinity());
}

static double cpt(
    std::initializer_list<repo::lib::RepoVector3D64> deltas) 
{
    auto th = 0.0;
    for (const auto& d : deltas) {
        th = std::max({ th, std::abs(d.x), std::abs(d.y), std::abs(d.z) });
    }
	return th * 1e-9;
}

double geometry::contactThreshold(const repo::lib::RepoTriangle& a, const repo::lib::RepoTriangle& b)
{
    return cpt({ 
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
     });
}

double geometry::contactThreshold(
	const repo::lib::RepoBounds& a,
	const repo::lib::RepoBounds& b
)
{
    return cpt({
        a.min() - b.min(),
        a.min() - b.max(),
		a.max() - b.min(),
		a.max() - b.max(),
        a.size(),
		b.size()
    });
}

double geometry::orient(const repo::lib::RepoVector3D64& p, const repo::lib::RepoVector3D64& q, const repo::lib::RepoVector3D64& r, const repo::lib::RepoVector3D64& s)
{
	return predicates::orient3d((double*)&p, (double*)&q, (double*)&r, (double*)&s);
}

repo::lib::RepoVector3D64 geometry::minimumSeparatingAxis(const repo::lib::RepoBounds& a, const repo::lib::RepoBounds& b)
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
        auto t = std::max(ii, kk) + ((std::min(jj, ll) - std::max(ii, kk)) * 0.5);
		*ip = i + L * t;
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

double geometry::intersects(const repo::lib::RepoVector3D64& origin,
    const repo::lib::RepoVector3D64& direction,
    const repo::lib::RepoTriangle& triangle,
    int* edges,
    bool* degenerate)
{
	// This implementation is based on Woop & Walds Watertight Ray/Triangle
    // intersection, with adjustments to report edge and coplanarity
    // checks.

    if (edges) {
        *edges = 0;
    }

    // Sven Woop, Carsten Benthin, and Ingo Wald. Watertight Ray/Triangle
    // Intersection. Journal of Computer Graphics Techniques (JCGT), Vol. 2,
    // No. 1, 65-82, 2013. http://jcgt.org/published/0002/01/05/

    // 1. Move triangle into ray space
    // Find the dominant axis of the ray direction
    int kz = 0;
    double absDir[3] = { std::abs(direction.x), std::abs(direction.y), std::abs(direction.z) };
    if (absDir[1] > absDir[0]) kz = 1;
    if (absDir[2] > absDir[kz]) kz = 2;
    int kx = (kz + 1) % 3;
    int ky = (kz + 2) % 3;

    // Swap kx and ky to preserve winding if direction[kz] < 0
    double dirArray[3] = { direction.x, direction.y, direction.z };
    if (dirArray[kz] < 0.0) std::swap(kx, ky);

    // Ray origin and direction
    double Sx = dirArray[kx] / dirArray[kz];
    double Sy = dirArray[ky] / dirArray[kz];
    double Sz = 1.0 / dirArray[kz];

    // Triangle vertices relative to ray origin
    double v0[3] = { triangle.a.x - origin.x, triangle.a.y - origin.y, triangle.a.z - origin.z };
    double v1[3] = { triangle.b.x - origin.x, triangle.b.y - origin.y, triangle.b.z - origin.z };
    double v2[3] = { triangle.c.x - origin.x, triangle.c.y - origin.y, triangle.c.z - origin.z };

    // Permute vertices into ray space
    double Ax = v0[kx] - Sx * v0[kz];
    double Ay = v0[ky] - Sy * v0[kz];
    double Bx = v1[kx] - Sx * v1[kz];
    double By = v1[ky] - Sy * v1[kz];
    double Cx = v2[kx] - Sx * v2[kz];
    double Cy = v2[ky] - Sy * v2[kz];

    // Compute edge functions
    double U = Cx * By - Cy * Bx;
    double V = Ax * Cy - Ay * Cx;
    double W = Bx * Ay - By * Ax;

	// Determine if the triangle test itself is degenerate
    double th = std::max({
		std::abs(Ax), std::abs(Ay), std::abs(Bx), std::abs(By), std::abs(Cx), std::abs(Cy)
	}) * 1.2e-6;

    if (std::abs(U) < th && std::abs(V) < th && std::abs(W) < th) {
        if(degenerate) {
            *degenerate = true;
		}
        return std::numeric_limits<double>::infinity();
    }
    double det = U + V + W;

    if ((U < 0.0 || V < 0.0 || W < 0.0) && (U > 0.0 || V > 0.0 || W > 0.0))
        return std::numeric_limits<double>::infinity();

    // Compute scaled hit distance to triangle
    double Az = Sz * v0[kz];
    double Bz = Sz * v1[kz];
    double Cz = Sz * v2[kz];
    double T = U * Az + V * Bz + W * Cz;

    double t = T / det;
    if (t < 0.0)
        return std::numeric_limits<double>::infinity();

    // It is important to identify the case where the intersection lies exactly on
    // an edge, because in this case we will get the same result for another triangle
    // sharing the edge (as we should), and so the upstream algorithm may want to
    // consider this.

    if (edges) {
        if (U == 0.0) {
            (*edges)++;
        }
        if (V == 0.0) {
            (*edges)++;
        }
        if (W == 0.0) {
            (*edges)++;
        }
    }

    return t;
}

repo::lib::RepoLine geometry::closestPoints(
    const repo::lib::RepoBounds& a, 
    const repo::lib::RepoBounds& b)
{
    repo::lib::RepoVector3D64 _a;
	repo::lib::RepoVector3D64 _b;

    if (a.min().x >= b.max().x) {
		_a.x = a.min().x;
        _b.x = b.max().x;
    }
    else if (b.min().x >= a.max().x) {
        _a.x = a.max().x;
		_b.x = b.min().x;
    }
    else {
		_a.x = (std::max(a.min().x, b.min().x) + std::min(a.max().x, b.max().x)) * 0.5;
        _b.x = _a.x;
    }

    if (a.min().y >= b.max().y) {
        _a.y = a.min().y;
        _b.y = b.max().y;
    }
    else if (b.min().y >= a.max().y) {
        _a.y = a.max().y;
        _b.y = b.min().y;
    }
    else {
        _a.y = (std::max(a.min().y, b.min().y) + std::min(a.max().y, b.max().y)) * 0.5;
        _b.y = _a.y;
    }

    if (a.min().z >= b.max().z) {
        _a.z = a.min().z;
        _b.z = b.max().z;
    }
    else if (b.min().z >= a.max().z) {
        _a.z = a.max().z;
        _b.z = b.min().z;
    }
    else {
        _a.z = (std::max(a.min().z, b.min().z) + std::min(a.max().z, b.max().z)) * 0.5;
        _b.z = _a.z;
    }

	return { _a, _b };
}

// Local template to implement time-of-contact for different primitives, since
// the algorithm is substantially the same regardless, so long as the primitive
// supports the closestPoints() operation, the translation operation via
// the addition operator, and the contactThreshold() function.

// See:
// Interactive Continuous Collision Detection for Non-Convex Polyhedra. Xinyu
// Zhang, Minkyoung Lee, Young J. Kim. The Visual Computer: International
// Journal of Computer Graphics, Volume 22, Issue 9. 
// https://graphics.ewha.ac.kr/FAST/FAST.pdf
// and
// PolyDepth: Real-time Penetration Depth Computation using Iterative Contact-
// Space Projection. Changsoo Je, Min Tang, Youngeun Lee, Minkyoung Lee, Young
// J.Kim.ACM Transactions on Graphics(ToG 2012), Volume 31, Issue 1, Article
// 5, pp. 1 - 14, January 1, 2012. https://arxiv.org/abs/1508.06181

template<typename Primitive>
static double timeOfContactT(
    const Primitive& a,
    const Primitive& b,
    const repo::lib::RepoVector3D64& v,
    double contact
)
{
    if (!contact) {
		contact = geometry::contactThreshold(a, b);
    }

    // Uses conservative advancement to compute the time-of-contact.
    // This involves using the closest points along the direction vector to move
    // the bounds closer step-wise until they intersect.

    auto at = a;
    auto t = 0.0;

    while (true) {

        auto N = closestPoints(at, b);
        auto n = N.end - N.start;
        auto d = n.norm();

        if (d <= contact) {
            return t;
        }

        auto mu = v.dotProduct(n.normalized());

        // If there is no contact, but also moving along v will not bring
        // the bounds any closer, then they will never come into contact.
        if (mu <= 0.0) {
            return std::numeric_limits<double>::infinity();
        }

        // Limit the timestep to ensure that when contact is non-zero, the
        // translation always ends up between the contact region and the true
        // surface, and does not overshot into an actual collision.
        // A scalar that is too large can result in overshoot. A small scalar
        // will slow convergence, but not affect the correctness of the result,
        // so if there is any uncertainty, err towards a smaller value. The value
        // is empirically validated by the TimeOfContactTriangles unit test.

        t += (d / mu) * 0.95;

        if (t >= 1.0) {
            return t;
        }

        at = a + (v * t);
    }
}

double geometry::timeOfContact(
    const repo::lib::RepoBounds& a,
    const repo::lib::RepoBounds& b,
    const repo::lib::RepoVector3D64& v,
    double contact
) 
{
	return timeOfContactT(a, b, v, contact);
}

double geometry::timeOfContact(
    const repo::lib::RepoTriangle& a,
    const repo::lib::RepoTriangle& b,
    const repo::lib::RepoVector3D64& v,
    double contact
)
{
	return timeOfContactT(a, b, v, contact);
}

bool geometry::isClosedAndManifold(
    const std::vector<repo::lib::repo_face_t>& triangles
)
{
	return isClosedAndManifold(triangles.data(), triangles.size());
}

bool geometry::isClosedAndManifold(
    const repo::lib::repo_face_t* triangles,
    size_t numTriangles
)
{
    // A mesh is closed if every edge has at least one sibling, and manifold if
    // there is exactly one sibling (that is, there are no T-junctions or 
    // interior triangles).

    // If a mesh is closed & manifold, it is possible to determine precisely
    // whether a point is on the inside or outside of a mesh, for example using
    // the ray-casting method.

    // This test is insensitive to winding order, as the ray-cast test is
    // insensitive too.

    struct edge_hash {
        std::size_t operator () (const std::pair<size_t, size_t>& p) const {
            // In the real world, the number of vertices in a single mesh will
            // not come close to the limits of a uint32.
            return p.first << 32 ^ p.second;
        }
    };

    std::unordered_map<std::pair<size_t, size_t>, size_t, edge_hash> edge_map;

    for (size_t i = 0; i < numTriangles; i++)
    {
		const auto& tri = triangles[i];
  
        auto edges = {
            std::make_pair(std::min(tri[0], tri[1]), std::max(tri[0], tri[1])),
            std::make_pair(std::min(tri[1], tri[2]), std::max(tri[1], tri[2])),
            std::make_pair(std::min(tri[2], tri[0]), std::max(tri[2], tri[0]))
        };

        for (const auto& edge : edges) {
            edge_map[edge]++;
        }
    }

    for (const auto& [edge, count] : edge_map) {
        if (count != 2) {
            return false;
        }
    }

    return true;
}
