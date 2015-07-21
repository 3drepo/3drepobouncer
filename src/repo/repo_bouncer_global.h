#pragma once
#include <cstdint>

#if defined(_WIN32) || defined(_WIN64)
#   define REPO_DECL_EXPORT __declspec(dllexport)
#   define REPO_DECL_IMPORT __declspec(dllimport)
#else
#   define REPO_DECL_EXPORT
#   define REPO_DECL_IMPORT
#endif

//------------------------------------------------------------------------------
#if defined(REPO_API_LIBRARY)
#   define REPO_API_EXPORT REPO_DECL_EXPORT
#else
#   define REPO_API_EXPORT REPO_DECL_IMPORT
#endif