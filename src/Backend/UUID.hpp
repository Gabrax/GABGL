#pragma once
#include <cstdint>
#include <cstddef>
#include <random>

static std::random_device s_RandomDevice;
static std::mt19937_64 s_Engine(s_RandomDevice());
static std::uniform_int_distribution<uint64_t> s_UniformDistribution;

struct UUID
{
  UUID() : m_UUID(s_UniformDistribution(s_Engine)){}

  explicit UUID(const uint64_t uuid) : m_UUID(uuid){}
	UUID(const UUID&) = default;

  explicit operator uint64_t() const { return m_UUID; }
private:
	uint64_t m_UUID;
};

namespace std
{
	template <typename T> struct hash;

	template<>
	struct hash<UUID>
	{
		std::size_t operator()(const UUID& uuid) const noexcept {
			return static_cast<uint64_t>(uuid);
		}
	};
}
