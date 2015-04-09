#ifndef _UTILS_H_
# define _UTILS_H_

#define ERR(fmt, ...) fprintf(stderr, "Error:%s:%d: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define WAR(fmt, ...) fprintf(stderr, "Warning:%s:%d: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define INF(fmt, ...) fprintf(stdout, "%s:%d: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)

#endif /* !_UTILS_H_ */

