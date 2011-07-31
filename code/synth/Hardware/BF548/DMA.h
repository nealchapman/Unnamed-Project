#ifndef __DMA_H__
#define __DMA_H__

void configureDMA(uint16_t DMAChannel, uint16_t destination, uint16_t *startAddress, uint16_t direction, uint16_t wordSize, uint16_t interruptEnable, uint16_t flow, uint16_t size);
void startDMA(uint16_t DMAChannel)
void stopDMA(uint16_t DMAChannel);

#endif //__DMA_H__
