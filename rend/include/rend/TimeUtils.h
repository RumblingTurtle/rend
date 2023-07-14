#pragma once
#include <chrono>

namespace rend::time {
typedef std::chrono::high_resolution_clock::time_point TimePoint;

typedef std::chrono::seconds Seconds;
typedef std::chrono::milliseconds Milliseconds;
typedef std::chrono::microseconds Microseconds;
typedef std::chrono::nanoseconds Nanoseconds;

inline TimePoint now() { return std::chrono::high_resolution_clock::now(); }

template <typename DurationType>
inline float time_difference(TimePoint t1, TimePoint t2) {
  return t1 < t2 ? std::chrono::duration_cast<DurationType>(t2 - t1).count()
                 : std::chrono::duration_cast<DurationType>(t1 - t2).count();
}

// Duration cast rounds off the seconds so here is a hacky solution
template <> inline float time_difference<Seconds>(TimePoint t1, TimePoint t2) {
  return time_difference<Milliseconds>(t1, t2) * 0.001f;
}

} // namespace rend::time
