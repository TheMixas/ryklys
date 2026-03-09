#ifndef RYKLYS_BACKEND_ENV_H
#define RYKLYS_BACKEND_ENV_H

#include <cstdlib>
#include <string>

inline std::string getEnvOrDefault(const char *name, const char *defaultValue) {
    const char *value = std::getenv(name);
    return (value != nullptr) ? value : defaultValue;
}

#endif // RYKLYS_BACKEND_ENV_H
