/* Продолжительность фаз RESET в единицах TSLOT */
#define RESET_DURATION_TSLOTS           8

/* Временные параметры обмена в мкс */
#define NORMAL_TLOWR_MIN                1
#define NORMAL_TLOWR_MAX                15
#define NORMAL_TSLOT_MIN                60
#define NORMAL_TSLOT_MAX                120
#define NORMAL_BIT1_MIN                 NORMAL_TLOWR_MIN
#define NORMAL_BIT1_MAX                 NORMAL_TLOWR_MAX
#define NORMAL_BIT0_MIN                 NORMAL_TSLOT_MIN
#define NORMAL_BIT0_MAX                 NORMAL_TSLOT_MAX
#define NORMAL_TREC_MIN                 1
#define NORMAL_RESET                    (RESET_DURATION_TSLOTS * NORMAL_TSLOT_MIN)
#define NORMAL_PRESENCE_START_MIN       15
#define NORMAL_PRESENCE_START_MAX       60
#define NORMAL_PRESENCE_MIN             60
#define NORMAL_PRESENCE_MAX             240
#define OVERDRIVE_TLOWR_MIN             1
#define OVERDRIVE_TLOWR_MAX             2
#define OVERDRIVE_TSLOT_MIN             6
#define OVERDRIVE_TSLOT_MAX             16
#define OVERDRIVE_BIT1_MIN              OVERDRIVE_TLOWR_MIN
#define OVERDRIVE_BIT1_MAX              OVERDRIVE_TLOWR_MAX
#define OVERDRIVE_BIT0_MIN              OVERDRIVE_TSLOT_MIN
#define OVERDRIVE_BIT0_MAX              OVERDRIVE_TSLOT_MAX
#define OVERDRIVE_TREC_MIN              1
#define OVERDRIVE_RESET                 (RESET_DURATION_TSLOTS * OVERDRIVE_TSLOT_MIN)
#define OVERDRIVE_PRESENCE_START_MIN    2

// original?
#define OVERDRIVE_PRESENCE_START_MAX    6
// real? -- bug!
//#define OVERDRIVE_PRESENCE_START_MAX    30

#define OVERDRIVE_PRESENCE_MIN          8
#define OVERDRIVE_PRESENCE_MAX          24

#define NORMAL_BIT1                     ((NORMAL_BIT1_MIN + NORMAL_BIT1_MAX) >> 1)
#define NORMAL_BIT0                     ((NORMAL_BIT0_MIN + NORMAL_BIT0_MAX) >> 1)
#define OVERDRIVE_BIT1                  ((OVERDRIVE_BIT1_MIN + OVERDRIVE_BIT1_MAX) >> 1)
#define OVERDRIVE_BIT0                  ((OVERDRIVE_BIT0_MIN + OVERDRIVE_BIT0_MAX) >> 1)
