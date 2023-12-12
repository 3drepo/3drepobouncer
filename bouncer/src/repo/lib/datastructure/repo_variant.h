#pragma once
#include <iostream>
#include <boost/variant.hpp>
#include <boost/core/demangle.hpp>
#include <string>
#include "../../repo_bouncer_global.h"

namespace repo {

    namespace lib {
        typedef REPO_API_EXPORT boost::variant<int, double, std::string, bool, boost::blank, uint64_t, float> repoVariant;
        class REPO_API_EXPORT RepoVariant:public repoVariant {
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
            
            std::string getVariantType() const {
                try {
                    return boost::core::demangle(typeid(*this).name());
                }
                catch (const boost::bad_get& e) {
                    // Handle the case where the type is not supported
                    return "Unsupported type";
                }
            }

            bool isEmpty() const {
                return this->which() == 0;
            }

            bool isBool() const {
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
