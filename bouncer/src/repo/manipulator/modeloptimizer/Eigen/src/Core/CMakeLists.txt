#THIS IS AN AUTOMATICALLY GENERATED FILE - DO NOT OVERWRITE THE CONTENT!
#If you need to update the sources/headers/sub directory information, run updateSources.py at project root level
#If you need to import an extra library or something clever, do it on the CMakeLists.txt at the root level
#If you really need to overwrite this file, be aware that it will be overwritten if updateSources.py is executed.


add_subdirectory(arch)
add_subdirectory(functors)
add_subdirectory(products)
add_subdirectory(util)
set(HEADERS
	${HEADERS}
	${CMAKE_CURRENT_SOURCE_DIR}/Array.h
	${CMAKE_CURRENT_SOURCE_DIR}/ArrayBase.h
	${CMAKE_CURRENT_SOURCE_DIR}/ArrayWrapper.h
	${CMAKE_CURRENT_SOURCE_DIR}/Assign.h
	${CMAKE_CURRENT_SOURCE_DIR}/AssignEvaluator.h
	${CMAKE_CURRENT_SOURCE_DIR}/Assign_MKL.h
	${CMAKE_CURRENT_SOURCE_DIR}/BandMatrix.h
	${CMAKE_CURRENT_SOURCE_DIR}/Block.h
	${CMAKE_CURRENT_SOURCE_DIR}/BooleanRedux.h
	${CMAKE_CURRENT_SOURCE_DIR}/CommaInitializer.h
	${CMAKE_CURRENT_SOURCE_DIR}/ConditionEstimator.h
	${CMAKE_CURRENT_SOURCE_DIR}/CoreEvaluators.h
	${CMAKE_CURRENT_SOURCE_DIR}/CoreIterators.h
	${CMAKE_CURRENT_SOURCE_DIR}/CwiseBinaryOp.h
	${CMAKE_CURRENT_SOURCE_DIR}/CwiseNullaryOp.h
	${CMAKE_CURRENT_SOURCE_DIR}/CwiseTernaryOp.h
	${CMAKE_CURRENT_SOURCE_DIR}/CwiseUnaryOp.h
	${CMAKE_CURRENT_SOURCE_DIR}/CwiseUnaryView.h
	${CMAKE_CURRENT_SOURCE_DIR}/DenseBase.h
	${CMAKE_CURRENT_SOURCE_DIR}/DenseCoeffsBase.h
	${CMAKE_CURRENT_SOURCE_DIR}/DenseStorage.h
	${CMAKE_CURRENT_SOURCE_DIR}/Diagonal.h
	${CMAKE_CURRENT_SOURCE_DIR}/DiagonalMatrix.h
	${CMAKE_CURRENT_SOURCE_DIR}/DiagonalProduct.h
	${CMAKE_CURRENT_SOURCE_DIR}/Dot.h
	${CMAKE_CURRENT_SOURCE_DIR}/EigenBase.h
	${CMAKE_CURRENT_SOURCE_DIR}/ForceAlignedAccess.h
	${CMAKE_CURRENT_SOURCE_DIR}/Fuzzy.h
	${CMAKE_CURRENT_SOURCE_DIR}/GeneralProduct.h
	${CMAKE_CURRENT_SOURCE_DIR}/GenericPacketMath.h
	${CMAKE_CURRENT_SOURCE_DIR}/GlobalFunctions.h
	${CMAKE_CURRENT_SOURCE_DIR}/IO.h
	${CMAKE_CURRENT_SOURCE_DIR}/Inverse.h
	${CMAKE_CURRENT_SOURCE_DIR}/Map.h
	${CMAKE_CURRENT_SOURCE_DIR}/MapBase.h
	${CMAKE_CURRENT_SOURCE_DIR}/MathFunctions.h
	${CMAKE_CURRENT_SOURCE_DIR}/MathFunctionsImpl.h
	${CMAKE_CURRENT_SOURCE_DIR}/Matrix.h
	${CMAKE_CURRENT_SOURCE_DIR}/MatrixBase.h
	${CMAKE_CURRENT_SOURCE_DIR}/NestByValue.h
	${CMAKE_CURRENT_SOURCE_DIR}/NoAlias.h
	${CMAKE_CURRENT_SOURCE_DIR}/NumTraits.h
	${CMAKE_CURRENT_SOURCE_DIR}/PermutationMatrix.h
	${CMAKE_CURRENT_SOURCE_DIR}/PlainObjectBase.h
	${CMAKE_CURRENT_SOURCE_DIR}/Product.h
	${CMAKE_CURRENT_SOURCE_DIR}/ProductEvaluators.h
	${CMAKE_CURRENT_SOURCE_DIR}/Random.h
	${CMAKE_CURRENT_SOURCE_DIR}/Redux.h
	${CMAKE_CURRENT_SOURCE_DIR}/Ref.h
	${CMAKE_CURRENT_SOURCE_DIR}/Replicate.h
	${CMAKE_CURRENT_SOURCE_DIR}/ReturnByValue.h
	${CMAKE_CURRENT_SOURCE_DIR}/Reverse.h
	${CMAKE_CURRENT_SOURCE_DIR}/Select.h
	${CMAKE_CURRENT_SOURCE_DIR}/SelfAdjointView.h
	${CMAKE_CURRENT_SOURCE_DIR}/SelfCwiseBinaryOp.h
	${CMAKE_CURRENT_SOURCE_DIR}/Solve.h
	${CMAKE_CURRENT_SOURCE_DIR}/SolveTriangular.h
	${CMAKE_CURRENT_SOURCE_DIR}/SolverBase.h
	${CMAKE_CURRENT_SOURCE_DIR}/StableNorm.h
	${CMAKE_CURRENT_SOURCE_DIR}/Stride.h
	${CMAKE_CURRENT_SOURCE_DIR}/Swap.h
	${CMAKE_CURRENT_SOURCE_DIR}/Transpose.h
	${CMAKE_CURRENT_SOURCE_DIR}/Transpositions.h
	${CMAKE_CURRENT_SOURCE_DIR}/TriangularMatrix.h
	${CMAKE_CURRENT_SOURCE_DIR}/VectorBlock.h
	${CMAKE_CURRENT_SOURCE_DIR}/VectorwiseOp.h
	${CMAKE_CURRENT_SOURCE_DIR}/Visitor.h
	CACHE STRING "HEADERS" FORCE)

