#pragma once

#ifdef __cplusplus
extern "C" { // only need to export C interface if
             // used by C++ source code
#endif

#include <stdint.h>

#include "config.h"
#include "fifo.h"
#include "logging.h"
#include "rastacrc.h"
#include "rastadeferqueue.h"
#include "udp.h"
#include "transport.h"

/**
 * maximum size of messages in the defer queue in bytes
 */
#define MAX_DEFER_QUEUE_MSG_SIZE 1000


/**
 * representation of the tls_state of a redundancy channel as defined in 6.6.4.1
 */
typedef enum {
    /**
     * Redundancy channel is up
     */
    RASTA_RED_UP,

    /**
     * redundancy channel is down
     */
    RASTA_RED_CLOSED
} rasta_redundancy_state;

/**
 * representation of a RaSTA redundancy channel
 */
typedef struct {
    /**
     * the RaSTA ID of the remote entity this channel is bound to
     */
    unsigned long associated_id;

    /**
     * current tls_state of the redundancy channel
     */
    rasta_redundancy_state current_state;

    /**
     * the type of checksum that is used for all messages
     */
    struct crc_options checksum_type;

    /**
     * next sequence number to send
     */
    unsigned long seq_tx;

    /**
     * next expected sequence number to be received
     */
    unsigned long seq_rx;

    /**
     * the defer queue
     */
    struct defer_queue defer_q;

    /**
     * used to store all received packets within a diagnose window
     */
    struct defer_queue diagnostics_packet_buffer;

    /**
     * the FIFO where the messages for the upper layer are stored
     */
    fifo_t *fifo_recv;

    /**
     * the transport channels of the partner (client) when running in server mode.
     * these are dynamically added when a message from the corresponding channel is received.
     */
    rasta_transport_channel *connected_channels;



    /**
     * the amount of discovered partner transport channels
     */
    unsigned int connected_channel_count;

    /**
     * the total amount of transport channels
     */
    unsigned int transport_channel_count;

    /**
     * logger used for logging
     */
    struct logger_t logger;

    /**
     * 1 if the redundancy channel is open, 0 if it is closed
     */
    int is_open;

    /**
     * configuration parameters of the redundancy layer
     */
    struct RastaConfigInfoRedundancy configuration_parameters;

    /**
     * Hashing context for en/decoding SR layer PDUs
     */
    rasta_hashing_context_t hashing_context;
} rasta_redundancy_channel;

/**
 * initializes a new RaSTA redundancy channel
 * @param logger the logger that is used to log information
 * @param config the configuration for the redundancy layer
 * @param transport_channel_count the amount of transport channels of this redundancy channel
 * @param id the RaSTA ID that is associated with this redundancy channel
 * @return an initialized redundancy channel
 */
void red_f_init(struct logger_t logger, struct RastaConfigInfo config, unsigned int transport_channel_count,
                unsigned long id, rasta_redundancy_channel *channel);

/**
 * the f_receive function of the redundancy layer
 * @param channel the redundancy channel that is used
 * @param packet the packet that has been received over UDP
 * @param channel_id the index of the transport channel, the @p packet has been received
 */
void red_f_receiveData(rasta_redundancy_channel *channel, struct RastaRedundancyPacket packet, int channel_id);

/**
 * the f_deferTmo function of the redundancy layer
 * @param channel the redundancy channel that is used
 */
void red_f_deferTmo(rasta_redundancy_channel *channel);

/**
 * blocks until the tls_state is closed and all notification threads terminate
 * @param channel the channel that is used
 */
void rasta_red_wait_for_close(rasta_redundancy_channel *channel);

/**
 * adds a (discovered) transport channel to the @p channel
 * @param channel the redundancy channel where the transport channel will be added
 * @param ip the remote IPv4 of the transport channel
 * @param port the remote port of the transport channel
 */
void rasta_red_add_transport_channel(rasta_redundancy_channel *channel,
#ifdef USE_TCP
                                     struct rasta_transport_state transport_state,
#endif
                                     char *ip, uint16_t port);

/**
 * frees memory for the @p channel
 * @param channel the channel that is freed
 */
void red_f_cleanup(rasta_redundancy_channel *channel);

#ifdef __cplusplus
}
#endif