#include "error.h"
#include "globals.h"
#include "handlers.h"
#include "io.h"
#include "metadata.h"
#include "ui.h"

int dump_metadatas() {
    if (app_state.user_approval == false) {
        app_state.bytes_transferred = 0;
#ifdef SCREEN_SIZE_WALLET
        message_pair_t msg = {"Backup", "password list?"};
#else
        message_pair_t msg = {"Backup", "password list"};
#endif
        ui_request_user_approval(&msg);
        return 0;
    }

    size_t remaining_bytes_count = sizeof(N_storage.metadatas) - app_state.bytes_transferred;
    size_t payload_size;
    int status = 0;

    if (remaining_bytes_count < MAX_PAYLOAD_SIZE) {
        app_state.user_approval = false;
        payload_size = remaining_bytes_count;
        G_io_apdu_buffer[TRANSFER_FLAG_OFFSET] = LAST_CHUNK;
        ui_idle();
    } else {
        payload_size = MAX_PAYLOAD_SIZE;
        G_io_apdu_buffer[TRANSFER_FLAG_OFFSET] = MORE_DATA_INCOMING;
    }

    /* Only the bytes up to the logical end of the database are meaningful. Past it, the flash
     * still holds nicknames from deleted entries and from earlier, larger databases, so send
     * zeroes instead of the raw slack space. The two terminator bytes are zero as well, so the
     * exported stream is unchanged for a database that has no slack. */
    size_t live_size = find_free_metadata();
    if (live_size > sizeof(N_storage.metadatas)) {
        live_size = sizeof(N_storage.metadatas);
    }
    size_t live_bytes = 0;
    if (app_state.bytes_transferred < live_size) {
        live_bytes = live_size - app_state.bytes_transferred;
        if (live_bytes > payload_size) {
            live_bytes = payload_size;
        }
    }

    memcpy(&G_io_apdu_buffer[TRANSFER_PAYLOAD_OFFSET],
           (const unsigned char *) N_storage.metadatas + app_state.bytes_transferred,
           live_bytes);
    memset(&G_io_apdu_buffer[TRANSFER_PAYLOAD_OFFSET + live_bytes], 0, payload_size - live_bytes);

    app_state.bytes_transferred += payload_size;

    status = io_send_response_pointer(G_io_apdu_buffer,
                                      payload_size + TRANSFER_PAYLOAD_OFFSET,
                                      SWO_SUCCESS);
    if (status > 0) {
        // API_LEVEL >= 24 status can be positive (response length) / negative (error)
        // API_LEVEL  < 24 status is 0 / -1
        status = 0;
    }
    return status;
}
