#pragma once

#include <unordered_map>

struct run;
extern struct run current_run;

#define SPACE(f, n) fprintf(f, "%*s", n, "")
#include "helper.h"
#include "sockets.h"
#include "sys.h"
#include "tracee.h"

class connection;
class sock;
struct run {
	/* list of all traced processes in the system */
	std::vector<struct proc_tr *> proc_list;
	/* list of all traced threads in the system */
	std::vector<struct thread_tr *> thread_list;
	/* list of all syscalls that happen, in order that they are observed */
	std::vector<Syscall *> syscall_list;

	std::vector<sock *> sock_list;

	std::vector<connection *> connection_list;
	/*A map of tids to thread structures*/
	std::unordered_map<int, struct thread_tr *> traces; 

	std::unordered_map<int, std::unordered_map<int, class sock *> > sockets;
	std::unordered_map<connid, connection *> connections;
};

extern struct run current_run;

#include <cstdio>
void run_serialize(struct run *run, FILE *f);

