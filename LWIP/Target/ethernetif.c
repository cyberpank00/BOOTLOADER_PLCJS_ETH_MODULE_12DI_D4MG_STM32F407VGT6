/**
 * @file  ethernetif.c
 * @brief Bare-metal Ethernet interface for LwIP (NO_SYS = 1).
 *
 * Uses STM32 HAL ETH v2 API with RxAllocate/RxLink/TxFree callbacks.
 */

#include "main.h"
#include "lwip/opt.h"
#include "lwip/pbuf.h"
#include "lwip/timeouts.h"
#include "netif/ethernet.h"
#include "netif/etharp.h"
#include "ethernetif.h"
#include "eth_custom_phy_interface.h"
#include <string.h>

#define IFNAME0 's'
#define IFNAME1 't'

ETH_HandleTypeDef          heth;
ETH_TxPacketConfigTypeDef  TxConfig;

static ETH_DMADescTypeDef  DMARxDscrTab[ETH_RX_DESC_CNT]
                            __attribute__((section(".bss"), aligned(4)));
static ETH_DMADescTypeDef  DMATxDscrTab[ETH_TX_DESC_CNT]
                            __attribute__((section(".bss"), aligned(4)));

static user_phy_Object_t   phyObj;
static volatile uint8_t    s_frame_received;

/* ---- PHY I/O wrappers -------------------------------------------------- */
static int32_t phy_io_init(void)    { return 0; }
static int32_t phy_io_deinit(void)  { return 0; }

static int32_t phy_io_read(uint32_t addr, uint32_t reg, uint32_t *val)
{
    if (HAL_ETH_ReadPHYRegister(&heth, addr, reg, val) == HAL_OK)
        return 0;
    return -1;
}

static int32_t phy_io_write(uint32_t addr, uint32_t reg, uint32_t val)
{
    if (HAL_ETH_WritePHYRegister(&heth, addr, reg, val) == HAL_OK)
        return 0;
    return -1;
}

static int32_t phy_io_tick(void)
{
    return (int32_t)HAL_GetTick();
}

/* ---- HAL ETH Rx/Tx callbacks ------------------------------------------- */
void HAL_ETH_RxAllocateCallback(uint8_t **buff)
{
    struct pbuf *p = pbuf_alloc(PBUF_RAW, ETH_RX_BUF_SIZE, PBUF_POOL);
    if (p != NULL) {
        *buff = p->payload;
    } else {
        *buff = NULL;
    }
}

void HAL_ETH_RxLinkCallback(void **pStart, void **pEnd,
                             uint8_t *buff, uint16_t Length)
{
    struct pbuf **ppStart = (struct pbuf **)pStart;
    struct pbuf **ppEnd   = (struct pbuf **)pEnd;

    /* Find the pbuf that owns this buffer by backing up from buff. */
    struct pbuf *p = (struct pbuf *)((uint8_t *)buff - offsetof(struct pbuf, payload));
    /* Actually, pbuf_alloc with PBUF_POOL puts payload right after the struct,
       but the canonical way with the new HAL is to use custom pbufs.
       For simplicity in bare-metal we use PBUF_RAM and set len properly. */

    /* With PBUF_POOL, payload is at a known offset. The simplest approach
       is to create a fresh ref pbuf pointing to this buffer. */
    struct pbuf *q = pbuf_alloc(PBUF_RAW, Length, PBUF_RAM);
    if (q == NULL) return;
    memcpy(q->payload, buff, Length);

    q->next    = NULL;
    q->tot_len = 0;
    q->len     = Length;

    if (!*ppStart) {
        *ppStart = q;
    } else {
        (*ppEnd)->next = q;
    }
    *ppEnd = q;

    for (struct pbuf *r = *ppStart; r != NULL; r = r->next) {
        r->tot_len += Length;
    }
}

void HAL_ETH_TxFreeCallback(uint32_t *buff)
{
    pbuf_free((struct pbuf *)buff);
}

void HAL_ETH_RxCpltCallback(ETH_HandleTypeDef *handlerEth)
{
    (void)handlerEth;
    s_frame_received = 1;
}

/* ---- Low-level init ----------------------------------------------------- */
static void low_level_init(struct netif *netif)
{
    uint8_t mac[6] = { 0x00, 0x80, 0xE1, 0x00, 0x00, 0x01 };

    heth.Instance            = ETH;
    heth.Init.MACAddr        = mac;
    heth.Init.MediaInterface = HAL_ETH_RMII_MODE;
    heth.Init.RxDesc         = DMARxDscrTab;
    heth.Init.TxDesc         = DMATxDscrTab;
    heth.Init.RxBuffLen      = ETH_RX_BUF_SIZE;

    HAL_ETH_Init(&heth);

    memset(&TxConfig, 0, sizeof(TxConfig));
    TxConfig.Attributes   = ETH_TX_PACKETS_FEATURES_CSUM
                          | ETH_TX_PACKETS_FEATURES_CRCPAD;
    TxConfig.ChecksumCtrl = ETH_CHECKSUM_IPHDR_PAYLOAD_INSERT_PHDR_CALC;
    TxConfig.CRCPadCtrl   = ETH_CRC_PAD_INSERT;

    netif->hwaddr_len = 6;
    memcpy(netif->hwaddr, mac, 6);
    netif->mtu   = 1500;
    netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP;

    /* PHY init */
    user_phy_IOCtx_t io = {
        .Init     = phy_io_init,
        .DeInit   = phy_io_deinit,
        .ReadReg  = phy_io_read,
        .WriteReg = phy_io_write,
        .GetTick  = phy_io_tick,
    };
    USER_PHY_RegisterBusIO(&phyObj, &io);
    USER_PHY_Init(&phyObj);

    HAL_ETH_Start_IT(&heth);
}

/* ---- Transmit ----------------------------------------------------------- */
static err_t low_level_output(struct netif *netif, struct pbuf *p)
{
    (void)netif;

    uint32_t          idx = 0;
    ETH_BufferTypeDef bufs[ETH_TX_DESC_CNT];
    memset(bufs, 0, sizeof(bufs));

    for (struct pbuf *q = p; q != NULL; q = q->next) {
        if (idx >= ETH_TX_DESC_CNT) return ERR_BUF;
        bufs[idx].buffer = q->payload;
        bufs[idx].len    = q->len;
        if (idx > 0) bufs[idx - 1].next = &bufs[idx];
        idx++;
    }

    TxConfig.Length   = p->tot_len;
    TxConfig.TxBuffer = bufs;
    TxConfig.pData    = p;

    pbuf_ref(p);

    HAL_StatusTypeDef s = HAL_ETH_Transmit_IT(&heth, &TxConfig);
    if (s != HAL_OK) {
        pbuf_free(p);
        return ERR_IF;
    }
    return ERR_OK;
}

/* ---- Receive ------------------------------------------------------------ */
void ethernetif_input(struct netif *netif)
{
    if (!s_frame_received) return;
    s_frame_received = 0;

    struct pbuf *p = NULL;
    HAL_ETH_ReadData(&heth, (void **)&p);

    while (p != NULL) {
        if (netif->input(p, netif) != ERR_OK) {
            pbuf_free(p);
        }
        p = NULL;
        HAL_ETH_ReadData(&heth, (void **)&p);
    }
}

err_t ethernetif_init(struct netif *netif)
{
    netif->name[0]    = IFNAME0;
    netif->name[1]    = IFNAME1;
    netif->output     = etharp_output;
    netif->linkoutput = low_level_output;
    low_level_init(netif);
    return ERR_OK;
}

/* ---- HAL ETH MSP (clock, GPIO, IRQ) ------------------------------------ */
void HAL_ETH_MspInit(ETH_HandleTypeDef *ethHandle)
{
    (void)ethHandle;
    GPIO_InitTypeDef gi = {0};

    __HAL_RCC_ETH_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    /* RMII pins:
     * PA1  = ETH_REF_CLK, PA2 = ETH_MDIO, PA7 = ETH_CRS_DV
     * PB11 = ETH_TX_EN, PB12 = ETH_TXD0, PB13 = ETH_TXD1
     * PC1  = ETH_MDC, PC4 = ETH_RXD0, PC5 = ETH_RXD1
     */
    gi.Mode      = GPIO_MODE_AF_PP;
    gi.Pull      = GPIO_NOPULL;
    gi.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    gi.Alternate = GPIO_AF11_ETH;

    gi.Pin = GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_7;
    HAL_GPIO_Init(GPIOA, &gi);

    gi.Pin = GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13;
    HAL_GPIO_Init(GPIOB, &gi);

    gi.Pin = GPIO_PIN_1 | GPIO_PIN_4 | GPIO_PIN_5;
    HAL_GPIO_Init(GPIOC, &gi);

    HAL_NVIC_SetPriority(ETH_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(ETH_IRQn);
}

void HAL_ETH_MspDeInit(ETH_HandleTypeDef *ethHandle)
{
    (void)ethHandle;
    __HAL_RCC_ETH_CLK_DISABLE();
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_7);
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13);
    HAL_GPIO_DeInit(GPIOC, GPIO_PIN_1 | GPIO_PIN_4 | GPIO_PIN_5);
    HAL_NVIC_DisableIRQ(ETH_IRQn);
}
