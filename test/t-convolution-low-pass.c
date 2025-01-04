/*
 * Copyright (c) 2021-2025 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ufw/compiler.h>
#include <ufw/test/tap.h>

#include <ufw/convolution-low-pass.h>

CONV_LOW_PASS_API(clp, int)
CONV_LOW_PASS(clp, int)

CONV_LOW_PASS_MEDIAN_API(clp, int)
CONV_LOW_PASS_MEDIAN(clp, int)

#define T_SIZE 8

int
main(UNUSED int argc, UNUSED char *argv[])
{
    const int data[] = {11, 22, 33, 44, 55, 66, 77, 88};
    const int null_field[] = {0, 0, 0, 0, 0, 0, 0, 0};
    int temp[] = {0, 0, 0, 0, 0, 0, 0, 0};
    int buffer[T_SIZE];

    plan(39);

    clp w;
    clp_init(&w, buffer, T_SIZE);
    ok(memcmp(w.win, null_field, sizeof(null_field)) == 0, "init correct");
    ok(w.first == true, "not full");
    ok(clp_avg(&w) == 0, "empty average gives 0");
    ok(clp_median(&w, temp) == 0, "empty average gives 0");

    clp_update(&w, data[0]);

    ok(clp_has_min_values(&w, 1) == true, "one value");
    ok(clp_has_min_values(&w, 8) == false, "has not 8 values");
    ok(clp_has_min_values(&w, 100) == false, "has not 100 values");
    ok(w.first == true, "not full");

    ok(clp_avg(&w) == data[0], "single value is average");
    ok(clp_median(&w, temp) == data[0], "single value is median");

    for (int i = 1; i < T_SIZE - 1; ++i)
        clp_update(&w, data[i]);

    ok(clp_has_min_values(&w, 1) == true, "one value");
    ok(clp_has_min_values(&w, 7) == true, "has 7 values");
    ok(clp_has_min_values(&w, 8) == false, "has not 8 values");
    ok(clp_has_min_values(&w, 100) == false, "has not 100 values");
    ok(w.first == true, "not full");

    ok(clp_avg(&w) == 44, "average of seven is 44");
    ok(clp_median(&w, temp) == 44, "median is also 44");

    clp_update(&w, data[7]);
    ok(w.first == false, "not full");

    ok(clp_avg(&w) == 49, "average is 49.5, integer truncate is 49");
    ok(memcmp(w.win, data, sizeof(data)) == 0, "values stored correctly");

    ok(clp_median(&w, temp) == 49, "median is also 49.5 (49)");
    ok(memcmp(temp, data, sizeof(data)) == 0, "sorted values still sorted");

    clp_update(&w, data[T_SIZE - 1]);
    ok(w.first == false, "enter second iteration");

    for (int i = T_SIZE - 2; i >= 0; --i)
        clp_update(&w, data[i]);

    ok(clp_has_min_values(&w, 1) == true, "one value");
    ok(clp_has_min_values(&w, 8) == true, "has 8 values");
    ok(clp_has_min_values(&w, 100) == false, "has not 100 values");
    ok(w.first == false, "still beyond first iteration");

    ok(clp_avg(&w) == 49, "average is 49.5, integer truncate is 49");
    ok(memcmp(w.win, data, sizeof(data)) != 0, "values stored in other order");

    ok(clp_median(&w, temp) == 49, "median is also 49.5 (49)");
    ok(memcmp(temp, data, sizeof(data)) == 0, "values get sorted");

    for (int i = T_SIZE - 1; i >= 0; --i)
        clp_update(&w, data[i]);

    ok(clp_has_min_values(&w, 1) == true, "one value");
    ok(clp_has_min_values(&w, 8) == true, "has 8 values");
    ok(clp_has_min_values(&w, 100) == false, "has not 100 values");
    ok(w.first == false, "still beyond first iteration");

    ok(clp_avg(&w) == 49, "average is 49.5, integer truncate is 49");
    ok(memcmp(w.win, data, sizeof(data)) != 0, "values stored in other order");

    ok(clp_median(&w, temp) == 49, "median is also 49.5 (49)");
    ok(memcmp(temp, data, sizeof(data)) == 0, "values get sorted");

    return EXIT_SUCCESS;
}
