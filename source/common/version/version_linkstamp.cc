#ifdef ENVOY_MANUAL_STAMP
#include "common/version/version_linkstamp.h"
#endif

// NOLINT(namespace-envoy)
extern const char build_scm_revision[];
extern const char build_scm_status[];

const char build_scm_revision[] = BUILD_SCM_REVISION;
const char build_scm_status[] = BUILD_SCM_STATUS;
