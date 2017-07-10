/* ./hi_dmac.h
 *
 * History:
 *      17-August-2006 create this file
 */

#ifndef __HI_DMAC_H__
#define __HI_DMAC_H__

/*the defination for the peripheral*/

#define __KCOM_HI_DMAC_INTER__

/*modified by z40717 for X5HD MPW 2010-6-2 17:25 */
#define DMAC_SCI_RX_REQ  	          0
#define DMAC_SCI_TX_REQ  	          1 
#define DMAC_SSP_RX_REQ 	          2
#define DMAC_SSP_TX_REQ 	          3
#define DMAC_RESERVED_REQ4   4
#define DMAC_RESERVED_REQ5   5
#define DMAC_RESERVED_REQ6   6
#define DMAC_RESERVED_REQ7   7
#define DMAC_RESERVED_REQ8   8
#define DMAC_RESERVED_REQ9   9
#define DMAC_SIO0_RX_REQ     10
#define DMAC_SIO0_TX_REQ     11
#define DMAC_SIO1_RX_REQ     12
#define DMAC_SIO1_TX_REQ     13
#define DMAC_SIO2_RX_REQ     14
#define DMAC_SIO2_TX_REQ     15

/*sturcture for LLI*/
typedef struct dmac_lli
{
    unsigned int src_addr;                     /*source address*/
    unsigned int dst_addr;                     /*destination address*/
    unsigned int next_lli;                     /*pointer to next LLI*/
    unsigned int lli_transfer_ctrl;                /*control word*/
} dmac_lli;


int dma_driver_init(struct inode * inode, struct file * file);
int dma_driver_exit(struct inode * inode, struct file * file);
int  dmac_channelclose(unsigned int channel);
int  dmac_register_isr(unsigned int channel, void *pisr);
int  dmac_channel_free(unsigned int channel);
int  free_dmalli_space(unsigned int *ppheadlli, unsigned int page_num);
int  dmac_start_llim2p(unsigned int channel, unsigned int *pfirst_lli, unsigned int uwperipheralid);
int  dmac_buildllim2m(unsigned int * ppheadlli, unsigned int pdest, unsigned int psource,
                      unsigned int totaltransfersize,
                      unsigned int uwnumtransfers);
int  dmac_buildllim2m_opentv5(unsigned int * ppheadlli, unsigned int pdest, unsigned int psource,
                      unsigned int totaltransfersize,
                      unsigned int uwnumtransfers);
int  dmac_channelstart(unsigned int u32channel);
int  dmac_start_llim2m(unsigned int channel, unsigned int *pfirst_lli);
int  dmac_channel_allocate(void *pisr);
int  allocate_dmalli_space(unsigned int *ppheadlli, unsigned int page_num);
int  dmac_buildllim2p( unsigned int *ppheadlli, unsigned int *pmemaddr,
                       unsigned int uwperipheralid, unsigned int totaltransfersize,
                       unsigned int uwnumtransfers, unsigned int burstsize);
int  dmac_start_m2p(unsigned int channel, unsigned int pmemaddr, unsigned int uwperipheralid,
                    unsigned int uwnumtransfers,
                    unsigned int next_lli_addr);
int  dmac_start_m2m(unsigned int channel, unsigned int psource, unsigned int pdest, unsigned int uwnumtransfers);
int  dmac_wait(unsigned int channel);

#endif
