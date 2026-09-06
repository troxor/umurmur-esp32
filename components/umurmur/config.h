#ifndef CONFIG_H
#define CONFIG_H

#define USE_MBEDTLS
/* SSLIMP_VERSION shown in Server_run banner */
#define SSLIMP_VERSION "mbedTLS-ESP-IDF"
/* Upstream sets this in CMakeLists (v0.4.1 → "Loopy"); ESP skips that generate step. */
#define UMURMUR_CODENAME "Loopy"

#endif /* CONFIG_H */
