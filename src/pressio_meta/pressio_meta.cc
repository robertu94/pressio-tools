#include "libpressio_meta.h"
#include "pressio_tools_version.h"

#if LIBPRESSIO_TOOLS_HAS_ERROR_INJECTOR
#include <libpressio_error_injector.h>
#endif

#if LIBPRESSIO_TOOLS_HAS_OPT
#include <libpressio_opt.h>
#endif

#if LIBPRESSIO_TOOLS_HAS_TTHRESH
#include <libpressio_tthresh.h>
#endif

#if LIBPRESSIO_TOOLS_HAS_RMETRIC
#include <libpressio_r_metric.h>
#endif

#if LIBPRESSIO_TOOLS_HAS_NVCOMP
#include <libpressio_nvcomp_version.h>
#endif

#if LIBPRESSIO_TOOLS_HAS_ADIOS2
#include <libpressio_adios2.h>
#endif

#if LIBPRESSIO_TOOLS_HAS_ADIOS1
#include <libpressio_adios1.h>
#endif

#if LIBPRESSIO_TOOLS_HAS_FRSZ
#include <frsz.h>
#endif

#if LIBPRESSIO_TOOLS_HAS_SPERR
#include <libpressio-sperr.h>
#endif

#if LIBPRESSIO_TOOLS_HAS_LC
#include <lc_libpressio.h>
#endif

#if LIBPRESSIO_TOOLS_HAS_HLRCOMPRESS
#include <libpressio-hlrcompress.h>
#endif

#if LIBPRESSIO_TOOLS_HAS_DCTZ
#include <libpressio_dctz.h>
#endif
/**
 * some linux distributions (i.e. Ubuntu) delay shared library loading
 * until the library is referenced.  This library exists to reference all of the 
 * first party libraries that we for one reason or another cannot include in the main
 * libpressio repository
 */
extern "C" void libpressio_register_all() {
#if LIBPRESSIO_TOOLS_HAS_ERROR_INJECTOR
  libpressio_register_error_injector();
#endif

#if LIBPRESSIO_TOOLS_HAS_OPT
  libpressio_register_libpressio_opt();
#endif

#if LIBPRESSIO_TOOLS_HAS_TTHRESH
  libpressio_register_tthresh();
#endif

#if LIBPRESSIO_TOOLS_HAS_RMETRIC
  libpressio_register_r_metric();
#endif

#if LIBPRESSIO_TOOLS_HAS_NVCOMP
  libpressio_register_nvcomp();
#endif


#if LIBPRESSIO_TOOLS_HAS_ADIOS2
  libpressio_register_adios2();
#endif

#if LIBPRESSIO_TOOLS_HAS_ADIOS1
  libpressio_register_adios1();
#endif

#if LIBPRESSIO_TOOLS_HAS_FRSZ
  register_frsz();
#endif

#if LIBPRESSIO_TOOLS_HAS_SPERR
  register_libpressio_sperr();
#endif

#if LIBPRESSIO_TOOLS_HAS_LC
  libpressio_register_lc();
#endif

#if LIBPRESSIO_TOOLS_HAS_HLRCOMPRESS
  register_libpressio_hlrcompress();
#endif

#if LIBPRESSIO_TOOLS_HAS_DCTZ
  dctz_libpressio_register_all();
#endif
};
