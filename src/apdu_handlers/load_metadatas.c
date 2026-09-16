#include "io.h"

#include "error.h"
#include "globals.h"
#include "handlers.h"
#include "metadata.h"
#include "ui.h"

/* Drop a database that a transfer already started rewriting. Called on the error paths so a
 * broken restore never leaves a mix of the old and the new image in NVM. */
static void abort_started_restore(void) {
    if (metadata_restore_in_progress()) {
        abort_metadata_restore();
        app_state.bytes_transferred = 0;
    }
}

int load_metadatas(uint8_t p1, uint8_t p2, const buf_t *input) {
    if ((p1 != 0 && p1 != LAST_CHUNK) || p2 != 0) {
        abort_started_restore();
        return io_send_sw(SWO_INCORRECT_P1_P2);
    }
    if (app_state.user_approval == false) {
        app_state.bytes_transferred = 0;
#ifdef SCREEN_SIZE_WALLET
        message_pair_t msg = {"Restore", "password list?"};
#else
        message_pair_t msg = {"Restore", "password list"};
#endif
        ui_request_user_approval(&msg);
        return 0;
    }

    if (input->size > sizeof(N_storage.metadatas) - app_state.bytes_transferred) {
        abort_started_restore();
        return io_send_sw(SWO_WRONG_DATA_LENGTH);
    }

    if (app_state.bytes_transferred == 0) {
        // Open the transaction before the first byte lands, so an interrupted transfer is
        // caught at the next startup instead of surviving as a corrupted database.
        begin_metadata_restore();
    }

    // Backstop: override_metadatas() validates the destination itself, so a transfer offset
    // that ever escaped the length check above cannot become an out-of-bounds NVM write.
    if (override_metadatas(app_state.bytes_transferred, (void *) input->bytes, input->size) != OK) {
        abort_started_restore();
        return io_send_sw(SWO_WRONG_DATA_LENGTH);
    }
    app_state.bytes_transferred += input->size;

    if (app_state.bytes_transferred >= sizeof(N_storage.metadatas) || p1 == LAST_CHUNK) {
        /* The host may stop short of the full region. Drop whatever the previous database left
         * past the delivered bytes before parsing: an image ending on a record boundary with
         * no terminator would otherwise run into the old records, and the parser would accept
         * the resulting old/new mix as a complete database. */
        clear_metadatas_from(app_state.bytes_transferred);
        // reset state
        app_state.user_approval = false;
        ui_idle();
        if (compact_metadata() != OK) {
            abort_metadata_restore();
            app_state.bytes_transferred = 0;
            return io_send_sw(SW_METADATAS_PARSING_ERROR);
        }
        end_metadata_restore();
    }

    return io_send_sw(SWO_SUCCESS);
}
