# CRASH RECOVERY

Forward Security with Crash Recovery for Secure Logs is described in the
following paper, referenced as [BN19].

```
@misc{cryptoeprint:2019/506,
      author = {Erik-Oliver Blass and Guevara Noubir},
      title = {Forward Security with Crash Recovery for Secure Logs},
      howpublished = {Cryptology {ePrint} Archive, Paper 2019/506},
      year = {2019},
      url = {https://eprint.iacr.org/2019/506}
}
```

# Git Repositories

The original source code in C++ for MacOS from Florian Hördt's master thesis is available here:
```
[git repository Master Thesis](https://github.com/Genfood/secure-logging-cr/)
```

The current C implementation lives currently in
```
[git repository fork of syslog-ng] (https://github.com/nbsd/syslog-ng/tree/slog-crash-recovery)
```

# Standalone cli tools

This example Logger cr_logger is a standalone cli program.
It encrypts a given line orientated plain text file.

The encryption takes place in a way, that the Verifier cr_verifier can not only restore the
original plain text, it can also detect manipulations and can also compensate a specific amount of
manipulations, see [BN19].

Both Logger and Verifier to use the same initial key. Currently the input log file does not need
to be conform with a specification like RFC 5424. The only limitation is, that the byte length of
each line will be limited to 2048 octets (bytes). UTF-8 symbols are supported.
NOTE: One UTF-8 symbol can consist of up to 4 octets.

Later this way of logging will be integrated in syslog-ng together with a log rotation.

Note: Here in this context, cr stands for Crash Recovery and has nothing to do with Criterion unit test utility macros.


## Crash Recovery Logger (cr_logger)

Converts a plain log file into a encrypted crash recovery log file.

```
./cr_logger --help
Usage:
  cr_logger [OPTION…] - logger (Crash Recovery)

Help Options:
  -h, --help        Show help options

Application Options:
  -k, --key         Full file name (path) of the master key
  -i, --in          Full file name (path) of the line-oriented input plain text file
  -o, --out         Full file name (path) of the encrypted output file
  -m, --maxlogs     Number of log lines the original plain log file provides
```


## Crash Recovery Verifier (cr_verifier)

The example Verifier  cr_verifier is a cli program that decryptes the output file from the cr_logger,
and detects manipulation.

```
./cr_verifier --help
Usage:
  cr_verifier [OPTION…] - verifier (Crash Recovery)

Optimized for CPU instruction set SSE2

Help Options:
  -h, --help        Show help options

Application Options:
  -k, --key         Full file name (path) of the master key
  -i, --in          Full file name (path) of encrypted log file
  -o, --out         Full file name (path) of decrypted log file
  -m, --maxlogs     Number of log lines the original plain log file provides
```

# Build of Crash Recovery utils and syslog-ng integration

Both applications are build when module secure-logging is build.

Build configurations are provided for Autotools and CMake.

To build, check and install Crash Recovery along syslog-ng, the following
build scripts can be used:
- module/secure-logging/tests/run_cmake_rebuild.sh
- module/secure-logging/tests/run_autotools_rebuild.sh

## Build Aliase

See also module/secure-logging/tests/README.md

alias runauto='(
 cd ${SYSLOG_DIR}/modules/secure-logging/tests/ &&
 ${SYSLOG_DIR}/modules/secure-logging/tests/run_autotools_rebuild.sh
)'

alias runcmake='(
 cd ${SYSLOG_DIR}/modules/secure-logging/tests/ &&
 ${SYSLOG_DIR}/modules/secure-logging/tests/run_cmake_rebuild.sh
)'

alias runclean='(
 cd ${SYSLOG_DIR}/modules/secure-logging/tests/ &&
 ${SYSLOG_DIR}/modules/secure-logging/tests/run_clean.sh
)'

alias stylesl='(
  cd ${SYSLOG_DIR} &&
  ${SYSLOG_DIR}/scripts/style-checker.sh format
)'


## Preprocessors

Here in syslog-ng, the CPU is configured by the following preprocessors in
header <syslog-nb>\modules\secure-logging\crashrecovery\cr_verifier\cr_plain_gauss_helper.h

One and only 1 of the 3 preprocessors must be set to 1 and the other must be set to 0.

- CPU_AVX2: Modern Intel x86_64 CPU with AVX2 support
- CPU_SSE2: Common Intel x86_64 CPU with SSE2 support (only 5% less performant compared to AVX2 but
avialable on most Intel CPUs)
- CPU_OTHER: Used for older Intel CPUs, ARM CPU or ARM64 CPU (Raspberry Pi)

### Default CPU configuration when \#if defined(__x86_64__)
```
\#define CPU_AVX2  0
\#define CPU_SSE2  1
\#define CPU_OTHER 0
```
For ARM and ARM64 no optimization is provided currently.
These are the  preprocessors used for ARM CPUs:
```
\#define CPU_AVX2  0
\#define CPU_SSE2  0
\#define CPU_OTHER 1
```

## build scripts

Run from  project root directoy one of the build scripts:
 **run_cmake_rebuild.sh** or **run_automake_rebuild.sh**.
Note: Existing binaries will be deleted and created newly.

Here the PREFIX for build configuration is "$HOME/Software/install".


## Aliases (test driven development, runtime impact)

```
SYSLOG_DIR="$HOME/Software/syslog-ng"
export SYSLOG_DIR

TESTBAK_DIR="$HOME/test_slog"
export TESTBAK_DIR

SW_INSTALL_DIR="$HOME/Software/install"
export SW_INSTALL_DIR
``` 

```
# -- Crash Recovery Standalone -----
# --- log file with 5000 lines
alias crlog='${SW_INSTALL_DIR}/bin/cr_logger \
--key "${TESTBAK_DIR}/cr/master.key" \
--in "${TESTBAK_DIR}/cr/msg5000.txt" \
--out "${TESTBAK_DIR}/cr/msg5000.enc" \
--maxlogs 5000'
```

```
alias crverify='${SW_INSTALL_DIR}/bin/cr_verifier \
--key "${TESTBAK_DIR}/cr/master.key" \
--in "${TESTBAK_DIR}/cr/msg5000.enc" \
--out "${TESTBAK_DIR}/cr/msg5000_verifier.txt" \
--maxlogs 5000'
```

The test folder, here $HOME/test_slog/cr/, provides a plain text file with
5000 lines each line is shorter than 2048 characters/symbols.

Once the aliases crlog and crverify have been provided in ~/.bashrc,
just call them from command line.

**crlog** will read the input file msg5000.txt and create the encrypted
version of if as msg5000.enc.

**crverify** will read the input file msg5000.enc and in case the file has not
been modified, the output file msg5000_verifier.txt is the same
as the loggers input file msg5000.txt.

NOTE:
The runtime T grows cubically to the count of input file lines n: T(n) = O(n^3)


# Example Usage of cr_logger and cr_verifier

It might be useful to provide aliases or wrapper scripts.

NOTE: maxlogs is the number of lines in the input file, e.g. 5000 for file msg5000.txt
Both logger and verifier do use the same key.
Both logger and verifier need to know this number currently. Later, the verifier
will be able to get the number of log lines by itself and the parameter will be
removed for the verifier.

# Path

Both programs are located after build in the bin directory specified by the PREFIX.
When
```
PREFIX=$HOME/Software/install
```
then cr_logger and cr_verifier are provided here:
```
 $HOME/Software/install/bin
```

## key

Here the key is just called master.key. In real systems the master key
would never ever by used directly instead a derived host key would be used.
The key can be just a random number.

The use must prvode a 32 byte long key, e.g.:
```
m@air:~/test_slog/cr$ hexdump -C ./master.key
00000000  a7 15 31 f9 be ba c3 69  3c 60 73 b8 e4 41 f0 5a  |..1....i<`s..A.Z|
00000010  ea 28 b1 2b d0 ae 4a 32  ea fd 55 92 fd 94 98 98  |.(.+..J2..U.....|
00000020
m@air:~/test_slog/cr$
```

## Alias crlog -> cr_logger

```
m@air:~/Software/syslog-ng/modules/secure-logging/crashrecovery$ crlog
key (initial key): /home/m/test_slog/cr/master.key
in (plain log file): /home/m/test_slog/cr/msg5000.txt
out (enc log file): /home/m/test_slog/cr/msg5000.enc
maxlogs (count of log lines): 5000
szMasterKeyPath: /home/m/test_slog/cr/master.key
szInputFileName: /home/m/test_slog/cr/msg5000.txt
szOutputFileName: /home/m/test_slog/cr/msg5000.enc
szOutputDir: /home/m/test_slog/cr/
maxlogs: 5000
2026-06-05 10:28:01
ctx->sessionKey (32 bytes):
00000000: a7 15 31 f9 be ba c3 69  3c 60 73 b8 e4 41 f0 5a  |..1....i<`s..A.Z|
00000016: ea 28 b1 2b d0 ae 4a 32  ea fd 55 92 fd 94 98 98  |.(.+..J2..U.....|
masterKey (32 bytes):
00000000: a7 15 31 f9 be ba c3 69  3c 60 73 b8 e4 41 f0 5a  |..1....i<`s..A.Z|
00000016: ea 28 b1 2b d0 ae 4a 32  ea fd 55 92 fd 94 98 98  |.(.+..J2..U.....|
cr_Init, fileSize: 11873664, ctx->m: 5622, LOG_LEN: 2112
gpa_logs->len: 5000

  process line 5000 of 5000

cr_logger: 5000 Logs have been written successfully
Crash Recovery Logger (read + encrypt): 278 milliseconds

2026-06-05 10:28:01
m@air:~/Software/syslog-ng/modules/secure-logging/crashrecovery$ 

```


## Alias crverify -> cr_verifier

```
m@air:~/Software/syslog-ng/modules/secure-logging/crashrecovery$ crverify
key: /home/m/test_slog/cr/master.key
in: /home/m/test_slog/cr/msg5000.enc
out: /home/m/test_slog/cr/msg5000_verifier.txt
maxlogs: 5000
ctx.masterKeyPath: /home/m/test_slog/cr/master.key
ctx.logFileDirectory: /home/m/test_slog/cr/
ctx.inEncFilePath: /home/m/test_slog/cr/msg5000.enc
ctx.outPlainFilePath: /home/m/test_slog/cr/msg5000_verifier.txt
ctx.outProtocolPath: /home/m/test_slog/cr/msg5000.enc.verifier_protocol.txt
ctx.n: 5000
ctx.m: 5622
2026-06-05 10:30:00
Detected 5000 different log entries, line 9
-- GAUSS -----
2026-06-05 10:30:00
cr_pgh_Solve, Mat->rows: 5622, Mat->colsInBits: 5000, Mat->buckets: 20, gpa->len: 5622
cr_pgh_ForwardReduction Mat->rows: 5622, Mat->colsInBits: 5000, Mat->buckets: 20
cr_pgh_ForwardReduction Imat->rows: 5622, Imat->colsInBits: 5622, Imat->buckets: 22
  ForwardReduction 5000 of 5000

-- Gaussian Elimination (Forward Reduction): 147 milliseconds
cr_pgh_RangOf, Mat->rows: 5622, Mat->colsInBits: 5000, Mat->buckets: 20
Detected Rank: 5000
cr_pgh_ApplyBookkeeping Imat->rows: 5622, Imat->colsInBits: 5622, Imat->buckets: 22
cr_pgh_ApplyBookkeeping, gpa->len: 5622
  Bookkeeping 5622 of 5622

-- Gaussian Elimination (Bookkeeping): 339 milliseconds
cr_pgh_BackSubstituion, Mat->rows: 5622, Mat->colsInBits 5000, Mat->buckets: 20
cr_pgh_BackSubstituion, gpa->len: 5622
  BackSubstitution 5000 of 5000

-- Gaussian Elimination (Back Substitution): 84 milliseconds
2026-06-05 10:30:00
ctx.outFile: /home/m/test_slog/cr/msg5000_verifier.txt
line 27, rank: 5000
gpa_c->len: 5622
   decrypted log line 1 of 5000
   decrypted log line 513 of 5000
   decrypted log line 1025 of 5000
   decrypted log line 1537 of 5000
   decrypted log line 2049 of 5000
   decrypted log line 2561 of 5000
   decrypted log line 3073 of 5000
   decrypted log line 3585 of 5000
   decrypted log line 4097 of 5000
   decrypted log line 4609 of 5000
   decrypted log line 5000 of 5000

2026-06-05 10:30:01

-- Clean up ---
cr_Verify, cr_verifySingleLogFile returns res.code: 1, res.success: 1

-- Crash Recovery Verifier (read + decrypt): 739 milliseconds
0 minutes and 1 seconds
active: CPU_SSE2

2026-06-05 10:30:01
m@air:~/Software/syslog-ng/modules/secure-logging/crashrecovery$ 
```

## no tampering or specified number of changes (square root of n)

```
m@air:~/Software/syslog-ng/modules/secure-logging/crashrecovery$ gotest
m@air:~/test_slog$ cd cr
m@air:~/test_slog/cr$ ls
currentSession.key  currentSession.key_2026-06-05T102801.bak  gf1_pg2229.txt  gf2_pg2230.txt  master.key  msg5000.enc  msg5000.enc.verifier_protocol.txt  msg5000.txt  msg5000_verifier.txt
```

```
m@air:~/test_slog/cr$ sha256sum ./msg5000.txt
99bf0928576b549e2913bffcfb3512b91087c919267cb2cba3a9c6b323ff1ce6  ./msg5000.txt
```

```
m@air:~/test_slog/cr$ sha256sum ./msg5000_verifier.txt 
99bf0928576b549e2913bffcfb3512b91087c919267cb2cba3a9c6b323ff1ce6  ./msg5000_verifier.txt
```


# Crash Recovery and syslog-ng

Crash Recovery is provided parallel to secure-logging.

Only the syslog-ng.conf defines which logging variant shall be used.

Diffrent to secure-logging for Crash Recovery a destination driver is used.


## Example Configuration of syslog-ng using Crash Recovery (syslog-ng.conf)

```
@version: 4.8
@include "scl.conf"
@module "cr_destination"

@define mypath "/tmp/test_slog/data"

options {
    flush_lines (0);
    time_reopen (10);
    log_fifo_size (1000);
    chain_hostnames (off);
    use_dns (no);
    use_fqdn (no);
    create_dirs (no);
    keep_hostname (yes);
    use-uniqid(yes);
};

source s_local {
        system();
        internal();
};

source s_network {
        network(
                transport("udp")
                port(7777)
                flags(store-raw-message)
        );
};

# cr_destination provides a log rotation where the log rotates after exact
# count of log entries and this number is defined by logrotcnt(<number>).
#
# logrotcnt's number must be not too small. The greater the number, the
# better recovery works.
# On the other side the logrotcnt number must not be
# huge becasue verification / recovery would take too long.
# see https://eprint.iacr.org/2019/506.pdf, p 22, fig 3

destination d_local_cr {
    cr_destination(
        filenametoken("`mypath`/cr_log"),
        keypath("`mypath`/master.key"),
        dir("`mypath`"),
        mode("plain_enc"), # provide plain logs as well as Crash Recovery encrypted files
        logrotcnt(4096),  # MAX_MESSAGES_PER_FILE
    );
};

log {
        source(s_network);
        destination(d_local_cr);
};

```

# Crash Recovery Source Tree

## Git Branch
slog-crash-recovery

```
-- Git remote repositories
origin	git@github.com:nbsd/syslog-ng.git (fetch)
origin	git@github.com:nbsd/syslog-ng.git (push)
upstream	https://github.com/syslog-ng/syslog-ng.git (fetch)
upstream	https://github.com/syslog-ng/syslog-ng.git (push)
```


## Crash Recovery in filesystem
```
~/Software/syslog-ng/modules/secure-logging/crashrecovery$ tree
.
├── CMakeLists.txt
├── cr_common
│   ├── CMakeLists.txt
│   ├── cr_crypto.c
│   ├── cr_crypto.h
│   ├── cr_pi_shared.c
│   ├── cr_pi_shared.h
│   ├── cr_randomstuff.c
│   ├── cr_randomstuff.h
│   └── Makefile.am
├── cr_destination
│   ├── CMakeLists.txt
│   ├── cr_destination.c
│   ├── cr_destination-grammar.ym
│   ├── cr_destination.h
│   ├── cr_destination-parser.c
│   ├── cr_destination-parser.h
│   ├── cr_destination-plugin.c
│   ├── cr_destination_worker.c
│   ├── cr_destination_worker.h
│   └── Makefile.am
├── cr_logger
│   ├── CMakeLists.txt
│   ├── cr_logger.c
│   ├── cr_pi_logger.c
│   ├── cr_pi_logger_context.h
│   ├── cr_pi_logger.h
│   └── Makefile.am
├── cr_verifier
│   ├── CMakeLists.txt
│   ├── cr_matrix.c
│   ├── cr_matrix.h
│   ├── cr_pi_types.h
│   ├── cr_pi_verifier.c
│   ├── cr_pi_verifier.h
│   ├── cr_plain_gauss_helper.c
│   ├── cr_plain_gauss_helper.h
│   ├── cr_result.h
│   ├── cr_verifier.c
│   └── Makefile.am
├── lib
│   ├── cfg-grammar.y -> ../../../../lib/cfg-grammar.y
│   └── merge-grammar.py -> ../../../../lib/merge-grammar.py
├── Makefile.am
└── README.md
```

## Special Note

The destinatioin cr_destination is not located directly inside modules. 

Instead the destination driver is located in modules/secure-logging/crashrecovery/.

The build system was not able to compile cr_destinamton-grammar.ym (bison, flex) because
cfg-grammar.y and merge-grammar.py where not found.
This is the reason why those files are provided as link.

When creating an archive of the source, ensure that links are vaild when the 
archive is restored. Use tar or use backup script 
modules/secure-logging/tests/tarbackup.sh.


## Special Note 2

The used links let GitHub CI copy right check fail: 

```
Run tests/copyright/check.sh . .
debug: more verbose copyright log in ./copyright-run.log
error: mismatch modules/secure-logging/crashrecovery/lib/merge-grammar.py expected:LGPLv2.1+_SSL,non-balabit detected:LGPLv2.1+_SSL
error: mismatch modules/tarbackup.sh expected:GPLv2+_SSL detected:LGPLv2.1+_SSL,non-balabit
2
Error: Process completed with exit code 1.
```

