#ifndef TYPES_H_
#define TYPES_H_

#include <Hevc.h>
#include <map>
#include <memory>

typedef std::map<uint32_t, std::shared_ptr<HEVC::VPS>> VPSMap;
typedef std::map<uint32_t, std::shared_ptr<HEVC::SPS>> SPSMap;
typedef std::map<uint32_t, std::shared_ptr<HEVC::PPS>> PPSMap;

typedef HEVC::Parser::Info ParserInfo;

#endif
