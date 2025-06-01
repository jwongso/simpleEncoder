#ifndef I_HELPER_H
#define I_HELPER_H

#include <stdint.h>
#include <vector>
#include <string>
#include <functional>

namespace utils
{

class IHelper
{
public:
    virtual ~IHelper() = default;

    virtual void read_as_chars(const std::vector<uint8_t>& input,
                              uint32_t pos,
                              uint32_t length,
                              char* target) = 0;

    virtual uint32_t read_as_uint32_big(const std::vector<uint8_t>& input, uint32_t pos) = 0;

    virtual uint32_t read_as_uint32_little(const std::vector<uint8_t>& input, uint32_t pos) = 0;

    virtual uint16_t read_as_uint16(const std::vector<uint8_t>& input, uint32_t pos) = 0;

    virtual std::string generate_output_file(const std::string& input_file,
                                           const std::string& extension) = 0;

    virtual void log(const std::function<void(const std::string&, const std::string&)>& callback,
                    uint32_t id,
                    const std::string& status) = 0;
};

} // utils

#endif // I_HELPER_H
