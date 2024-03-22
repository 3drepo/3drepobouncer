#pragma once
#include <iostream>
#include <boost/variant.hpp>
#include <string>
#include "../../repo_bouncer_global.h"
#include "repo/lib/repo_log.h"


/*typedef signed char OdInt8;
typedef short OdInt16;
typedef unsigned char OdUInt8;
typedef unsigned short OdUInt16;*/

#if   ULONG_MAX == 0xFFFFFFFFUL
#define OD_SIZEOF_LONG  4
#elif (ULONG_MAX > 0xFFFFFFFFU && ULONG_MAX == 0xFFFFFFFFFFFFFFFFU) || (defined(sparc) && defined(_LP64))
#define OD_SIZEOF_LONG  8
#else
#error "Unsupported number of *bytes* in `long' type!"
#endif

#if OD_SIZEOF_LONG == 4
typedef long OdInt32;
typedef unsigned long OdUInt32;
#else // assumes 4-byte int type
typedef int OdInt32;
typedef unsigned int OdUInt32;
#endif

namespace repo {
    namespace lib {
        using boostVariantType = boost::variant<int, double, std::string, bool, uint64_t, float, long,long long,OdUInt32>;
        enum RepoDataType { INT, DOUBLE,STRING, BOOL,UINT64, FLOAT,LONG,LONGLONG,ODUINT32,OTHER };
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

            RepoVariant(const long long& data) : boostVariantType(data){};

            RepoVariant(const OdUInt32& data) : boostVariantType(data){};

            repo::lib::RepoDataType getVariantType() {
                const std::vector<repo::lib::RepoDataType> mapping = {repo::lib::RepoDataType::INT,
                                                                      repo::lib::RepoDataType::DOUBLE,
                                                                      repo::lib::RepoDataType::STRING,
                                                                      repo::lib::RepoDataType::BOOL,
                                                                      repo::lib::RepoDataType::UINT64,
                                                                      repo::lib::RepoDataType::FLOAT,
                                                                      repo::lib::RepoDataType::LONG,
                                                                      repo::lib::RepoDataType::LONGLONG,
                                                                      repo::lib::RepoDataType::ODUINT32,
                                                                      repo::lib::RepoDataType::OTHER};
                auto typeIdx = which();
                return ((typeIdx > mapping.size())? repo::lib::RepoDataType::OTHER : mapping[typeIdx]);
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
                    case repo::lib::RepoDataType::LONGLONG: {
                        t = boost::get<long long>(*this);
                        break;
                    }
                    case repo::lib::RepoDataType::ODUINT32: {
                        t = boost::get<OdUInt32>(*this);
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