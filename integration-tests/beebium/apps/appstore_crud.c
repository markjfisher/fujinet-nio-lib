#include "fujinet-nio.h"

#include <stdio.h>
#include <string.h>

#define NS "test.app"
#define EMPTY_NS "empty.app"

static uint8_t io_buf[160];
static uint8_t read_buf[48];
static uint8_t key_data[96];
static char key_name[16];
static fn_appstore_io_t io = { io_buf, sizeof(io_buf) };

static void fail_result(const char *op, uint8_t result)
{
    printf("%s FAIL R%u\n", op, (unsigned) result);
}

static void fail_text(const char *op, const char *text)
{
    printf("%s FAIL %s\n", op, text);
}

static uint8_t stat_expect(const char *key, uint8_t exists, uint16_t size)
{
    fn_appstore_stat_t stat;
    uint8_t result;

    result = fn_appstore_stat(&io, NS, key, &stat);
    if (result != FN_OK) {
        fail_result("STAT", result);
        return 0;
    }
    if (stat.exists != exists || (uint16_t) stat.size_bytes != size ||
        stat.size_bytes_high != 0) {
        fail_text("STAT", key);
        return 0;
    }
    return 1;
}

static uint8_t put_expect(const char *key, const char *value)
{
    fn_appstore_write_t wr;
    uint16_t len = (uint16_t) strlen(value);
    uint8_t result;

    result = fn_appstore_write(&io, NS, key, 0, (const uint8_t *) value, len, &wr);
    if (result != FN_OK) {
        fail_result("PUT", result);
        return 0;
    }
    if (wr.offset != 0 || wr.bytes_written != len) {
        fail_text("PUT", key);
        return 0;
    }
    return 1;
}

static uint8_t get_expect(const char *key, const char *value)
{
    fn_appstore_read_t rr;
    uint16_t len = (uint16_t) strlen(value);
    uint8_t result;

    memset(read_buf, 0xA5, sizeof(read_buf));
    result = fn_appstore_read(&io, NS, key, 0, read_buf, sizeof(read_buf) - 1, &rr);
    if (result != FN_OK) {
        fail_result("GET", result);
        return 0;
    }
    if (rr.offset != 0 || rr.bytes_read != len ||
        (rr.flags & (FN_APPSTORE_READ_EXISTS | FN_APPSTORE_READ_EOF)) !=
            (FN_APPSTORE_READ_EXISTS | FN_APPSTORE_READ_EOF)) {
        fail_text("GET", key);
        return 0;
    }
    read_buf[rr.bytes_read] = 0;
    if (strcmp((const char *) read_buf, value) != 0) {
        fail_text("VALUE", key);
        return 0;
    }
    return stat_expect(key, 1, len);
}

static uint8_t missing_read_expect(const char *key)
{
    fn_appstore_read_t rr;
    uint8_t result;

    result = fn_appstore_read(&io, NS, key, 0, read_buf, sizeof(read_buf), &rr);
    if (result != FN_OK) {
        fail_result("MISS", result);
        return 0;
    }
    if ((rr.flags & FN_APPSTORE_READ_EXISTS) != 0 ||
        (rr.flags & FN_APPSTORE_READ_EOF) == 0 || rr.bytes_read != 0) {
        fail_text("MISS", key);
        return 0;
    }
    return stat_expect(key, 0, 0);
}

static uint8_t list_expect(const char *namespace_name,
                           uint8_t expected_count,
                           const char *a,
                           const char *b,
                           const char *c)
{
    fn_appstore_list_t list;
    const char *expected;
    uint16_t off = 0;
    uint8_t i;
    uint8_t result;

    memset(key_data, 0xA5, sizeof(key_data));
    result = fn_appstore_list(&io, namespace_name, 0, key_data, sizeof(key_data), &list);
    if (result != FN_OK) {
        fail_result("LIST", result);
        return 0;
    }
    if (list.start_index != 0 || list.key_count != expected_count ||
        (list.flags & FN_APPSTORE_LIST_MORE) != 0) {
        fail_text("LIST", namespace_name);
        return 0;
    }

    for (i = 0; i < expected_count; ++i) {
        expected = (i == 0) ? a : ((i == 1) ? b : c);
        result = fn_appstore_list_next_key(key_data, list.key_data_len, &off,
                                           key_name, sizeof(key_name));
        if (result != FN_OK || strcmp(key_name, expected) != 0) {
            fail_text("KEY", expected);
            return 0;
        }
    }
    if (off != list.key_data_len) {
        fail_text("KEYLEN", namespace_name);
        return 0;
    }
    return 1;
}

static uint8_t delete_expect(const char *key, uint8_t expected_deleted)
{
    fn_appstore_delete_t del;
    uint8_t result;

    result = fn_appstore_delete(&io, NS, key, &del);
    if (result != FN_OK) {
        fail_result("DEL", result);
        return 0;
    }
    if (del.deleted != expected_deleted) {
        fail_text("DEL", key);
        return 0;
    }
    return 1;
}

int main(void)
{
    puts("ASTORE START");

    if (!list_expect(EMPTY_NS, 0, "", "", "")) {
        return 1;
    }
    puts("EMPTY OK");

    if (!put_expect("alpha", "one") ||
        !put_expect("beta", "two") ||
        !put_expect("gamma", "three")) {
        return 1;
    }
    puts("PUT3 OK");

    if (!list_expect(NS, 3, "alpha", "beta", "gamma")) {
        return 1;
    }
    puts("LIST3 OK");

    if (!get_expect("alpha", "one") ||
        !get_expect("beta", "two") ||
        !get_expect("gamma", "three")) {
        return 1;
    }
    puts("GET3 OK");

    if (!put_expect("beta", "two-up")) {
        return 1;
    }
    puts("UPD beta two-up");

    if (!get_expect("alpha", "one") ||
        !get_expect("beta", "two-up") ||
        !get_expect("gamma", "three")) {
        return 1;
    }
    puts("POSTUP OK");

    if (!delete_expect("gamma", 1) ||
        !missing_read_expect("gamma")) {
        return 1;
    }
    puts("DEL gamma OK");

    if (!list_expect(NS, 2, "alpha", "beta", "")) {
        return 1;
    }
    puts("LIST2 OK");

    if (!get_expect("alpha", "one") ||
        !get_expect("beta", "two-up")) {
        return 1;
    }
    puts("LEFT alpha one");
    puts("LEFT beta two-up");

    if (!delete_expect("missing", 0)) {
        return 1;
    }
    puts("DELMISS OK");

    puts("ASTORE OK");
    return 0;
}
