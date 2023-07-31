#include <stdio.h>
#include <stdlib.h>
#include <sys/sysctl.h>
#include <errno.h>
#include <libproc.h>
#include <fcntl.h>
#include <string.h>

#include "lib.h"

int print_process_info(int pid) {
    struct kinfo_proc proc_info;
    size_t proc_info_size = sizeof(proc_info);

    int mib[4] = {CTL_KERN, KERN_PROC, KERN_PROC_PID, pid};

    int ret = sysctl(mib, 4, &proc_info, &proc_info_size, NULL, 0);

    if (ret != 0) {
        perror("sysctl");
        return 1;
    }
    
    if (proc_info.kp_proc.p_pid == 0) {
        printf("No process with PID %d\n", pid);
        return 1;
    }

    printf("Process information for PID %d:\n", pid);
    printf("  Process Name: %s\n", proc_info.kp_proc.p_comm);
    printf("  Process ID: %d\n", proc_info.kp_proc.p_pid);
    printf("  Parent Process ID: %d\n", proc_info.kp_eproc.e_ppid);
    printf("  User ID: %d\n", proc_info.kp_eproc.e_ucred.cr_uid);

    return 0;
}

int print_io_statistics(int pid) {
    struct proc_taskinfo task_info;
    int task_info_size = proc_pidinfo(pid, PROC_PIDTASKINFO, 0, &task_info, sizeof(task_info));

    if (task_info_size <= 0) {
        perror("proc_pidinfo");
        return 1;
    }

    printf("I/O statistics for PID %d:\n", pid);
    printf("  Bytes Read: %llu (%s)\n", task_info.pti_total_user, human_readable_size(task_info.pti_total_user));
    printf("  Bytes Written: %llu (%s)\n", task_info.pti_total_system, human_readable_size(task_info.pti_total_system));
    printf("  Messages Sent: %d\n", task_info.pti_messages_sent);
    printf("  Messages Received: %d\n", task_info.pti_messages_received);
    printf("  Mach System Calls: %d\n", task_info.pti_syscalls_mach);
    printf("  Unix System Calls: %d\n", task_info.pti_syscalls_unix);
    printf("  Page-ins: %d\n", task_info.pti_pageins);
    printf("  Page-faults: %d\n", task_info.pti_faults);

    return 0;
}

typedef struct {
    int id;
    const char *name;
} fdtype_map_entry;

const fdtype_map_entry fdtype_map[] = {
    {PROX_FDTYPE_ATALK, "ATALK   "},
    {PROX_FDTYPE_VNODE, "VNODE   "},
    {PROX_FDTYPE_SOCKET, "SOCKET  "},
    {PROX_FDTYPE_PSHM, "PSHM    "},
    {PROX_FDTYPE_PSEM, "PSEM    "},
    {PROX_FDTYPE_KQUEUE, "KQUEUE  "},
    {PROX_FDTYPE_PIPE, "PIPE    "},
    {PROX_FDTYPE_FSEVENTS, "FSEVENTS"},
};

const char *fdtype_to_string(int fdtype) {
    for (size_t i = 0; i < sizeof(fdtype_map) / sizeof(fdtype_map_entry); i++) {
        if (fdtype_map[i].id == fdtype) {
            return fdtype_map[i].name;
        }
    }
    return "UNKNOWN ";
}

const char *openflags_human(int openflags) {
    char prefix[24] = {0};
    static char buffer[128] = {0};
    if ((openflags & FREAD) > 0) {
        if ((openflags & FWRITE) > 0) {
            strcpy(prefix, "RW");
        } else {
            strcpy(prefix, "RD");
        }
    } else if ((openflags & FWRITE) > 0) {
        strcpy(prefix, "WR");
    } else {
        strcpy(prefix, "??");
    }

    if (openflags > 3) {
        sprintf(buffer, "%s 0x%06x", prefix, openflags);
    } else {
        sprintf(buffer, "%s         ", prefix);
    }

    return buffer;    
}

int print_open_files(int pid) {
    int buffer_size = proc_pidinfo(pid, PROC_PIDLISTFDS, 0, NULL, 0);
    if (buffer_size <= 0) {
        perror("proc_pidinfo");
        return 1;
    }

    struct proc_fdinfo *fds = (struct proc_fdinfo *)malloc(buffer_size);
    if (fds == NULL) {
        perror("malloc");
        return 1;
    }

    int actual_size = proc_pidinfo(pid, PROC_PIDLISTFDS, 0, fds, buffer_size);
    if (actual_size <= 0) {
        perror("proc_pidinfo");
        free(fds);
        return 1;
    }

    int num_fds = actual_size / sizeof(struct proc_fdinfo);
    printf("Open file handles for PID %d:\n", pid);

    for (int i = 0; i < num_fds; i++) {
        printf("  %d\t %s", fds[i].proc_fd, fdtype_to_string(fds[i].proc_fdtype));        
        
        if (fds[i].proc_fdtype == PROX_FDTYPE_VNODE) {
            struct vnode_fdinfowithpath vnode_info;
            int vnode_info_size = proc_pidfdinfo(pid, fds[i].proc_fd, PROC_PIDFDVNODEPATHINFO, &vnode_info, sizeof(vnode_info));            
            if (vnode_info_size > 0) {
                struct proc_fileinfo *fdinfo = &vnode_info.pfi;
                printf(" %s %llu (%s)\t\t %s ", openflags_human(fdinfo->fi_openflags), fdinfo->fi_offset, human_readable_size(fdinfo->fi_offset), vnode_info.pvip.vip_path);
            }
        }
        
        printf("\n");
    }

    free(fds);

    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <pid>\n", argv[0]);
        return 1;
    }

    int pid = atoi(argv[1]);

    if (print_process_info(pid) != 0) {
        return 1;
    }
    puts("");
    if (print_io_statistics(pid) != 0) {
        return 1;
    }
    puts("");
    if (print_open_files(pid) != 0) {
        return 1;
    }
    
    return 0;
}

