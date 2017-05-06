/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of Freescale Semiconductor, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef _PIN_MUX_H_
#define _PIN_MUX_H_


/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*! @brief Direction type  */
typedef enum _pin_mux_direction
{
  kPIN_MUX_DirectionInput = 0U,         /* Input direction */
  kPIN_MUX_DirectionOutput = 1U,        /* Output direction */
  kPIN_MUX_DirectionInputOrOutput = 2U  /* Input or output direction */
} pin_mux_direction_t;

/*!
 * @addtogroup pin_mux
 * @{
 */

/*******************************************************************************
 * API
 ******************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif


/*!
 * @brief Configures pin routing and optionally pin electrical features.
 *
 */
void BOARD_InitPins(void);

#define BOARD_INITLEDS_GPIOD_4_DIRECTION                kPIN_MUX_DirectionOutput   /*!< Direction of GPIOD_4 signal */
#define BOARD_INITLEDS_GPIOD_5_DIRECTION                kPIN_MUX_DirectionOutput   /*!< Direction of GPIOD_5 signal */
#define BOARD_INITLEDS_GPIOD_6_DIRECTION                kPIN_MUX_DirectionOutput   /*!< Direction of GPIOD_6 signal */
#define BOARD_INITLEDS_GPIOD_7_DIRECTION                kPIN_MUX_DirectionOutput   /*!< Direction of GPIOD_7 signal */

/*!
 * @brief Configures pin routing and optionally pin electrical features.
 *
 */
void BOARD_InitLEDs(void);


/*!
 * @brief Configures pin routing and optionally pin electrical features.
 *
 */
void BOARD_InitRGB(void);

#define BOARD_INITXCVR_GPIOB_3_DIRECTION                 kPIN_MUX_DirectionInput   /*!< Direction of GPIOB_3 signal */
#define BOARD_INITXCVR_GPIOB_10_DIRECTION               kPIN_MUX_DirectionOutput   /*!< Direction of GPIOB_10 signal */
#define BOARD_INITXCVR_GPIOB_19_DIRECTION               kPIN_MUX_DirectionOutput   /*!< Direction of GPIOB_19 signal */

/*!
 * @brief Configures pin routing and optionally pin electrical features.
 *
 */
void BOARD_InitXCVR(void);

#define BOARD_INITBUTTONS_GPIOE_4_DIRECTION              kPIN_MUX_DirectionInput   /*!< Direction of GPIOE_4 signal */
#define BOARD_INITBUTTONS_GPIOE_3_DIRECTION              kPIN_MUX_DirectionInput   /*!< Direction of GPIOE_3 signal */
#define BOARD_INITBUTTONS_GPIOD_1_DIRECTION              kPIN_MUX_DirectionInput   /*!< Direction of GPIOD_1 signal */
#define BOARD_INITBUTTONS_GPIOA_19_DIRECTION             kPIN_MUX_DirectionInput   /*!< Direction of GPIOA_19 signal */

/*!
 * @brief Configures pin routing and optionally pin electrical features.
 *
 */
void BOARD_InitButtons(void);


/*!
 * @brief Configures pin routing and optionally pin electrical features.
 *
 */
void BOARD_InitSPI(void);


/*!
 * @brief Configures pin routing and optionally pin electrical features.
 *
 */
void BOARD_InitI2C(void);


/*!
 * @brief Configures pin routing and optionally pin electrical features.
 *
 */
void BOARD_InitUART(void);

#if defined(__cplusplus)
}
#endif

/*!
 * @}
 */
#endif /* _PIN_MUX_H_ */

/*******************************************************************************
 * EOF
 ******************************************************************************/
