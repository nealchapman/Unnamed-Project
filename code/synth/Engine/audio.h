#ifndef __AUDIO_H__
#define __AUDIO_H__

void initAudio(void);
void initCodec(void);
void configureAudioOut(int codec);
void enableAudioOut(void);
__attribute__((interrupt_handler))
static void audioISR(void);
void configureAudioDMA(void);
void setAudioDMAISR(void);

#endif //__AUDIO_H__
