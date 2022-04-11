# name of your application
APPLICATION = range_test

# If no BOARD is found in the environment, use this default:
BOARD ?= native

# enable PA
CFLAGS += -DCONFIG_IEEE802154_DEFAULT_TXPOWER=3

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../RIOT

# use the default network interface for the board
USEMODULE += netdev_default
USEMODULE += auto_init_gnrc_netif

USEMODULE += gnrc_ipv6_default
USEMODULE += gnrc_udp

USEMODULE += gnrc_icmpv6_echo
USEMODULE += sema_inv

USEMODULE += ztimer_no_periph_rtt
# USEMODULE += periph_uart_nonblocking

ifeq (same54-xpro, $(BOARD))
  USEMODULE += at86rf215
  USEMODULE += vfs_default
endif

USEMODULE += shell
USEMODULE += shell_commands
USEMODULE += ps

USEMODULE += periph_rtt
USEMODULE += xtimer

USEMODULE += rtt_rtc

# enable to only test sub-GHz radio
#USEMODULE += at86rf215_subghz

# selectively disable modulations to test
#DISABLE_MODULE += netdev_ieee802154_oqpsk
#DISABLE_MODULE += netdev_ieee802154_mr_oqpsk
#DISABLE_MODULE += netdev_ieee802154_mr_ofdm
#DISABLE_MODULE += netdev_ieee802154_mr_fsk

# blocking send can lead to hangs
DISABLE_MODULE += at86rf215_blocking_send

# Comment this out to disable code in RIOT that does safety checking
# which is not needed in a production environment but helps in the
# development process
DEVELHELP ?= 1

# Change this to 0 show compiler invocation lines by default:
QUIET ?= 1

# CFLAGS += -DIEEE802154_DEFAULT_PANID=$(DEFAULT_PAN_ID)

include $(RIOTBASE)/Makefile.include

# Set a custom channel if needed
include $(RIOTMAKE)/default-radio-settings.inc.mk
