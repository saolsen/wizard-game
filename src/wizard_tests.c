// Tests shit
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <stdbool.h>

#include "greatest.h"

TEST test_a_good_thing(void) {
    ASSERT(true);
    PASS();
}

TEST test_a_bad_thing(void) {
    ASSERT(1);
    PASS();
}

SUITE(wizard_tests) {
    RUN_TEST(test_a_good_thing);
    RUN_TEST(test_a_bad_thing);
}

GREATEST_MAIN_DEFS();

int main(int argc, char **argv) {
    GREATEST_MAIN_BEGIN();
    RUN_SUITE(wizard_tests);
    GREATEST_MAIN_END();
}