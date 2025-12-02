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

namespace repo {
	namespace linearalgebra {
		namespace matrix {
			/*
			* The following are convenience types for performing simple matrix operations
			* on mapped data structures.
			* 
			* The types are based on the equivalent Eigen expressions, and are intended
            * to follow the same API so use of these types is interchangable with Eigen
            * types (though they cannot be mixed). Using these types however ensures that
            * no dynamic allocations are made.
			* 
			* These are intended for small problems where element-wise formulas are
			* available. Complex or large problems should just use Eigen.
			*/

            struct Base {
                virtual double& operator()(size_t row, size_t col = 0) const = 0;
                virtual size_t rows() const = 0;
                virtual size_t cols() const = 0;
            };

            struct Product {
                Product(Base& A, Base& B) : A(A), B(B) {
                }

                double operator()(size_t row, size_t col = 0) const {
                    double sum = 0.0;
                    for (size_t i = 0; i < B.rows(); i++) {
                        sum += A(row, i) * B(i, col);
                    }
                    return sum;
                }

                size_t rows() const {
                    return A.rows();
                }

            private:
                Base& A;
                Base& B;
            };

            struct Transpose : public Base {
                Transpose(Base& view) : view(view) {
                }

                double& operator()(size_t row, size_t col = 0) const override {
                    return view(col, row);
                }

                size_t rows() const override {
                    return view.cols();
                }

                size_t cols() const override {
                    return view.rows();
                }

                Product operator* (Base& other) {
                    return Product(*this, other);
                }

            private:
                Base& view;
            };

            struct View : public Base {
                View(double* data, size_t rows, size_t cols, size_t stride) : data(data), numRows(rows), numCols(cols), stride(stride) {
                }

                double& operator()(size_t row, size_t col = 0) const override {
                    return data[row * stride + col];
                }

                size_t rows() const override {
                    return numRows;
                }

                size_t cols() const override {
                    return numCols;
                }

                void setZero() {
                    for (size_t r = 0; r < numRows; r++) {
                        for (size_t c = 0; c < numCols; c++) {
                            operator()(r, c) = 0.0;
                        }
                    }
                }

                Transpose transpose() {
                    return Transpose(*this);
                }

                Product operator* (Base& other) {
                    return Product(*this, other);
                }

            private:
                double* data;
                size_t numRows;
                size_t numCols;
                size_t stride;
            };
		}
	}
}