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

#define INITIAL_FRAME_DELAY_US  (200000) /* 200 ms */

#define TEST_OFDM
#define TEST_OQPSK
#define TEST_LEGCAY_OQPSK
#define TEST_FSK

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

static unsigned idx;
static test_result_t *results[GNRC_NETIF_NUMOF];

static void _netapi_set_forall(netopt_t opt, const void *data, size_t data_len)
{
    unsigned i = 0;
    for (gnrc_netif_t *netif = gnrc_netif_iter(NULL); netif; netif = gnrc_netif_iter(netif)) {
        if (gnrc_netapi_set(netif->pid, opt, 0, data, data_len) < 0) {
            printf("[%d] failed setting %x to %x\n", netif->pid, opt, *(uint8_t*) data);

            if (results[i]) {
                results[i][idx].invalid = true;
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
#else
static unsigned _get_OFDM_combinations(void)
{
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
#else
static unsigned _get_OQPSK_combinations(void)
{
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
#else
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
    netif -= 6; // XXX

    if (results[netif] == NULL) {
        results[netif] = calloc(_get_combinations(), sizeof(test_result_t));
    }

    results[netif][idx].pkts_send++;
    if (results[netif][idx].rtt_ticks == 0) {
        results[netif][idx].rtt_ticks = xtimer_ticks_from_usec(INITIAL_FRAME_DELAY_US).ticks32;
    }
}

xtimer_ticks32_t range_test_get_timeout(kernel_pid_t netif)
{
    netif -= 6; // XXX
    xtimer_ticks32_t t = {
        .ticks32 = results[netif][idx].rtt_ticks + results[netif][idx].rtt_ticks / 10
    };

    return t;
}

void range_test_add_measurement(kernel_pid_t netif, int rssi_local, int rssi_remote, uint32_t ticks)
{
    netif -= 6; // XXX
    results[netif][idx].pkts_rcvd++;
    results[netif][idx].rssi_sum[0] += rssi_local;
    results[netif][idx].rssi_sum[1] += rssi_remote;
    results[netif][idx].rtt_ticks = (results[netif][idx].rtt_ticks + ticks) / 2;
}

void range_test_print_results(void)
{
    printf("modulation;iface;sent;received;RSSI_local;RSSI_remote;RTT\n");
    for (unsigned i = 0; i < _get_combinations(); ++i) {
        for (int j = 0; j < GNRC_NETIF_NUMOF; ++j) {
            xtimer_ticks32_t ticks = {
                .ticks32 = results[j][i].rtt_ticks
            };

            printf("\"");
            _set(i, false);
            printf("\";");

            if (results[j][i].invalid) {
                puts(" INVALID");
            } else {
                printf("%d;", j);
                printf("%d;", results[j][i].pkts_send);
                printf("%d;", results[j][i].pkts_rcvd);
                printf("%ld;", results[j][i].rssi_sum[0] / results[j][i].pkts_rcvd);
                printf("%ld;", results[j][i].rssi_sum[1] / results[j][i].pkts_rcvd);
                printf("%ld", xtimer_usec_from_ticks(ticks));
                printf("\t|\t%d %%\n", (100 * results[j][i].pkts_rcvd) / results[j][i].pkts_send);
            }

            memset(&results[j][i], 0, sizeof(results[j][i]));
        }
    }

    range_test_start();
}

bool range_test_set_next_modulation(void)
{
    if (++idx >= _get_combinations()) {
        return false;
    }

    _set_modulation(idx);

    return true;
}

void range_test_start(void)
{
    idx = 0;
    netopt_enable_t disable = NETOPT_DISABLE;
    _netapi_set_forall(NETOPT_ACK_REQ, &disable, sizeof(disable));

    _set_modulation(idx);
}
