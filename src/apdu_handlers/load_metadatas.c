#include <string.h>

#include "io.h"

#include "error.h"
#include "globals.h"
#include "handlers.h"
#include "metadata.h"
#include "ui.h"

/* Image being restored. It only reaches NVM once complete and validated. */
static uint8_t restore_image[MAX_METADATAS];

void discard_metadata_restore(void) {
    explicit_bzero(restore_image, sizeof(restore_image));
    app_state.bytes_transferred = 0;
}

int load_metadatas(uint8_t p1, uint8_t p2, const buf_t *input) {
    if ((p1 != 0 && p1 != LAST_CHUNK) || p2 != 0) {
        discard_metadata_restore();
        return io_send_sw(SWO_INCORRECT_P1_P2);
    }
    if (app_state.user_approval == false) {
        discard_metadata_restore();
#ifdef SCREEN_SIZE_WALLET
        message_pair_t msg = {"Restore", "password list?"};
#else
        message_pair_t msg = {"Restore", "password list"};
#endif
        ui_request_user_approval(&msg);
        return 0;
    }

    if ((app_state.bytes_transferred > sizeof(restore_image)) ||
        (input->size > sizeof(restore_image) - app_state.bytes_transferred)) {
        discard_metadata_restore();
        return io_send_sw(SWO_WRONG_DATA_LENGTH);
    }
    memcpy(&restore_image[app_state.bytes_transferred], input->bytes, input->size);
    app_state.bytes_transferred += input->size;

    if (app_state.bytes_transferred >= sizeof(restore_image) || p1 == LAST_CHUNK) {
        // reset state
        app_state.user_approval = false;
        ui_idle();
        // Bytes past the delivered ones stay zero, so a short image ends on a terminator.
        const bool valid = (validate_metadata_image(restore_image) == OK) &&
                           (commit_metadata_image(restore_image) == OK);
        discard_metadata_restore();
        if (!valid) {
            return io_send_sw(SW_METADATAS_PARSING_ERROR);
        }
    }

    return io_send_sw(SWO_SUCCESS);
}
