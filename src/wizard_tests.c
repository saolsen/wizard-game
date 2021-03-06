#define WIZARD_TESTING

#include "greatest.h"

#include "wizard.h"
#include "wizard_network.h"
#include "wizard_message.h"
#include "wizard_simulation.h"

#include <stdlib.h>
#include <stdio.h>

TEST test_a_good_thing(void) {
    ASSERT(1);
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
    RUN_SUITE(wizard_network_tests);
    RUN_SUITE(wizard_message_tests);
    RUN_SUITE(wizard_simulation_tests);
    GREATEST_MAIN_END();
}
