/* 
 * Copyright (c) 2015-2017, Gregory M. Kurtzer. All rights reserved.
 * 
 * Copyright (c) 2016-2017, The Regents of the University of California,
 * through Lawrence Berkeley National Laboratory (subject to receipt of any
 * required approvals from the U.S. Dept. of Energy).  All rights reserved.
 * 
 * This software is licensed under a customized 3-clause BSD license.  Please
 * consult LICENSE file distributed with the sources of this project regarding
 * your rights to use or distribute this software.
 * 
 * NOTICE.  This Software was developed under funding from the U.S. Department of
 * Energy and the U.S. Government consequently retains certain rights. As such,
 * the U.S. Government has been granted for itself and others acting on its
 * behalf a paid-up, nonexclusive, irrevocable, worldwide license in the Software
 * to reproduce, distribute copies to the public, prepare derivative works, and
 * perform publicly and display publicly, and to permit other to do so. 
 * 
 */


#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>

#include "config.h"
#include "util/file.h"
#include "util/fork.h"
#include "util/util.h"
#include "util/registry.h"
#include "util/config_parser.h"
#include "util/privilege.h"
#include "lib/image/image.h"
#include "lib/runtime/runtime.h"

#ifndef SYSCONFDIR
#error SYSCONFDIR not defined
#endif

#ifndef LIBEXECDIR
#error LIBEXECDIR is not defined
#endif


int main(int argc, char **argv) {
    struct image_object image;

    singularity_config_init(joinpath(SYSCONFDIR, "/singularity/singularity.conf"));
    singularity_registry_init();
    singularity_priv_init();

    singularity_registry_set("WRITABLE", "1");

    image = singularity_image_init(singularity_registry_get("IMAGE"));

    singularity_image_open(&image, O_RDWR);

    singularity_runtime_tmpdir(singularity_image_sessiondir(&image));
    singularity_runtime_ns(SR_NS_MNT);

    singularity_image_bind(&image);
    singularity_image_mount(&image, singularity_runtime_rootfs(NULL));

    singularity_message(DEBUG, "Setting SINGULARITY_ROOTFS to: %s\n", singularity_runtime_rootfs(NULL));
    setenv("SINGULARITY_ROOTFS", singularity_runtime_rootfs(NULL), 1);

    argv[0] = joinpath(LIBEXECDIR, "/singularity/import.sh");

    if ( is_exec(argv[0]) != 0 ) {
        singularity_message(ERROR, "Could not locate import driver: %s\n", argv[0]);
        ABORT(255);
    }

    return(singularity_fork_exec(argv));
}
