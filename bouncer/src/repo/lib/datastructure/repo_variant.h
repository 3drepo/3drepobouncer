#pragma once
#include <iostream>
#include <boost/variant.hpp>
#include <boost/core/demangle.hpp>
#include <string>
#include "../../repo_bouncer_global.h"

namespace repo {

    namespace lib {
        using repoVariant = boost::variant<int, double, std::string, bool, boost::blank, uint64_t, float,long, unsigned long, __int64>;
        enum class RepoDataType { STRING, FLOAT, BOOL, DOUBLE, INT, UINT64,LONG,ULONG,INT64,OTHER};
        class RepoVariant:private repoVariant {
        private:
            RepoDataType type = RepoDataType::OTHER;
            using boost::variant<int, double, std::string, bool, boost::blank, uint64_t, float,long, unsigned long, __int64>::variant;  // Inherit constructors
        public:
            using boost::variant<int, double, std::string, bool, boost::blank, uint64_t, float, long, unsigned long, __int64>::operator=;  // Inherit = operator

            // Default constructor
            RepoVariant() = default;

            RepoVariant(const bool& data) : repoVariant(data), type(RepoDataType::BOOL) {};

            RepoVariant(const int& data) : repoVariant(data), type(RepoDataType::INT) {};

            RepoVariant(const double& data) : repoVariant(data), type(RepoDataType::DOUBLE) {};

            RepoVariant(const uint64_t& data) : repoVariant(data), type(RepoDataType::UINT64) {};

            RepoVariant(const float& data) : repoVariant(data), type(RepoDataType::FLOAT) {};

            RepoVariant(const std::string& data) : repoVariant(data), type(RepoDataType::STRING) {};

            RepoVariant(const long& data) : repoVariant(data), type(RepoDataType::LONG) {};

            RepoVariant(const unsigned long& data) : repoVariant(data), type(RepoDataType::ULONG) {};

            RepoVariant(const __int64& data) : repoVariant(data), type(RepoDataType::INT64) {};
            
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
                case repo::lib::RepoDataType::LONG:
                case repo::lib::RepoDataType::ULONG:
                case repo::lib::RepoDataType::INT64:
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
        };

    }

}
