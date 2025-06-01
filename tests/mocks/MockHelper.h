#ifndef MOCK_HELPER_H
#define MOCK_HELPER_H

#include "tests/mocks/IHelper.h"
#include <gmock/gmock.h>

namespace utils
{
namespace testing
{

class MockHelper : public IHelper
{
public:
    MOCK_METHOD(void, read_as_chars, (const std::vector<uint8_t>&, uint32_t, uint32_t, char*), (override));
    MOCK_METHOD(uint32_t, read_as_uint32_big, (const std::vector<uint8_t>&, uint32_t), (override));
    MOCK_METHOD(uint32_t, read_as_uint32_little, (const std::vector<uint8_t>&, uint32_t), (override));
    MOCK_METHOD(uint16_t, read_as_uint16, (const std::vector<uint8_t>&, uint32_t), (override));
    MOCK_METHOD(std::string, generate_output_file, (const std::string&, const std::string&), (override));
    MOCK_METHOD(void, log, (const std::function<void(const std::string&, const std::string&)>&, uint32_t, const std::string&), (override));
};

}  // namespace testing
}  // namespace utils

#endif  // MOCK_HELPER_H
