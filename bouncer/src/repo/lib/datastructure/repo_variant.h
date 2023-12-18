#pragma once
#include <iostream>
#include <boost/variant.hpp>
#include <boost/core/demangle.hpp>
#include <string>
#include "../../repo_bouncer_global.h"

namespace repo {

    namespace lib {
        using repoVariant = boost::variant<int, double, std::string, bool, boost::blank, uint64_t, float>;
        enum class RepoDataType { STRING, FLOAT, BOOL, DOUBLE, INT, UINT64, OTHER};
        class RepoVariant:private repoVariant {
        private:
            template <typename T>
            RepoDataType getVariantType(const T& value) const {
                try {
                    std::string dataTypeName = boost::core::demangle(typeid(T).name());
                    if ("int" == dataTypeName) {
                        return RepoDataType::INT;
                    }
                    else if ("double" == dataTypeName) {
                        return RepoDataType::DOUBLE;
                    }
                    else if ("std::string" == dataTypeName)
                    {
                        return RepoDataType::STRING;
                    }
                    else if ("bool" == dataTypeName)
                    {
                        return RepoDataType::BOOL;
                    }
                    else if ("float" == dataTypeName)
                    {
                        return RepoDataType::FLOAT;
                    }
                    else if ("uint64_t" == dataTypeName)
                    {
                        return RepoDataType::UINT64;
                    }
                }
                catch (const boost::bad_get& e) {
                    // Handle the case where the type is not supported
                    return RepoDataType::OTHER;
                }
            }

        public:
           using boost::variant<int, double, std::string, bool, boost::blank, uint64_t, float>::variant;  // Inherit constructors

            // Default constructor
            RepoVariant() = default;

            RepoVariant(const std::string& other) : repoVariant(other) {};

            RepoVariant(bool& other) : repoVariant(other) {};

            RepoVariant(int& other) : repoVariant(other) {};

            RepoVariant(double& other) : repoVariant(other) {};

            RepoVariant(uint64_t& other) : repoVariant(other) {};

            RepoVariant(float& other) : repoVariant(other) {};

            RepoVariant(std::string& other) : repoVariant(other) {};


            RepoVariant& operator=(const std::string& str) {
                repoVariant::operator=(str);
                return *this;
            }
            
            // New function to convert any data type to RepoVariant
            template <typename T>
            RepoVariant convertToRepoVariant(const T& value) {
                RepoDataType dataType = getVariantType(value);
                switch (dataType)
                {
                case repo::lib::RepoDataType::STRING:
                case repo::lib::RepoDataType::FLOAT:
                case repo::lib::RepoDataType::BOOL:
                case repo::lib::RepoDataType::DOUBLE:
                case repo::lib::RepoDataType::INT:
                case repo::lib::RepoDataType::UINT64:
                    return RepoVariant(&value);
                    break;
                case repo::lib::RepoDataType::OTHER:
                    return NULL;
                    break;
                default:
                    break;
                }
            }

            RepoDataType getVariantType() const {
                try {
                    std::string dataTypeName = boost::core::demangle(typeid(*this).name());
                    if ("int" == dataTypeName) {
                        return RepoDataType::INT;
                    }
                    else if ("double" == dataTypeName) {
                        return RepoDataType::DOUBLE;
                    }
                    else if ("std::string" == dataTypeName)
                    {
                        return RepoDataType::STRING;
                    }
                    else if ("bool" == dataTypeName)
                    {
                        return RepoDataType::BOOL;
                    }
                    else if ("float" == dataTypeName)
                    {
                        return RepoDataType::FLOAT;
                    }
                    else if ("uint64_t" == dataTypeName)
                    {
                        return RepoDataType::UINT64;
                    }
                }
                catch (const boost::bad_get& e) {
                    // Handle the case where the type is not supported
                    return RepoDataType::OTHER;
                }
            }
                 

            bool isEmpty() const {
                return this->which() == 0;
            }

            bool toBool() const {
                return boost::get<bool>(this) != nullptr;
            }

            int toInt() const {
                try {
                    return boost::get<int>(*this);
                }
                catch (const boost::bad_get& e) {
                    throw std::runtime_error("Failed to convert variant to int");
                }
            }

            double toDouble() const {
                try {
                    return boost::get<double>(*this);
                }
                catch (const boost::bad_get& e) {
                    throw std::runtime_error("Failed to convert variant to double");
                }
            }

            std::string toString() const {
                try {
                    return boost::get<std::string>(*this);
                }
                catch (const boost::bad_get& e) {
                    throw std::runtime_error("Failed to convert variant to std::string");
                }
            }

            uint64_t toUint64() const {
                try {
                    return boost::get<uint64_t>(*this);
                }
                catch (const boost::bad_get& e) {
                    throw std::runtime_error("Failed to convert variant to uint64_t");
                }
            }

            float toFloat() const {
                try {
                    return boost::get<float>(*this);
                }
                catch (const boost::bad_get& e) {
                    throw std::runtime_error("Failed to convert variant to float");
                }
            }

            static std::unordered_map<std::string, RepoVariant> convertToRepoVariant(const std::unordered_map<std::string, std::string>& inputMap) {
                std::unordered_map<std::string, RepoVariant> result;

                for (const auto& entry : inputMap) {
                    result[entry.first] = RepoVariant(entry.second);
                }

                return result;
            }
        };

    }  // namespace lib

}  // namespace repo
