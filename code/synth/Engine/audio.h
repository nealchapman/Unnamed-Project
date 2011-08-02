#ifndef __AUDIO_H__
#define __AUDIO_H__

void configureAudioOut(const supportedCodecs codec)
void enableAudioOut(void)
__attribute__((interrupt_handler))
static void audioISR(void);

#endif //__AUDIO_H__
