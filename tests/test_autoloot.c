#include <stdio.h>

extern int test_autoloot_wrapper(int filter_result);

int main(void) {
    if (!test_autoloot_wrapper(1) || !test_autoloot_wrapper(-1)) {
        fputs("Autoloot hook corrupted arguments or returned through the wrong branch.\n", stderr);
        return 1;
    }
    return 0;
}
