typedef struct cfg {
  long  port;
  char *iface;
  char *server;
  FILE *fstream;
  int   use_udp;
  int   use_rudp;
} cfg_t;
