#pragma once

// On device, os_nvm.h is what declares nvm_write. The unit-test mocks already provide it
// through os.h, so this header only needs to exist for the sources that include it directly.
#include "os.h"
