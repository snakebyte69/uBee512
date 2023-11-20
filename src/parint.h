/* Microbee parallel port peripheral header */
#ifndef HEADER_PARINT_H
#define HEADER_PARINT_H

// All communication with peripheral connected to the Microbee
// parallel port (PIO port A) is done through functions in this
// structure.

// The initialisation function is called when the peripheral is
// "attached" to the parallel port, and conversely the
// deinitialisation function is called when the peripheral is detached
// from the parallel port.

// When writing data, data is first written to the peripheral using
// the write function, then the ready function is called to signal the
// availability of new data.  When the data has been processed the
// strobe function is called.

// When reading, the peripheral may call the strobe function to signal
// the presence of new data, which is then fetched using the read
// function, the pio will signal acknowledgement of the data by then
// calling the ready function.

// the poll() function is called when the PIO is polled for an interrupt
// condition, but before the interrupt flag is tested.

typedef struct {
    int (*init)(void);          /* initialisation */
    int (*deinit)(void);        /* de-initialisation */
    int (*reset)(void);
    void (*poll)(void);
    void (*ready)(void);
    void (*strobe)(void);
    uint8_t (*read)(void);
    void (*write)(uint8_t);
} parint_ops_t;

#endif /* HEADER_PARINT_H */
