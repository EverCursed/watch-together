#ifndef ERRORS_H
#define ERRORS_H

#define SUCCESS 0

#define UNKNOWN_ERROR -1
#define NO_MEMORY -2
#define NOT_ENOUGH_DATA -3
#define UNKNOWN_VIDEO_FORMAT -4

#define SUCCEEDED(a) (a >= SUCCESS)
#define ERROR(a) (a < SUCCESS)

#endif
