#ifndef POTI_H
#define POTI_H

#define POTI_VER "0.1.0"
#define POTI_API

#define POTI_NUM f32

POTI_API int poti_init(int flags);
POTI_API int poti_quit();

POTI_API int poti_loop();

#endif /* POTI_H */
