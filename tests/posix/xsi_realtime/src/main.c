/*
 * Copyright (c) 2025 Marvin Ouma <pancakesdeath@protonmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

extern void before(void *arg);
extern void after(void *arg);

ZTEST_SUITE(xsi_realtime, NULL, NULL, before, NULL, after);
