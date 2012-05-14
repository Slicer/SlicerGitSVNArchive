#ifndef __StringContains_h
#define __StringContains_h
#include <string>

/** convert std::string to upper case in place */
inline char myUpper(char a) { return toupper(a); }

inline void strupper(std::string &s)
{
  std::transform(s.begin(),s.end(),s.begin(),myUpper);
}

inline bool
StringContains(const std::string &string,const std::string pattern)
{
  return string.find(pattern) != std::string::npos;
}


#endif // __StringContains_h
