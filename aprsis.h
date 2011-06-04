#define APRSIS_LOGIN "user %s pass %s vers aprsmap 0.0"

typedef struct _aprsis_ctx {
    int sockfd;

    char *host;
    char *port;
    char *user;
    char *pass;

    double latitude;
    double longitude;
    int radius;

    FILE *log_file;

} aprsis_ctx;

aprsis_ctx *aprsis_new(const char *host, const char *port, const char *user, const char *pass);
int aprsis_read(aprsis_ctx *ctx, char *buf, size_t len);
int aprsis_write(aprsis_ctx *ctx, char *buf, size_t len);
void aprsis_write_log(aprsis_ctx *ctx, char *buf, size_t len);
int aprsis_set_log(aprsis_ctx *ctx, FILE *log_file);
int aprsis_connect(aprsis_ctx *ctx);
void aprs_set_filter(aprsis_ctx *ctx, double latitude, double longitude, int radius);
int aprsis_login(aprsis_ctx *ctx);
void aprsis_close(aprsis_ctx *ctx);

