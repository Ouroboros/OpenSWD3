#pragma once

#include <map>
#include <string>

#include "core/types.h"

namespace swd3test
{

// The golden hashes of a results file: a scenario name (which may hold spaces), a space and its hash on each line.
std::map<std::string, std::string> LoadGolden(const std::string& path);

// Compares the hashes of the scenarios run with the golden ones (a report callback of the golden runners), printing the
// first mismatches.
class GoldenCheck
{
public:
    explicit GoldenCheck(const std::map<std::string, std::string>& golden);

    void operator()(const std::string& name, swd3::u64 hash);

    int compared = 0;
    int mismatches = 0;

private:
    const std::map<std::string, std::string>& golden;
};

}  // namespace swd3test
