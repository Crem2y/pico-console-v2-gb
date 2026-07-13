#include "psram_apsxx04.h"

uint32_t psram_size;
static int _cs_pin = 0;

static void __no_inline_not_in_flash_func(qmi_wait_busy_clear)(void) {
    while (qmi_hw->direct_csr & QMI_DIRECT_CSR_BUSY_BITS) {
        __asm volatile ("nop");
    }
}

static void __no_inline_not_in_flash_func(qmi_wait_tx_empty)(void) {
    while (!(qmi_hw->direct_csr & QMI_DIRECT_CSR_TXEMPTY_BITS)) {
        __asm volatile ("nop");
    }
}

static void __no_inline_not_in_flash_func(psram_direct_cmd_cs1)(uint8_t cmd, uint8_t clkdiv) {
    qmi_hw->direct_csr =
            ((uint32_t)clkdiv << QMI_DIRECT_CSR_CLKDIV_LSB) |
            QMI_DIRECT_CSR_EN_BITS |
            QMI_DIRECT_CSR_AUTO_CS1N_BITS;

    qmi_wait_busy_clear();

    // nopush=1, plain serial tx
    qmi_hw->direct_tx =
            ((uint32_t)cmd) |
            QMI_DIRECT_TX_NOPUSH_BITS;

    qmi_wait_busy_clear();

    qmi_hw->direct_csr = 0;
}

static void __no_inline_not_in_flash_func(apsxx04_calc_timing)(uint32_t sys_hz,
                                uint8_t *clkdiv_out,
                                uint8_t *rxdelay_out,
                                uint8_t *max_select_out,
                                uint8_t *min_deselect_out) {
    uint32_t divisor = (sys_hz + APSXX04_MAX_HZ - 1u) / APSXX04_MAX_HZ;
    if (divisor == 0) divisor = 1;

    // if (divisor == 1 && sys_hz > 100000000u) {
    //     divisor = 2;
    // }

    uint32_t rxdelay = divisor;
    if ((sys_hz / divisor) > 100000000u) {
        rxdelay += 1;
    }

    // max_select <= 8us
    // min_deselect >= 18ns
    uint64_t clock_period_fs = 1000000000000000ull / (uint64_t)sys_hz;
    uint32_t max_select = (uint32_t)((125ull * 1000000ull) / clock_period_fs);
    uint32_t min_deselect = (uint32_t)(
        ((18ull * 1000000ull + (clock_period_fs - 1ull)) / clock_period_fs)
        - ((uint64_t)(divisor + 1u) / 2ull)
    );

    if ((int32_t)min_deselect < 0) min_deselect = 0;
    if (max_select > 0x3f) max_select = 0x3f;
    if (min_deselect > 0x1f) min_deselect = 0x1f;
    if (rxdelay > 0x7) rxdelay = 0x7;
    if (divisor > 0xff) divisor = 0xff;

    *clkdiv_out = (uint8_t)divisor;
    *rxdelay_out = (uint8_t)rxdelay;
    *max_select_out = (uint8_t)max_select;
    *min_deselect_out = (uint8_t)min_deselect;
}

bool __no_inline_not_in_flash_func(apsxx04_init_psram_cs1)(void) {
    gpio_set_function(_cs_pin, GPIO_FUNC_XIP_CS1);
    qmi_wait_busy_clear();
    psram_direct_cmd_cs1(0x35, 10);

    uint8_t clkdiv, rxdelay, max_select, min_deselect;
    apsxx04_calc_timing(clock_get_hz(clk_sys),
                        &clkdiv, &rxdelay, &max_select, &min_deselect);

    // cooldown=1, pagebreak=1024, max_select/min_deselect/rxdelay/clkdiv
    qmi_hw->m[1].timing =
            (1u << QMI_M0_TIMING_COOLDOWN_LSB) |
            (3u << QMI_M0_TIMING_PAGEBREAK_LSB) |   // 1024-byte pagebreak
            ((uint32_t)max_select   << QMI_M0_TIMING_MAX_SELECT_LSB) |
            ((uint32_t)min_deselect << QMI_M0_TIMING_MIN_DESELECT_LSB) |
            ((uint32_t)rxdelay      << QMI_M0_TIMING_RXDELAY_LSB) |
            ((uint32_t)clkdiv       << QMI_M0_TIMING_CLKDIV_LSB);

    // prefix_len = 8, dummy_len = 24, read cmd = 0xEB
    qmi_hw->m[1].rfmt =
            (QMI_M0_RFMT_PREFIX_WIDTH_VALUE_Q << QMI_M0_RFMT_PREFIX_WIDTH_LSB) |
            (QMI_M0_RFMT_ADDR_WIDTH_VALUE_Q   << QMI_M0_RFMT_ADDR_WIDTH_LSB)   |
            (QMI_M0_RFMT_SUFFIX_WIDTH_VALUE_Q << QMI_M0_RFMT_SUFFIX_WIDTH_LSB) |
            (QMI_M0_RFMT_DUMMY_WIDTH_VALUE_Q  << QMI_M0_RFMT_DUMMY_WIDTH_LSB)  |
            (QMI_M0_RFMT_DATA_WIDTH_VALUE_Q   << QMI_M0_RFMT_DATA_WIDTH_LSB)   |
            (QMI_M0_RFMT_PREFIX_LEN_VALUE_8   << QMI_M0_RFMT_PREFIX_LEN_LSB)   |
            (QMI_M0_RFMT_DUMMY_LEN_VALUE_24   << QMI_M0_RFMT_DUMMY_LEN_LSB);

    qmi_hw->m[1].rcmd = 0xEBu;

    // prefix_len = 8, write cmd = 0x38
    qmi_hw->m[1].wfmt =
            (QMI_M0_WFMT_PREFIX_WIDTH_VALUE_Q << QMI_M0_WFMT_PREFIX_WIDTH_LSB) |
            (QMI_M0_WFMT_ADDR_WIDTH_VALUE_Q   << QMI_M0_WFMT_ADDR_WIDTH_LSB)   |
            (QMI_M0_WFMT_SUFFIX_WIDTH_VALUE_Q << QMI_M0_WFMT_SUFFIX_WIDTH_LSB) |
            (QMI_M0_WFMT_DUMMY_WIDTH_VALUE_Q  << QMI_M0_WFMT_DUMMY_WIDTH_LSB)  |
            (QMI_M0_WFMT_DATA_WIDTH_VALUE_Q   << QMI_M0_WFMT_DATA_WIDTH_LSB)   |
            (QMI_M0_WFMT_PREFIX_LEN_VALUE_8   << QMI_M0_WFMT_PREFIX_LEN_LSB);

    qmi_hw->m[1].wcmd = 0x38u;

    xip_ctrl_hw->ctrl |= XIP_CTRL_WRITABLE_M1_BITS;

    return true;
}

int32_t psram_init(int cs_pin) {
  _cs_pin = cs_pin;

  if (!apsxx04_init_psram_cs1()) {
    psram_size = 0;
    return -1;
  }
  
  int32_t ret = psram_test();
  if(ret < 0) {
    psram_size = 0;
    return ret;
  }

  psram_size = psram_read_size();
  //psram_size = (2 * 1024 * 1024);
  return 0;
}

int32_t psram_test(void) {
  volatile uint8_t *p = PSRAM_BASE_UNCACHED;
  const uint32_t test_size = 1 * 1024 * 1024;
  uint8_t test_data = 0x10;

  for(uint32_t i=0; i<test_size; i++) {
      p[i] = (uint8_t)(test_data + i);
  }
  for(uint32_t i=0; i<test_size; i++) {
    if(p[i] != (uint8_t)(test_data + i)) return -((int32_t)i + 100000000);
  }

  return 0;
}

uint32_t psram_read_size(void) {
  volatile uint8_t* p16 = PSRAM_BASE_UNCACHED + APS1604_MAX_ADDR;
  volatile uint8_t* p32 = PSRAM_BASE_UNCACHED + APS3204_MAX_ADDR;
  volatile uint8_t* p64 = PSRAM_BASE_UNCACHED + APS6404_MAX_ADDR;
  volatile uint8_t* p128 = PSRAM_BASE_UNCACHED + APS12804_MAX_ADDR;

  p16[0] = 0x16;
  p32[0] = 0x32;
  if(p16[0] == 0x32) {
    return (APS1604_MAX_ADDR + 1);
  }
  p64[0] = 0x64;
  if(p32[0] == 0x64) {
    return (APS3204_MAX_ADDR + 1);
  }
  p128[0] = 0x28;
  if(p64[0] == 0x28) {
    return (APS6404_MAX_ADDR + 1);
  }
  return (APS12804_MAX_ADDR + 1);;
}