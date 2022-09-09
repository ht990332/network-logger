#include <gio/gio.h>
#include <glib.h>
#include <glib/gprintf.h>
#include <event.h>

#define MAXLINE 256
#define PROJECT_NAME "network-logger"

guint PORT = 19532;
gchar *buffer = NULL;
struct sockaddr_in serveraddress;

static GOptionEntry entries[] = {
    { "port", 'p',  0, G_OPTION_ARG_INT, & PORT, "Port to listen on", "19532" },
    {  NULL }
};

void udp_event_callback(int udp_socket, short B, void * C) {
    guint address_len;
    // unlike malloc, g_malloc0 zeros allocated memory
    buffer = (gchar *) g_malloc0(sizeof(gchar) * MAXLINE);

    // https://pubs.opengroup.org/onlinepubs/007904875/functions/recvfrom.html
    gint n = recvfrom(udp_socket, (gchar *) buffer, MAXLINE,
        MSG_WAITALL, (struct sockaddr * ) & serveraddress,
        &address_len);

    // Null-terminate the string.
    buffer[n] = '\0';

    // Push to system journal log.
    // On Windows, probably just comment out g_log_set_writer_func(g_log_writer_journald, NULL, NULL);
    g_log_set_writer_func(g_log_writer_journald, NULL, NULL);
    g_log_structured(G_LOG_DOMAIN, G_LOG_LEVEL_INFO,
        "MY_APPLICATION_CUSTOM_FIELD", "INFO:",
        "MESSAGE", buffer);
    g_free(buffer);
    buffer = NULL;
}

gint main(gint argc, gchar **argv) {

    GError *error = NULL;
    GOptionContext *optContext;

    optContext = g_option_context_new("- Port Listener");
    g_option_context_add_main_entries(optContext, entries, NULL);
    if (!g_option_context_parse(optContext, &argc, &argv, &error)) {
        g_printf("Option parsing failed: %s\n", error -> message);
        exit(1);
    }

    // https://man7.org/linux/man-pages/man7/ip.7.html
    gint udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_socket < 0) {
        g_error("Socket creation failed");
    }

    // clean buffers   
    memset(&serveraddress, 0, sizeof(serveraddress));

    // https://docs.oracle.com/cd/E19620-01/805-4041/6j3r8iu2l/index.html
    serveraddress.sin_family = AF_INET;
    serveraddress.sin_addr.s_addr = INADDR_ANY;
    serveraddress.sin_port = htons(PORT);

    // https://pubs.opengroup.org/onlinepubs/009695399/functions/bind.html
    if (bind(udp_socket, (const struct sockaddr*) &serveraddress, sizeof(serveraddress)) != 0) {
        g_warning("Port binding failed. Exiting...");
        exit(1);
    }

    g_printf("Listening to UDP messages on port: %d.\n", PORT);

    event_init();

    struct event udp_event;

    event_set(&udp_event, udp_socket, EV_READ | EV_PERSIST, udp_event_callback, &udp_socket);
    event_add(&udp_event, 0);

    event_dispatch();
    close(udp_socket);
    event_del(&udp_event);
    return 0;
}