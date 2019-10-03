#include "common.h"
#include <string.h>

#include "periph/gpio.h"
#include "periph/pm.h"

#include "luid.h"
#include "thread.h"
#include "fmt.h"
#include "net/loramac.h"
#include "shell.h"

static loramac_state_t state;

static int cmd_set(int argc, char **argv)
{
    char *usage = "usage: set (deveui|appeui|appkey) value";
    if (argc == 3) {
        if (strcmp(argv[1], "deveui") == 0) {
            if (fmt_strlen(argv[2]) != LORAMAC_DEVEUI_LEN * 2) {
                printf("DEVEUI should be %d bytes\n", LORAMAC_DEVEUI_LEN);
                return 1;
            }
            fmt_hex_bytes(state.deveui, argv[2]);
        } else if (strcmp(argv[1], "appeui") == 0) {
            if (fmt_strlen(argv[2]) != LORAMAC_APPEUI_LEN * 2) {
                printf("APPEUI should be %d bytes\n", LORAMAC_APPEUI_LEN);
                return 1;
            }
            fmt_hex_bytes(state.appeui, argv[2]);
        } else if (strcmp(argv[1], "appkey") == 0) {
            if (fmt_strlen(argv[2]) != LORAMAC_APPKEY_LEN * 2) {
                printf("APPKEY should be %d bytes\n", LORAMAC_APPKEY_LEN);
                return 1;
            }
            fmt_hex_bytes(state.appkey, argv[2]);
        } else {
            puts(usage);
            return 1;
        }
    } else {
        puts(usage);
        return 1;
    }
    return 0;
}

static int cmd_get(int argc, char **argv)
{
    char *usage = "usage: get (deveui|appeui|appkey)";
    if (argc == 2) {
        if (strcmp(argv[1], "deveui") == 0) {
            char s[LORAMAC_DEVEUI_LEN*2+1];
            int n;
            n = fmt_bytes_hex(s, state.deveui, LORAMAC_DEVEUI_LEN);
            s[n] = 0;
            printf("deveui=%s\n", s);
        } else if (strcmp(argv[1], "appeui") == 0) {
            char s[LORAMAC_APPEUI_LEN*2+1];
            int n;
            n = fmt_bytes_hex(s, state.appeui, LORAMAC_APPEUI_LEN);
            s[n] = 0;
            printf("appeui=%s\n", s);
        } else if (strcmp(argv[1], "appkey") == 0) {
            char s[LORAMAC_APPKEY_LEN*2+1];
            int n;
            n = fmt_bytes_hex(s, state.appkey, LORAMAC_APPKEY_LEN);
            s[n] = 0;
            printf("appkey=%s\n", s);
        } else {
            puts(usage);
            return 1;
        }
    } else {
        puts(usage);
        return 1;
    }
    return 0;
}

static int cmd_show(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    print_loramac_state (&state);
    return 0;
}

static int cmd_load(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    load_from_flash (&state);
    return 0;
}

static int cmd_erase(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    puts("Erasing stored parameters.");
    erase_flash();
    return 0;
}

static int cmd_save(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    save_to_flash (&state);
    return 0;
}

static int cmd_reboot(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    pm_reboot();
    return 0;
}

static const shell_command_t shell_commands[] = {
    { "set",    "set values",           cmd_set    },
    { "get",    "get values",           cmd_get    },
    { "show",   "show configuration",   cmd_show   },
    { "load",   "load flash contents",  cmd_load   },
    { "erase",  "clear flash contents", cmd_erase  },
    { "save",   "save to flash",        cmd_save   },
    { "reboot", "reboot",               cmd_reboot },
    { NULL, NULL, NULL }
};

static char _stack[THREAD_STACKSIZE_DEFAULT];
void *blink_led(void *arg) {
    (void)arg;

    gpio_init(LED1_PIN, GPIO_OUT);
    while (true) {
        gpio_toggle(LED1_PIN);
        xtimer_sleep(1);
    }
}

void enter_configuration_mode(void) {
    thread_create(_stack, sizeof(_stack), THREAD_PRIORITY_MAIN - 1,
        THREAD_CREATE_WOUT_YIELD | THREAD_CREATE_STACKTEST,
        blink_led, NULL, "led");

    char line_buf[SHELL_DEFAULT_BUFSIZE];

    if (load_from_flash (&state) < 0) {
        memcpy(state.magic, "RIOT", 4);
        luid_get(state.deveui, LORAMAC_DEVEUI_LEN);
    }
    shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);
}
