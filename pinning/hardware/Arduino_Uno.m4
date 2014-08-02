
ifdef(`conf_STATUSLED_BOOTED', `dnl
pin(STATUSLED_BOOTED, PB5, OUTPUT)
')dnl

ifdef(`conf_W5100', `dnl
/* port the Wiznet W5100 is attached to */
pin(SPI_CS_W5100, SPI_CS_HARDWARE)
')dnl
