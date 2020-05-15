/*
 * include/at86rf230.h - AT86RF230/AT86RF231 protocol and register definitions
 *
 * Written 2008-2011 by Werner Almesberger
 * Copyright 2008-2011 Werner Almesberger
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */


#ifndef AT86RF230_H
#define	AT86RF230_H

enum {
	AT86RF230_REG_WRITE	= 0xc0, /* 11... */
	AT86RF230_REG_READ	= 0x80,	/* 10... */
	AT86RF230_BUF_WRITE	= 0x60,	/* 011... */
	AT86RF230_BUF_READ	= 0x20,	/* 001... */
	AT86RF230_SRAM_WRITE	= 0x40,	/* 010... */
	AT86RF230_SRAM_READ	= 0x00 	/* 000... */
};

#define	MAX_PSDU	127	/* octets, see AT86RF230 manual section 8.1  */
#define	SRAM_SIZE	128


/* --- Registers ----------------------------------------------------------- */

enum {
	REG_TRX_STATUS		= 0x01,
	REG_TRX_STATE		= 0x02,
	REG_TRX_CTRL_0		= 0x03,

	REG_TRX_CTRL_1		= 0x04,	/* 231 only */

	REG_PHY_TX_PWR		= 0x05,
	REG_PHY_RSSI		= 0x06,
	REG_PHY_ED_LEVEL	= 0x07,
	REG_PHY_CC_CCA		= 0x08,
	REG_CCA_THRES		= 0x09,

	REG_RX_CTRL		= 0x0a,	/* 231 only */
	REG_SFD_VALUE		= 0x0b,	/* 231 only */
	REG_TRX_CTRL_2		= 0x0c,	/* 231 only */
	REG_ANT_DIV		= 0x0d,	/* 231 only */

	REG_IRQ_MASK		= 0x0e,
	REG_IRQ_STATUS		= 0x0f,
	REG_VREG_CTRL		= 0x10,
	REG_BATMON		= 0x11,
	REG_XOSC_CTRL		= 0x12,

	REG_RX_SYN		= 0x15,	/* 231 only */
	REG_XAH_CTRL_1		= 0x17,	/* 231 only */
	REG_FTN_CTRL		= 0x18,	/* 231 only */

	REG_PLL_CF		= 0x1a,
	REL_PLL_DCU		= 0x1b,
	REG_PART_NUM		= 0x1c,
	REG_VERSION_NUM		= 0x1d,
	REG_MAN_ID_0		= 0x1e,
	REG_MAN_ID_1		= 0x1f,
	REG_SHORT_ADDR_0	= 0x20,
	REG_SHORT_ADDR_1	= 0x21,
	REG_PAN_ID_0		= 0x22,
	REG_PAN_ID_1		= 0x23,
	REG_IEEE_ADDR_0		= 0x24,
	REG_IEEE_ADDR_1		= 0x25,
	REG_IEEE_ADDR_2		= 0x26,
	REG_IEEE_ADDR_3		= 0x27,
	REG_IEEE_ADDR_4		= 0x28,
	REG_IEEE_ADDR_5		= 0x29,
	REG_IEEE_ADDR_6		= 0x2a,
	REG_IEEE_ADDR_7		= 0x2b,

	REG_XAH_CTRL_0		= 0x2c,	/* XAH_CTRL in 230 */
	REG_CSMA_SEED_0		= 0x2d,
	REG_CSMA_SEED_1		= 0x2e,
	REG_CSMA_BE		= 0x2f,	/* 231 only */

	REG_CONT_TX_0		= 0x36,
	REG_CONT_TX_1		= 0x3d,	/* 230 only */
};

/* --- TRX_STATUS --- ------------------------------------------------------ */

#define	CCA_DONE	(1 << 7)
#define	CCA_STATUS	(1 << 6)

#define	TRX_STATUS_SHIFT	0
#define	TRX_STATUS_MASK		0x1f

enum {
	TRX_STATUS_P_ON			= 0x00,	/* reset default */
	TRX_STATUS_BUSY_RX		= 0x01,
	TRX_STATUS_BUSY_TX		= 0x02,
	TRX_STATUS_RX_ON		= 0x06,
	TRX_STATUS_TRX_OFF		= 0x08,
	TRX_STATUS_PLL_ON		= 0x09,
	TRX_STATUS_SLEEP		= 0x0f,
	TRX_STATUS_BUSY_RX_AACK		= 0x11,
	TRX_STATUS_BUSY_TX_ARET		= 0x12,
	TRX_STATUS_RX_AACK_ON		= 0x16,
	TRX_STATUS_TX_ARET_ON		= 0x19,
	TRX_STATUS_RX_ON_NOCLK		= 0x1c,
	TRX_STATUS_RX_AACK_ON_NOCLK	= 0x1d,
	TRX_STATUS_BUSY_RX_AACK_NOCLK	= 0x1e,
	TRX_STATUS_TRANSITION		= 0x1f	/* ..._IN_PROGRESS */
};

/* --- TRX_STATE ----------------------------------------------------------- */

#define	TRAC_STATUS_SHIFT	5
#define	TRAC_STATUS_MASK	7

enum {
	TRAC_STATUS_SUCCESS			= 0,	/* reset default */
	TRAC_STATUS_SUCCESS_DATA_PENDING	= 1,
	TRAC_STATUS_SUCCESS_WAIT_FOR_ACK	= 2,	/* 231 only */
	TRAC_STATUS_CHANNEL_ACCESS_FAILURE	= 3,
	TRAC_STATUS_NO_ACK			= 5,
	TRAC_STATUS_INVALID			= 7
};

#define	TRX_CMD_SHIFT	0
#define	TRX_CMD_MASK	0x1f

enum {
	TRX_CMD_NOP		= 0x00,	/* reset default */
	TRX_CMD_TX_START	= 0x02,
	TRX_CMD_FORCE_TRX_OFF	= 0x03,
	TRX_CMD_FORCE_PLL_ON	= 0x04,	/* 231 only */
	TRX_CMD_RX_ON		= 0x06,
	TRX_CMD_TRX_OFF		= 0x08,
	TRX_CMD_PLL_ON		= 0x09,
	TRX_CMD_RX_AACK_ON	= 0x16,
	TRX_CMD_TX_ARET_ON	= 0x19,
};

/* --- TRX_CTRL_0 ---------------------------------------------------------- */

#define	PAD_IO_SHIFT	6
#define	PAD_IO_MASK	3

enum {
	PAD_IO_2mA,	/* reset default */
	PAD_IO_4mA,
	PAD_IO_6mA,
	PAD_IO_8mA
};

#define	PAD_IO_CLKM_SHIFT	4
#define	PAD_IO_CLKM_MASK	3

enum {
	PAD_IO_CLKM_2mA,
	PAD_IO_CLKM_4mA,	/* reset default */
	PAD_IO_CLKM_5mA,
	PAD_IO_CLKM_8mA,
};

#define	CLKM_SHA_SEL		(1 << 3)

#define	CLKM_CTRL_SHIFT	0
#define	CLKM_CTRL_MASK	7

enum {
	CLKM_CTRL_OFF	= 0,
	CLKM_CTRL_1MHz	= 1,	/* reset default */
	CLKM_CTRL_2MHz	= 2,
	CLKM_CTRL_4MHz	= 3,
	CLKM_CTRL_8MHz	= 4,
	CLKM_CTRL_16MHz	= 5
};

/* --- TRX_CTRL_1 (231 only) ----------------------------------------------- */

#define	PA_EXT_EN		(1 << 7)
#define	IRQ_2_EXT_EN		(1 << 6)
#define	TX_AUTO_CRC_ON		(1 << 5)	/* 231 location */
#define	RX_BL_CTRL		(1 << 4)

#define	SPI_CMD_MODE_SHIFT	2
#define	SPI_CMD_MODE_MASK	3

enum {
	SPI_CMD_MODE_EMPTY	= 0,	/* reset default */
	SPI_CMD_MODE_TRX_STATUS	= 1,
	SPI_CMD_MODE_PHY_RSSI	= 2,
	SPI_CMD_MODE_IRQ_STATUS	= 3,
};

#define	IRQ_MASK_MODE		(1 << 1)
#define	IRQ_POLARITY		(1 << 0)

/* --- PHY_TX_PWR ---------------------------------------------------------- */

#define	TX_AUTO_CRC_ON_230	(1 << 7)	/* 230 location */

#define	PA_BUF_LT_SHIFT	6
#define	PA_BUF_LT_MASK	3

#define	PA_LT_SHIFT	4
#define	PA_LT_MASK	3

#define	TX_PWR_SHIFT	0
#define	TX_PWR_MASK	0x0f

/* --- PHY_RSSI ------------------------------------------------------------ */

#define	RX_CRC_VALID	(1 << 7)

#define	RND_VALUE_SHIFT	5		/* 231 only */
#define	RND_VALUE_MASK	3

#define	RSSI_SHIFT	0
#define	RSSI_MASK	0x1f

/* --- PHY_CC_CCA ---------------------------------------------------------- */

#define	CCA_REQUEST	(1 << 7)

#define	CCA_MODE_SHIFT	5
#define	CCA_MODE_MASK	3

enum {
	CCA_MODE_CARRIER_OR_ENERGY	= 0,	/* 231 only */
	CCA_MODE_ENERGY			= 1,	/* reset default */
	CCA_MODE_CARRIER		= 2,
	CCA_MODE_CARRIER_AND_ENERGY	= 3
};

#define	CHANNEL_SHIFT	0
#define	CHANNEL_MASK	0x1f

/* --- CCA_THRES ----------------------------------------------------------- */

#define	CCA_ED_THRES_SHIFT	0
#define	CCA_ED_THRES_MASK	0x0f

/* --- RX_CTRL (231 only) -------------------------------------------------- */

#define	PDT_THRES_SHIFT	0
#define	PDT_THRES_MASK	0x0f

enum {
	PDT_THRES_DEFAULT	= 0x07,	/* reset default */
	PDT_THRES_DIVERSITY	= 0x03,
};

/* --- TRX_CTRL_2 (231 only) ----------------------------------------------- */

#define	RX_SAFE_MODE		(1 << 7)

#define	OQPSK_DATA_RATE_SHIFT	0
#define	OQPSK_DATA_RATE_MASK	3

enum {
	OQPSK_DATA_RATE_250	= 0,	/* reset default */
	OQPSK_DATA_RATE_500	= 1,
	OQPSK_DATA_RATE_1000	= 2,
	OQPSK_DATA_RATE_2000	= 3
};

/* --- ANT_DIV (231 only) -------------------------------------------------- */

#define	ANT_SEL		(1 << 7)
#define	ANT_DIV_EN	(1 << 3)
#define	ANT_EXT_SW_EN	(1 << 2)

#define	ANT_CTRL_SHIFT	0
#define	ANT_CTRL_MASK	3

enum {
	ANT_CTRL_ANT_0	= 1,
	ANT_CTRL_ANT_1	= 2,
	ANT_CTRL_NODIV	= 3,	/* reset default */
};

/* --- IRQ_MASK/IRQ_STATUS ------------------------------------------------- */

enum {
	IRQ_PLL_LOCK	= 1 << 0,
	IRQ_PLL_UNLOCK	= 1 << 1,
	IRQ_RX_START	= 1 << 2,
	IRQ_TRX_END	= 1 << 3,
	IRQ_CCA_ED_DONE	= 1 << 4,	/* 231 only */
	IRQ_AMI		= 1 << 5,	/* 231 only */
	IRQ_TRX_UR	= 1 << 6,
	IRQ_BAT_LOW	= 1 << 7
};

/* --- VREG_CTRL ----------------------------------------------------------- */

#define	AVREG_EXT	(1 << 7)
#define	AVDD_OK		(1 << 6)
#define	DVREG_EXT	(1 << 3)
#define	DVDD_OK		(1 << 2)

/* --- BATMON -------------------------------------------------------------- */

#define	BATMON_OK	(1 << 5)
#define	BATMON_HR	(1 << 4)

#define	BATMON_VTH_SHIFT	0
#define	BATMON_VTH_MASK		0x0f

/* --- XOSC_CTRL ----------------------------------------------------------- */

#define	XTAL_MODE_SHIFT	4
#define	XTAL_MODE_MASK	0x0f

enum {
	XTAL_MODE_OFF	= 0x0,	/* 230 only */
	XTAL_MODE_EXT	= 0x4,
	XTAL_MODE_INT	= 0xf	/* reset default */
};

#define	XTAL_TRIM_SHIFT	4
#define	XTAL_TRIM_MASK	0x0f

/* --- RX_SYN (231 only) --------------------------------------------------- */

#define	RX_PDT_DIS		(1 << 7)

#define	RX_PDT_LEVEL_SHIFT	0
#define	RX_PDT_LEVEL_MASK	0xf

/* --- XAH_CTRL_1 (231 only) ----------------------------------------------- */

#define	AACK_FLTR_RES_FT	(1 << 5)
#define	AACK_UPLD_RES_FT	(1 << 4)
#define	AACK_ACK_TIME		(1 << 2)
#define	AACK_PROM_MODE		(1 << 1)

/* --- FTN_CTRL (231 only) ------------------------------------------------- */

#define	FTN_START		(1 << 7)

/* --- PLL_CF -------------------------------------------------------------- */

#define	PLL_CF_START	(1 << 7)

/* --- PLL_DCU ------------------------------------------------------------- */

#define	PLL_DCU_START	(1 << 7)

/* --- XAH_CTRL_0 (XAH_CTRL in 230) ---------------------------------------- */

#define	MAX_FRAME_RETRIES_SHIFT	4
#define	MAX_FRAME_RETRIES_MASK	0x0f

#define	MAX_CSMA_RETRIES_SHIFT	1
#define	MAX_CSMA_RETRIES_MASK	0x07

#define	SLOTTED_OPERATION	(1 << 0)	/* 231 only */

/* --- CSMA_SEED_1 --------------------------------------------------------- */

#define	MIN_BE_SHIFT_230	6	/* 230 location */
#define	MIN_BE_MASK_230		3

#define	AACK_FVN_MODE_SHIFT	6	/* 231 only */
#define	AACK_FVN_MODE_MASK	3

enum {
	AACK_FVN_MODE_0		= 0,
	AACK_FVN_MODE_01	= 1,	/* reset default */
	AACK_FVN_MODE_012	= 2,
	AACK_FVN_MODE_ANY	= 3
};

#define	AACK_SET_PD		(1 << 5)
#define	AACK_DIS_ACK		(1 << 4)	/* 231 only */
#define	I_AM_COORD		(1 << 3)

#define CSMA_SEED_1_SHIFT	0
#define	CSMA_SEED_1_MASK	7

/* --- CSMA_BE ------------------------------------------------------------- */

#define	MAX_BE_SHIFT		4
#define	MAX_BE_MASK		0x0f

#define	MIN_BE_SHIFT		0	/* 231 location */
#define	MIN_BE_MASK		0x0f

/* --- REG_CONT_TX_0 ------------------------------------------------------- */

#define	CONT_TX_MAGIC		0x0f

/* --- REG_CONT_TX_1 (230 only) -------------------------------------------- */

#define	CONT_TX_MOD		0x00	/* modulated */
#define	CONT_TX_M2M		0x10	/* f_CH-2 MHz */
#define	CONT_TX_M500K		0x80	/* f_CH-0.5 MHz */
#define	CONT_TX_P500K		0xc0	/* f_CH+0.5 MHz */

#endif /* !AT86RF230_H */
