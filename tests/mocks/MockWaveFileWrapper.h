#ifndef MOCK_WAVE_FILE_WRAPPER_H
#define MOCK_WAVE_FILE_WRAPPER_H

#include "tests/mocks/IWaveFileWrapper.h"
#include <gmock/gmock.h>

namespace utils
{
namespace testing
{

class MockWaveFileWrapper : public IWaveFileWrapper
{
public:
    MOCK_METHOD(bool, is_valid, (), (const, override));
    MOCK_METHOD(bool, get_wave_data, (WaveHeader&, int16_t*&, int16_t*&), (const, override));
};

}  // namespace testing
}  // namespace utils

#endif  // MOCK_WAVE_FILE_WRAPPER_H
