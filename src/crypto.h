#ifndef CRYPTO_H
#define CRYPTO_H

// Computes SHA-256 hash of input string, outputs 64-char hex string + null terminator
void sha256(const char *input, char *output);

#endif
