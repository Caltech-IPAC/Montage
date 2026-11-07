/*
 * Look up keys with json_val when the layout string is short.
 * json_val copies the key and its tail into buffers, so a key that is as
 * long as the layout, or longer, must not write past those buffers.
 * Built with AddressSanitizer so any bad read or write aborts the run.
 */
#include <stdio.h>
#include <string.h>

#include "json.h"

static int checks;
static int failures;

static void check(int ok, const char *name)
{
    ++checks;
    if(!ok)
    {
        ++failures;
        printf("FAIL %s\n", name);
    }
}

static void expect_missing(const char *layout, const char *key)
{
    char val[256];
    char name[256];

    snprintf(name, sizeof(name), "layout [%s] key [%s] is not found", layout, key);
    check(json_val(layout, key, val) == NULL, name);
}

static void expect_value(const char *layout, const char *key, const char *expected)
{
    char val[256];
    char name[256];
    char *result;

    snprintf(name, sizeof(name), "layout [%s] key [%s] is [%s]", layout, key, expected);
    result = json_val(layout, key, val);
    check(result != NULL && strcmp(result, expected) == 0, name);
}

int main(void)
{
    /* The padding key makes this nested layout longer than the keys. */
    const char *nested =
        "{\"background\": {}, \"pad\": \"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\"}";
    const char *found =
        "{\"background\": {\"noise\": 1.5, \"bg11\": 2}, \"count\": 7, "
        "\"pad\": \"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\"}";

    /* Layouts shorter than the key. */
    expect_missing("{}", "background.noise");
    expect_missing("{\"a\": 1}", "background.noise");

    /* Layout and key of the same length (10 characters). */
    expect_missing("{\"abcd\":1}", "abcdefghij");

    /* The lookup recurses into an empty sub-structure that is shorter than the key. */
    expect_missing(nested, "background.noise");

    /* Keys with nothing before the first separator. */
    expect_missing("{\"a\": 1}", "");
    expect_missing("{\"a\": 1}", ".a");
    expect_missing("{\"a\": 1}", "[0]");

    /* Keys that are present still return their values. */
    expect_value(found, "background.noise", "1.5");
    expect_value(found, "background.bg11", "2");
    expect_value(found, "count", "7");

    printf("%d checks, %d failed\n", checks, failures);

    return failures != 0;
}
