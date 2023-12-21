/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @brief phy init config parameters. These are passed to phy at init.
 */

#ifndef __PACK_DEF__
#define __PACK_DEF__

#define __NRF_WIFI_PKD __attribute__((__packed__))
#define __NRF_WIFI_ALIGN_4 __attribute__((__aligned__(4)))
#define __NRF_WIFI_PKD_ALIGN_4 __attribute__((__packed__, __aligned__(4)))

#endif
