/*
 * Copyright (C) 2019 ML!PA Consulting GmbH
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     examples
 * @{
 *
 * @file
 * @brief       Application to test different PHY modulations
 *
 * @author      Benjamin Valentin <benjamin.valentin@ml-pa.com>
 *
 * @}
 */

#include <stdio.h>
#include <string.h>

#include "thread.h"
#include "mutex.h"
#include "net/gnrc.h"

#include "shell.h"
#include "shell_commands.h"
#include "range_test.h"

#ifdef MODULE_NETDEV_IEEE802154_MR_OFDM
#define TEST_OFDM
#endif

#ifdef MODULE_NETDEV_IEEE802154_MR_OQPSK
#define TEST_OQPSK
#endif

#ifdef MODULE_NETDEV_IEEE802154_OQPSK
#define TEST_LEGCAY_OQPSK
#endif

#ifdef MODULE_NETDEV_IEEE802154_MR_FSK
#define TEST_FSK
#endif

typedef struct {
    netopt_t opt;
    uint32_t data;
    size_t   data_len;
} netopt_val_t;

typedef struct {
    const char name[32];
    size_t opt_num;
    netopt_val_t opt[6];
} netopt_setting_t;

typedef struct {
    const char name[32];
    netopt_t opt;
    size_t data_len;
    uint8_t num_settings;
    struct {
        const char name[32];
        uint32_t data;
    } settings[];
} netopt_list_t;

__attribute__((unused))
static int _print(char *str, size_t len, unsigned idx);

#ifdef TEST_OFDM
static const netopt_list_t ofdm_options = {
    .name = "option",
    .opt  = NETOPT_MR_OFDM_OPTION,
    .data_len = 1,
    .num_settings = 4,
    .settings = {
        {
            .name = "1",
            .data = 1
        },
        {
            .name = "2",
            .data = 2
        },
        {
            .name = "3",
            .data = 3
        },
        {
            .name = "4",
            .data = 4
        },
    },
};

static const netopt_list_t ofdm_mcs = {
    .name = "MCS",
    .opt  = NETOPT_MR_OFDM_MCS,
    .data_len = 1,
    .num_settings = 7,
    .settings = {
        {
            .name = "BPSK, ½ rate, 4x rep",
            .data = 0
        },
        {
            .name = "BPSK, ½ rate, 2x rep",
            .data = 1
        },
        {
            .name = "QPSK, ½ rate, 2x rep",
            .data = 2
        },
        {
            .name = "QPSK, ½ rate",
            .data = 3
        },
        {
            .name = "QPSK, ¾ rate",
            .data = 4
        },
        {
            .name = "16-QAM, ½ rate",
            .data = 5
        },
        {
            .name = "16-QAM, ¾ rate",
            .data = 6
        },
    },
};
#endif /* OFDM */
#ifdef TEST_OQPSK
static const netopt_list_t oqpsk_rates = {
    .name = "rate",
    .opt  = NETOPT_MR_OQPSK_RATE,
    .data_len = 1,
    .num_settings = 4,
    .settings = {
        {
            .name = "0",
            .data = 0
        },
        {
            .name = "1",
            .data = 1
        },
        {
            .name = "2",
            .data = 2
        },
        {
            .name = "3",
            .data = 3
        },
    },
};

static const netopt_list_t oqpsk_chips = {
    .name = "chip/s",
    .opt  = NETOPT_MR_OQPSK_CHIPS,
    .data_len = 2,
    .num_settings = 4,
    .settings = {
        {
            .name = "100k",
            .data = 100
        },
        {
            .name = "200k",
            .data = 200
        },
        {
            .name = "1000k",
            .data = 1000
        },
        {
            .name = "2000k",
            .data = 2000
        },
    },
};
#endif /* O-QPSK */
#ifdef TEST_LEGCAY_OQPSK
static const netopt_list_t legacy_oqpsk_rates = {
    .name = "rate",
    .opt  = NETOPT_OQPSK_RATE,
    .data_len = 1,
    .num_settings = 2,
    .settings = {
        {
            .name = "legacy",
            .data = 0
        },
        {
            .name = "legacy HDR",
            .data = 1
        },
    },
};
#endif /* TEST_LEGCAY_OQPSK */
#ifdef TEST_FSK
static const netopt_list_t fsk_idx = {
    .name = "index",
    .opt  = NETOPT_MR_FSK_MODULATION_INDEX,
    .data_len = 1,
    .num_settings = 8,
    .settings = {
        {
            .name = "3/8",
            .data = 24,
        },
        {
            .name = "1/2",
            .data = 32,
        },
        {
            .name = "3/4",
            .data = 48,
        },
        {
            .name = "1",
            .data = 64,
        },
        {
            .name = "5/4",
            .data = 80,
        },
        {
            .name = "3/2",
            .data = 96,
        },
        {
            .name = "7/4",
            .data = 112,
        },
        {
            .name = "2",
            .data = 128,
        },
    },
};

static const netopt_list_t fsk_srate = {
    .name = "srate",
    .opt  = NETOPT_MR_FSK_SRATE,
    .data_len = 2,
    .num_settings = 6,
    .settings = {
        {
            .name = "50 kHz",
            .data = 50,
        },
        {
            .name = "100 kHz",
            .data = 100,
        },
        {
            .name = "150 kHz",
            .data = 150,
        },
        {
            .name = "200 kHz",
            .data = 200,
        },
        {
            .name = "300 kHz",
            .data = 300,
        },
        {
            .name = "400 kHz",
            .data = 400,
        },
    },
};

static const netopt_list_t fsk_mord = {
    .name = "order",
    .opt  = NETOPT_MR_FSK_MODULATION_ORDER,
    .data_len = 1,
    .num_settings = 2,
    .settings = {
        {
            .name = "2-FSK",
            .data = 2,
        },
        {
            .name = "4-FSK",
            .data = 4,
        },
    },
};

static const netopt_list_t fsk_fec = {
    .name = "FEC",
    .opt  = NETOPT_MR_FSK_FEC,
    .data_len = 1,
    .num_settings = 3,
    .settings = {
        {
            .name = "none",
            .data = IEEE802154_FEC_NONE,
        },
        {
            .name = "RSC",
            .data = IEEE802154_FEC_RSC,
        },
        {
            .name = "NRNSC",
            .data = IEEE802154_FEC_NRNSC,
        },
    },
};
#endif

static uint8_t _payload_idx;
static const uint16_t payloads[] = {
    16, 128, 512, 1024
};
/* tx times based on slowest modulation */
/* since we can't get proper TX confirmatio from RIOT :( */
static const uint32_t max_delay_ms[] = {
    185, 500, 1600, 3000
};

static unsigned idx;
static test_result_t *results[GNRC_NETIF_NUMOF];

#ifdef MODULE_VFS_DEFAULT
#include <fcntl.h>
#include "vfs_default.h"

#ifndef DATA_DIR
#define DATA_DIR VFS_DEFAULT_DATA "/range"
#endif

static int _result_fd;

static ssize_t vfs_write_string(int fd, const char *src)
{
    return vfs_write(fd, src, strlen(src));
}

static void file_store_open(unsigned num)
{
    char buffer[48];

    if (_result_fd != 0) {
        vfs_close(_result_fd);
    }

    vfs_mkdir(DATA_DIR, 0777);
    snprintf(buffer, sizeof(buffer), DATA_DIR "/%u.csv", num);

    _result_fd = vfs_open(buffer, O_CREAT | O_WRONLY, 0644);
    if (_result_fd < 0) {
        printf("can't create file: %d\n", _result_fd);
        return;
    }

    vfs_write_string(_result_fd,
                     "modulation;iface;payload;sent;received;RSSI_local;RSSI_remote;RTT\n");
}

static void file_store_close(void)
{
    if (_result_fd <= 0) {
        return;
    }

    vfs_close(_result_fd);
    _result_fd = 0;
}

static void file_store_add(unsigned iface, test_result_t *result)
{
    static char line[128];

    if (_result_fd <= 0) {
        return;
    }

    if (result->invalid) {
        return;
    }

    line[0] = '"';
    int res = _print(&line[1], sizeof(line) - 1, idx) + 1;

    snprintf(&line[res], sizeof(line) - res, "\";%u;%u;%u;%u;%d;%d;%u\n",
             iface,
             result->payload_size,
             result->pkts_send,
             result->pkts_rcvd,
             (int)(result->rssi_sum[0] / result->pkts_rcvd),
             (int)(result->rssi_sum[1] / result->pkts_rcvd),
             (unsigned)result->rtt_ticks);
    vfs_write_string(_result_fd, line);
}
#else
static inline void file_store_open(unsigned num) { (void)num; }
static inline void file_store_close(void) {}
static inline void file_store_add(unsigned iface, test_result_t *result)
{
    (void)iface;
    (void)result;
}
#endif

static void _netapi_set_forall(netopt_t opt, const void *data, size_t data_len)
{
    unsigned i = 0;
    for (unsigned pid = range_test_radio_pid(); i < range_test_radio_numof(); ++pid) {
        int res;
        while ((res = gnrc_netapi_set(pid, opt, 0, data, data_len)) == -EBUSY) {
            /* at86rf215 driver needs some time in busy state */
            xtimer_msleep(1);
        }
        if (res < 0) {
            printf("[%d] failed setting %x to %x\n", pid, opt, *(uint8_t*) data);

            if (results[i]) {
                for (unsigned j = 0; j < ARRAY_SIZE(payloads); ++j) {
                    results[i][idx + j].invalid = true;
                }
            }
        }
        ++i;
    }
}

static void _set_from_netopt_list(const netopt_list_t *l, unsigned idx, bool do_set) {
    printf("%s = %s", l->name, l->settings[idx].name);

    if (do_set) {
        _netapi_set_forall(l->opt, &l->settings[idx].data, l->data_len);
    }
}

static int _print_from_netopt_list(char *str, size_t size, const netopt_list_t *l, unsigned idx)
{
    return snprintf(str, size, "%s = %s", l->name, l->settings[idx].name);
}

static int _advance_str(char **str, size_t *len, int res)
{
    *str += res;
    *len -= res;
    return res;
}

#ifdef TEST_OFDM
static unsigned _get_OFDM_combinations(void)
{
    static unsigned combinations;

    if (combinations == 0) {
        combinations = ofdm_options.num_settings * ofdm_mcs.num_settings;
    }

    return combinations;
}

static int _set_OFDM(unsigned setting, bool do_set)
{
    for (unsigned i = 0; i < ofdm_options.num_settings; ++i) {
        for (unsigned j = 0; j < ofdm_mcs.num_settings; ++j) {
            if (setting-- == 0) {
                printf("OFDM ");
                _set_from_netopt_list(&ofdm_options, i, do_set);
                printf(", ");
                _set_from_netopt_list(&ofdm_mcs, j, do_set);
                return 0;
            }
        }
    }

    return -1;
}

static int _print_OFDM(char *str, size_t len, unsigned setting)
{
    for (unsigned i = 0; i < ofdm_options.num_settings; ++i) {
        for (unsigned j = 0; j < ofdm_mcs.num_settings; ++j) {
            if (setting-- == 0) {
                int res, total = 0;
                res = snprintf(str, len, "OFDM ");
                total += _advance_str(&str, &len, res);
                res = _print_from_netopt_list(str, len, &ofdm_options, i);
                total += _advance_str(&str, &len, res);
                res = snprintf(str, len, ", ");
                total += _advance_str(&str, &len, res);
                res = _print_from_netopt_list(str, len, &ofdm_mcs, j);
                total += _advance_str(&str, &len, res);
                return total;
            }
        }
    }

    return 0;
}
#else
static unsigned _get_OFDM_combinations(void)
{
    return 0;
}

static int _print_OFDM(char *str, size_t len, unsigned setting)
{
    (void)str;
    (void)len;
    (void)setting;

    return 0;
}

static int _set_OFDM(unsigned setting, bool do_set)
{
    (void) setting;
    (void) do_set;

    return -1;
}
#endif  /* OFDM */

#ifdef TEST_OQPSK
static unsigned _get_OQPSK_combinations(void)
{
    static unsigned combinations;

    if (combinations == 0) {
        combinations = oqpsk_rates.num_settings * oqpsk_chips.num_settings;
    }

    return combinations;
}

static int _set_OQPSK(unsigned setting, bool do_set)
{
    for (unsigned i = 0; i < oqpsk_rates.num_settings; ++i) {
        for (unsigned j = 0; j < oqpsk_chips.num_settings; ++j) {
            if (setting-- == 0) {
                printf("O-QPSK ");
                _set_from_netopt_list(&oqpsk_rates, i, do_set);
                printf(", ");
                _set_from_netopt_list(&oqpsk_chips, j, do_set);
                return 0;
            }
        }
    }

    return -1;
}

static int _print_OQPSK(char *str, size_t len, unsigned setting)
{
    for (unsigned i = 0; i < oqpsk_rates.num_settings; ++i) {
        for (unsigned j = 0; j < oqpsk_chips.num_settings; ++j) {
            if (setting-- == 0) {
                int res, total = 0;
                res = snprintf(str, len, "O-QPSK ");
                total += _advance_str(&str, &len, res);
                res = _print_from_netopt_list(str, len, &oqpsk_rates, i);
                total += _advance_str(&str, &len, res);
                res = snprintf(str, len, ", ");
                total += _advance_str(&str, &len, res);
                res = _print_from_netopt_list(str, len, &oqpsk_chips, j);
                total += _advance_str(&str, &len, res);
                return total;
            }
        }
    }

    return 0;
}
#else
static unsigned _get_OQPSK_combinations(void)
{
    return 0;
}

static int _print_OQPSK(char *str, size_t len, unsigned setting)
{
    (void)str;
    (void)len;
    (void)setting;

    return 0;
}

static int _set_OQPSK(unsigned setting, bool do_set)
{
    (void) setting;
    (void) do_set;

    return -1;
}
#endif  /* O-QPSK */

#ifdef TEST_LEGCAY_OQPSK
static unsigned _get_legacy_OQPSK_combinations(void)
{
    return legacy_oqpsk_rates.num_settings;
}

static int _set_legacy_OQPSK(unsigned setting, bool do_set)
{
    for (unsigned i = 0; i < legacy_oqpsk_rates.num_settings; ++i) {
        if (setting-- == 0) {
            printf("O-QPSK ");
            _set_from_netopt_list(&legacy_oqpsk_rates, i, do_set);
            return 0;
        }
    }

    return -1;
}

static int _print_legacy_OQPSK(char *str, size_t len, unsigned setting)
{
    for (unsigned i = 0; i < legacy_oqpsk_rates.num_settings; ++i) {
        if (setting-- == 0) {
                int res, total = 0;
                res = snprintf(str, len, "O-QPSK ");
                total += _advance_str(&str, &len, res);
                res = _print_from_netopt_list(str, len, &legacy_oqpsk_rates, i);
                total += _advance_str(&str, &len, res);
                return total;
        }
    }

    return 0;
}

#else
static int _print_legacy_OQPSK(char *str, size_t len, unsigned setting)
{
    (void)str;
    (void)len;
    (void)setting;

    return 0;
}

static unsigned _get_legacy_OQPSK_combinations(void)
{
    return 0;
}

static int _set_legacy_OQPSK(unsigned setting, bool do_set)
{
    (void) setting;
    (void) do_set;

    return -1;
}
#endif  /* legacy O-QPSK */

#ifdef TEST_FSK
static unsigned _get_FSK_combinations(void)
{
    static unsigned combinations;

    if (combinations == 0) {
        combinations = fsk_fec.num_settings * fsk_mord.num_settings
                     * fsk_srate.num_settings * fsk_idx.num_settings;
    }

    return combinations;
}

static int _set_FSK(unsigned setting, bool do_set)
{
    for (unsigned i = 0; i < fsk_srate.num_settings; ++i) {
        for (unsigned j = 0; j < fsk_idx.num_settings; ++j) {
            for (unsigned k = 0; k < fsk_mord.num_settings; ++k) {
                for (unsigned l = 0; l < fsk_fec.num_settings; ++l) {
                    if (setting-- == 0) {
                        printf("FSK ");
                        _set_from_netopt_list(&fsk_srate, i, do_set);
                        printf(", ");
                        _set_from_netopt_list(&fsk_idx, j, do_set);
                        printf(", ");
                        _set_from_netopt_list(&fsk_mord, k, do_set);
                        printf(", ");
                        _set_from_netopt_list(&fsk_fec, l, do_set);
                        return 0;
                    }
                }
            }
        }
    }

    return -1;
}

static int _print_FSK(char *str, size_t len, unsigned setting)
{
    for (unsigned i = 0; i < oqpsk_rates.num_settings; ++i) {
        for (unsigned j = 0; j < oqpsk_chips.num_settings; ++j) {
            for (unsigned k = 0; k < fsk_mord.num_settings; ++k) {
                for (unsigned l = 0; l < fsk_fec.num_settings; ++l) {
                    if (setting-- == 0) {
                        int res, total = 0;
                        res = snprintf(str, len, "FSK ");
                        total += _advance_str(&str, &len, res);
                        res = _print_from_netopt_list(str, len, &fsk_srate, i);
                        total += _advance_str(&str, &len, res);
                        res = snprintf(str, len, ", ");
                        total += _advance_str(&str, &len, res);
                        res = _print_from_netopt_list(str, len, &fsk_idx, j);
                        total += _advance_str(&str, &len, res);
                        res = snprintf(str, len, ", ");
                        total += _advance_str(&str, &len, res);
                        res = _print_from_netopt_list(str, len, &fsk_mord, k);
                        total += _advance_str(&str, &len, res);
                        res = snprintf(str, len, ", ");
                        total += _advance_str(&str, &len, res);
                        res = _print_from_netopt_list(str, len, &fsk_fec, l);
                        total += _advance_str(&str, &len, res);
                        return total;
                    }
                }
            }
        }
    }

    return 0;
}
#else
static unsigned _get_FSK_combinations(void)
{
    return 0;
}

static int _set_FSK(unsigned setting, bool do_set)
{
    (void) setting;
    (void) do_set;

    return -1;
}

static int _print_FSK(char *str, size_t len, unsigned setting)
{
    (void)str;
    (void)len;
    (void)setting;

    return 0;
}
#endif  /* OFDM */

static unsigned _get_combinations(void)
{
    return _get_OFDM_combinations() + _get_OQPSK_combinations()
         + _get_legacy_OQPSK_combinations() + _get_FSK_combinations();
}

static int _set(unsigned idx, bool do_set)
{
    if (idx < _get_OQPSK_combinations()) {
        return _set_OQPSK(idx, do_set);
    } else {
        idx -= _get_OQPSK_combinations();
    }

    if (idx < _get_legacy_OQPSK_combinations()) {
        return _set_legacy_OQPSK(idx, do_set);
    } else {
        idx -= _get_legacy_OQPSK_combinations();
    }

    if (idx < _get_OFDM_combinations()) {
        return _set_OFDM(idx, do_set);
    } else {
        idx -= _get_OFDM_combinations();
    }

    if (idx < _get_FSK_combinations()) {
        return _set_FSK(idx, do_set);
    } else {
        idx -= _get_FSK_combinations();
    }

    return -1;
}

static int _print(char *str, size_t len, unsigned idx)
{
    if (idx < _get_OQPSK_combinations()) {
        return _print_OQPSK(str, len, idx);
    } else {
        idx -= _get_OQPSK_combinations();
    }

    if (idx < _get_legacy_OQPSK_combinations()) {
        return _print_legacy_OQPSK(str, len, idx);
    } else {
        idx -= _get_legacy_OQPSK_combinations();
    }

    if (idx < _get_OFDM_combinations()) {
        return _print_OFDM(str, len, idx);
    } else {
        idx -= _get_OFDM_combinations();
    }

    if (idx < _get_FSK_combinations()) {
        return _print_FSK(str, len, idx);
    } else {
        idx -= _get_FSK_combinations();
    }

    return 0;
}

static void _set_modulation(unsigned idx)
{
    printf("[%d] Set ", idx);

#ifdef TEST_OQPSK
    if (idx == 0) {
        uint32_t data = IEEE802154_PHY_MR_OQPSK;
        _netapi_set_forall(NETOPT_IEEE802154_PHY, &data, 1);
    }
#endif
#ifdef TEST_LEGCAY_OQPSK
    if (idx == _get_OQPSK_combinations()) {
        uint32_t data = IEEE802154_PHY_OQPSK;
        _netapi_set_forall(NETOPT_IEEE802154_PHY, &data, 1);
    }
#endif
#ifdef TEST_OFDM
    if (idx == _get_OQPSK_combinations() + _get_legacy_OQPSK_combinations()) {
        uint32_t data = IEEE802154_PHY_MR_OFDM;
        _netapi_set_forall(NETOPT_IEEE802154_PHY, &data, 1);
    }
#endif
#ifdef TEST_FSK
    if (idx == _get_OFDM_combinations() + _get_OQPSK_combinations() + _get_legacy_OQPSK_combinations()) {
        uint32_t data = IEEE802154_PHY_MR_FSK;
        _netapi_set_forall(NETOPT_IEEE802154_PHY, &data, 1);
    }
#endif

    _set(idx, true);

    puts("");
}

void range_test_begin_measurement(kernel_pid_t netif)
{
    unsigned _idx = idx * ARRAY_SIZE(payloads) + _payload_idx;
    netif -= range_test_radio_pid();

    if (results[netif] == NULL) {
        results[netif] = calloc(_get_combinations() * ARRAY_SIZE(payloads), sizeof(test_result_t));
        if (results[netif] == NULL) {
            puts("Out of memory!");
            return;
        }
    }

    results[netif][_idx].pkts_send++;
    if (results[netif][_idx].rtt_ticks == 0) {
        results[netif][_idx].rtt_ticks = max_delay_ms[_payload_idx] * US_PER_MS;
    }
}

uint32_t range_test_get_timeout(kernel_pid_t netif)
{
    netif -= range_test_radio_pid();

    unsigned _idx = idx * ARRAY_SIZE(payloads) + _payload_idx;
    uint32_t t = results[netif][_idx].rtt_ticks
               + results[netif][_idx].rtt_ticks / 10;

    return t;
}

void range_test_add_measurement(kernel_pid_t netif, uint32_t ticks,
                                int rssi_local, int rssi_remote,
                                unsigned lqi_local, unsigned lqi_remote,
                                uint16_t payload_size)
{
    unsigned _idx = idx * ARRAY_SIZE(payloads) + _payload_idx;
    netif -= range_test_radio_pid();

    results[netif][_idx].pkts_rcvd++;
    results[netif][_idx].rssi_sum[0] += rssi_local;
    results[netif][_idx].rssi_sum[1] += rssi_remote;
    results[netif][_idx].lqi_sum[0] += lqi_local;
    results[netif][_idx].lqi_sum[1] += lqi_remote;
    results[netif][_idx].rtt_ticks = ticks;
    results[netif][_idx].payload_size = payload_size;
}

void range_test_print_results(void)
{
    printf("modulation;payload;iface;sent;received;LQI_local;LQI_remote;RSSI_local;RSSI_remote;RTT\n");
    for (unsigned i = 0; i < _get_combinations() * ARRAY_SIZE(payloads); ++i) {
        for (unsigned j = 0; j < range_test_radio_numof(); ++j) {
            uint32_t ticks = results[j][i].rtt_ticks;

            printf("\"");
            _set(i / ARRAY_SIZE(payloads), false);
            printf("\";");

            if (results[j][i].invalid) {
                puts(" INVALID");
            } else {
                printf("%d;", j);
                printf("%d;", results[j][i].payload_size);
                printf("%d;", results[j][i].pkts_send);
                printf("%d;", results[j][i].pkts_rcvd);
                printf("%ld;", results[j][i].lqi_sum[0] / results[j][i].pkts_rcvd);
                printf("%ld;", results[j][i].lqi_sum[1] / results[j][i].pkts_rcvd);
                printf("%ld;", results[j][i].rssi_sum[0] / results[j][i].pkts_rcvd);
                printf("%ld;", results[j][i].rssi_sum[1] / results[j][i].pkts_rcvd);
                printf("%ld", xtimer_usec_from_ticks(ticks));
                printf("\t|\t%d %%", (100 * results[j][i].pkts_rcvd) / results[j][i].pkts_send);
                printf(" max = %lu byte/s", (results[j][i].payload_size * US_PER_SEC) / ticks);
                printf(" avg = %lu byte/s", (results[j][i].pkts_rcvd * results[j][i].payload_size * 1000) /
                                            range_test_period_ms());
                puts("");
            }

            memset(&results[j][i], 0, sizeof(results[j][i]));
        }
    }

    range_test_start();
}

uint16_t range_test_payload_size(void)
{
    return payloads[_payload_idx];
}

bool range_test_set_next_modulation(void)
{
    for (unsigned i = 0; i < range_test_radio_numof(); ++i) {
        file_store_add(i, &results[i][idx * ARRAY_SIZE(payloads) + _payload_idx]);
    }

    if (++_payload_idx < ARRAY_SIZE(payloads)) {
        printf("\tusing %u byte payload\n", range_test_payload_size());
        return true;
    }
    _payload_idx = 0;

    if (++idx >= _get_combinations()) {
        return false;
    }

    _set_modulation(idx);

    return true;
}

void range_test_init(void)
{
    idx = 0;
    netopt_enable_t disable = NETOPT_DISABLE;
    _netapi_set_forall(NETOPT_ACK_REQ, &disable, sizeof(disable));

    LED0_OFF;

    _set_modulation(idx);
}

void range_test_start(void)
{
    static unsigned count;
    file_store_open(count++);
}

void range_test_end(void)
{
    file_store_close();
    LED0_OFF;
}
