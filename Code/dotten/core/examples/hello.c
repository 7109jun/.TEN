#include <stdio.h>
#include <ten/ten.h>

int main(void)
{
    int rc = ten_init();
    if (rc != TEN_OK)
    {
        fprintf(stderr, "ten_init failed: %d\n", rc);
        return 1;
    }

    printf(".TEN version: %s\n", ten_version());
    printf("initialized: %d\n", ten_is_initialized());

    ten_shutdown();
    printf("initialized after shutdown: %d\n", ten_is_initialized());

    return 0;
}
