#include "poti.h"

int main(int argc, char ** argv) {
	poti_init(0);
	poti_loop();
	poti_deinit();
	return 0;
}
