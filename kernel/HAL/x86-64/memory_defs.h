/*PAGING DEFS*/
#define PAGE_SIZE 4096
#define PAT_ENTRY(value, index) (value << (index*8))
#define PAT_UC ((uint64_t)0)
#define PAT_WC ((uint64_t)1)
#define PAT_WT ((uint64_t)4)
#define PAT_WP ((uint64_t)5)
#define PAT_WB ((uint64_t)6)
#define PAT_UC_OVERRIDABLE ((uint64_t)7)

#define PAT_VALUE \
	PAT_ENTRY(PAT_UC, 7) | \
	PAT_ENTRY(PAT_UC_OVERRIDABLE, 6) |\
	PAT_ENTRY(PAT_WT, 5) | \
	PAT_ENTRY(PAT_WC, 4) | \
	PAT_ENTRY(PAT_UC, 3) | \
	PAT_ENTRY(PAT_UC_OVERRIDABLE, 2) | \
	PAT_ENTRY(PAT_WT, 1) | \
	PAT_ENTRY(PAT_WB, 0)
#define IA32_PAT_MSR 0x277

