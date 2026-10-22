#include <stdint.h>
#include <string.h>

#include "unity.h"

#include "handlers.h"
#include "hid_mapping.h"
#include "metadata.h"
#include "types.h"
#include "fake_vars.h"
#include "Mockio.h"
#include "Mockui.h"

static uint8_t response[IO_APDU_BUFFER_SIZE];
static size_t response_len;
static uint16_t last_sw;
static size_t idle_calls;

static int fake_io_send_response_buffers(const buffer_t *rdatalist,
                                         size_t count,
                                         uint16_t sw,
                                         int cmock_num_calls) {
    (void) cmock_num_calls;
    last_sw = sw;
    if (count == 0) {
        return 0;
    }
    TEST_ASSERT_EQUAL(1, count);
    TEST_ASSERT_TRUE(rdatalist[0].size <= sizeof(response));
    memcpy(response, rdatalist[0].ptr, rdatalist[0].size);
    response_len = rdatalist[0].size;
    return (int) response_len;
}

static void fake_ui_idle(int cmock_num_calls) {
    (void) cmock_num_calls;
    idle_calls++;
}

static int get_config(uint8_t p1, uint8_t p2) {
    const buf_t input = {.bytes = NULL, .size = 0};
    return get_app_config(p1, p2, &input);
}

void setUp(void) {
    memset(&N_storage_rw, 0, sizeof(N_storage_rw));
    memset(response, 0, sizeof(response));
    response_len = 0;
    last_sw = 0;
    idle_calls = 0;
    Mockio_Init();
    Mockui_Init();
    io_send_response_buffers_StubWithCallback(fake_io_send_response_buffers);
    ui_idle_StubWithCallback(fake_ui_idle);
}

void tearDown(void) {
    Mockio_Verify();
    Mockui_Verify();
    Mockio_Destroy();
    Mockui_Destroy();
}

// Storage size on 4 bytes, keyboard layout, Enter option, then generation version
static void test_get_app_config(void) {
    N_storage_rw.keyboard_layout = HID_MAPPING_AZERTY;
    N_storage_rw.press_enter_after_typing = true;
    const uint8_t expected[] =
        {0x00, 0x00, 0x10, 0x00, HID_MAPPING_AZERTY, 0x01, META_CURRENT_VERSION};

    TEST_ASSERT_EQUAL(0, get_config(0, 0));
    TEST_ASSERT_EQUAL_HEX16(SWO_SUCCESS, last_sw);
    TEST_ASSERT_EQUAL(sizeof(expected), response_len);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(expected, response, sizeof(expected));
    TEST_ASSERT_EQUAL(1, idle_calls);
}

static void test_get_app_config_default_settings(void) {
    const uint8_t expected[] =
        {0x00, 0x00, 0x10, 0x00, HID_MAPPING_NONE, 0x00, META_CURRENT_VERSION};

    TEST_ASSERT_EQUAL(0, get_config(0, 0));
    TEST_ASSERT_EQUAL_HEX8_ARRAY(expected, response, sizeof(expected));
}

static void test_get_app_config_rejects_p1_p2(void) {
    TEST_ASSERT_EQUAL(0, get_config(1, 0));
    TEST_ASSERT_EQUAL_HEX16(SWO_INCORRECT_P1_P2, last_sw);
    TEST_ASSERT_EQUAL(0, get_config(0, 1));
    TEST_ASSERT_EQUAL_HEX16(SWO_INCORRECT_P1_P2, last_sw);
    TEST_ASSERT_EQUAL(0, response_len);
    TEST_ASSERT_EQUAL(0, idle_calls);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_get_app_config);
    RUN_TEST(test_get_app_config_default_settings);
    RUN_TEST(test_get_app_config_rejects_p1_p2);
    return UNITY_END();
}
