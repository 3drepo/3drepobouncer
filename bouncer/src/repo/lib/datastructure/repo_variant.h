#pragma once
#include <iostream>
#include <boost/variant.hpp>
#include <string>
#include "../../repo_bouncer_global.h"
#include "repo/lib/repo_log.h"

namespace repo {
    namespace lib {
        using boostVariantType = boost::variant<int, double, std::string, bool, uint64_t, float, long,unsigned long>;
        enum RepoDataType { INT, DOUBLE,STRING, BOOL,UINT64, FLOAT,LONG,UNSIGNEDLONG,OTHER };
        class RepoVariant : private boostVariantType {
        public:
            using boostVariantType::operator=;

            RepoVariant() : boostVariantType() {};

            RepoVariant(const bool& data) : boostVariantType(data){};

            RepoVariant(const int& data) : boostVariantType(data){};

            RepoVariant(const double& data) : boostVariantType(data){};

            RepoVariant(const uint64_t& data) : boostVariantType(data){};

            RepoVariant(const float& data) : boostVariantType(data){};

            RepoVariant(const std::string& data) : boostVariantType(data){};

            RepoVariant(const long& data) : boostVariantType(data){};


            repo::lib::RepoDataType getVariantType() {
                const std::vector<repo::lib::RepoDataType> mapping = {repo::lib::RepoDataType::INT,
                                                                      repo::lib::RepoDataType::DOUBLE,
                                                                      repo::lib::RepoDataType::STRING,
                                                                      repo::lib::RepoDataType::BOOL,
                                                                      repo::lib::RepoDataType::UINT64,
                                                                      repo::lib::RepoDataType::FLOAT,
                                                                      repo::lib::RepoDataType::LONG,
                                                                      repo::lib::RepoDataType::OTHER};
                auto typeIdx = which();
                return (typeIdx > mapping.size())? repo::lib::RepoDataType::OTHER : mapping[typeIdx];
            }

            bool isEmpty() const {
                return this->empty();
            }

            template <class T>
            bool getBaseData(T& t) {
                switch (getVariantType()) {
                    case repo::lib::RepoDataType::INT: {
                        t = boost::get<int>(*this);
                        break;
                    }
                    case repo::lib::RepoDataType::DOUBLE: {
                        t = boost::get<double>(*this);
                        break;
                    }
                    /*case repo::lib::RepoDataType::STRING: {
                        t = boost::get<std::string>(*this);
                        break;
                    }*/
                    case repo::lib::RepoDataType::BOOL: {
                        t = boost::get<bool>(*this);
                        break;
                    }
                    case repo::lib::RepoDataType::UINT64: {
                        t = boost::get<uint64_t>(*this);
                        break;
                    }
                    case repo::lib::RepoDataType::FLOAT: {
                        t = boost::get<float>(*this);
                        break;
                    }
                    case repo::lib::RepoDataType::LONG: {
                        t = boost::get<long>(*this);
                        break;
                    }
                    case repo::lib::RepoDataType::UNSIGNEDLONG: {
                        t = boost::get<unsigned long>(*this);
                        break;
                    }
                    default: {
                        repoError << "Failed to convert RepoVariant type to base datatype.";
                        return false;
                    }
                }
                return true;
            }

            bool getStringData(std::string &str) {
                if(getVariantType()==repo::lib::RepoDataType::STRING){
                    str = boost::get<std::string>(*this);
                    return true;
                }
                return false;
            } 
        };
    }
}