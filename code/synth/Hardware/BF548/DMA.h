#ifndef __DMA_H__
#define __DMA_H__

int requestDMA(uint32_t channel, const char *device_id);
void freeDMA(uint32_t channel);
void enableDMA(uint32_t channel);
void disableDMA(uint32_t channel);

void setDMAStartAddr(uint32_t channel, uint32_t addr);
void setDMANextDescAddr(uint32_t channel, uint32_t addr);
void setDMAXCount(uint32_t channel, uint16_t xCount);
void setDMAXModify(uint32_t channel, uint16_t xModify);
void setDMAYCount(uint32_t channel, uint16_t yCount);
void setDMAYModify(uint32_t channel, uint16_t yModify);
void setDMAConfig(uint32_t channel, uint16_t config);
uint16_t setBFINDMAConfig(char direction, char flowMode, char intrMode, char DMAMode, char width);
uint16_t getDMACurrIRQStatus(uint32_t channel);
uint16_t getDMACurrXCount(uint32_t channel);
uint16_t getDMACurrYCount(uint32_t channel);
void setDMASG(uint32_t channel, struct dmasg_t *sg, uint32_t nrSG);
void DMADisableIRQ(uint32_t channel);
void DMAEnableIRQ(uint32_t channel);
void clearDMAIRQStatus(uint32_t channel);
uint32_t setDMACallback(uint32_t channel, dmaInterruptT callback, void *data);

#endif //__DMA_H__
