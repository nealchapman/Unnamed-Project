int32_t scale[12] = {
	MAKE_FREQUENCY(261.63),
	MAKE_FREQUENCY(277.18),
	MAKE_FREQUENCY(293.66),
	MAKE_FREQUENCY(311.13),
	MAKE_FREQUENCY(329.63),
	MAKE_FREQUENCY(349.23),
	MAKE_FREQUENCY(369.99),
	MAKE_FREQUENCY(392.0),
	MAKE_FREQUENCY(415.30),
	MAKE_FREQUENCY(440.0),
	MAKE_FREQUENCY(466.16),
	MAKE_FREQUENCY(493.88)
};

uint16_t note2Frequency(int note)
{

	uint32_t frequency = 0;

	if(note >= 0)
		frequency = (1<<(note/12))*scale[note % 12];
	else
		frequency = scale[12 + (note % 12)] / (1<<(note/12));

	return (uint16_t)((frequency/SAMPLE_RATE));
}
