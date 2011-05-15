#define APRSIS_LOGIN "user %s pass %s vers aprsmap 0.0 filter r/55/-4/600\n"

typedef struct _aprsis_ctx {
    int sockfd;
    char *host;
    char *port;
    char *user;
    char *pass;
} aprsis_ctx;

aprsis_ctx *aprsis_new(const char *host, const char *port, const char *user, const char *pass);
int aprsis_connect(aprsis_ctx *ctx);
int aprsis_login(aprsis_ctx *ctx);
void aprsis_close(aprsis_ctx *ctx);

