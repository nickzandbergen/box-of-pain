#include <cstdio>
#include <cstdlib>
#include <signal.h>
#include <sys/ptrace.h>
#include <sys/reg.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/syscall.h>
#include <functional>
#include <sys/socket.h>
/* based on code from
 * https://blog.nelhage.com/2010/08/write-yourself-an-strace-in-70-lines-of-code/
 */

class Syscall;
struct trace {
	int pid;
	int status;
	bool exited;
	int sysnum;
	Syscall *syscall;
};

struct trace *traces;

#define MAX_PARAMS 6 /* linux has 6 register parameters */

int param_map[MAX_PARAMS] = {
	RDI,
	RSI,
	RDX,
	R10,
	R8,
	R9
};

enum syscall_state {
	STATE_UNCALLED,
	STATE_CALLED,
	STATE_DONE,
};

class Syscall {
	public:
		int frompid;
		unsigned long number;
		unsigned long params[MAX_PARAMS];
		unsigned long retval;
		enum syscall_state state;

		Syscall() {
			frompid = pid;
			state = STATE_CALLED;
			number = ptrace(PTRACE_PEEKUSER, frompid, sizeof(long)*ORIG_RAX);
			for(int i=0;i<MAX_PARAMS;i++) {
				params[i] = ptrace(PTRACE_PEEKUSER, frompid, sizeof(long)*param_map[i]);
			}
			init();
			start();
		}

		virtual void finish() { }
		virtual void start() { }
		virtual void init() { };

		virtual bool operator ==(const Syscall &other) const {
			return number == other.number;
		}
};

class SysRead : public Syscall {
	public:
};

class SysRecvfrom : public Syscall {
	public:
		int socket;
		void *buffer;
		size_t length;
		int flags;
		struct sockaddr *addr;
		socklen_t *addrlen;

		void init() {
			socket = params[0];
			buffer = (void *)params[1];
			length = params[2];
			flags = params[3];
			addr = (struct sockaddr *)params[4];
			addrlen = (socklen_t *)params[5];
		}

		bool operator ==(const SysRecvfrom &other) const {
			/* Simple "fuzzy" comparison: socket is the same */
			/* TODO: check addr for source, etc */
			return socket == other.socket;
		}

		void start() { }
		void finish() { }
};

/* HACK: there are not this many syscalls, but there is no defined "num syscalls" to
 * use. */
std::function<Syscall *()> syscallmap[1024] = {
	[SYS_read] = [](){return new SysRead();},
	[SYS_recvfrom] = [](){return new SysRecvfrom();},
};

int wait_for_syscall(void)
{
	int status;
	while(1) {
		int pid;
		if((pid=waitpid(-1, &status, 0)) == -1) {
			return -1;
		}
		traces[pid].status = status;
		if(WIFSTOPPED(status) && WSTOPSIG(status) & 0x80) {
			return pid;
		}
		if(WIFEXITED(status)) {
			traces[pid].exited = true;
			return pid;
		}
	}
}

int do_trace()
{
	int syscall, retval;
	for(int i=0;i<num_traces;i++) {
		traces[i].sysnum = -1;
    	waitpid(traces[i].pid, &status, 0);
		ptrace(PTRACE_SETOPTIONS, traces[i].pid, 0, PTRACE_O_TRACESYSGOOD);
		ptrace(PTRACE_SYSCALL, traces[i].pid, 0, 0);
	}

	while(true) {
		int pid;
		if((pid=wait_for_syscall()) == -1) break;

		if(traces[pid].sysnum == -1) {
			traces[pid].sysnum = ptrace(PTRACE_PEEKUSER, pid, sizeof(long)*ORIG_RAX);
			if(errno != 0) break;

			fprintf(stderr, "[%d]: syscall %d entry\n", pid, traces[pid].sysnum);
			traces[pid].syscall = NULL;
			try { traces[pid].syscall = syscallmap[traces[pid].sysnum](); } catch (std::bad_function_call e) {}
		} else {
			int retval = ptrace(PTRACE_PEEKUSER, pid, sizeof(long)*RAX);
			if(errno != 0) break;
			fprintf(stderr, "[%d]: syscall %d exit -> %d\n", pid, traces[pid].sysnum, retval);
			if(
		}

		syscall = ptrace(PTRACE_PEEKUSER, pid, sizeof(long)*ORIG_RAX);
		if(errno != 0) break;
		fprintf(stderr, "syscall(%d) = ", syscall);
		Syscall *s = NULL;
		try {
			s = syscallmap[syscall]();
		} catch(std::bad_function_call e) {

		}
		
		if(wait_for_syscall(pid) != 0) break;
		retval = ptrace(PTRACE_PEEKUSER, pid, sizeof(long)*RAX);
		if(errno != 0) break;
		fprintf(stderr, "%d\n", retval);

		if(s) {
			s->retval = retval;
			s->state = STATE_DONE;
			s->finish();
		}
		
	}
	return 0;
}

int main(int argc, char **argv)
{
	for(int i=1;i<argc;i++) {
		pid = fork();
		if(pid == 0) {
			ptrace(PTRACE_TRACEME);
			kill(getpid(), SIGSTOP);
			if(execlp(argv[i], argv[i], NULL) == -1) {
				fprintf(stderr, "failed to execute %s\n", argv[1]);
			}
			exit(255);
		}
	}
	do_trace();
}
