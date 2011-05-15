#define APRSIS_LOGIN "user %s pass %s vers aprsmap 0.0 filter r/%.0f/%.0f/%d\n"

typedef struct _aprsis_ctx {
    int sockfd;

    char *host;
    char *port;
    char *user;
    char *pass;

    double latitude;
    double longitude;
    int radius;

} aprsis_ctx;

aprsis_ctx *aprsis_new(const char *host, const char *port, const char *user, const char *pass);
int aprsis_connect(aprsis_ctx *ctx);
void aprs_set_filter(aprsis_ctx *ctx, double latitude, double longitude, int radius);
int aprsis_login(aprsis_ctx *ctx);
void aprsis_close(aprsis_ctx *ctx);

