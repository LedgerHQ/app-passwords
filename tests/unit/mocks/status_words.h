#pragma once

// Pulled in transitively by error.h (-> metadata.h, password.h). The
// production header defines SWO_* status word constants which are never
// referenced by the code we build for unit tests, so an empty mock is
// sufficient to satisfy the #include directive.
