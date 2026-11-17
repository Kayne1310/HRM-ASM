#ifndef INPUT_H
#define INPUT_H
#include "models.h"
int readLine(FILE *stream, char *buffer, size_t size);
/* 1 OK, 0 EOF, -1 too long */ int parseInteger(const char *text, long long min, long long max,
                                                long long *value);
int parseHalfHours(const char *text, int maxUnits, int *value);
int askText(const char *prompt, char *output, size_t size, int allowEmpty);
int askInteger(const char *prompt, long long min, long long max, long long *value);
int askDate(const char *prompt, Date *date);
int askHours(const char *prompt, int minUnits, int maxUnits, int *value);
int confirm(const char *prompt);
int inputEnded(void);
#endif
