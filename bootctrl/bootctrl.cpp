#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <hardware/hardware.h>
#include <hardware/boot_control.h>
#include <android-base/logging.h>
#include <fs_mgr.h>
#include <cutils/properties.h>
#define LOG_TAG "bootctrl"
#include <log/log.h>
#include <utils/Log.h>

#include "bootctrl.h"
#if !defined(ARCH_X86)
#include "include/sd_misc.h"
#endif

static struct fstab* fstab = NULL;
static char *blk_dev_path = NULL;

static void free_fstab(void)
{
    fs_mgr_free_fstab(fstab);
}

static char *get_device_path(const char *mount_point)
{
    struct fstab_rec *rec = NULL;
    char *source = NULL;

    rec = fs_mgr_get_entry_for_mount_point(fstab, mount_point);
    if (!rec) {
        ALOGE("%s failed to get entry for %s \n", __func__ , mount_point);
        return NULL;
    }

    source = strdup(rec->blk_device);
    return source;
}

static int bootctrl_read_metadata(boot_ctrl_t *bctrl)
{
    int fd, err;
    ssize_t sz, size;
    char *buf = (char *)bctrl;

    fd = open(blk_dev_path, O_RDONLY);
    if (fd < 0) {
        err = errno;
        ALOGE("%s Error opening metadata file: %s\n", __func__ ,strerror(errno));
        return -err;
    }
    if (lseek(fd, OFFSETOF_SLOT_SUFFIX, SEEK_SET) < 0) {
        err = errno;
        ALOGE("%s Error seeking to metadata offset: %s\n", __func__ ,strerror(errno));
        close(fd);
        return -err;
    }
    size = sizeof(boot_ctrl_t);

    do {
        sz = read(fd, buf, size);
        if (sz == 0) {
            break;
        } else if (sz < 0) {
            if (errno == EINTR) {
                continue;
            }
            err = -errno;
            ALOGE("%s Error reading metadata file\n", __func__);
            close(fd);
            return err;
        }
        size -= sz;
        buf += sz;
    } while(size > 0);

    close(fd);

    /* Check bootctrl magic number */
    if (bctrl->magic != BOOTCTRL_MAGIC) {
        ALOGE("metadata is not initialised or corrupted %x.\n", bctrl->magic);
        return -EIO;
    }
    return 0;
}

static int bootctrl_write_metadata(boot_ctrl_t *bctrl)
{
    int fd, err;
    ssize_t sz, size;
    char *buf = (char *)bctrl;

    fd = open(blk_dev_path, O_RDWR);
    if (fd < 0) {
        err = errno;
        ALOGE("%s Error opening metadata file: %s\n", __func__,strerror(errno));
        return -err;
    }

    if (lseek(fd, OFFSETOF_SLOT_SUFFIX, SEEK_SET) < 0) {
        err = errno;
        ALOGE("%s Error seeking to metadata offset: %s\n", __func__ ,strerror(errno));
        close(fd);
        return -err;
    }
    size = sizeof(boot_ctrl_t);
    do {
        sz = write(fd, buf, size);
        if (sz == 0) {
            break;
        } else if (sz < 0) {
            if (errno == EINTR) {
                continue;
            }
            err = -errno;
            ALOGE("%s Error Writing metadata file\n",__func__);
            close(fd);
            return err;
        }
        size -= sz;
        buf += sz;
    } while(size > 0);

    close(fd);
    return 0;
}

void bootctrl_init(boot_control_module_t *module __unused)
{
    ALOGI("boot control HAL init");

    if(blk_dev_path == NULL) {
        /* Initial read fstab */
        fstab = fs_mgr_read_fstab_default();
        if (!fstab) {
            ALOGE("failed to read default fstab");
        }
        blk_dev_path = get_device_path("/misc");

        /* Free fstab */
        free_fstab();
    }

    ALOGI("%s misc blk device path = %s\n", __func__ ,blk_dev_path);
}

unsigned bootctrl_get_number_slots(boot_control_module_t *module __unused)
{
    return 2;
}

int bootctrl_get_active_slot()
{
    int fd, err, slot;
    ssize_t size = COMMAND_LINE_SIZE, sz;
    off_t off;
    char *buf, *ptr;
    char *str;

    fd = open(COMMAND_LINE_PATH, O_RDONLY);
    if (fd < 0) {
        err = -errno;
        ALOGE("%s error reading commandline\n", __func__);
        return err;
    }
    ptr = buf = (char *)malloc(size);
    if (!buf) {
        err = -errno;
        ALOGE("%s Error allocating memory\n", __func__);
        close(fd);
        return err;
    }
    do {
        sz = read(fd, buf, size);
        if (sz == 0) {
            break;
        } else if (sz < 0) {
            if (errno == EINTR) {
                continue;
            }
            err = -errno;
            ALOGE("%s Error reading file\n",__func__);
            free(ptr);
            close(fd);
            return err;
        }
        size -= sz;
        buf += sz;
    } while(size > 0);
    str = strstr((char *)ptr, SLOT_SUFFIX_STR);
    if (!str) {
        err = -EIO;
        ALOGE("%s cannot find %s in kernel commandline.\n", __func__ , SLOT_SUFFIX_STR);
        free(ptr);
        close(fd);
        return err;
    }
    str += sizeof(SLOT_SUFFIX_STR);
    slot = (*str == 'a') ? 0 : 1;
    free(ptr);
    close(fd);

    return slot;
}

int emmc_set_active_boot_part(int slot)
{
    struct msdc_ioctl st_ioctl_arg;
    unsigned int bootpart = 0;

    int fd = open("/dev/misc-sd", O_RDWR);
    if (fd >= 0) {
        memset(&st_ioctl_arg,0,sizeof(struct msdc_ioctl));
        st_ioctl_arg.host_num = 0;
        st_ioctl_arg.opcode = MSDC_SET_BOOTPART;
        st_ioctl_arg.total_size = 1;
        if (slot)
            bootpart = 2;
        else
            bootpart = 1;
        st_ioctl_arg.buffer = &bootpart;

        int ret = ioctl(fd, MSDC_SET_BOOTPART, &st_ioctl_arg);

        if (ret < 0) {
            ALOGE("ioctl boot_part fail: %s\n", strerror(errno));
            return 1;
        }

        ALOGE("switch bootpart to  = %d, ret = %d\n", bootpart, ret);
        close(fd);
        return 0;
    } else {
        ALOGE("open /dev/misc-sd fail\n");
        return 1;
    }
}

int ufs_set_active_boot_part(int boot)
{
    struct ufs_ioctl_query_data idata;
    unsigned char buf[1];
    int fd, ret = 0;

    fd = open("/dev/block/sdc", O_RDWR);

    if (fd < 0) {
        printf("%s: open device failed, err: %d\n", __func__, fd);
        ret = -1;
        goto out;
    }

    buf[0] = boot;           /* 1: BootLU A, 2: BootLU B */

    idata.opcode = UPIU_QUERY_OPCODE_WRITE_ATTR;
    idata.idn = QUERY_ATTR_IDN_BOOT_LUN_EN;
    idata.idx = 0;
    idata.buf_ptr = &buf[0];
    idata.buf_byte = 1;

    ret = ioctl(fd, UFS_IOCTL_QUERY, &idata);
    if(ret < 0)
        printf("ufs_set boot_part fail: %s\n", strerror(errno));
out:
    if(fd >= 0)
        close(fd);
    return ret;
}

int switch_pl_boot_part(unsigned slot)
{
    char propbuf[PROPERTY_VALUE_MAX];
    int ret = 0;
    int boot_part = 0;

    /* slot 0 is A , slot 1 is B */
    if (slot >= 2) {
        ALOGE("%s Wrong Slot value %u\n", __func__ , slot);
        return -1;
    }

    if(slot)
      boot_part = 2;
    else
      boot_part = 1;

    /* EMMC */
    property_get("ro.vendor.mtk_emmc_support", propbuf, "");
    if(!strncmp(propbuf, "1",1)) {
        ALOGI("emmc_set_active_boot_part\n");
        ret = emmc_set_active_boot_part(slot);
        if(ret) {
            ALOGE("emmc set boot_part fail\n");
            return -1;
        }
        return 1;
    }
    /* UFS */
    property_get("ro.vendor.mtk_ufs_booting", propbuf, "");
    if(!strncmp(propbuf, "1",1)) {
        ALOGI ("mtk_ufs propbuf\n");
        ret = ufs_set_active_boot_part(boot_part);
        if(ret) {
            ALOGE("ufs set boot_part fail\n");
            return -1;
        }
        return 1;
    }
    ALOGE("Un support phone type\n");
    return -1;
}

uint32_t bootctrl_get_current_slot(boot_control_module_t *module __unused)
{
    ALOGI("boot control bootctrl_get_current_slot\n");

    uint32_t slot = 0;

    slot = bootctrl_get_active_slot();

    ALOGI("bootctrl_get_current_slot %d\n", slot);
    return slot;
}

int bootctrl_mark_boot_successful(boot_control_module_t *module __unused)
{
    ALOGI("boot control bootctrl_mark_boot_successful\n");
    int ret;
    uint32_t slot = 0;
    boot_ctrl_t metadata;
    slot_metadata_t *slotp;

    ret = bootctrl_read_metadata(&metadata);
    if (ret < 0) {
        return ret;
    }

    slot = bootctrl_get_active_slot();
    if (slot < 0) {
        ALOGE("bootctrl_mark_boot_successful fail , slot = \n");
        return slot;
    }
    slotp = &metadata.slot_info[slot];
    slotp->successful_boot = 1;
    slotp->retry_count = 3;

    return bootctrl_write_metadata(&metadata);
}

int bootctrl_set_active_boot_slot(boot_control_module_t *module __unused,
    unsigned slot)
{
    ALOGI("boot control bootctrl_set_active_boot_slot , slot is %d\n", slot);
    int ret, slot2;
    boot_ctrl_t metadata;
    slot_metadata_t *slotp;

    if (slot >= 2) {
        ALOGE("%s Wrong Slot value %u\n", __func__ , slot);
        return -EINVAL;
    }
    ret = bootctrl_read_metadata(&metadata);
    if (ret < 0) {
        return ret;
    }
    /* Set highest prioority and reset retry count */
    slotp = &metadata.slot_info[slot];
    slotp->successful_boot = 0;
    slotp->priority = 7;
    slotp->retry_count = 3;
    slotp->normal_boot = 1;

    /* Set lower priority to another slot */
    slot2 = (slot == 0) ? 1 : 0;
    slotp = &metadata.slot_info[slot2];
    if (slotp->priority >= 7) {
        slotp->priority = 6;
    }
    slotp->retry_count = 3;
    ret = bootctrl_write_metadata(&metadata);
    if (ret < 0) {
        return ret;
    }

    ret = switch_pl_boot_part(slot);
    if (ret < 0) {
        ALOGE("bootctrl_set_active_boot_slot switch boot part fail\n");
        return ret;
    }
    return 0;
}

int bootctrl_set_slot_as_unbootable(boot_control_module_t *module __unused,
    unsigned slot)
{
    ALOGI("boot control bootctrl_set_slot_as_unbootable\n");
    int ret;
    boot_ctrl_t metadata;
    slot_metadata_t *slotp;

    if (slot >= 2) {
        ALOGE("%s Wrong Slot value %u\n", __func__ , slot);
        return -EINVAL;
    }
    ret = bootctrl_read_metadata(&metadata);
    if (ret < 0) {
        return ret;
    }
    /* Set zero to priority ,successful_boot and retry_count */
    slotp = &metadata.slot_info[slot];
    slotp->successful_boot = 0;
    slotp->priority = 0;
    slotp->retry_count = 0;
    ret = bootctrl_write_metadata(&metadata);
    if (ret < 0) {
        return ret;
    }

    return 0;
}

int bootctrl_is_slot_bootable(boot_control_module_t *module __unused,
    unsigned slot)
{
    ALOGI("boot control bootctrl_is_slot_bootable\n");
    int ret;
    boot_ctrl_t metadata;

    /* slot 0 is A , slot 1 is B */
    if (slot >= 2) {
        ALOGE("%s Wrong slot value %u\n", __func__,slot);
        return -EINVAL;
    }
    ret = bootctrl_read_metadata(&metadata);
    if (ret < 0) {
        return ret;
    }

    return (metadata.slot_info[slot].priority != 0);
}

int bootctrl_get_bootup_status(boot_control_module_t *module __unused,
    unsigned slot)
{
    ALOGI("bootctrl bootctrl_get_bootup_status\n");
    int ret = -1;
    slot_metadata_t *slotp;
    boot_ctrl_t metadata;

    if(slot >= 2) {
        ALOGE("%s Wrong slot value %u\n", __func__,slot);
        return -1;
    }

    ret = bootctrl_read_metadata(&metadata);
    if (ret < 0) {
        return ret;
    }

    slotp = &metadata.slot_info[slot];

    ALOGI("bootctrl bootctrl_get_bootup_status = %d\n", slotp->successful_boot);
    return slotp->successful_boot;
}

const char *bootctrl_get_suffix(boot_control_module_t *module __unused,
    unsigned slot)
{
    ALOGI("boot control bootctrl_get_suffix\n");
    static const char* suffix[2] = {BOOTCTRL_SUFFIX_A, BOOTCTRL_SUFFIX_B};
    if (slot >= 2)
        return NULL;
    return suffix[slot];
}

static struct hw_module_methods_t bootctrl_methods = {
    .open  = NULL,
};

/* Boot Control Module implementation */
boot_control_module_t HAL_MODULE_INFO_SYM = {
    .common = {
        .tag                 = HARDWARE_MODULE_TAG,
        .module_api_version  = BOOT_CONTROL_MODULE_API_VERSION_0_1,
        .hal_api_version     = HARDWARE_HAL_API_VERSION,
        .id                  = BOOT_CONTROL_HARDWARE_MODULE_ID,
        .name                = "boot_control HAL",
        .author              = "Mediatek Corporation",
        .methods             = &bootctrl_methods,
    },
    .init                 = bootctrl_init,
    .getNumberSlots       = bootctrl_get_number_slots,
    .getCurrentSlot       = bootctrl_get_current_slot,
    .markBootSuccessful   = bootctrl_mark_boot_successful,
    .setActiveBootSlot    = bootctrl_set_active_boot_slot,
    .setSlotAsUnbootable  = bootctrl_set_slot_as_unbootable,
    .isSlotBootable       = bootctrl_is_slot_bootable,
    .isSlotMarkedSuccessful = bootctrl_get_bootup_status,
    .getSuffix            = bootctrl_get_suffix,
};

