#include <fcntl.h>
#include <rp6502.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "game.h"
#include "scene.h"
#include "talk.h"
#include "ui.h"
#include "msg.h"

#define PARTY_SIZE 5
#define NAME_MAX 9
#define HEALTH_CAP 139.0f
#define DEATH_HEALTH_CAP 105.0f
#define WEEK_DAYS 7
#define LAST_LANDMARK 17
#define DALLES 16
#define FERRY_CENTS 500L
#define TOLL_BASE_CENTS 500L
#define TOLL_PER_OX_CENTS 50L
#define OXEN_CAP 20
#define FOOD_CAP 2000
#define PART_CAP 3
#define BULLETS_PER_BOX 20
#define START_YEAR 1848
#define MAX_REST_DAYS 9

#define LM_OTHER 0
#define LM_STORE 1
#define LM_RIVER 2

enum { PACE_STEADY, PACE_STRENUOUS, PACE_GRUELING };
enum { RATION_FILLING, RATION_MEAGER, RATION_BARE };
enum { PART_WHEEL, PART_AXLE, PART_TONGUE, PART_COUNT };
enum { ITEM_OXEN, ITEM_CLOTHING, ITEM_AMMO, ITEM_WHEEL, ITEM_AXLE, ITEM_TONGUE, ITEM_FOOD, ITEM_COUNT };

typedef enum {
    ACT_CONTINUE,
    ACT_SUPPLIES,
    ACT_MAP,
    ACT_PACE,
    ACT_RATIONS,
    ACT_REST,
    ACT_TRADE,
    ACT_HUNT,
    ACT_BUY,
    ACT_TALK,
    ACT_QUIT
} action_t;

typedef struct {
    const char *name;
    uint8_t type;
    uint8_t leg;
    int8_t alt_leg;
} landmark_t;

typedef struct {
    uint16_t miles;
    uint8_t per_day;
    uint8_t to;
} leg_t;

typedef struct {
    uint8_t depth;
    uint16_t width;
    uint8_t swift;
    uint8_t bottom;
    uint8_t extra;
} river_t;

typedef struct {
    int lo;
    int hi;
} range_t;

#define GRAVE_FILE "oregon.grv"
#define EPITAPH_MAX 29

typedef struct {
    uint8_t leg;
    int left;
    char name[NAME_LEN];
    char epitaph[EPITAPH_MAX + 1];
} grave_t;

typedef struct {
    char name[NAME_LEN];
    int affl;
    int affl_days;
} member_t;

typedef struct {
    member_t party[PARTY_SIZE];
    int np;
    int ox2;
    int clothing;
    int bullets;
    int spares[PART_COUNT];
    int food;
    long money;
    int pace;
    int rations;
    int occ;
    int month;
    int day;
    int year;
    int lm;
    int leg;
    float dist;
    float miles;
    float health;
    float fs;
    float ar;
    float as;
    int tm;
    int weather;
    bool precip;
    float tr;
    float ts;
    float hr;
    int broken;
    bool crossing;
    bool said;
    bool won;
    int talk_turn;
    char leader[NAME_LEN];
    unsigned long visited;
    grave_t grave;
    bool has_grave;
} game_t;

/* Index 0 is Independence. A landmark's leg is the one it departs on. */
static const landmark_t landmarks[18] = {
    {"Independence", LM_STORE, 0, -1},
    {"Kansas River", LM_RIVER, 1, -1},
    {"Big Blue River", LM_RIVER, 2, -1},
    {"Fort Kearney", LM_STORE, 3, -1},
    {"Chimney Rock", LM_OTHER, 4, -1},
    {"Fort Laramie", LM_STORE, 5, -1},
    {"Independence Rock", LM_OTHER, 6, -1},
    {"South Pass", LM_OTHER, 7, 8},
    {"Fort Bridger", LM_STORE, 9, -1},
    {"Green River", LM_RIVER, 10, -1},
    {"Soda Springs", LM_OTHER, 11, -1},
    {"Fort Hall", LM_STORE, 12, -1},
    {"Snake River", LM_RIVER, 13, -1},
    {"Fort Boise", LM_STORE, 14, -1},
    {"Blue Mountains", LM_OTHER, 15, 16},
    {"Fort Walla Walla", LM_STORE, 17, -1},
    {"The Dalles", LM_OTHER, 18, -1},
    {"Willamette Valley", LM_OTHER, 0, -1},
};

static const leg_t legs[19] = {
    {102, 20, 1}, {83, 20, 2}, {119, 20, 3}, {250, 20, 4}, {86, 20, 5},
    {190, 12, 6}, {102, 12, 7}, {57, 12, 9}, {125, 12, 8}, {162, 12, 10},
    {144, 12, 10}, {57, 12, 11}, {182, 12, 12}, {114, 12, 13}, {160, 12, 14},
    {55, 12, 15}, {125, 12, 16}, {120, 12, 16}, {100, 12, 17},
};

/* Indexed by river number, which river_index() gives from the landmark. */
static const river_t rivers[4] = {
    {1, 600, 3, 0, 2},
    {1, 220, 2, 1, 0},
    {20, 400, 5, 2, 2},
    {6, 1000, 7, 2, 1},
};

/* Monthly base temperature (F) and precipitation chance (per mille), by zone. */
static const int8_t climate_temp[5][12] = {
    {9, 13, 23, 36, 45, 55, 60, 58, 50, 39, 25, 14},
    {3, 8, 16, 29, 39, 49, 55, 53, 43, 31, 17, 7},
    {3, 7, 12, 22, 32, 42, 51, 49, 38, 27, 13, 6},
    {-1, 4, 12, 23, 32, 41, 49, 47, 37, 26, 11, 1},
    {10, 16, 22, 30, 38, 45, 54, 52, 43, 32, 20, 12},
};
static const uint16_t climate_rain[5][12] = {
    {39, 42, 78, 99, 144, 144, 117, 120, 126, 90, 57, 45},
    {15, 15, 30, 63, 90, 99, 81, 66, 48, 30, 15, 15},
    {15, 15, 27, 48, 63, 39, 30, 18, 27, 27, 21, 15},
    {15, 21, 36, 69, 75, 39, 24, 15, 33, 42, 24, 18},
    {45, 39, 39, 36, 36, 27, 9, 9, 18, 30, 39, 42},
};

static const int month_len[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
static const char *const month_name[12] = {
    "January", "February", "March", "April", "May", "June",
    "July", "August", "September", "October", "November", "December"};
static const char *const weather_name[10] = {
    "very cold", "cold", "cool", "warm", "hot", "very hot",
    "rainy", "snowy", "very rainy", "very snowy"};
static const char *const affliction_name[9] = {
    "a broken arm", "a broken leg", "a snakebite", "exhaustion", "typhoid",
    "cholera", "measles", "dysentery", "a fever"};
static const char *const part_name[PART_COUNT] = {"wheel", "axle", "tongue"};
static const char *const part_plural[PART_COUNT] = {"wagon wheels", "wagon axles", "wagon tongues"};

static const char *const occ_name[3] = {"Banker", "Carpenter", "Farmer"};
static const long occ_money[3] = {160000L, 80000L, 40000L};
static const char *const default_names[PARTY_SIZE - 1] = {"Amos", "Clara", "Ned", "Hattie"};

/* Base unit prices in cents: Matt's price, and the value used for trading. */
static const char *const item_label[ITEM_COUNT] = {
    "Oxen (each)", "Clothing (set)", "Ammo (box of 20)",
    "Wagon wheel", "Wagon axle", "Wagon tongue", "Food (per pound)"};
static const long fort_base_cents[ITEM_COUNT] = {2000L, 1000L, 200L, 1000L, 1000L, 1000L, 20L};
static const long trade_cents[ITEM_COUNT] = {2000L, 1000L, 10L, 1000L, 1000L, 1000L, 20L};
static const char *const item_plural[ITEM_COUNT] = {
    "oxen", "sets of clothing", "bullets", "wagon wheels", "wagon axles", "wagon tongues", "pounds of food"};
static const char *const item_singular[ITEM_COUNT] = {
    "ox", "set of clothing", "bullet", "wagon wheel", "wagon axle", "wagon tongue", "pound of food"};

static const char *const action_label[11] = {
    "Continue on the trail",
    "Check supplies",
    "Look at the map",
    "Change pace",
    "Change food rations",
    "Stop to rest",
    "Attempt to trade",
    "Hunt for food",
    "Buy supplies",
    "Talk to people",
    "Give up the journey",
};

static void rest_days(game_t *g, int n);
static void day_tick(game_t *g, bool travel);
static void draw_status(game_t *g);
static void illness(game_t *g);

static float rndf(void)
{
    return (float)rand() / ((float)RAND_MAX + 1.0f);
}

static bool chance(float p)
{
    return rndf() < p;
}

static int rnd(int n)
{
    return rand() % n;
}

static int ranged(int lo, int hi)
{
    return lo + rnd(hi - lo + 1);
}

static int imin(int a, int b)
{
    return a < b ? a : b;
}

static float round1(float x)
{
    return (float)((int)(x * 10.0f + 0.5f)) / 10.0f;
}

static void say(game_t *g, const char *text)
{
    puts(text);
    g->said = true;
}

static int zone_of(int lm)
{
    return (lm > 2) + (lm > 5) + (lm > 10) + (lm > 13);
}

static int river_index(int lm)
{
    switch (lm) {
    case 1: return 0;
    case 2: return 1;
    case 9: return 2;
    default: return 3;
    }
}

static bool blocked(const game_t *g)
{
    return g->broken != 0 || g->ox2 <= 0;
}

static int whole_oxen(const game_t *g)
{
    return g->ox2 / 2;
}

static const char *health_word(float h)
{
    static const char *const words[4] = {"Good", "Fair", "Poor", "Very poor"};
    int band = (int)(h / 35.0f);
    if (band < 0)
        band = 0;
    if (band > 3)
        band = 3;
    return words[band];
}

static void print_oxen(int ox2)
{
    printf(msg_get(0), ox2 / 2);
    if (ox2 & 1)
        printf(msg_get(1));
}

static void advance_day(game_t *g)
{
    if (++g->day > month_len[g->month]) {
        g->day = 1;
        if (++g->month > 11) {
            g->month = 0;
            g->year++;
        }
    }
}

static int member_pick(const game_t *g)
{
    int z = rnd(g->np);
    if (z == 0 && g->np > 1)
        z = 1;
    return z;
}

static void member_die(game_t *g, int i, const char *cause)
{
    char buf[80];

    sprintf(buf, "%s has died of %s.", g->party[i].name, cause);
    say(g, buf);
    g->party[i] = g->party[g->np - 1];
    g->np--;
    if (g->health > DEATH_HEALTH_CAP)
        g->health = DEATH_HEALTH_CAP;
}

static void afflict(game_t *g, int i, int kind)
{
    char buf[80];

    g->party[i].affl = kind;
    g->party[i].affl_days = kind <= 1 ? 30 : 10;
    sprintf(buf, "%s has %s.", g->party[i].name, affliction_name[kind]);
    say(g, buf);
}

static void drown(game_t *g, float p)
{
    int i, lowest = g->np > 1 ? 1 : 0;

    for (i = g->np - 1; i >= lowest; i--) {
        if (chance(p))
            member_die(g, i, "drowning");
    }
}

static void report_loss(game_t *g, bool *head, const char *heading, int n, const char *label)
{
    if (!*head) {
        say(g, heading);
        *head = true;
    }
    printf(msg_get(2), n, label);
}

static void lose_count(game_t *g, int *count, float p, const char *label, const char *heading, bool *head)
{
    int n;

    if (*count <= 0 || !chance(p))
        return;
    n = ranged(1, *count);
    *count -= n;
    report_loss(g, head, heading, n, label);
}

static bool lose_goods(game_t *g, float p, const char *heading)
{
    bool head = false;
    int i;

    lose_count(g, &g->clothing, p, "sets of clothing", heading, &head);
    lose_count(g, &g->bullets, p, "bullets", heading, &head);
    for (i = 0; i < PART_COUNT; i++)
        lose_count(g, &g->spares[i], p, part_plural[i], heading, &head);
    lose_count(g, &g->food, p, "pounds of food", heading, &head);
    return head;
}

static bool lose_oxen(game_t *g, float p, const char *heading)
{
    bool head = false;
    int k, n = 0;

    for (k = whole_oxen(g); k > 0; k--) {
        if (chance(p)) {
            g->ox2 -= 2;
            n++;
        }
    }
    if (n)
        report_loss(g, &head, heading, n, "oxen");
    return n > 0;
}

static void item_take(game_t *g, int item, int n)
{
    switch (item) {
    case ITEM_OXEN: g->ox2 -= 2 * n; break;
    case ITEM_CLOTHING: g->clothing -= n; break;
    case ITEM_AMMO: g->bullets -= n; break;
    case ITEM_WHEEL: g->spares[PART_WHEEL] -= n; break;
    case ITEM_AXLE: g->spares[PART_AXLE] -= n; break;
    case ITEM_TONGUE: g->spares[PART_TONGUE] -= n; break;
    case ITEM_FOOD: g->food -= n; break;
    }
}

static void item_add(game_t *g, int item, int n)
{
    switch (item) {
    case ITEM_OXEN: g->ox2 += 2 * n; break;
    case ITEM_CLOTHING: g->clothing += n; break;
    case ITEM_AMMO: g->bullets += n; break;
    case ITEM_WHEEL: g->spares[PART_WHEEL] += n; break;
    case ITEM_AXLE: g->spares[PART_AXLE] += n; break;
    case ITEM_TONGUE: g->spares[PART_TONGUE] += n; break;
    case ITEM_FOOD: g->food += n; break;
    }
}

static int item_have(const game_t *g, int item)
{
    switch (item) {
    case ITEM_OXEN: return whole_oxen(g);
    case ITEM_CLOTHING: return g->clothing;
    case ITEM_AMMO: return g->bullets;
    case ITEM_WHEEL: return g->spares[PART_WHEEL];
    case ITEM_AXLE: return g->spares[PART_AXLE];
    case ITEM_TONGUE: return g->spares[PART_TONGUE];
    case ITEM_FOOD: return g->food;
    }
    return 0;
}

static int item_cap(int item)
{
    switch (item) {
    case ITEM_OXEN: return OXEN_CAP;
    case ITEM_FOOD: return FOOD_CAP;
    case ITEM_WHEEL:
    case ITEM_AXLE:
    case ITEM_TONGUE: return PART_CAP;
    }
    return -1;
}

static void member_afflict_random(game_t *g)
{
    int i = member_pick(g);

    afflict(g, i, chance(0.5f) ? 0 : 1);
}

static void illness(game_t *g)
{
    int i = member_pick(g);

    g->hr = 20.0f;
    if (g->party[i].affl >= 0) {
        member_die(g, i, "illness");
        return;
    }
    afflict(g, i, ranged(3, 8));
}

static void broken_wagon(game_t *g)
{
    int p = rnd(PART_COUNT);
    char buf[80];

    sprintf(buf, "A wagon %s is broken.", part_name[p]);
    say(g, buf);
    if (ui_ask_int(msg_get(3), 1, 2) == 1 && chance(0.5f))
        say(g, msg_get(4));
    else if (g->spares[p] > 0) {
        g->spares[p]--;
        say(g, msg_get(5));
    } else {
        g->broken = p + 1;
        say(g, msg_get(6));
    }
    rest_days(g, 1);
}

static void fire(game_t *g)
{
    say(g, msg_get(7));
    lose_goods(g, 0.5f, "The fire destroyed:");
}

static void abandoned_wagon(game_t *g)
{
    bool found = false;
    int n, p;

    if (chance(0.5f)) {
        n = ranged(1, 3);
        g->clothing += n;
        found = true;
    }
    if (chance(0.5f)) {
        n = ranged(1, 3);
        g->bullets += 21 * n;
        found = true;
    }
    for (p = 0; p < PART_COUNT; p++) {
        if (chance(0.5f)) {
            n = ranged(1, 3);
            if (g->spares[p] + n <= PART_CAP) {
                g->spares[p] += n;
                found = true;
            }
        }
    }
    say(g, found ? "You found supplies in an abandoned wagon." : "The abandoned wagon is empty.");
}

static void thief(game_t *g)
{
    int k = rnd(4), n = 0;

    switch (k) {
    case 0:
        if (whole_oxen(g) > 0) {
            n = ranged(1, imin(whole_oxen(g), 100));
            g->ox2 -= 2 * n;
            printf(msg_get(8), n);
        }
        break;
    case 1:
        if (g->clothing > 0) {
            n = ranged(1, imin(g->clothing, 100));
            g->clothing -= n;
            printf(msg_get(9), n);
        }
        break;
    case 2:
        if (g->bullets > 0) {
            n = ranged(1, imin(g->bullets, 100));
            g->bullets -= n;
            printf(msg_get(10), n);
        }
        break;
    case 3:
        if (g->food > 0) {
            n = ranged(1, imin(g->food, 100));
            g->food -= n;
            printf(msg_get(11), n);
        }
        break;
    }
    if (n)
        g->said = true;
}

/* The graves are records in GRAVE_FILE. A grave lies on a leg, some miles
   before its end. */
static bool grave_next(int leg, int below, grave_t *out)
{
    int fd = open(GRAVE_FILE, O_RDONLY);
    grave_t rec;
    bool found = false;

    if (fd < 0)
        return false;
    while (read(fd, &rec, sizeof(rec)) == (int)sizeof(rec)) {
        if (rec.leg == leg && rec.left < below && (!found || rec.left > out->left)) {
            *out = rec;
            found = true;
        }
    }
    close(fd);
    return found;
}

static void grave_select(game_t *g)
{
    g->has_grave = grave_next(g->leg, (int)g->dist + 1, &g->grave);
}

static void grave_write(game_t *g)
{
    grave_t rec;
    int fd;

    memset(&rec, 0, sizeof(rec));
    rec.leg = (uint8_t)g->leg;
    rec.left = (int)g->dist;
    strcpy(rec.name, g->leader);
    printf(msg_get(12), EPITAPH_MAX);
    ui_read_line(rec.epitaph, EPITAPH_MAX + 1);
    fd = open(GRAVE_FILE, O_WRONLY | O_CREAT | O_APPEND);
    if (fd < 0)
        return;
    write(fd, &rec, sizeof(rec));
    close(fd);
}

static void show_grave(const grave_t *gr)
{
    printf(msg_get(13), gr->name, gr->epitaph);
    ui_pause();
}

static void show_map(game_t *g)
{
    int i;

    ui_clear();
    printf(msg_get(14));
    for (i = 0; i < (int)(sizeof(landmarks) / sizeof(landmarks[0])); i++) {
        bool seen = (g->visited >> i) & 1UL;
        printf(msg_get(15), seen ? "[x]" : "[ ]", landmarks[i].name,
               i == g->lm ? "  <- you are here" : "");
    }
    printf(msg_get(16), (int)g->miles);
    printf(msg_get(17), landmarks[legs[g->leg].to].name, (int)(g->dist + 0.5f));
    ui_pause();
}

static float event_chance(const game_t *g, int k, int zone)
{
    switch (k) {
    case 0: return g->as > 30.0f ? 1.0f : 0.0f;
    case 1: return 0.007f;
    case 3: return 0.01f + g->health / 1500.0f;
    case 4: return g->has_grave && g->dist < g->grave.left ? 1.0f : 0.0f;
    case 5: return g->food == 0 ? 0.05f : 0.0f;
    case 6: return (g->weather > 7 ? 1.0f : 0.0f) + (g->tm < 2 ? 0.15f : 0.0f);
    case 7: return 0.06f;
    case 8: return 0.04f + (zone > 2 ? 0.03f : 0.0f);
    case 9: return 0.02f;
    case 10: return zone > 2 ? 0.05f : 0.0f;
    case 11: return (g->month >= 4 && g->month <= 8) ? 0.04f : 0.0f;
    case 12: return 0.01f;
    case 13: return 0.02f;
    case 14: return g->ar < 0.1f ? 0.5f : 0.0f;
    }
    return 0.0f;
}

static void run_event(game_t *g, int k)
{
    switch (k) {
    case 0:
        say(g, msg_get(18));
        rest_days(g, ranged(1, 10));
        break;
    case 1:
        if (g->tm > 2)
            afflict(g, member_pick(g), 2);
        break;
    case 3:
        illness(g);
        break;
    case 4: {
        grave_t here = g->grave;

        say(g, msg_get(19));
        if (ui_ask_int(msg_get(20), 1, 2) == 1)
            show_grave(&here);
        g->has_grave = grave_next(g->leg, here.left, &g->grave);
        break;
    }
    case 5:
        g->food += 30;
        say(g, msg_get(21));
        break;
    case 6:
        if (g->tm < 2) {
            say(g, msg_get(22));
            g->as += 8.0f;
            rest_days(g, 1);
        } else if (g->tm > 3) {
            say(g, msg_get(23));
            g->ar += 1.0f;
            rest_days(g, 1);
        }
        break;
    case 7:
        if (g->lm > 11 && g->tm < 5) {
            say(g, msg_get(24));
            if (chance(0.5f))
                rest_days(g, 1);
        } else if (g->lm <= 11 && g->tm == 5) {
            say(g, msg_get(25));
        }
        break;
    case 8:
        if (chance(0.33f)) {
            broken_wagon(g);
        } else if (chance(0.5f)) {
            if (g->ox2 > 0) {
                g->ox2--;
                say(g, g->ox2 % 2 == 0 ? "One of the oxen has died." : "One of the oxen is injured.");
            }
        } else {
            member_afflict_random(g);
        }
        break;
    case 9:
        say(g, chance(0.5f) ? "You lost the trail." : "You took the wrong trail.");
        rest_days(g, ranged(1, 5));
        break;
    case 10:
        if (chance(0.5f)) {
            say(g, msg_get(26));
            g->hr += 10.0f;
        } else {
            say(g, msg_get(27));
            rest_days(g, ranged(1, 10));
        }
        break;
    case 11:
        g->food += 20;
        say(g, msg_get(28));
        break;
    case 12:
        switch (rnd(3)) {
        case 0:
            fire(g);
            break;
        case 1: {
            char buf[80];
            sprintf(buf, "%s is lost.", g->party[member_pick(g)].name);
            say(g, buf);
            rest_days(g, ranged(1, 5));
            break;
        }
        default:
            say(g, msg_get(29));
            rest_days(g, ranged(1, 3));
            break;
        }
        break;
    case 13:
        if (chance(0.5f))
            abandoned_wagon(g);
        else
            thief(g);
        break;
    case 14:
        if (chance(0.2f)) {
            say(g, msg_get(30));
            g->hr += 20.0f;
        } else if (chance(0.5f)) {
            say(g, msg_get(31));
            g->hr += 10.0f;
        } else {
            say(g, msg_get(32));
        }
        break;
    }
}

static void run_events(game_t *g)
{
    int zone = zone_of(g->lm);
    int k;

    for (k = 0; k < 15 && g->np > 0; k++) {
        float p = event_chance(g, k, zone);
        if (p > 0.0f && chance(p)) {
            run_event(g, k);
            if (blocked(g))
                break;
        }
    }
}

static void rest_days(game_t *g, int n)
{
    int i;

    for (i = 0; i < n && g->np > 0; i++)
        day_tick(g, false);
}

static void day_tick(game_t *g, bool travel)
{
    static const float zt_tab[6] = {2.0f, 1.0f, 0.0f, 0.0f, 1.0f, 2.0f};
    int zone, i, afflicted = 0, p;
    float qt, qp, zc, zf, zp, speed, v, snow, ratio;

    if (g->np == 0)
        return;
    zone = zone_of(g->lm);
    qt = climate_temp[zone][g->month];
    qp = climate_rain[zone][g->month] / 1000.0f;
    g->hr = 0.0f;
    if (g->health > HEALTH_CAP)
        g->health = HEALTH_CAP;

    if (travel)
        run_events(g);

    for (i = 0; i < g->np; i++) {
        if (g->party[i].affl >= 0) {
            if (--g->party[i].affl_days < 1)
                g->party[i].affl = -1;
            else
                afflicted++;
        }
    }

    if (chance(0.5f)) {
        int t = (int)qt + rnd(41);
        g->precip = chance(qp);
        g->tm = (t + 10) / 20;
        if (g->tm < 0)
            g->tm = 0;
        if (g->tm > 5)
            g->tm = 5;
        g->weather = g->tm;
    }

    g->tr = 0.0f;
    g->ts = 0.0f;
    if (g->precip) {
        bool heavy = chance(0.3f);
        if (g->tm < 2) {
            g->weather = heavy ? 9 : 7;
            g->ts = heavy ? 6.4f : 1.6f;
        } else {
            g->weather = heavy ? 8 : 6;
            g->tr = heavy ? 0.8f : 0.2f;
        }
    } else {
        g->weather = g->tm;
    }

    zc = 5.0f - 2.0f * g->tm - (float)g->clothing / (float)g->np;
    if (zc < 0.0f)
        zc = 0.0f;
    zf = g->food == 0 ? 8.0f : 2.0f * g->rations;
    p = travel ? g->pace + 1 : 0;
    zp = 2.0f * p + (g->weather > 5 ? 1.0f : 0.0f) + (g->weather > 7 ? 1.0f : 0.0f);
    if (zc > 0.5f || g->food == 0)
        g->fs += 0.8f;
    else
        g->fs *= 0.5f;
    g->health = 0.9f * g->health + zt_tab[g->tm] + zc + zf + zp + g->fs + afflicted + g->hr;
    {
        int eat = g->np * (3 - g->rations);
        g->food = g->food > eat ? g->food - eat : 0;
    }
    if (g->health > HEALTH_CAP && !g->crossing)
        illness(g);
    if (g->health > HEALTH_CAP)
        g->health = HEALTH_CAP;

    g->ar = 0.9f * g->ar + g->tr;
    g->as = 0.97f * g->as + g->ts;
    if (g->as > 0.0f && (g->tm > 2 || g->weather == 8)) {
        g->ar += 0.5f;
        g->as -= 5.0f;
        if (g->as < 0.0f)
            g->as = 0.0f;
    }

    if (travel && g->np > 0 && g->leg >= 0) {
        snow = 1.0f - g->as / 40.0f;
        if (snow < 0.0f)
            snow = 0.0f;
        ratio = (g->ox2 / 2.0f) / 4.0f;
        if (ratio > 1.0f)
            ratio = 1.0f;
        speed = legs[g->leg].per_day * ratio * (g->pace + 2) / 2.0f;
        v = speed * (1.0f - 0.1f * afflicted) * snow;
        if (v < 0.0f)
            v = 0.0f;
        if (1.1f * v > g->dist)
            v = g->dist;
        g->dist -= v;
        g->miles += v;
        if (g->dist < 0.0f)
            g->dist = 0.0f;
    }

    advance_day(g);
}

static void travel_week(game_t *g)
{
    int d;

    g->said = false;
    for (d = 0; d < WEEK_DAYS && g->dist > 0.0f && g->np > 0 && !blocked(g); d++) {
        char line[8];

        day_tick(g, true);
        if (g->said && g->np > 0)
            ui_pause();
        g->said = false;
        if (d == WEEK_DAYS - 1 || g->dist <= 0.0f || g->np == 0 || blocked(g))
            break;
        draw_status(g);
        printf(msg_get(160));
        ui_read_line(line, sizeof(line));
        if (line[0] == 'm' || line[0] == 'M')
            break;
    }
    g->said = false;
}
static scene_t scene_for(const game_t *g)
{
    int type = landmarks[g->lm].type;

    if (type == LM_RIVER)
        return SCENE_RIVER;
    if (type == LM_STORE)
        return SCENE_FORT;
    return zone_of(g->lm) >= 2 ? SCENE_MOUNTAIN : SCENE_PRAIRIE;
}

static void draw_status(game_t *g)
{
    int i, m = g->month, winter = (m == 10 || m == 11 || m <= 1);
    int next = legs[g->leg].to;

    scene_draw(scene_for(g), winter);
    ui_clear();
    printf(msg_get(33), month_name[m], g->day, g->year, (int)g->miles);
    printf(msg_get(34), weather_name[g->weather]);
    printf(msg_get(35), landmarks[next].name, (int)(g->dist + 0.5f));
    printf(msg_get(36),
           health_word(g->health),
           g->pace == PACE_STEADY ? "Steady" : g->pace == PACE_STRENUOUS ? "Strenuous" : "Grueling",
           g->rations == RATION_FILLING ? "Filling" : g->rations == RATION_MEAGER ? "Meager" : "Bare-bones");
    printf(msg_get(37));
    ui_print_money(g->money);
    printf(msg_get(38), g->food, g->bullets, g->clothing);
    printf(msg_get(39));
    print_oxen(g->ox2);
    printf(msg_get(40), g->spares[PART_WHEEL], g->spares[PART_AXLE], g->spares[PART_TONGUE]);
    printf(msg_get(41));
    for (i = 0; i < g->np; i++) {
        if (g->party[i].affl >= 0)
            printf(msg_get(42), g->party[i].name, affliction_name[g->party[i].affl]);
        else
            printf(msg_get(43), g->party[i].name);
    }
    printf(msg_get(44));
}

static void resolve_broken(game_t *g)
{
    if (g->broken && g->spares[g->broken - 1] > 0) {
        g->spares[g->broken - 1]--;
        g->broken = 0;
        say(g, msg_get(45));
    }
}

static const int store_order[ITEM_COUNT] = {ITEM_OXEN, ITEM_FOOD, ITEM_CLOTHING, ITEM_AMMO,
                                             ITEM_WHEEL, ITEM_AXLE, ITEM_TONGUE};

static void matts_store(game_t *g)
{
    int yoke = 0, food = 0, cloth = 0, ammo = 0, part[PART_COUNT] = {0, 0, 0};
    int choice, i;
    long bill;
    const char *labels[7] = {"Oxen, yoke of two", "Food (per pound)", "Clothing (set)",
                             "Ammo (box of 20)", "Wagon wheel", "Wagon axle", "Wagon tongue"};
    int limits[7] = {9, 2000, 99, 99, PART_CAP, PART_CAP, PART_CAP};
    long unit[7] = {4000L, 20L, 1000L, 200L, 1000L, 1000L, 1000L};
    int *qty[7];

    qty[0] = &yoke;
    qty[1] = &food;
    qty[2] = &cloth;
    qty[3] = &ammo;
    qty[4] = &part[0];
    qty[5] = &part[1];
    qty[6] = &part[2];

    scene_draw(SCENE_STORE, false);
    for (;;) {
        bill = 0;
        for (i = 0; i < 7; i++)
            bill += unit[i] * *qty[i];
        ui_clear();
        printf(msg_get(46));
        ui_print_money(g->money);
        printf(msg_get(47));
        for (i = 0; i < 7; i++) {
            printf(msg_get(48), i + 1, labels[i]);
            ui_print_money(unit[i]);
            printf(msg_get(49), *qty[i]);
        }
        printf(msg_get(50));
        ui_print_money(bill);
        printf(msg_get(51));
        choice = ui_ask_int(msg_get(52), 0, 7);
        if (choice == 0) {
            if (bill > g->money) {
                say(g, msg_get(53));
                ui_pause();
                continue;
            }
            if (yoke == 0) {
                say(g, msg_get(54));
                ui_pause();
                continue;
            }
            g->money -= bill;
            item_add(g, ITEM_OXEN, 2 * yoke);
            g->food += food;
            g->clothing += cloth;
            g->bullets += ammo * BULLETS_PER_BOX;
            for (i = 0; i < PART_COUNT; i++)
                g->spares[i] += part[i];
            return;
        }
        *qty[choice - 1] = ui_ask_int(msg_get(55), 0, limits[choice - 1]);
    }
}

static void fort_store(game_t *g)
{
    int q = (g->lm > 2) + (g->lm > 4) + (g->lm > 7) + (g->lm > 10) + (g->lm > 12) + (g->lm > 13);
    long mult = 4L + q;
    int choice, item, i, cap, afford, n;
    long price;

    scene_draw(SCENE_STORE, false);
    for (;;) {
        ui_clear();
        printf(msg_get(56));
        ui_print_money(g->money);
        printf(msg_get(57));
        for (i = 0; i < ITEM_COUNT; i++) {
            item = store_order[i];
            printf(msg_get(58), i + 1, item_label[item]);
            ui_print_money(fort_base_cents[item] * mult / 4L);
            printf(msg_get(59));
        }
        printf(msg_get(60));
        choice = ui_ask_int(msg_get(61), 0, ITEM_COUNT);
        if (choice == 0)
            return;
        item = store_order[choice - 1];
        price = fort_base_cents[item] * mult / 4L;
        afford = (int)(g->money / price);
        if (afford == 0) {
            say(g, msg_get(62));
            ui_pause();
            continue;
        }
        cap = item_cap(item);
        if (cap >= 0) {
            int room = cap - item_have(g, item);
            if (room <= 0) {
                if (item == ITEM_OXEN)
                    say(g, msg_get(63));
                else if (item == ITEM_FOOD)
                    say(g, msg_get(64));
                else
                    say(g, msg_get(65));
                ui_pause();
                continue;
            }
            afford = imin(afford, room);
        }
        n = ui_ask_int(msg_get(66), 0, afford);
        if (n == 0)
            continue;
        g->money -= price * n;
        item_add(g, item, n);
    }
}

static void do_trade(game_t *g)
{
    int x, y, give, get, cap;
    float z;
    char buf[120];

    if (chance(0.05f)) {
        say(g, msg_get(67));
        rest_days(g, 1);
        return;
    }
    x = rnd(ITEM_COUNT);
    y = rnd(ITEM_COUNT);
    if (y == x)
        y = (y + 1) % ITEM_COUNT;
    z = (float)trade_cents[x] / ((float)trade_cents[y] * (1.0f + 1.2f * rndf()));
    if (z >= 1.0f) {
        give = 1;
        get = (int)z;
    } else {
        give = (int)(1.0f / z);
        get = 1;
    }
    if ((x == ITEM_AMMO && y == ITEM_FOOD) || (x == ITEM_FOOD && y == ITEM_AMMO)) {
        give *= 50;
        get *= 50;
    }
    cap = item_cap(y);
    if (cap >= 0 && item_have(g, y) + get > cap) {
        say(g, msg_get(68));
        rest_days(g, 1);
        return;
    }
    printf(msg_get(69),
           give, give == 1 ? item_singular[x] : item_plural[x],
           get, get == 1 ? item_singular[y] : item_plural[y]);
    if (ui_ask_int(msg_get(70), 1, 2) == 2) {
        rest_days(g, 1);
        return;
    }
    if (item_have(g, x) < give) {
        say(g, msg_get(71));
    } else {
        item_take(g, x, give);
        item_add(g, y, get);
        sprintf(buf, "Trade complete. You gave %d %s.", give, item_plural[x]);
        say(g, buf);
    }
    rest_days(g, 1);
}

static void do_hunt(game_t *g)
{
    static const range_t rabbit = {1, 5}, squirrel = {1, 3}, deer = {60, 75},
                         antlered = {50, 70}, bear = {100, 200}, buffalo = {850, 1000};
    const range_t *list[6];
    int avail = 0, shots, i, kills = 0;
    long gained = 0;

    if (g->bullets <= 0) {
        say(g, msg_get(72));
        ui_pause();
        return;
    }
    shots = ui_ask_int(msg_get(73), 0, g->bullets);
    if (shots == 0)
        return;
    g->bullets -= shots;

    list[avail++] = &rabbit;
    list[avail++] = &squirrel;
    list[avail++] = &deer;
    if (g->lm >= 4 && g->lm <= 12)
        list[avail++] = &antlered;
    if (g->lm >= 7)
        list[avail++] = &bear;
    if (g->lm <= 6)
        list[avail++] = &buffalo;

    for (i = 0; i < shots; i++) {
        const range_t *a;
        if (!chance(0.4f))
            continue;
        a = list[rnd(avail)];
        gained += ranged(a->lo, a->hi);
        kills++;
    }
    if (g->food + gained > FOOD_CAP)
        gained = FOOD_CAP - g->food;
    if (gained < 0)
        gained = 0;
    g->food += (int)gained;
    printf(msg_get(74), kills, gained);
    g->said = true;
    rest_days(g, 1);
}

static void print_help(int first, int count)
{
    int i;

    for (i = 0; i < count; i++)
        printf(msg_get(first + i));
}

static void change_pace(game_t *g)
{
    ui_clear();
    print_help(152, 4);
    printf(msg_get(75));
    g->pace = ui_ask_int(msg_get(76), 1, 3) - 1;
}

static void change_rations(game_t *g)
{
    ui_clear();
    print_help(156, 4);
    printf(msg_get(77));
    g->rations = ui_ask_int(msg_get(78), 1, 3) - 1;
}

static void check_supplies(game_t *g)
{
    ui_clear();
    printf(msg_get(79));
    print_oxen(g->ox2);
    printf(msg_get(80),
           g->clothing, g->bullets, g->spares[PART_WHEEL], g->spares[PART_AXLE], g->spares[PART_TONGUE]);
    printf(msg_get(81), g->food);
    ui_print_money(g->money);
    printf(msg_get(82));
    ui_pause();
}

static bool confirm_quit(void)
{
    return ui_ask_int(msg_get(83), 1, 2) == 1;
}

static bool action_menu(game_t *g, bool at_landmark)
{
    for (;;) {
        action_t acts[11];
        int n = 0, c, i;
        bool store = landmarks[g->lm].type == LM_STORE;

        if (g->said) {
            ui_pause();
            g->said = false;
        }
        resolve_broken(g);
        draw_status(g);
        acts[n++] = ACT_CONTINUE;
        acts[n++] = ACT_SUPPLIES;
        acts[n++] = ACT_MAP;
        acts[n++] = ACT_PACE;
        acts[n++] = ACT_RATIONS;
        acts[n++] = ACT_REST;
        acts[n++] = ACT_TRADE;
        if (!at_landmark)
            acts[n++] = ACT_HUNT;
        if (store)
            acts[n++] = ACT_BUY;
        if (at_landmark && g->lm < TALK_LANDMARKS)
            acts[n++] = ACT_TALK;
        acts[n++] = ACT_QUIT;
        for (i = 0; i < n; i++)
            printf(msg_get(84), i + 1, action_label[acts[i]]);
        c = ui_ask_int(msg_get(85), 1, n);

        switch (acts[c - 1]) {
        case ACT_CONTINUE:
            if (!blocked(g))
                return true;
            if (g->broken)
                say(g, msg_get(86));
            else
                say(g, msg_get(87));
            break;
        case ACT_SUPPLIES:
            check_supplies(g);
            break;
        case ACT_PACE:
            change_pace(g);
            break;
        case ACT_RATIONS:
            change_rations(g);
            break;
        case ACT_REST: {
            int days;
            ui_clear();
            days = ui_ask_int(msg_get(88), 0, MAX_REST_DAYS);
            rest_days(g, days);
            break;
        }
        case ACT_TRADE:
            do_trade(g);
            break;
        case ACT_HUNT:
            do_hunt(g);
            break;
        case ACT_BUY:
            if (g->lm == 0)
                matts_store(g);
            else
                fort_store(g);
            break;
        case ACT_MAP:
            show_map(g);
            break;
        case ACT_TALK:
            talk_to_people(g->lm, &g->talk_turn);
            g->said = true;
            break;
        case ACT_QUIT:
            if (confirm_quit())
                return false;
            break;
        }
        if (g->np == 0)
            return false;
    }
}

static void announce_next(game_t *g)
{
    char buf[120];

    sprintf(buf, "The next landmark is %s, %d miles away.",
            landmarks[legs[g->leg].to].name, (int)legs[g->leg].miles);
    say(g, buf);
    grave_select(g);
}

static void start_leg(game_t *g)
{
    g->leg = landmarks[g->lm].leg;
    g->dist = legs[g->leg].miles;
    announce_next(g);
}

static void split_choice(game_t *g)
{
    const landmark_t *lm = &landmarks[g->lm];
    int a = lm->leg, b = lm->alt_leg;

    printf(msg_get(89));
    printf(msg_get(90), landmarks[legs[a].to].name, (int)legs[a].miles);
    printf(msg_get(91), landmarks[legs[b].to].name, (int)legs[b].miles);
    g->leg = ui_ask_int(msg_get(92), 1, 2) == 1 ? a : b;
    g->dist = legs[g->leg].miles;
    announce_next(g);
}

static void ford(game_t *g, const river_t *r, float depth, float ix)
{
    float rd = round1(depth);

    if (rd < 2.5f) {
        if (r->bottom == 0) {
            say(g, msg_get(93));
        } else if (r->bottom == 1) {
            if (chance(0.4f / ix)) {
                say(g, msg_get(94));
                rest_days(g, 1);
            } else {
                say(g, msg_get(95));
            }
        } else if (chance(0.16f / ix)) {
            say(g, msg_get(96));
            if (!lose_goods(g, 0.1f + 0.3f * rndf(), "You lost:"))
                say(g, msg_get(97));
        } else {
            say(g, msg_get(98));
        }
    } else if (rd < 3.0f) {
        say(g, msg_get(99));
        rest_days(g, 1);
    } else {
        say(g, msg_get(100));
        lose_goods(g, rd / 10.0f / ix, "You lost:");
        lose_oxen(g, (rd - 1.0f) / 10.0f / ix, "You lost oxen:");
        drown(g, (rd - 2.5f) / 10.0f / ix);
    }
}

static bool float_across(game_t *g, float depth, float swift, float ix)
{
    float rd = round1(depth);
    float tip = rd > 2.5f ? (swift / 20.0f) / ix : 0.0f;

    if (rd < 1.5f) {
        say(g, msg_get(101));
        return false;
    }
    rest_days(g, 1);
    if (chance(tip)) {
        say(g, msg_get(102));
        lose_goods(g, (0.4f + swift / 25.0f) / ix, "You lost:");
        drown(g, ((swift - 3.0f) / 15.0f) / ix);
    } else {
        say(g, msg_get(103));
    }
    return true;
}

static bool ferry(game_t *g, float depth, float swift)
{
    float p = 0.0f;

    if (round1(depth) < 2.5f) {
        say(g, msg_get(104));
        return false;
    }
    if (g->money < FERRY_CENTS) {
        say(g, msg_get(105));
        return false;
    }
    g->money -= FERRY_CENTS;
    rest_days(g, ranged(2, 6));
    if (swift > 5.0f)
        p += 0.05f;
    if (swift > 10.0f)
        p += 0.1f;
    if (chance(p)) {
        say(g, msg_get(106));
        lose_goods(g, 0.8f, "You lost:");
        drown(g, 0.2f);
        lose_oxen(g, 0.5f, "You lost oxen:");
    } else {
        say(g, msg_get(107));
    }
    return true;
}

static bool guide(game_t *g, const river_t *r, float depth)
{
    int n = ranged(2, 3);

    printf(msg_get(108), n);
    if (ui_ask_int(msg_get(109), 1, 2) == 2)
        return false;
    if (g->clothing < n) {
        printf(msg_get(110), n);
        return false;
    }
    g->clothing -= n;
    if (round1(depth) <= 2.4f)
        ford(g, r, depth, 5.0f);
    else
        float_across(g, depth, r->swift + g->ar, 5.0f);
    return true;
}

static void river_crossing(game_t *g, int lm)
{
    const river_t *r = &rivers[river_index(lm)];

    for (;;) {
        float depth = r->depth + 2.0f * g->ar;
        float swift = r->swift + g->ar;
        int width = (int)(r->width + 15.0f * g->ar);
        int tenths = (int)(round1(depth) * 10.0f + 0.5f);
        int opt[4], n = 0, c, done = 0;
        const char *labels[4] = {"Ford the river", "Caulk the wagon and float it", "Take the ferry", "Hire the guide"};

        printf(msg_get(111), width, tenths / 10, tenths % 10);
        opt[n++] = 0;
        opt[n++] = 1;
        if (r->extra == 2)
            opt[n++] = 2;
        if (r->extra == 1)
            opt[n++] = 3;
        opt[n++] = 4;
        for (c = 0; c < n; c++)
            printf(msg_get(112), c + 1, opt[c] == 4 ? "Wait a day" : labels[opt[c]]);
        c = ui_ask_int(msg_get(113), 1, n);

        g->crossing = true;
        switch (opt[c - 1]) {
        case 0:
            ford(g, r, depth, 1.0f);
            done = 1;
            break;
        case 1:
            done = float_across(g, depth, swift, 1.0f);
            break;
        case 2:
            done = ferry(g, depth, swift);
            break;
        case 3:
            done = guide(g, r, depth);
            break;
        case 4:
            say(g, msg_get(114));
            rest_days(g, 1);
            break;
        }
        g->crossing = false;
        if (done || g->np == 0)
            return;
        if (g->said) {
            ui_pause();
            g->said = false;
        }
    }
}

static void columbia_raft(game_t *g)
{
    int h;

    g->crossing = true;
    for (h = 0; h < 3 && g->np > 0; h++) {
        if (chance(2.0f / 3.0f)) {
            say(g, msg_get(115));
            drown(g, 0.6f);
            lose_oxen(g, 0.6f, "You lost oxen:");
            lose_goods(g, 0.7f, "You lost:");
        } else {
            say(g, msg_get(116));
            drown(g, 0.15f);
            lose_oxen(g, 0.3f, "You lost oxen:");
            lose_goods(g, 0.5f, "You lost:");
        }
    }
    g->crossing = false;
}

static bool dalles_choice(game_t *g)
{
    for (;;) {
        long toll = TOLL_BASE_CENTS + TOLL_PER_OX_CENTS * ((g->ox2 + 1) / 2);
        int c;

        printf(msg_get(117));
        printf(msg_get(118));
        printf(msg_get(119));
        ui_print_money(toll);
        printf(msg_get(120));
        c = ui_ask_int(msg_get(121), 1, 2);
        if (c == 1) {
            columbia_raft(g);
            if (g->np > 0)
                say(g, msg_get(122));
            g->won = g->np > 0;
            return false;
        }
        if (g->money < toll) {
            say(g, msg_get(123));
            ui_pause();
            continue;
        }
        g->money -= toll;
        g->leg = 18;
        g->dist = legs[18].miles;
        announce_next(g);
        return true;
    }
}

static bool arrive(game_t *g)
{
    char buf[80];

    g->lm = legs[g->leg].to;
    g->visited |= 1UL << g->lm;
    sprintf(buf, "You have reached %s.", landmarks[g->lm].name);
    say(g, buf);
    if (g->lm == LAST_LANDMARK) {
        g->won = true;
        return false;
    }
    if (landmarks[g->lm].type == LM_RIVER) {
        river_crossing(g, g->lm);
        if (g->np == 0)
            return false;
    }
    if (g->lm == DALLES)
        return dalles_choice(g);
    if (landmarks[g->lm].alt_leg >= 0)
        split_choice(g);
    else
        start_leg(g);
    return true;
}

static void ask_name(char *dest, const char *prompt, const char *fallback)
{
    char line[NAME_LEN + 8];

    fputs(prompt, stdout);
    ui_read_line(line, sizeof(line));
    if (!line[0])
        strcpy(line, fallback);
    line[NAME_MAX] = '\0';
    strcpy(dest, line);
}

static void setup_party(game_t *g)
{
    int i;
    char prompt[48];

    ui_clear();
    printf(msg_get(124));
    printf(msg_get(125));
    ui_print_money(occ_money[0]);
    printf(msg_get(126));
    ui_print_money(occ_money[1]);
    printf(msg_get(127));
    ui_print_money(occ_money[2]);
    printf(msg_get(128));
    g->occ = ui_ask_int(msg_get(129), 1, 3);
    g->money = occ_money[g->occ - 1];
    printf(msg_get(130), occ_name[g->occ - 1]);

    ask_name(g->party[0].name, "Name of the party leader: ", "Pioneer");
    strcpy(g->leader, g->party[0].name);
    for (i = 1; i < PARTY_SIZE; i++) {
        sprintf(prompt, "Name of companion %d (Enter for %s): ", i, default_names[i - 1]);
        ask_name(g->party[i].name, prompt, default_names[i - 1]);
    }
    for (i = 0; i < PARTY_SIZE; i++)
        g->party[i].affl = -1;
    g->np = PARTY_SIZE;
}

static void choose_departure(game_t *g)
{
    ui_clear();
    printf(msg_get(131));
    g->month = ui_ask_int(msg_get(132), 1, 5) + 1;
    g->day = 1;
    g->year = START_YEAR;
}

static void start_conditions(game_t *g)
{
    int month1 = g->month + 1;
    int qt = climate_temp[0][g->month];

    g->ar = 7.0f - month1 + rndf();
    g->as = g->month == 2 ? 12.0f * rndf() : 0.0f;
    g->tm = (qt + 10) / 20;
    if (g->tm > 5)
        g->tm = 5;
    g->weather = g->tm;
    g->precip = false;
}

static long points_for(const game_t *g)
{
    int band = (int)(g->health / 35.0f);
    long pts = 0;

    if (band > 3)
        band = 3;
    pts += g->np * (500L - 100L * band);
    pts += 50L;
    pts += (long)(whole_oxen(g)) * 4L;
    pts += (long)(g->spares[0] + g->spares[1] + g->spares[2]) * 2L;
    pts += (long)g->clothing * 2L;
    pts += g->bullets / 50;
    pts += g->food / 25;
    pts += g->money / 500L;
    return pts * g->occ;
}

static void show_arrival(game_t *g, long pts)
{
    const char *rating = pts >= 6000 ? "Trail guide" : pts >= 3000 ? "Adventurer" : "Greenhorn";

    ui_clear();
    printf(msg_get(133));
    printf(msg_get(134), month_name[g->month], g->day, g->year);
    printf(msg_get(135), g->np, PARTY_SIZE);
    printf(msg_get(136), pts);
    printf(msg_get(137), rating);
    ui_pause();
}

void game_play(game_result_t *result)
{
    game_t g;
    bool keep_going = true;
    bool at_landmark = true;
    long pts = 0;

    memset(&g, 0, sizeof(g));
    g.leg = -1;
    g.visited = 1UL;
    setup_party(&g);
    choose_departure(&g);
    start_conditions(&g);
    matts_store(&g);
    start_leg(&g);

    while (keep_going && g.np > 0 && !g.won) {
        if (!action_menu(&g, at_landmark))
            break;
        travel_week(&g);
        if (g.np == 0)
            break;
        at_landmark = g.dist <= 0.0f;
        if (at_landmark)
            keep_going = arrive(&g);
    }

    if (g.won) {
        pts = points_for(&g);
        show_arrival(&g, pts);
    } else if (g.np == 0) {
        ui_clear();
        printf(msg_get(138));
        grave_write(&g);
        ui_pause();
    }
    result->score = g.won ? pts : 0L;
    strcpy(result->leader, g.party[0].name);
}
