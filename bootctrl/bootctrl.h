#ifndef _BOOTCTRL_H_
#define _BOOTCTRL_H_

#include <stdint.h>

#define OFFSETOF_SLOT_SUFFIX 2048

#define BOOTCTRL_MAGIC 0x19191100
#define BOOTCTRL_SUFFIX_A           "_a"
#define BOOTCTRL_SUFFIX_B           "_b"
#define BOOT_CONTROL_VERSION    1

#define BOOTCTRL_PROPERTY "ro.boot.slot_suffix"
#define SLOT_SUFFIX_STR "androidboot.slot_suffix="
#define COMMAND_LINE_PATH "/proc/cmdline"
#define COMMAND_LINE_SIZE 2048

#define UFS_IOCTL_QUERY         0x5388

typedef enum query_opcode {
  UPIU_QUERY_OPCODE_READ_DESC = 0x1,
  UPIU_QUERY_OPCODE_READ_ATTR = 0x3,
  UPIU_QUERY_OPCODE_WRITE_ATTR = 0x4,
  UPIU_QUERY_OPCODE_READ_FLAG = 0x5,
} query_opcode;

typedef enum attr_idn {
  QUERY_ATTR_IDN_BOOT_LUN_EN = 0x00,
  QUERY_ATTR_IDN_DEVICE_FFU_STATUS = 0x14,
} attr_idn;

typedef struct ufs_ioctl_query_data {
	uint32_t opcode;
	uint8_t idn;
	uint8_t idx;
	uint16_t buf_byte;
	uint8_t *buf_ptr;
} ufs_ioctl_query_data;

typedef struct slot_metadata {
    uint8_t priority : 3;
    uint8_t retry_count : 3;
    uint8_t successful_boot : 1;
    uint8_t normal_boot : 1;
} slot_metadata_t;

typedef struct boot_ctrl {
    /* Magic for identification */
    uint32_t magic;

    /* Version of struct. */
    uint8_t version;

    /* Information about each slot. */
    slot_metadata_t slot_info[2];

    uint8_t recovery_retry_count;
} boot_ctrl_t;
#endif /* _BOOTCTRL_H_ */
