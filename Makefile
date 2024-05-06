build:
	pio run

upload:
	pio run -t upload

monitor:
	pio device monitor

clean:
	rm -fR .pio/
