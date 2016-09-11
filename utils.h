#ifndef BUNKERBUILDER_UTILS_H
#define BUNKERBUILDER_UTILS_H

namespace bb {
template<class T>
T div_floor(T x, T y) {
  T q = x / y;
  T r = x % y;
  if ((r != 0) && ((r < 0) != (y < 0))) --q;
  return q;
}

template<class T>
T clamp(T value, T min, T max) {
  return value < min ? min : value > max ? max : value;
}

template<class T>
T limit_abs(T value, T limit) {
  if (value < -limit)
    return -limit;
  if (value > limit)
    return limit;
  return value;
}

string format(string fmt, ...) {
  int size = ((int) fmt.size()) * 2 + 50;       // Use a rubric appropriate for
  // your code
  std::string str;
  va_list ap;
  while (1) {                   // Maximum two passes on a POSIX system...
    str.resize(size);
    va_start(ap, fmt);
    int n = vsnprintf((char *) str.data(), size, fmt.c_str(), ap);
    va_end(ap);
    if (n > -1 && n < size) {   // Everything worked
      str.resize(n);
      return str;
    }
    if (n > -1)                 // Needed size returned
      size = n + 1;             // For null char
    else
      size *= 2;                // Guess at a larger size (OS specific)
  }
}

template<class T>
struct Event {
  vector<function<void(T *t)>> handlers;

  void run(T *t) {
    for (auto &handler : handlers) {
      handler(t);
    }
  }
};

struct EnumClassHash {
  template<typename T>
  std::size_t operator()(T t) const {
    return static_cast<std::size_t>(t);
  }
};
}

#endif //BUNKERBUILDER_UTILS_H