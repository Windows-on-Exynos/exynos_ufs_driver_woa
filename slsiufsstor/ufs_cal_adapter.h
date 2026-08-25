#pragma once

#include <ntddk.h>

typedef UCHAR u8;
typedef ULONG u32;

struct uic_pwr_mode {
    u8 lane;
    u8 gear;
    u8 mode;
    u8 hs_series;
};

struct ufs_cal_param {
    void *host;
    u8 available_lane;
    u8 target_lane;
    u32 mclk_rate;
    u8 board;
    struct uic_pwr_mode *pmd;
};

typedef enum {
    UFS_CAL_NO_ERROR = 0,
    UFS_CAL_TIMEOUT,
    UFS_CAL_ERROR,
    UFS_CAL_INV_ARG,
} ufs_cal_errno;

enum {
    __BRD_SMDK,
    __BRD_UNIV,
};

#define BRD_SMDK (1U << __BRD_SMDK)
#define BRD_UNIV (1U << __BRD_UNIV)

ufs_cal_errno ufs_cal_post_h8_enter(struct ufs_cal_param *p);
ufs_cal_errno ufs_cal_pre_h8_exit(struct ufs_cal_param *p);
ufs_cal_errno ufs_cal_post_pmc(struct ufs_cal_param *p);
ufs_cal_errno ufs_cal_pre_pmc(struct ufs_cal_param *p);
ufs_cal_errno ufs_cal_post_link(struct ufs_cal_param *p);
ufs_cal_errno ufs_cal_pre_link(struct ufs_cal_param *p);
ufs_cal_errno ufs_cal_init(struct ufs_cal_param *p, int idx);

void ufs_lld_dme_set(void *host, u32 address, u32 value);
void ufs_lld_dme_get(void *host, u32 address, u32 *value);
void ufs_lld_dme_peer_set(void *host, u32 address, u32 value);
void ufs_lld_pma_write(void *host, u32 value, u32 address);
u32 ufs_lld_pma_read(void *host, u32 address);
void ufs_lld_unipro_write(void *host, u32 value, u32 address);
void ufs_lld_udelay(u32 value);
void ufs_lld_usleep_delay(u32 minimum, u32 maximum);
unsigned long ufs_lld_get_time_count(unsigned long offset);
unsigned long ufs_lld_calc_timeout(const unsigned int milliseconds);
