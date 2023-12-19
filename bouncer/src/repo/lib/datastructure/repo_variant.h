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
            RepoDataType type = RepoDataType::OTHER;
            using boost::variant<int, double, std::string, bool, boost::blank, uint64_t, float>::variant;  // Inherit constructors
        public:

            // Default constructor
            RepoVariant() = default;

            RepoVariant(const std::string& data) : repoVariant(data),type(RepoDataType::STRING) {};

            RepoVariant(bool& data) : repoVariant(data), type(RepoDataType::BOOL) {};

            RepoVariant(int& data) : repoVariant(data), type(RepoDataType::INT) {};

            RepoVariant(double& data) : repoVariant(data), type(RepoDataType::DOUBLE) {};

            RepoVariant(uint64_t& data) : repoVariant(data), type(RepoDataType::UINT64) {};

            RepoVariant(float& data) : repoVariant(data), type(RepoDataType::FLOAT) {};

            RepoVariant(std::string& data) : repoVariant(data), type(RepoDataType::STRING) {};


            RepoVariant& operator=(const std::string& str) {
                repoVariant::operator=(str);
                return *this;
            }
            
            // New function to convert any data type to RepoVariant
            template <typename T>
            RepoVariant convertToRepoVariant(const T& value) {
                switch (type)
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
                    return type;
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
