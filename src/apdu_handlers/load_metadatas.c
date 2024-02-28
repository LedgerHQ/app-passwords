#include <io.h>

#include "error.h"
#include "globals.h"
#include "load_metadatas.h"
#include "metadata.h"
#include "password_ui_flows.h"

int load_metadatas(uint8_t p1, uint8_t p2, const buf_t *input) {
    if ((p1 != 0 && p1 != P1_LAST_CHUNK) || p2 != 0) {
        return io_send_sw(SW_WRONG_P1P2);
    }
    if (app_state.user_approval == false) {
        app_state.bytes_transferred = 0;
        message_pair_t msg = {"Overwrite", "metadatas ?"};
        ui_request_user_approval(&msg);
        return 0;
    }

    if (input->size > sizeof(N_storage.metadatas) - app_state.bytes_transferred) {
        return io_send_sw(SW_WRONG_DATA_LENGTH);
    }

    override_metadatas(app_state.bytes_transferred, (void *) input->bytes, input->size);
    app_state.bytes_transferred += input->size;

    if (app_state.bytes_transferred >= sizeof(N_storage.metadatas) || p1 == P1_LAST_CHUNK) {
        // reset state
        app_state.user_approval = false;
        ui_idle();
        if (compact_metadata()) {
            return io_send_sw(SW_METADATAS_PARSING_ERROR);
        }
    }

    return io_send_sw(SW_OK);
}
