/*
 * Copyright (c) 2026 orbisai0security <mediratta01.pally@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <check.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>

/* Forward declaration of the actual production function */
extern void CLI_processInput(uint8_t *inputBuffer, size_t bufferSize, size_t bufferCapacity);

START_TEST(test_buffer_reads_never_exceed_declared_length)
{
    /* Invariant: Buffer reads never exceed the declared length */
    const char *payloads[] = {
        "valid",                    /* Valid input */
        "A",                        /* Boundary: single char */
        "AAAAAAAAAAAAAAAAAAAAAAAA"  /* Exploit: 24 chars, exceeds typical buffer */
    };
    int num_payloads = sizeof(payloads) / sizeof(payloads[0]);

    for (int i = 0; i < num_payloads; i++) {
        /* Allocate buffer with guard pages before and after */
        const size_t buffer_capacity = 16; /* Simulate typical buffer size */
        const size_t guard_size = 8;
        uint8_t *full_buffer = malloc(buffer_capacity + 2 * guard_size);
        ck_assert_ptr_nonnull(full_buffer);
        
        uint8_t *inputBuffer = full_buffer + guard_size;
        
        /* Fill guard regions with sentinel values */
        memset(full_buffer, 0xAA, guard_size);
        memset(inputBuffer + buffer_capacity, 0xBB, guard_size);
        
        /* Copy payload into buffer, truncating if necessary */
        size_t payload_len = strlen(payloads[i]);
        size_t copy_len = payload_len < buffer_capacity ? payload_len : buffer_capacity;
        memcpy(inputBuffer, payloads[i], copy_len);
        
        /* Call the actual production function */
        CLI_processInput(inputBuffer, copy_len, buffer_capacity);
        
        /* Verify guard regions remain unchanged */
        for (size_t j = 0; j < guard_size; j++) {
            ck_assert_uint_eq(full_buffer[j], 0xAA);
            ck_assert_uint_eq(inputBuffer[buffer_capacity + j], 0xBB);
        }
        
        free(full_buffer);
    }
}
END_TEST

Suite *security_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("Security");
    tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_buffer_reads_never_exceed_declared_length);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = security_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}