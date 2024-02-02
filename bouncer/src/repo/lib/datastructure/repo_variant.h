#pragma once
#include <iostream>
#include <boost/variant.hpp>
#include <string>
#include "../../repo_bouncer_global.h"
#include "repo/lib/repo_log.h"

namespace repo {

    namespace lib {
        using repoVariant = boost::variant<int, double, std::string, bool, uint64_t, float, long>;
        enum RepoDataType { STRING, FLOAT, BOOL, DOUBLE, INT, UINT64, LONG,OTHER };
        class RepoVariant :private repoVariant {
        public:
            using repoVariant::operator=;

            RepoVariant() : repoVariant() {};

            RepoVariant(const bool& data) : repoVariant(data){};

            RepoVariant(const int& data) : repoVariant(data){};

            RepoVariant(const double& data) : repoVariant(data){};

            RepoVariant(const uint64_t& data) : repoVariant(data){};

            RepoVariant(const float& data) : repoVariant(data){};

            RepoVariant(const std::string& data) : repoVariant(data){};

            RepoVariant(const long& data) : repoVariant(data){};

            repo::lib::RepoDataType getVariantType() {
                switch (which()) {
                case 0:
                    return repo::lib::RepoDataType::INT;
                case 1:
                    return repo::lib::RepoDataType::DOUBLE;
                case 2:
                    return repo::lib::RepoDataType::STRING;
                case 3:
                    return repo::lib::RepoDataType::BOOL;
                case 4:
                    return repo::lib::RepoDataType::UINT64;
                case 5:
                    return repo::lib::RepoDataType::FLOAT;
                case 6:
                    return repo::lib::RepoDataType::LONG;
                }
                return repo::lib::RepoDataType::OTHER;
            }

            bool isEmpty() const {
                return this->empty();
            }

            bool toBool() const {
                return boost::get<bool>(*this);
            }

            int toInt() const {
                try {
                    return boost::get<int>(*this);
                }
                catch (const boost::bad_get& e) {
                    repoError << "Failed to convert variant to int";
                }
            }

            double toDouble() const {
                try {
                    return boost::get<double>(*this);
                }
                catch (const boost::bad_get& e) {
                    repoError << "Failed to convert variant to double";
                }
            }

            std::string toString() const {
                try {
                    return boost::get<std::string>(*this);
                }
                catch (const boost::bad_get& e) {
                    repoError << "Failed to convert variant to std::string";
                }
            }

            uint64_t toUint64() const {
                try {
                    return boost::get<uint64_t>(*this);
                }
                catch (const boost::bad_get& e) {
                    repoError << "Failed to convert variant to uint64_t";
                }
            }

            float toFloat() const {
                try {
                    return boost::get<float>(*this);
                }
                catch (const boost::bad_get& e) {
                    repoError << "Failed to convert variant to float";
                }
            }
        };

    }
}