#include "dump_metadatas.h"
#include "error.h"
#include "globals.h"
#include "io.h"
#include "password_ui_flows.h"


int dump_metadatas() {
    if (app_state.user_approval == false) {
        app_state.bytes_transferred = 0;
        message_pair_t msg = {"Transfer", "metadatas ?"};
        ui_request_user_approval(&msg);
        return 0;
    }

    size_t remaining_bytes_count = sizeof(N_storage.metadatas) - app_state.bytes_transferred;
    size_t payload_size;

    if (remaining_bytes_count < MAX_PAYLOAD_SIZE) {
        app_state.user_approval = false;
        payload_size = remaining_bytes_count;
        G_io_apdu_buffer[TRANSFER_FLAG_OFFSET] = LAST_CHUNK;
        ui_idle();
    } else {
        payload_size = MAX_PAYLOAD_SIZE;
        G_io_apdu_buffer[TRANSFER_FLAG_OFFSET] = MORE_DATA_INCOMING;
    }

    memcpy(&G_io_apdu_buffer[TRANSFER_PAYLOAD_OFFSET],
           (const void*) N_storage.metadatas + app_state.bytes_transferred,
           payload_size);

    app_state.bytes_transferred += payload_size;

    return io_send_response_pointer(G_io_apdu_buffer,
                                    payload_size + TRANSFER_PAYLOAD_OFFSET,
                                    SW_OK);
}
