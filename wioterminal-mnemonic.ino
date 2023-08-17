#include <SparkFunBQ27441.h>

#include "queue.h"
#include "debounce.h"
#include "canvas.h"
#include "sha256.h"
#include "wordlist.h"

/**********************************************************************
 *                             CONFIG                                 *
 **********************************************************************/

#define MIN_BYTES_FEED 512

#define BUTTON_DEBOUNCE 300

#define BATTERY_CAPACITY 650

#define ENTROPY_BYTES SHA256_BLOCK_SIZE
#define ENTROPY_BITS (SHA256_BLOCK_SIZE * 8)

#define CHECKSUM_BITS (ENTROPY_BITS / 32)
#define CHECKSUM_BYTES (CHECKSUM_BITS / 8)

#define MNEMONIC_WORDS ((ENTROPY_BITS + CHECKSUM_BITS) / WORDLIST_BITS)

/**********************************************************************
 *                               Core                                 *
 **********************************************************************/

Queue queue = Queue(8);

#define ACTION_BUTTON 1
#define ACTION_REDRAW 2

uint8_t entropy_chunk[3];

int entropy_bytes_feed = 0;

SHA256_CTX sha256_ctx;
uint8_t buf_entropy[ENTROPY_BYTES + CHECKSUM_BYTES];
uint8_t buf_checksum[SHA256_BLOCK_SIZE];

int mnemonic_created = 0;

int mnemonics[MNEMONIC_WORDS];

int battery_ready = 0;

int recovery_bits[WORDLIST_BITS + 1];
int recovery_cursor = 0;

/**********************************************************************
 *                                UI                                  *
 **********************************************************************/

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240

#define PADDING_LEFT 8
#define PADDING_TOP 0

#define TAB_ENTROPY 0
#define TAB_MNEMONIC 1
#define TAB_RECOVERY 2
#define TAB_BATTERY 3

#define PEAK_INPUTS 8

#define TAB_MIN TAB_ENTROPY
#define TAB_MAX TAB_BATTERY

#define DOTMAP_BITS 12

Canvas canvas = Canvas(SCREEN_WIDTH, SCREEN_HEIGHT, PADDING_LEFT, PADDING_TOP);

const char *TAB_INDICATOR = "<- PREV  NEXT ->";

const char *TAB_NAMES[] = {
    "ENTROPY",
    "MNEMONIC",
    "RECOVERY",
    "BATTERY",
};

int ui_idx_tab = TAB_ENTROPY;
int ui_idx_mnemonic = 0;

inline void ui_redraw()
{
    queue.send(ACTION_REDRAW, 0, 1);
}

void ui_switch_tab(int d)
{
    int idx = ui_idx_tab + d;
    if (idx > TAB_MAX)
    {
        idx = TAB_MIN;
    }
    else if (idx < TAB_MIN)
    {
        idx = TAB_MAX;
    }

    ui_idx_tab = idx;

    ui_redraw();
}

void ui_switch_mnemonic(int d)
{
    int idx = ui_idx_mnemonic + d;
    if (idx > MNEMONIC_WORDS - 1)
    {
        idx = 0;
    }
    else if (idx < 0)
    {
        idx = MNEMONIC_WORDS - 1;
    }
    ui_idx_mnemonic = idx;

    ui_redraw();
}

void ui_draw_head()
{
    canvas.string(TAB_INDICATOR)->nextX()->addX(32);
    canvas.stringf("[%d/%d] %s", ui_idx_tab + 1, TAB_MAX + 1, TAB_NAMES[ui_idx_tab]);
    canvas.resetX()->nextY()->nextY();
}

void ui_draw_tab_entropy()
{
    canvas.string("STICK UP/DOWN/LEFT/RIGHT TO FEED ENTROPY")->nextY()->nextY();

    canvas.stringf("BYTES FEED: %d/%d", entropy_bytes_feed, MIN_BYTES_FEED)->nextY()->nextY();

    if (mnemonic_created)
    {
        canvas.string("MNIMONIC GENERATED");
    }
    else
    {
        if (entropy_bytes_feed >= MIN_BYTES_FEED)
        {
            canvas.string("PRESS STICK TO GENERATE MNEMONIC");
        }
        else
        {
            canvas.string("ENTROPY NOT ENOUGH");
        }

        if (entropy_bytes_feed == 0)
        {
            return;
        }

        canvas.nextY()->nextY();

        canvas.string("LAST INPUT: ")->nextX();

        for (int i = 0; i < sizeof(entropy_chunk); i++)
        {
            canvas.stringf("%02x", entropy_chunk[i])->nextX()->addX(8);
        }
    }
}

void ui_draw_tab_mnemonic()
{
    if (!mnemonic_created)
    {
        canvas.string("NOT AVAILABLE");
        return;
    }

    canvas.string("UP/DOWN TO SWITCH MNEMONIC WORD")->nextY()->nextY();

    int v = mnemonics[ui_idx_mnemonic];

    canvas.stringf("[%d%d/%d] %s", (ui_idx_mnemonic + 1) / 10, (ui_idx_mnemonic + 1) % 10, MNEMONIC_WORDS, WORDLIST[v])->nextY()->nextY();

    canvas.string("DECIMAL")->addX(100)->string("BINARY")->resetX()->nextY();

    canvas.stringf("%d", v)->addX(100);

    for (int i = 0; i < WORDLIST_BITS; i++)
    {
        canvas.stringf("%d", (v >> (WORDLIST_BITS - 1 - i)) & 1)->nextX();
    }

    canvas.nextY()->nextY()->resetX();

    canvas.string("ONEKEY DOTMAP")->nextY();

    int mv = v + 1;

    for (int i = 0; i < DOTMAP_BITS; i++)
    {
        canvas.cube((mv >> (DOTMAP_BITS - 1 - i)) & 1)->nextX();

        if (i % 4 == 3)
        {
            canvas.addX(8);
        }
    }

    canvas.resetX();
}

void ui_draw_tab_recovery()
{
    int v = 0;

    for (int i = 0; i < (WORDLIST_BITS + 1); i++)
    {
        int b = recovery_bits[i] ? 1 : 0;
        int u = recovery_cursor == i ? 1 : 0;

        v = v | b << (WORDLIST_BITS - i);

        canvas.cube(b | (u << 1))->nextX();

        if (i % 4 == 3)
        {
            canvas.addX(8);
        }
    }

    canvas.nextY()->nextY()->resetX();

    if (v < 1 || v > WORDLIST_SIZE)
    {
        canvas.string("INVALID")->nextY();
        return;
    }

    v = v - 1;

    canvas.string(WORDLIST[v]);

    canvas.nextY()->nextY()->resetX();

    canvas.string("DECIMAL")->addX(100)->string("BINARY")->resetX()->nextY();

    canvas.stringf("%d", v)->addX(100);

    for (int i = 0; i < WORDLIST_BITS; i++)
    {
        canvas.stringf("%d", (v >> (WORDLIST_BITS - 1 - i)) & 1)->nextX();
    }
}

void ui_draw_tab_battery()
{
    if (!battery_ready)
    {
        canvas.string("NOT AVAILABLE");
        return;
    }

    canvas.stringf("SOC: %d%%", lipo.soc())->nextY();
    canvas.stringf("SOH: %d%%", lipo.soh())->nextY();
    canvas.stringf("POWER: %dmW", lipo.power())->nextY();
    canvas.stringf("VOLTAGE: %dmV", lipo.voltage())->nextY();
    canvas.stringf("CURRENT: %dmA", lipo.current(AVG))->nextY();
    canvas.stringf("CAPACITY FULL: %dmAh", lipo.capacity(FULL))->nextY();
    canvas.stringf("CAPACITY REMAIN: %dmAh", lipo.capacity(REMAIN))->nextY();
    canvas.stringf("TEMPERATURE: %dC", lipo.temperature())->nextY();
}

/**********************************************************************
 *                             ENTROPY                                *
 **********************************************************************/

void entropy_setup()
{
    pinMode(WIO_MIC, INPUT);

    sha256_init(&sha256_ctx);
}

void entropy_feed(int btn)
{
    if (mnemonic_created)
    {
        return;
    }

    // build chunk
    entropy_chunk[0] = SysTick->VAL & 0xff;
    entropy_chunk[1] = analogRead(WIO_MIC) & 0xff;
    entropy_chunk[2] = btn & 0xff;

    // update chunk
    sha256_update(&sha256_ctx, entropy_chunk, sizeof(entropy_chunk));

    entropy_bytes_feed += sizeof(entropy_chunk);

    ui_redraw();
}

void entropy_finish()
{
    if (mnemonic_created)
    {
        return;
    }

    // sha256 -> buf_entropy
    sha256_final(&sha256_ctx, buf_entropy);

    // buf_entropy -> checksum
    sha256_init(&sha256_ctx);
    sha256_update(&sha256_ctx, buf_entropy, ENTROPY_BYTES);
    sha256_final(&sha256_ctx, buf_checksum);

    // buf_entropy set checksum
    uint8_t c = 0;
    for (int i = 0; i < CHECKSUM_BITS; i++)
    {
        int b = ((buf_checksum[0] >> i) & 1) << i;

        c = c | b;
    }
    buf_entropy[ENTROPY_BYTES] = c;

    // create mnemonics
    for (int i = 0; i < MNEMONIC_WORDS; i++)
    {
        int v = 0;

        for (int j = 0; j < WORDLIST_BITS; j++)
        {
            int bit_i = i * WORDLIST_BITS + j;

            // I don't know why, but it has set be this way
            int b = ((buf_entropy[bit_i / 8] >> (8 - 1 - bit_i % 8)) & 1) << WORDLIST_BITS - 1 - j;

            v = v | b;
        }

        mnemonics[i] = v;
    }

    mnemonic_created = 1;

    ui_idx_mnemonic = 0;

    ui_redraw();
}

/**********************************************************************
 *                             RECOVERY                               *
 **********************************************************************/

void recovery_setup()
{
    memset(recovery_bits, 0, sizeof(recovery_bits));
}

void recovery_move_cursor(int d)
{
    int idx = recovery_cursor + d;
    if (idx >= (WORDLIST_BITS + 1))
    {
        idx = 0;
    }
    else if (idx < 0)
    {
        idx = WORDLIST_BITS;
    }
    recovery_cursor = idx;
    ui_redraw();
}

void recovery_set_and_next(int v)
{
    if (v)
    {
        v = 1;
    }
    else
    {
        v = 0;
    }
    recovery_bits[recovery_cursor] = v;
    recovery_move_cursor(1);
}

void recovery_reset()
{
    memset(recovery_bits, 0, sizeof(recovery_bits));
    recovery_cursor = 0;
    ui_redraw();
}

/**********************************************************************
 *                        BUTTON HANDLERS                             *
 **********************************************************************/

Debounce debounce = Debounce(BUTTON_DEBOUNCE);

#define DECLARE_ON_BUTTON(BTN)                \
    void on_button_##BTN()                    \
    {                                         \
        if (debounce.debounce(WIO_##BTN))     \
        {                                     \
            return;                           \
        }                                     \
        queue.send(ACTION_BUTTON, WIO_##BTN); \
    }

#define ATTACH_ON_BUTTON(BTN) \
    attachInterrupt(digitalPinToInterrupt(WIO_##BTN), on_button_##BTN, FALLING);

DECLARE_ON_BUTTON(KEY_B)
DECLARE_ON_BUTTON(KEY_C)

DECLARE_ON_BUTTON(5S_UP)
DECLARE_ON_BUTTON(5S_DOWN)
DECLARE_ON_BUTTON(5S_LEFT)
DECLARE_ON_BUTTON(5S_RIGHT)
DECLARE_ON_BUTTON(5S_PRESS)

void setup_buttons()
{
    // KEY_A conflict with 5S_UP
    ATTACH_ON_BUTTON(KEY_B)
    ATTACH_ON_BUTTON(KEY_C)

    ATTACH_ON_BUTTON(5S_UP)
    ATTACH_ON_BUTTON(5S_DOWN)
    ATTACH_ON_BUTTON(5S_LEFT)
    ATTACH_ON_BUTTON(5S_RIGHT)
    ATTACH_ON_BUTTON(5S_PRESS)
}

/**********************************************************************
 *                         BATTERY SETUP                              *
 **********************************************************************/

void setup_battery()
{
    if (lipo.begin())
    {
        battery_ready = 1;
        lipo.setCapacity(BATTERY_CAPACITY);
    }
}

/**********************************************************************
 *                             MAIN                                   *
 **********************************************************************/

void setup(void)
{
    entropy_setup();
    recovery_setup();
    canvas.setup();
    setup_buttons();
    setup_battery();

    ui_redraw();
}

void loop()
{
    Queue::Item item = queue.retrieve();

    if (item.kind == QUEUE_EMPTY)
    {
        return;
    }

    if (item.kind == ACTION_BUTTON)
    {
        switch (item.value)
        {
        case WIO_KEY_B:
            ui_switch_tab(1);
            break;
        case WIO_KEY_C:
            ui_switch_tab(-1);
            break;
        case WIO_5S_UP:
            if (ui_idx_tab == TAB_ENTROPY)
            {
                entropy_feed(1);
            }
            if (ui_idx_tab == TAB_MNEMONIC)
            {
                ui_switch_mnemonic(-1);
            }
            if (ui_idx_tab == TAB_RECOVERY)
            {
                recovery_set_and_next(1);
            }
            break;
        case WIO_5S_DOWN:
            if (ui_idx_tab == TAB_ENTROPY)
            {
                entropy_feed(2);
            }
            if (ui_idx_tab == TAB_MNEMONIC)
            {
                ui_switch_mnemonic(1);
            }
            if (ui_idx_tab == TAB_RECOVERY)
            {
                recovery_set_and_next(0);
            }
            break;
        case WIO_5S_LEFT:
            if (ui_idx_tab == TAB_ENTROPY)
            {
                entropy_feed(3);
            }
            if (ui_idx_tab == TAB_RECOVERY)
            {
                recovery_move_cursor(-1);
            }
            break;
        case WIO_5S_RIGHT:
            if (ui_idx_tab == TAB_ENTROPY)
            {
                entropy_feed(4);
            }
            if (ui_idx_tab == TAB_RECOVERY)
            {
                recovery_move_cursor(1);
            }
            break;
        case WIO_5S_PRESS:
            if (ui_idx_tab == TAB_ENTROPY)
            {
                if (entropy_bytes_feed >= MIN_BYTES_FEED)
                {
                    entropy_finish();
                }
            }
            if (ui_idx_tab == TAB_RECOVERY)
            {
                recovery_reset();
            }
            if (ui_idx_tab == TAB_BATTERY)
            {
                ui_redraw();
            }
            break;
        }
    }

    if (item.kind == ACTION_REDRAW)
    {
        canvas.refresh();

        ui_draw_head();

        if (ui_idx_tab == TAB_ENTROPY)
        {
            ui_draw_tab_entropy();
        }

        if (ui_idx_tab == TAB_MNEMONIC)
        {
            ui_draw_tab_mnemonic();
        }

        if (ui_idx_tab == TAB_RECOVERY)
        {
            ui_draw_tab_recovery();
        }

        if (ui_idx_tab == TAB_BATTERY)
        {
            ui_draw_tab_battery();
        }
    }
}
