/* 
 * Copyright (c) 2015-2016, Gregory M. Kurtzer. All rights reserved.
 * 
 * “Singularity” Copyright (c) 2016, The Regents of the University of California,
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>


#include "config.h"
#include "config_parser.h"
#include "message.h"
#include "util.h"
#include "privilege.h"
#include "sessiondir.h"
#include "singularity.h"
#include "file.h"


int main(int argc, char **argv) {
    int retval = 0;
    char *sessiondir;
    char *image = getenv("SINGULARITY_IMAGE");
    pid_t child_ns_pid;

    if ( image == NULL ) {
        message(ERROR, "SINGULARITY_IMAGE not defined!\n");
        ABORT(1);
    }

    priv_init();
    singularity_action_init();
    config_open("/etc/singularity/singularity.conf");
    singularity_rootfs_init(image, "/var/singularity/mnt", 0);

    message(VERBOSE, "SINGULARITY_IMAGE = '%s'\n", image);

    sessiondir = singularity_sessiondir(image);

    message(VERBOSE, "Sessiondir = '%s'\n", sessiondir);
    

    child_ns_pid = fork();

    if ( child_ns_pid == 0 ) {
        message(DEBUG, "Hello from NS child\n");

        singularity_ns_mnt_unshare();

        if ( singularity_rootfs_mount() < 0 ) {
            message(ERROR, "Failed moutning the image\n");
            ABORT(255);
        }

        singularity_rootfs_chroot();
//        system("/bin/ls -l /");

        singularity_mount_kernelfs();
        singularity_action_do(argc, argv);

        return(0);

    } else if ( child_ns_pid > 0 ) {
        int tmpstatus;

        message(DEBUG, "Waiting on NS child process\n");

        waitpid(child_ns_pid, &tmpstatus, 0);
        retval = WEXITSTATUS(tmpstatus);
    } else {
        message(ERROR, "Failed forking child process\n");
        ABORT(255);
    }

    if ( singularity_sessiondir_rm() < 0 ) {
        message(WARNING, "Could not remove sessiondir\n");
    }

    return(retval);

}