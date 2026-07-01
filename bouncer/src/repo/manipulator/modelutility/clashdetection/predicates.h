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

namespace predicates {
    void exactinit();
    double orient2d(double pa[2], double pb[2], double pc[2]);
    double orient3d(double pa[3], double pb[3], double pc[3], double pd[3]);
    double incircle(double pa[2], double pb[2], double pc[2], double pd[2]);
    double insphere(double pa[3], double pb[3], double pc[3], double pd[3], double pe[3]);
};