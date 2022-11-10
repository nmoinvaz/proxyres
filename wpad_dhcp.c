#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <time.h>

#include "log.h"
#include "util.h"
#include "util_socket.h"

#define DHCP_MAGIC          ("\x63\x82\x53\x63")
#define DHCP_MAGIC_LEN      (4)

#define DHCP_ACK            (5)
#define DHCP_INFORM         (8)

#define DHCP_BOOT_REQUEST   (1)
#define DHCP_BOOT_REPLY     (2)

#define DHCP_OPT_PAD        (0)
#define DHCP_OPT_MSGTYPE    (0x35)
#define DHCP_OPT_PARAMREQ   (0x37)
#define DHCP_OPT_WPAD       (0xfc)
#define DHCP_OPT_END        (0xff)

#define DHCP_OPT_MAX_LENGTH (312)

#define ETHERNET_TYPE       (1)
#define ETHERNET_LENGTH     (6)

typedef struct dhcp_msg {
    uint8_t op;    /* operation */
    uint8_t htype; /* hardware address type */
    uint8_t hlen;  /* hardware address len */
    uint8_t hops;
    uint32_t xid;  /* transaction id */
    uint16_t secs; /* seconds since protocol start */
    uint16_t flags;
    uint32_t ciaddr;    /* client IP */
    uint32_t yiaddr;    /* your IP */
    uint32_t siaddr;    /* server IP */
    uint32_t giaddr;    /* gateway IP */
    uint8_t chaddr[16]; /* client hardware address */
    uint8_t sname[64];  /* server name */
    uint8_t file[128];  /* bootstrap file */
    uint8_t options[DHCP_OPT_MAX_LENGTH];
} dhcp_msg;

typedef struct dhcp_option {
    uint8_t type;
    uint8_t length;
    uint8_t value[1];
} dhcp_option;

static inline bool dhcp_check_magic(uint8_t *options) {
    return memcmp(options, DHCP_MAGIC, DHCP_MAGIC_LEN) == 0;
}

static inline uint8_t *dhcp_copy_magic(uint8_t *options) {
    memcpy(options, DHCP_MAGIC, DHCP_MAGIC_LEN);
    return options + DHCP_MAGIC_LEN;
}

static inline uint8_t *dhcp_copy_option(uint8_t *options, dhcp_option *option) {
    memcpy(options, &option->type, sizeof(option->type));
    options += sizeof(option->type);
    memcpy(options, &option->length, sizeof(option->length));
    options += sizeof(option->length);
    if (option->length) {
        memcpy(options, option->value, option->length);
        options += option->length;
    }
    return options;
}

bool dhcp_send_inform(SOCKET sfd, uint32_t request_xid) {
    char hostname[HOST_MAX] = {0};
    struct sockaddr_in address = {0};
    struct servent *serv;

    // Get local hostname
    if (gethostname(hostname, sizeof(hostname)) == -1) {
        LOG_ERROR("Unable to get hostname (%d)\n", socketerr);
        return false;
    }
    hostname[sizeof(hostname) - 1] = 0;

    // Get hostent for local hostname
    struct hostent *localent = gethostbyname(hostname);
    if (!localent) {
        LOG_ERROR("Unable to get hostent for %s (%d)\n", hostname, socketerr);
        return false;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_NONE;
    if ((serv = getservbyname("bootps", "udp")) != NULL)
        address.sin_port = serv->s_port;

    // Construct request
    struct dhcp_msg request = {0};

    request.op = DHCP_BOOT_REQUEST;
    request.htype = ETHERNET_TYPE;
    request.hlen = ETHERNET_LENGTH;
    request.xid = request_xid;
    request.ciaddr = *(uint32_t *)*localent->h_addr_list;

    uint8_t *opts = request.options;

    // Construct request signature
    opts = dhcp_copy_magic(opts);

    // Construct request options
    dhcp_option opt_msg_type = {DHCP_OPT_MSGTYPE, 1, DHCP_INFORM};
    opts = dhcp_copy_option(opts, &opt_msg_type);
    dhcp_option opt_param_req = {DHCP_OPT_PARAMREQ, 1, DHCP_OPT_WPAD};
    opts = dhcp_copy_option(opts, &opt_param_req);
    dhcp_option opt_end = {DHCP_OPT_END, 0, 0};
    opts = dhcp_copy_option(opts, &opt_end);

    // Broadcast DHCP request
    int32_t request_len = (int32_t)(opts - (uint8_t *)&request);
    int sent = sendto(sfd, (const char *)&request, request_len, 0, (struct sockaddr *)&address, sizeof(address));
    return sent == request_len;
}

static bool dhcp_read_reply(SOCKET sfd, uint32_t request_xid, dhcp_msg *reply) {
    int response_len = recvfrom(sfd, (char *)reply, sizeof(dhcp_msg), 0, NULL, NULL);

    if (response_len <= sizeof(dhcp_msg) - DHCP_OPT_MAX_LENGTH) {
        LOG_ERROR("Unable to read DHCP response (%d:%d)\n", response_len, socketerr);
        return false;
    }

    if (reply->op != DHCP_BOOT_REPLY) {
        LOG_ERROR("Invalid DHCP response (op=%d)\n", reply->op);
        return false;
    }

    if (reply->xid != request_xid) {
        LOG_ERROR("Invalid DHCP response (xid %" PRIx32 ")\n", reply->xid);
        return false;
    }

    if (!dhcp_check_magic(reply->options)) {
        LOG_ERROR("Invalid DHCP response (magic %" PRIx32 ")\n", *(uint32_t *)reply->options);
        return false;
    }

    return true;
}

static uint8_t *dhcp_get_option(dhcp_msg *reply, uint8_t type, uint8_t *length) {
    uint8_t *opts = reply->options + DHCP_MAGIC_LEN;
    uint8_t *opts_end = reply->options + sizeof(reply->options);

    // Enumerate DHCP options
    while (opts < opts_end && *opts != DHCP_OPT_END) {
        if (*opts == DHCP_OPT_PAD) {
            opts++;
            continue;
        }

        // Parse option type and length
        uint8_t opt_type = *opts++;
        uint8_t opt_length = *opts++;

        // Check if option type matches
        if (opt_type == type) {
            // Allocate buffer to return option value
            uint8_t *value = calloc(opt_length + 1, sizeof(char));
            if (value)
                memcpy(value, opts, opt_length);

            // Optionally return option length
            if (length)
                *length = opt_length;

            return value;
        }

        opts += opt_length;
    }

    return NULL;
}

static bool dhcp_wait_for_reply(SOCKET sfd, int32_t timeout_sec) {
    fd_set read_fds;
    struct timeval tv;

    // Add socket to read set
    FD_ZERO(&read_fds);
    FD_SET(sfd, &read_fds);

    tv.tv_sec = timeout_sec;
    tv.tv_usec = 0;

    // Wait for reply on socket
    if (select((int)sfd, &read_fds, NULL, NULL, &tv) <= 0 || !FD_ISSET(sfd, &read_fds))
        return false;

    return true;
}

char *wpad_dhcp(int32_t timeout_sec) {
    struct sockaddr_in address = {0};
    struct protoent *proto;
    struct servent *serv;
    SOCKET sfd;

    if ((proto = getprotobyname("udp")) == NULL) {
        LOG_ERROR("Unable to get protocol by name\n");
        return NULL;
    }

    sfd = socket(AF_INET, SOCK_DGRAM, proto->p_proto);
    if (sfd == -1) {
        LOG_ERROR("Unable to create udp socket\n");
        return NULL;
    }

    int broadcast = 1;
    setsockopt(sfd, SOL_SOCKET, SO_BROADCAST, (const char *)&broadcast, sizeof(broadcast));
    int reuseaddr = 1;
    setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, (const char *)&reuseaddr, sizeof(reuseaddr));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    if ((serv = getservbyname("bootpc", "udp")) != NULL)
        address.sin_port = serv->s_port;

    if (bind(sfd, (struct sockaddr *)&address, sizeof(address)) == -1) {
        // Ignore failure if port is already bound
        if (socketerr != EACCES)
            LOG_ERROR("Unable to bind udp socket (%d)\n", socketerr);
        closesocket(sfd);
        return NULL;
    }

    srand((int)time(NULL));

    uint32_t request_xid = rand();

    // Send DHCPINFORM request to DHCP server
    if (!dhcp_send_inform(sfd, request_xid)) {
        LOG_ERROR("Unable to send DHCP inform\n");
        closesocket(sfd);
        return NULL;
    }

    if (!dhcp_wait_for_reply(sfd, timeout_sec)) {
        LOG_DEBUG("Unable to receive DHCP reply\n");
        closesocket(sfd);
        return NULL;
    }

    // Read reply from DHCP server
    dhcp_msg reply = {0};
    bool is_ok = dhcp_read_reply(sfd, request_xid, &reply);
    closesocket(sfd);
    if (!is_ok)
        return NULL;

    // Parse options in DHCP reply
    uint8_t opt_length = 0;
    uint8_t *opt = NULL;

    opt = dhcp_get_option(&reply, DHCP_OPT_MSGTYPE, &opt_length);
    if (opt_length != 1 || *opt != DHCP_ACK) {
        LOG_ERROR("Invalid DHCP reply (msgtype=%d)\n", *opt);
        return NULL;
    }
    free(opt);

    opt = dhcp_get_option(&reply, DHCP_OPT_WPAD, &opt_length);
    if (opt_length <= 0) {
        LOG_ERROR("Invalid DHCP reply (optlen=%d)\n", opt_length);
        return NULL;
    }

    return (char *)opt;
}