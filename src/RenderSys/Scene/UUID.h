#pragma once

#include <cstddef>
#include <cstdint>
//#include <xhash>

namespace RenderSys
{

class UUID
{
public:
    UUID();
    UUID(uint64_t uuid);
    UUID(const UUID&) = default;
    ~UUID() = default;

    operator uint64_t() const { return m_UUID; }

private:
    uint64_t m_UUID;
};

} // namespace RenderSys

// namespace std 
// {

// template<>
// struct hash<RenderSys::UUID>
// {
//     std::size_t operator()(const RenderSys::UUID& uuid) const
//     {
//         return hash<uint64_t>()((uint64_t)uuid);
//     }
// };
    
// }