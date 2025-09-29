
## 1. Setting up the environment

### 1.1 Record the configuration

Was getting tired of slow installations and compatibility errors so ended up going with AWS for
my virtual machine.

###### lscpu output

```sh
ubuntu@ip-172-31-28-98:~$ lscpu
Architecture:                       x86_64
  CPU op-mode(s):                   32-bit, 64-bit
  Address sizes:                    46 bits physical, 48 bits virtual
  Byte Order:                       Little Endian
CPU(s):                             4
  On-line CPU(s) list:              0-3
Vendor ID:                          GenuineIntel
  Model name:                       Intel(R) Xeon(R) CPU E5-2686 v4 @ 2.30GHz
    CPU family:                     6
    Model:                          79
    Thread(s) per core:             1
    Core(s) per socket:             4
    Socket(s):                      1
    Stepping:                       1
    BogoMIPS:                       4599.99
    Flags:                          fpu vme de pse tsc msr pae mce cx8 apic sep mtrr pge mca cmov pat pse36 clflush mmx fxsr sse sse2 ht syscall nx pdpe1gb rdtscp lm rep_good nopl xtopology cpuid tsc_known_freq pni pcl
                                    mulqdq ssse3 fma cx16 pcid sse4_1 sse4_2 x2apic movbe popcnt tsc_deadline_timer aes xsave avx f16c rdrand hypervisor lahf_lm abm pti fsgsbase bmi2 erms invpcid xsaveop
                                    t
Virtualization features:
  Hypervisor vendor:                Xen
  Virtualization type:              full
Caches (sum of all):
  L1d:                              128 KiB (4 instances)
  L1i:                              128 KiB (4 instances)
  L2:                               1 MiB (4 instances)
  L3:                               45 MiB (1 instance)
NUMA:
  NUMA node(s):                     1
  NUMA node0 CPU(s):                0-3
Vulnerabilities:
  Gather data sampling:             Not affected
  Ghostwrite:                       Not affected
  Indirect target selection:        Mitigation; Aligned branch/return thunks
  Itlb multihit:                    KVM: Mitigation: VMX unsupported
  L1tf:                             Mitigation; PTE Inversion
  Mds:                              Vulnerable: Clear CPU buffers attempted, no microcode; SMT Host state unknown
  Meltdown:                         Mitigation; PTI
  Mmio stale data:                  Vulnerable: Clear CPU buffers attempted, no microcode; SMT Host state unknown
  Reg file data sampling:           Not affected
  Retbleed:                         Not affected
  Spec rstack overflow:             Not affected
  Spec store bypass:                Vulnerable
  Spectre v1:                       Mitigation; usercopy/swapgs barriers and __user pointer sanitization
  Spectre v2:                       Mitigation; Retpolines; STIBP disabled; RSB filling; PBRSB-eIBRS Not affected; BHI Retpoline
  Srbds:                            Not affected
  Tsx async abort:                  Not affected
```

###### uname -r output

```
ubuntu@ip-172-31-28-98:~$ uname -r
6.15.0
```


###### cat /proc/version output

```
ubuntu@ip-172-31-28-98:~$ cat /proc/version
Linux version 6.15.0 (ubuntu@ip-172-31-28-98) (gcc (Ubuntu 13.3.0-6ubuntu2~24.04) 13.3.0, GNU ld (GNU Binutils for Ubuntu) 2.42) #1 SMP Fri Sep 19 17:03:26 UTC 2025
```

###### .config file

For BPF/eBPF functionality you will need to enable the BPF syscall like this:

```
CONFIG_BPF_SYSCALL=y
```

And you will need
```
CONFIG_BPF_JIT=y
```
for the Just-In-Time compiler with some more specific options being present as well.
All together the most important settings for BPF functionality are captured in this code snippet:

```
#
# BPF subsystem
#
CONFIG_BPF_SYSCALL=y
CONFIG_BPF_JIT=y
CONFIG_BPF_JIT_ALWAYS_ON=y
CONFIG_BPF_JIT_DEFAULT_ON=y
CONFIG_BPF_UNPRIV_DEFAULT_OFF=y
# CONFIG_BPF_PRELOAD is not set
CONFIG_BPF_LSM=y
# end of BPF subsystem
```

###### network stack debugging

A section is clearly designated for network debugging where as a default the settings area uninitialized:

```
# Networking Debugging
#
# CONFIG_NET_DEV_REFCNT_TRACKER is not set
# CONFIG_NET_NS_REFCNT_TRACKER is not set
# CONFIG_DEBUG_NET is not set
# CONFIG_DEBUG_NET_SMALL_RTNL is not set
# end of Networking Debugging
```

#### printk buffer size

The prinkt buffer size is determined by the following lines:

```
CONFIG_LOG_BUF_SHIFT=18
CONFIG_LOG_CPU_MAX_BUF_SHIFT=12
```

Where `CONFIG_LOG_BUF_SHIFT` is the binary logarithm of the max buffer size in bytes.
So in the case where it is set to 18 the max buffer size would be $2^{18} b = 256 kb$.
The `CONFIG_LOG_CPU_MAX_BUF_SHIFT` is equivalently the binary logarithm of the max buffer per CPU. In this case the max buffer per CPU would be $2^{12} b = 4 kb$

### 1.1. Application analysis


#### Execution flow diagram

![](attachment/7aeb17f8943687d32dd39e94422c753e.png)



#### Error injection

##### Port number 0
``` sh
ubuntu@ip-172-31-28-98:~/stuff/lab1/src$ strace ./client
execve("./client", ["./client"], 0x7ffc02147110 /* 27 vars */) = 0
brk(NULL)                               = 0x57f083bfa000
mmap(NULL, 8192, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0) = 0x73092843e000
access("/etc/ld.so.preload", R_OK)      = -1 ENOENT (No such file or directory)
openat(AT_FDCWD, "/etc/ld.so.cache", O_RDONLY|O_CLOEXEC) = 3
fstat(3, {st_mode=S_IFREG|0644, st_size=24751, ...}) = 0
mmap(NULL, 24751, PROT_READ, MAP_PRIVATE, 3, 0) = 0x730928437000
close(3)                                = 0
openat(AT_FDCWD, "/lib/x86_64-linux-gnu/libc.so.6", O_RDONLY|O_CLOEXEC) = 3
read(3, "\177ELF\2\1\1\3\0\0\0\0\0\0\0\0\3\0>\0\1\0\0\0\220\243\2\0\0\0\0\0"..., 832) = 832
pread64(3, "\6\0\0\0\4\0\0\0@\0\0\0\0\0\0\0@\0\0\0\0\0\0\0@\0\0\0\0\0\0\0"..., 784, 64) = 784
fstat(3, {st_mode=S_IFREG|0755, st_size=2125328, ...}) = 0
pread64(3, "\6\0\0\0\4\0\0\0@\0\0\0\0\0\0\0@\0\0\0\0\0\0\0@\0\0\0\0\0\0\0"..., 784, 64) = 784
mmap(NULL, 2170256, PROT_READ, MAP_PRIVATE|MAP_DENYWRITE, 3, 0) = 0x730928200000
mmap(0x730928228000, 1605632, PROT_READ|PROT_EXEC, MAP_PRIVATE|MAP_FIXED|MAP_DENYWRITE, 3, 0x28000) = 0x730928228000
mmap(0x7309283b0000, 323584, PROT_READ, MAP_PRIVATE|MAP_FIXED|MAP_DENYWRITE, 3, 0x1b0000) = 0x7309283b0000
mmap(0x7309283ff000, 24576, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_FIXED|MAP_DENYWRITE, 3, 0x1fe000) = 0x7309283ff000
mmap(0x730928405000, 52624, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_FIXED|MAP_ANONYMOUS, -1, 0) = 0x730928405000
close(3)                                = 0
mmap(NULL, 12288, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0) = 0x730928434000
arch_prctl(ARCH_SET_FS, 0x730928434740) = 0
set_tid_address(0x730928434a10)         = 2018
set_robust_list(0x730928434a20, 24)     = 0
rseq(0x730928435060, 0x20, 0, 0x53053053) = 0
mprotect(0x7309283ff000, 16384, PROT_READ) = 0
mprotect(0x57f075616000, 4096, PROT_READ) = 0
mprotect(0x73092847e000, 8192, PROT_READ) = 0
prlimit64(0, RLIMIT_STACK, NULL, {rlim_cur=8192*1024, rlim_max=RLIM64_INFINITY}) = 0
munmap(0x730928437000, 24751)           = 0
socket(AF_INET, SOCK_STREAM, IPPROTO_IP) = 3
connect(3, {sa_family=AF_INET, sin_port=htons(0), sin_addr=inet_addr("127.0.0.1")}, 16) = -1 ECONNREFUSED (Connection refused)
dup(2)                                  = 4
fcntl(4, F_GETFL)                       = 0x2 (flags O_RDWR)
getrandom("\x08\x5c\xd5\x88\x59\xca\x2c\x6d", 8, GRND_NONBLOCK) = 8
brk(NULL)                               = 0x57f083bfa000
brk(0x57f083c1b000)                     = 0x57f083c1b000
fstat(4, {st_mode=S_IFCHR|0620, st_rdev=makedev(0x88, 0x2), ...}) = 0
write(4, "connect: Connection refused\n", 28connect: Connection refused
) = 28
close(4)                                = 0
close(3)                                = 0
exit_group(1)                           = ?
+++ exited with 1 +++
```

##### Port number 65536
```sh
ubuntu@ip-172-31-28-98:~/stuff/lab1/src$ strace ./client
execve("./client", ["./client"], 0x7ffd74741a40 /* 27 vars */) = 0
brk(NULL)                               = 0x61e3f5c8f000
mmap(NULL, 8192, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0) = 0x7c82c667e000
access("/etc/ld.so.preload", R_OK)      = -1 ENOENT (No such file or directory)
openat(AT_FDCWD, "/etc/ld.so.cache", O_RDONLY|O_CLOEXEC) = 3
fstat(3, {st_mode=S_IFREG|0644, st_size=24751, ...}) = 0
mmap(NULL, 24751, PROT_READ, MAP_PRIVATE, 3, 0) = 0x7c82c6677000
close(3)                                = 0
openat(AT_FDCWD, "/lib/x86_64-linux-gnu/libc.so.6", O_RDONLY|O_CLOEXEC) = 3
read(3, "\177ELF\2\1\1\3\0\0\0\0\0\0\0\0\3\0>\0\1\0\0\0\220\243\2\0\0\0\0\0"..., 832) = 832
pread64(3, "\6\0\0\0\4\0\0\0@\0\0\0\0\0\0\0@\0\0\0\0\0\0\0@\0\0\0\0\0\0\0"..., 784, 64) = 784
fstat(3, {st_mode=S_IFREG|0755, st_size=2125328, ...}) = 0
pread64(3, "\6\0\0\0\4\0\0\0@\0\0\0\0\0\0\0@\0\0\0\0\0\0\0@\0\0\0\0\0\0\0"..., 784, 64) = 784
mmap(NULL, 2170256, PROT_READ, MAP_PRIVATE|MAP_DENYWRITE, 3, 0) = 0x7c82c6400000
mmap(0x7c82c6428000, 1605632, PROT_READ|PROT_EXEC, MAP_PRIVATE|MAP_FIXED|MAP_DENYWRITE, 3, 0x28000) = 0x7c82c6428000
mmap(0x7c82c65b0000, 323584, PROT_READ, MAP_PRIVATE|MAP_FIXED|MAP_DENYWRITE, 3, 0x1b0000) = 0x7c82c65b0000
mmap(0x7c82c65ff000, 24576, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_FIXED|MAP_DENYWRITE, 3, 0x1fe000) = 0x7c82c65ff000
mmap(0x7c82c6605000, 52624, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_FIXED|MAP_ANONYMOUS, -1, 0) = 0x7c82c6605000
close(3)                                = 0
mmap(NULL, 12288, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0) = 0x7c82c6674000
arch_prctl(ARCH_SET_FS, 0x7c82c6674740) = 0
set_tid_address(0x7c82c6674a10)         = 2031
set_robust_list(0x7c82c6674a20, 24)     = 0
rseq(0x7c82c6675060, 0x20, 0, 0x53053053) = 0
mprotect(0x7c82c65ff000, 16384, PROT_READ) = 0
mprotect(0x61e3e56e3000, 4096, PROT_READ) = 0
mprotect(0x7c82c66be000, 8192, PROT_READ) = 0
prlimit64(0, RLIMIT_STACK, NULL, {rlim_cur=8192*1024, rlim_max=RLIM64_INFINITY}) = 0
munmap(0x7c82c6677000, 24751)           = 0
socket(AF_INET, SOCK_STREAM, IPPROTO_IP) = 3
connect(3, {sa_family=AF_INET, sin_port=htons(0), sin_addr=inet_addr("127.0.0.1")}, 16) = -1 ECONNREFUSED (Connection refused)
dup(2)                                  = 4
fcntl(4, F_GETFL)                       = 0x2 (flags O_RDWR)
getrandom("\xcb\x99\x90\xf7\xdf\x96\x4d\x17", 8, GRND_NONBLOCK) = 8
brk(NULL)                               = 0x61e3f5c8f000
brk(0x61e3f5cb0000)                     = 0x61e3f5cb0000
fstat(4, {st_mode=S_IFCHR|0620, st_rdev=makedev(0x88, 0x2), ...}) = 0
write(4, "connect: Connection refused\n", 28connect: Connection refused
) = 28
close(4)                                = 0
close(3)                                = 0
exit_group(1)                           = ?
+++ exited with 1 +++
```

##### Port number -1
```sh
ubuntu@ip-172-31-28-98:~/stuff/lab1/src$ strace ./client
execve("./client", ["./client"], 0x7ffc6b170820 /* 27 vars */) = 0
brk(NULL)                               = 0x5ee35186f000
mmap(NULL, 8192, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0) = 0x71fc3175e000
access("/etc/ld.so.preload", R_OK)      = -1 ENOENT (No such file or directory)
openat(AT_FDCWD, "/etc/ld.so.cache", O_RDONLY|O_CLOEXEC) = 3
fstat(3, {st_mode=S_IFREG|0644, st_size=24751, ...}) = 0
mmap(NULL, 24751, PROT_READ, MAP_PRIVATE, 3, 0) = 0x71fc31757000
close(3)                                = 0
openat(AT_FDCWD, "/lib/x86_64-linux-gnu/libc.so.6", O_RDONLY|O_CLOEXEC) = 3
read(3, "\177ELF\2\1\1\3\0\0\0\0\0\0\0\0\3\0>\0\1\0\0\0\220\243\2\0\0\0\0\0"..., 832) = 832
pread64(3, "\6\0\0\0\4\0\0\0@\0\0\0\0\0\0\0@\0\0\0\0\0\0\0@\0\0\0\0\0\0\0"..., 784, 64) = 784
fstat(3, {st_mode=S_IFREG|0755, st_size=2125328, ...}) = 0
pread64(3, "\6\0\0\0\4\0\0\0@\0\0\0\0\0\0\0@\0\0\0\0\0\0\0@\0\0\0\0\0\0\0"..., 784, 64) = 784
mmap(NULL, 2170256, PROT_READ, MAP_PRIVATE|MAP_DENYWRITE, 3, 0) = 0x71fc31400000
mmap(0x71fc31428000, 1605632, PROT_READ|PROT_EXEC, MAP_PRIVATE|MAP_FIXED|MAP_DENYWRITE, 3, 0x28000) = 0x71fc31428000
mmap(0x71fc315b0000, 323584, PROT_READ, MAP_PRIVATE|MAP_FIXED|MAP_DENYWRITE, 3, 0x1b0000) = 0x71fc315b0000
mmap(0x71fc315ff000, 24576, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_FIXED|MAP_DENYWRITE, 3, 0x1fe000) = 0x71fc315ff000
mmap(0x71fc31605000, 52624, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_FIXED|MAP_ANONYMOUS, -1, 0) = 0x71fc31605000
close(3)                                = 0
mmap(NULL, 12288, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0) = 0x71fc31754000
arch_prctl(ARCH_SET_FS, 0x71fc31754740) = 0
set_tid_address(0x71fc31754a10)         = 2527
set_robust_list(0x71fc31754a20, 24)     = 0
rseq(0x71fc31755060, 0x20, 0, 0x53053053) = 0
mprotect(0x71fc315ff000, 16384, PROT_READ) = 0
mprotect(0x5ee3330af000, 4096, PROT_READ) = 0
mprotect(0x71fc3179e000, 8192, PROT_READ) = 0
prlimit64(0, RLIMIT_STACK, NULL, {rlim_cur=8192*1024, rlim_max=RLIM64_INFINITY}) = 0
munmap(0x71fc31757000, 24751)           = 0
socket(AF_INET, SOCK_STREAM, IPPROTO_IP) = 3
connect(3, {sa_family=AF_INET, sin_port=htons(65535), sin_addr=inet_addr("127.0.0.1")}, 16) = -1 ECONNREFUSED (Connection refused)
dup(2)                                  = 4
fcntl(4, F_GETFL)                       = 0x402 (flags O_RDWR|O_APPEND)
getrandom("\x9c\xe7\x59\xa5\xb3\xcd\x83\x35", 8, GRND_NONBLOCK) = 8
brk(NULL)                               = 0x5ee35186f000
brk(0x5ee351890000)                     = 0x5ee351890000
fstat(4, {st_mode=S_IFCHR|0620, st_rdev=makedev(0x88, 0x2), ...}) = 0
write(4, "connect: Connection refused\n", 28connect: Connection refused
) = 28
close(4)                                = 0
close(3)                                = 0
exit_group(1)                           = ?
+++ exited with 1 +++
```

##### Invalid IP address - Using the address 1

```sh
ubuntu@ip-172-31-28-98:~/stuff/lab1/src$ strace ./client
execve("./client", ["./client"], 0x7ffccc06d290 /* 27 vars */) = 0
brk(NULL)                               = 0x5d4c554b7000
mmap(NULL, 8192, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0) = 0x7f2c2863b000
access("/etc/ld.so.preload", R_OK)      = -1 ENOENT (No such file or directory)
openat(AT_FDCWD, "/etc/ld.so.cache", O_RDONLY|O_CLOEXEC) = 3
fstat(3, {st_mode=S_IFREG|0644, st_size=24751, ...}) = 0
mmap(NULL, 24751, PROT_READ, MAP_PRIVATE, 3, 0) = 0x7f2c28634000
close(3)                                = 0
openat(AT_FDCWD, "/lib/x86_64-linux-gnu/libc.so.6", O_RDONLY|O_CLOEXEC) = 3
read(3, "\177ELF\2\1\1\3\0\0\0\0\0\0\0\0\3\0>\0\1\0\0\0\220\243\2\0\0\0\0\0"..., 832) = 832
pread64(3, "\6\0\0\0\4\0\0\0@\0\0\0\0\0\0\0@\0\0\0\0\0\0\0@\0\0\0\0\0\0\0"..., 784, 64) = 784
fstat(3, {st_mode=S_IFREG|0755, st_size=2125328, ...}) = 0
pread64(3, "\6\0\0\0\4\0\0\0@\0\0\0\0\0\0\0@\0\0\0\0\0\0\0@\0\0\0\0\0\0\0"..., 784, 64) = 784
mmap(NULL, 2170256, PROT_READ, MAP_PRIVATE|MAP_DENYWRITE, 3, 0) = 0x7f2c28400000
mmap(0x7f2c28428000, 1605632, PROT_READ|PROT_EXEC, MAP_PRIVATE|MAP_FIXED|MAP_DENYWRITE, 3, 0x28000) = 0x7f2c28428000
mmap(0x7f2c285b0000, 323584, PROT_READ, MAP_PRIVATE|MAP_FIXED|MAP_DENYWRITE, 3, 0x1b0000) = 0x7f2c285b0000
mmap(0x7f2c285ff000, 24576, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_FIXED|MAP_DENYWRITE, 3, 0x1fe000) = 0x7f2c285ff000
mmap(0x7f2c28605000, 52624, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_FIXED|MAP_ANONYMOUS, -1, 0) = 0x7f2c28605000
close(3)                                = 0
mmap(NULL, 12288, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0) = 0x7f2c28631000
arch_prctl(ARCH_SET_FS, 0x7f2c28631740) = 0
set_tid_address(0x7f2c28631a10)         = 3577
set_robust_list(0x7f2c28631a20, 24)     = 0
rseq(0x7f2c28632060, 0x20, 0, 0x53053053) = 0
mprotect(0x7f2c285ff000, 16384, PROT_READ) = 0
mprotect(0x5d4c206c4000, 4096, PROT_READ) = 0
mprotect(0x7f2c2867b000, 8192, PROT_READ) = 0
prlimit64(0, RLIMIT_STACK, NULL, {rlim_cur=8192*1024, rlim_max=RLIM64_INFINITY}) = 0
munmap(0x7f2c28634000, 24751)           = 0
socket(AF_INET, SOCK_STREAM, IPPROTO_IP) = 3
dup(2)                                  = 4
fcntl(4, F_GETFL)                       = 0x402 (flags O_RDWR|O_APPEND)
getrandom("\xcc\xd4\x58\x57\xa3\x3e\xe8\x1f", 8, GRND_NONBLOCK) = 8
brk(NULL)                               = 0x5d4c554b7000
brk(0x5d4c554d8000)                     = 0x5d4c554d8000
fstat(4, {st_mode=S_IFCHR|0620, st_rdev=makedev(0x88, 0x2), ...}) = 0
write(4, "inet_pton: Success\n", 19inet_pton: Success
)    = 19
close(4)                                = 0
exit_group(1)                           = ?
+++ exited with 1 +++
```


## Buffer analysis

##### Client buffer:

To begin with we look at the client side buffer shown in this snippet:
```sh
char buf[64] = {0};

if (recv(fd, buf, sizeof(buf), 0) < 0)
```
This buffer is 64 bytes long.

We overflow it by changing the msg variable to "hello" written 13 times consecutively ($6\cdot13 = 65$).
###### Result:
In this case the message back to the client simply gets cut off:
```sh
Client 0 connected
Client 0 received: hellohellohellohellohellohellohellohellohellohellohellohellohell
Client 0 disconnected
```
This repeats for the other messages.

##### Server buffer

We now look at the server side buffer declared in this snippet:

```sh
char buf[4096];
ssize_t n;
```

In order to overflow this easily we slighty change the code such that msg is not constant.
Updated code:
```sh
        char msg[4100];
        memset(msg, 'A', 4095);
        memset(msg + 4095, 'B', 4) 
        msg[4099] = '\0';;
```

In this case the message gets split up into 2:

```sh
[server] received 4096 bytes
[server] received 3 bytes
```

### 1.2. Syscall analysis with `strace`

##### Client system calls:
```sh
execve("./client", ["./client"], 0x7fff8ca00f00 /* 27 vars */) = 0
brk(NULL)                               = 0x58c727176000
mmap(NULL, 8192, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0) = 0x76b898781000
access("/etc/ld.so.preload", R_OK)      = -1 ENOENT (No such file or directory)
openat(AT_FDCWD, "/etc/ld.so.cache", O_RDONLY|O_CLOEXEC) = 3
fstat(3, {st_mode=S_IFREG|0644, st_size=24751, ...}) = 0
mmap(NULL, 24751, PROT_READ, MAP_PRIVATE, 3, 0) = 0x76b89877a000
close(3)                                = 0
openat(AT_FDCWD, "/lib/x86_64-linux-gnu/libc.so.6", O_RDONLY|O_CLOEXEC) = 3
read(3, "\177ELF\2\1\1\3\0\0\0\0\0\0\0\0\3\0>\0\1\0\0\0\220\243\2\0\0\0\0\0"..., 832) = 832
pread64(3, "\6\0\0\0\4\0\0\0@\0\0\0\0\0\0\0@\0\0\0\0\0\0\0@\0\0\0\0\0\0\0"..., 784, 64) = 784
fstat(3, {st_mode=S_IFREG|0755, st_size=2125328, ...}) = 0
pread64(3, "\6\0\0\0\4\0\0\0@\0\0\0\0\0\0\0@\0\0\0\0\0\0\0@\0\0\0\0\0\0\0"..., 784, 64) = 784
mmap(NULL, 2170256, PROT_READ, MAP_PRIVATE|MAP_DENYWRITE, 3, 0) = 0x76b898400000
mmap(0x76b898428000, 1605632, PROT_READ|PROT_EXEC, MAP_PRIVATE|MAP_FIXED|MAP_DENYWRITE, 3, 0x28000) = 0x76b898428000
mmap(0x76b8985b0000, 323584, PROT_READ, MAP_PRIVATE|MAP_FIXED|MAP_DENYWRITE, 3, 0x1b0000) = 0x76b8985b0000
mmap(0x76b8985ff000, 24576, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_FIXED|MAP_DENYWRITE, 3, 0x1fe000) = 0x76b8985ff000
mmap(0x76b898605000, 52624, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_FIXED|MAP_ANONYMOUS, -1, 0) = 0x76b898605000
close(3)                                = 0
mmap(NULL, 12288, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0) = 0x76b898777000
arch_prctl(ARCH_SET_FS, 0x76b898777740) = 0
set_tid_address(0x76b898777a10)         = 2006
set_robust_list(0x76b898777a20, 24)     = 0
rseq(0x76b898778060, 0x20, 0, 0x53053053) = 0
mprotect(0x76b8985ff000, 16384, PROT_READ) = 0
mprotect(0x58c6efcdf000, 4096, PROT_READ) = 0
mprotect(0x76b8987c1000, 8192, PROT_READ) = 0
prlimit64(0, RLIMIT_STACK, NULL, {rlim_cur=8192*1024, rlim_max=RLIM64_INFINITY}) = 0
munmap(0x76b89877a000, 24751)           = 0
socket(AF_INET, SOCK_STREAM, IPPROTO_IP) = 3
connect(3, {sa_family=AF_INET, sin_port=htons(9023), sin_addr=inet_addr("127.0.0.1")}, 16) = 0
fstat(1, {st_mode=S_IFCHR|0620, st_rdev=makedev(0x88, 0x3), ...}) = 0
getrandom("\x5d\x14\x60\x4e\x95\x90\xc5\x5a", 8, GRND_NONBLOCK) = 8
brk(NULL)                               = 0x58c727176000
brk(0x58c727197000)                     = 0x58c727197000
write(1, "Client 0 connected\n", 19)    = 19
sendto(3, "hello", 5, 0, NULL, 0)       = 5
recvfrom(3, "hello", 64, 0, NULL, NULL) = 5
write(1, "Client 0 received: hello\n", 25) = 25
close(3)                                = 0
write(1, "Client 0 disconnected\n", 22) = 22
socket(AF_INET, SOCK_STREAM, IPPROTO_IP) = 3
connect(3, {sa_family=AF_INET, sin_port=htons(9023), sin_addr=inet_addr("127.0.0.1")}, 16) = 0
write(1, "Client 1 connected\n", 19)    = 19
sendto(3, "hello", 5, 0, NULL, 0)       = 5
```


##### Server system calls:

```sh
ubuntu@ip-172-31-28-98:~/stuff/lab1/src$ strace ./server 2
execve("./server", ["./server", "2"], 0x7ffeb0452ab8 /* 27 vars */) = 0
brk(NULL)                               = 0x62a0ee1f8000
mmap(NULL, 8192, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0) = 0x7f5ca8f06000
access("/etc/ld.so.preload", R_OK)      = -1 ENOENT (No such file or directory)
openat(AT_FDCWD, "/etc/ld.so.cache", O_RDONLY|O_CLOEXEC) = 3
fstat(3, {st_mode=S_IFREG|0644, st_size=24751, ...}) = 0
mmap(NULL, 24751, PROT_READ, MAP_PRIVATE, 3, 0) = 0x7f5ca8eff000
close(3)                                = 0
openat(AT_FDCWD, "/lib/x86_64-linux-gnu/libc.so.6", O_RDONLY|O_CLOEXEC) = 3
read(3, "\177ELF\2\1\1\3\0\0\0\0\0\0\0\0\3\0>\0\1\0\0\0\220\243\2\0\0\0\0\0"..., 832) = 832
pread64(3, "\6\0\0\0\4\0\0\0@\0\0\0\0\0\0\0@\0\0\0\0\0\0\0@\0\0\0\0\0\0\0"..., 784, 64) = 784
fstat(3, {st_mode=S_IFREG|0755, st_size=2125328, ...}) = 0
pread64(3, "\6\0\0\0\4\0\0\0@\0\0\0\0\0\0\0@\0\0\0\0\0\0\0@\0\0\0\0\0\0\0"..., 784, 64) = 784
mmap(NULL, 2170256, PROT_READ, MAP_PRIVATE|MAP_DENYWRITE, 3, 0) = 0x7f5ca8c00000
mmap(0x7f5ca8c28000, 1605632, PROT_READ|PROT_EXEC, MAP_PRIVATE|MAP_FIXED|MAP_DENYWRITE, 3, 0x28000) = 0x7f5ca8c28000
mmap(0x7f5ca8db0000, 323584, PROT_READ, MAP_PRIVATE|MAP_FIXED|MAP_DENYWRITE, 3, 0x1b0000) = 0x7f5ca8db0000
mmap(0x7f5ca8dff000, 24576, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_FIXED|MAP_DENYWRITE, 3, 0x1fe000) = 0x7f5ca8dff000
mmap(0x7f5ca8e05000, 52624, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_FIXED|MAP_ANONYMOUS, -1, 0) = 0x7f5ca8e05000
close(3)                                = 0
mmap(NULL, 12288, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0) = 0x7f5ca8efc000
arch_prctl(ARCH_SET_FS, 0x7f5ca8efc740) = 0
set_tid_address(0x7f5ca8efca10)         = 2028
set_robust_list(0x7f5ca8efca20, 24)     = 0
rseq(0x7f5ca8efd060, 0x20, 0, 0x53053053) = 0
mprotect(0x7f5ca8dff000, 16384, PROT_READ) = 0
mprotect(0x62a0c1457000, 4096, PROT_READ) = 0
mprotect(0x7f5ca8f46000, 8192, PROT_READ) = 0
prlimit64(0, RLIMIT_STACK, NULL, {rlim_cur=8192*1024, rlim_max=RLIM64_INFINITY}) = 0
munmap(0x7f5ca8eff000, 24751)           = 0
socket(AF_INET, SOCK_STREAM, IPPROTO_IP) = 3
setsockopt(3, SOL_SOCKET, SO_REUSEADDR, [1], 4) = 0
bind(3, {sa_family=AF_INET, sin_port=htons(9023), sin_addr=inet_addr("127.0.0.1")}, 16) = 0
listen(3, 128)                          = 0
write(2, "[server] listening on 127.0.0.1:"..., 37[server] listening on 127.0.0.1:9023
) = 37
accept4(3, NULL, NULL, SOCK_CLOEXEC
```

##### strace -c server output

```sh
% time     seconds  usecs/call     calls    errors syscall
------ ----------- ----------- --------- --------- ----------------
 27.62    0.001221          20        60           recvfrom
 19.07    0.000843          26        32           close
 16.99    0.000751          25        30           sendto
 13.01    0.000575          19        30           accept4
 10.38    0.000459          14        31           write
  4.39    0.000194         194         1           execve
  2.49    0.000110          13         8           mmap
  0.77    0.000034          11         3           mprotect
  0.77    0.000034          11         3           brk
  0.61    0.000027           9         3           fstat
  0.50    0.000022          11         2           openat
  0.45    0.000020          10         2           pread64
  0.36    0.000016          16         1           socket
  0.34    0.000015          15         1           munmap
  0.34    0.000015          15         1           bind
  0.25    0.000011          11         1         1 access
  0.23    0.000010          10         1           getrandom
  0.20    0.000009           9         1           read
  0.20    0.000009           9         1           listen
  0.20    0.000009           9         1           setsockopt
  0.18    0.000008           8         1           prlimit64
  0.16    0.000007           7         1           arch_prctl
  0.16    0.000007           7         1           set_tid_address
  0.16    0.000007           7         1           set_robust_list
  0.16    0.000007           7         1           rseq
------ ----------- ----------- --------- --------- ----------------
100.00    0.004420          20       218         1 total
```

We can see that the mmap is the most frequently used system call, being used 8 times, to manage memory, and the time it took to make all the calls was less than 1 microsecond.

##### strace -c client output

```sh
% time     seconds  usecs/call     calls    errors syscall
------ ----------- ----------- --------- --------- ----------------
 29.88    0.001193          13        90           write
 22.62    0.000903          30        30           connect
 15.61    0.000623          19        32           close
 14.08    0.000562          18        30           sendto
  9.09    0.000363          12        30           socket
  8.72    0.000348          11        30           recvfrom
  0.00    0.000000           0         1           read
  0.00    0.000000           0         3           fstat
  0.00    0.000000           0         8           mmap
  0.00    0.000000           0         3           mprotect
  0.00    0.000000           0         1           munmap
  0.00    0.000000           0         3           brk
  0.00    0.000000           0         2           pread64
  0.00    0.000000           0         1         1 access
  0.00    0.000000           0         1           execve
  0.00    0.000000           0         1           arch_prctl
  0.00    0.000000           0         1           set_tid_address
  0.00    0.000000           0         2           openat
  0.00    0.000000           0         1           set_robust_list
  0.00    0.000000           0         1           prlimit64
  0.00    0.000000           0         1           getrandom
  0.00    0.000000           0         1           rseq
------ ----------- ----------- --------- --------- ----------------
100.00    0.003992          14       273         1 total
```

### 1.3. Kernel-side observation with `bpftrace`

Got these results: 
```sh
@connect_accept_latency_us: 
[2, 4)                14 |@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@|
[4, 8)                 4 |@@@@@@@@@@@@@@                                      |
[8, 16)                2 |@@@@@@@                                             |
[16, 32)               4 |@@@@@@@@@@@@@@                                      |
[32, 64)               5 |@@@@@@@@@@@@@@@@@@                                  |
[64, 128)              0 |                                                    |
[128, 256)             0 |                                                    |
[256, 512)             1 |@@@                                                 |
[512, 1K)              0 |                                                    |
[1K, 2K)               0 |                                                    |
[2K, 4K)               0 |                                                    |
[4K, 8K)               0 |                                                    |
[8K, 16K)              0 |                                                    |
[16K, 32K)             0 |                                                    |
[32K, 64K)             0 |                                                    |
[64K, 128K)            0 |                                                    |
[128K, 256K)           0 |                                                    |
[256K, 512K)           0 |                                                    |
[512K, 1M)             0 |                                                    |
[1M, 2M)               0 |                                                    |
[2M, 4M)               0 |                                                    |
[4M, 8M)               0 |                                                    |
[8M, 16M)              1 |@@@                                                 |
```
We can see that the kernel system calls are in general much faster.

Here is my bpftrace script:
```sh
tracepoint:raw_syscalls:sys_enter
/ args->id == 43 || args->id == 288 / // 43=connect, 288=accept4 (Common IDs for x86_64)
{
    @start[tid] = nsecs;
}

tracepoint:raw_syscalls:sys_exit
/ @start[tid] && (args->id == 43 || args->id == 288) /
{
    $latency_us = (nsecs - @start[tid]) / 1000;
    @connect_accept_latency_us = hist($latency_us);
    delete(@start[tid]);
}

interval:s:30 {
    print("--- Connect/Accept Latency Distribution (us) ---");
    print(@connect_accept_latency_us);
    clear(@connect_accept_latency_us);
}
```

### 1.4. Adding `printk`

![](attachment/b64cd73a23f14fed4f45b707d4abf05b.png)
As we can see from the middle pane, the messages are printed to the kernel buffer. As a side note we can see that there are often only a few microseconds between when an accept call is made but about 1 milliseconds (1000 microseconds) between when a connect call is made and an accept call is made.


## 2. Syscall Extension

### 2.1. Implementing the system call