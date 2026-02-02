# Crash Recovery

The implementation is based on

```
@misc{cryptoeprint:2019/506,
      author = {Erik-Oliver Blass and Guevara Noubir},
      title = {Forward Security with Crash Recovery for Secure Logs},
      howpublished = {Cryptology {ePrint} Archive, Paper 2019/506},
      year = {2019},
      url = {https://eprint.iacr.org/2019/506}
}
```

- Crash Recovery is provided parallel to secure-logging.
- Only the syslog-ng.conf defines which logging variant shall be used.
- Different to secure-logging: Crash Recovery is based on a destination driver with its own log rotation.


For details of Crash Recovery see modules/secure-logging/crashrecovery/README.md



# Example syslog-ng.conf for Crash Recovery

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

