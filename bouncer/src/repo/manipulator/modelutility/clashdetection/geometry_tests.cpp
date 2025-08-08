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

using namespace geometry;

using Vector3 = repo::lib::RepoVector3D64;

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

    repo::lib::RepoLine min = results[0];
    for (int i = 1; i < 15; i++)
    {
        if (results[i].magnitude() < min.magnitude())
        {
            min = results[i];
        }
    }

    return min;
}