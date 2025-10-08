#include "kprint.h"
#include "serial.h"
#include "env.h"
#include "shell.h"
#include "../fs/fs.h"
#include "elf.h"
#include "idt.h"
#include "irq.h"
#include "proc.h"
#include "pmm.h"
#include "vm.h"
#include "tty.h"

struct embedded_bin {
    const char *name;
    char *start;
    char *end;
};

#define WEAK_BIN extern char __attribute__((weak))
WEAK_BIN _binary_lib_coreutils_target_release_ls_start[];
WEAK_BIN _binary_lib_coreutils_target_release_ls_end[];
WEAK_BIN _binary_lib_coreutils_target_release_cat_start[];
WEAK_BIN _binary_lib_coreutils_target_release_cat_end[];
WEAK_BIN _binary_lib_coreutils_target_release_echo_start[];
WEAK_BIN _binary_lib_coreutils_target_release_echo_end[];
WEAK_BIN _binary_lib_coreutils_target_release_mkdir_start[];
WEAK_BIN _binary_lib_coreutils_target_release_mkdir_end[];
WEAK_BIN _binary_lib_coreutils_target_release_rm_start[];
WEAK_BIN _binary_lib_coreutils_target_release_rm_end[];
WEAK_BIN _binary_lib_coreutils_target_release_cp_start[];
WEAK_BIN _binary_lib_coreutils_target_release_cp_end[];
WEAK_BIN _binary_lib_coreutils_target_release_mv_start[];
WEAK_BIN _binary_lib_coreutils_target_release_mv_end[];
WEAK_BIN _binary_lib_coreutils_target_release_pwd_start[];
WEAK_BIN _binary_lib_coreutils_target_release_pwd_end[];
WEAK_BIN _binary_lib_coreutils_target_release_touch_start[];
WEAK_BIN _binary_lib_coreutils_target_release_touch_end[];
WEAK_BIN _binary_lib_coreutils_target_release_chmod_start[];
WEAK_BIN _binary_lib_coreutils_target_release_chmod_end[];
WEAK_BIN _binary_lib_coreutils_target_release_chown_start[];
WEAK_BIN _binary_lib_coreutils_target_release_chown_end[];
WEAK_BIN _binary_lib_coreutils_target_release_ln_start[];
WEAK_BIN _binary_lib_coreutils_target_release_ln_end[];
WEAK_BIN _binary_lib_coreutils_target_release_df_start[];
WEAK_BIN _binary_lib_coreutils_target_release_df_end[];
WEAK_BIN _binary_lib_coreutils_target_release_du_start[];
WEAK_BIN _binary_lib_coreutils_target_release_du_end[];
WEAK_BIN _binary_lib_coreutils_target_release_head_start[];
WEAK_BIN _binary_lib_coreutils_target_release_head_end[];
WEAK_BIN _binary_lib_coreutils_target_release_tail_start[];
WEAK_BIN _binary_lib_coreutils_target_release_tail_end[];
WEAK_BIN _binary_lib_coreutils_target_release_grep_start[];
WEAK_BIN _binary_lib_coreutils_target_release_grep_end[];
WEAK_BIN _binary_lib_coreutils_target_release_sort_start[];
WEAK_BIN _binary_lib_coreutils_target_release_sort_end[];
WEAK_BIN _binary_lib_coreutils_target_release_uniq_start[];
WEAK_BIN _binary_lib_coreutils_target_release_uniq_end[];
WEAK_BIN _binary_lib_coreutils_target_release_wc_start[];
WEAK_BIN _binary_lib_coreutils_target_release_wc_end[];
WEAK_BIN _binary_lib_coreutils_target_release_cut_start[];
WEAK_BIN _binary_lib_coreutils_target_release_cut_end[];
WEAK_BIN _binary_lib_coreutils_target_release_paste_start[];
WEAK_BIN _binary_lib_coreutils_target_release_paste_end[];
WEAK_BIN _binary_lib_coreutils_target_release_tr_start[];
WEAK_BIN _binary_lib_coreutils_target_release_tr_end[];
WEAK_BIN _binary_lib_coreutils_target_release_sed_start[];
WEAK_BIN _binary_lib_coreutils_target_release_sed_end[];
WEAK_BIN _binary_lib_coreutils_target_release_awk_start[];
WEAK_BIN _binary_lib_coreutils_target_release_awk_end[];
WEAK_BIN _binary_lib_coreutils_target_release_find_start[];
WEAK_BIN _binary_lib_coreutils_target_release_find_end[];
WEAK_BIN _binary_lib_coreutils_target_release_xargs_start[];
WEAK_BIN _binary_lib_coreutils_target_release_xargs_end[];
WEAK_BIN _binary_lib_coreutils_target_release_tar_start[];
WEAK_BIN _binary_lib_coreutils_target_release_tar_end[];
WEAK_BIN _binary_lib_coreutils_target_release_gzip_start[];
WEAK_BIN _binary_lib_coreutils_target_release_gzip_end[];
WEAK_BIN _binary_lib_coreutils_target_release_gunzip_start[];
WEAK_BIN _binary_lib_coreutils_target_release_gunzip_end[];
WEAK_BIN _binary_lib_coreutils_target_release_bzcat_start[];
WEAK_BIN _binary_lib_coreutils_target_release_bzcat_end[];
WEAK_BIN _binary_lib_coreutils_target_release_bzip2_start[];
WEAK_BIN _binary_lib_coreutils_target_release_bzip2_end[];
WEAK_BIN _binary_lib_coreutils_target_release_xz_start[];
WEAK_BIN _binary_lib_coreutils_target_release_xz_end[];
WEAK_BIN _binary_lib_coreutils_target_release_unxz_start[];
WEAK_BIN _binary_lib_coreutils_target_release_unxz_end[];
WEAK_BIN _binary_lib_coreutils_target_release_shred_start[];
WEAK_BIN _binary_lib_coreutils_target_release_shred_end[];
WEAK_BIN _binary_lib_coreutils_target_release_dd_start[];
WEAK_BIN _binary_lib_coreutils_target_release_dd_end[];
WEAK_BIN _binary_lib_coreutils_target_release_od_start[];
WEAK_BIN _binary_lib_coreutils_target_release_od_end[];
WEAK_BIN _binary_lib_coreutils_target_release_hexdump_start[];
WEAK_BIN _binary_lib_coreutils_target_release_hexdump_end[];
WEAK_BIN _binary_lib_coreutils_target_release_strings_start[];
WEAK_BIN _binary_lib_coreutils_target_release_strings_end[];
WEAK_BIN _binary_lib_coreutils_target_release_file_start[];
WEAK_BIN _binary_lib_coreutils_target_release_file_end[];
WEAK_BIN _binary_lib_coreutils_target_release_stat_start[];
WEAK_BIN _binary_lib_coreutils_target_release_stat_end[];
WEAK_BIN _binary_lib_coreutils_target_release_readlink_start[];
WEAK_BIN _binary_lib_coreutils_target_release_readlink_end[];
WEAK_BIN _binary_lib_coreutils_target_release_basename_start[];
WEAK_BIN _binary_lib_coreutils_target_release_basename_end[];
WEAK_BIN _binary_lib_coreutils_target_release_dirname_start[];
WEAK_BIN _binary_lib_coreutils_target_release_dirname_end[];
WEAK_BIN _binary_lib_coreutils_target_release_realpath_start[];
WEAK_BIN _binary_lib_coreutils_target_release_realpath_end[];
WEAK_BIN _binary_lib_coreutils_target_release_mktemp_start[];
WEAK_BIN _binary_lib_coreutils_target_release_mktemp_end[];
WEAK_BIN _binary_lib_coreutils_target_release_tempfile_start[];
WEAK_BIN _binary_lib_coreutils_target_release_tempfile_end[];
WEAK_BIN _binary_lib_coreutils_target_release_seq_start[];
WEAK_BIN _binary_lib_coreutils_target_release_seq_end[];
WEAK_BIN _binary_lib_coreutils_target_release_factor_start[];
WEAK_BIN _binary_lib_coreutils_target_release_factor_end[];
WEAK_BIN _binary_lib_coreutils_target_release_numfmt_start[];
WEAK_BIN _binary_lib_coreutils_target_release_numfmt_end[];
WEAK_BIN _binary_lib_coreutils_target_release_shuf_start[];
WEAK_BIN _binary_lib_coreutils_target_release_shuf_end[];
WEAK_BIN _binary_lib_coreutils_target_release_tac_start[];
WEAK_BIN _binary_lib_coreutils_target_release_tac_end[];
WEAK_BIN _binary_lib_coreutils_target_release_nl_start[];
WEAK_BIN _binary_lib_coreutils_target_release_nl_end[];
WEAK_BIN _binary_lib_coreutils_target_release_pr_start[];
WEAK_BIN _binary_lib_coreutils_target_release_pr_end[];
WEAK_BIN _binary_lib_coreutils_target_release_fmt_start[];
WEAK_BIN _binary_lib_coreutils_target_release_fmt_end[];
WEAK_BIN _binary_lib_coreutils_target_release_fold_start[];
WEAK_BIN _binary_lib_coreutils_target_release_fold_end[];
WEAK_BIN _binary_lib_coreutils_target_release_expand_start[];
WEAK_BIN _binary_lib_coreutils_target_release_expand_end[];
WEAK_BIN _binary_lib_coreutils_target_release_unexpand_start[];
WEAK_BIN _binary_lib_coreutils_target_release_unexpand_end[];
WEAK_BIN _binary_lib_coreutils_target_release_yes_start[];
WEAK_BIN _binary_lib_coreutils_target_release_yes_end[];
WEAK_BIN _binary_lib_coreutils_target_release_false_start[];
WEAK_BIN _binary_lib_coreutils_target_release_false_end[];
WEAK_BIN _binary_lib_coreutils_target_release_true_start[];
WEAK_BIN _binary_lib_coreutils_target_release_true_end[];
WEAK_BIN _binary_lib_coreutils_target_release_test_start[];
WEAK_BIN _binary_lib_coreutils_target_release_test_end[];
WEAK_BIN _binary_lib_coreutils_target_release_expr_start[];
WEAK_BIN _binary_lib_coreutils_target_release_expr_end[];
WEAK_BIN _binary_lib_coreutils_target_release_date_start[];
WEAK_BIN _binary_lib_coreutils_target_release_date_end[];
WEAK_BIN _binary_lib_coreutils_target_release_sleep_start[];
WEAK_BIN _binary_lib_coreutils_target_release_sleep_end[];
WEAK_BIN _binary_lib_coreutils_target_release_timeout_start[];
WEAK_BIN _binary_lib_coreutils_target_release_timeout_end[];
WEAK_BIN _binary_lib_coreutils_target_release_nice_start[];
WEAK_BIN _binary_lib_coreutils_target_release_nice_end[];
WEAK_BIN _binary_lib_coreutils_target_release_nohup_start[];
WEAK_BIN _binary_lib_coreutils_target_release_nohup_end[];
WEAK_BIN _binary_lib_coreutils_target_release_time_start[];
WEAK_BIN _binary_lib_coreutils_target_release_time_end[];
WEAK_BIN _binary_lib_coreutils_target_release_printenv_start[];
WEAK_BIN _binary_lib_coreutils_target_release_printenv_end[];
WEAK_BIN _binary_lib_coreutils_target_release_printf_start[];
WEAK_BIN _binary_lib_coreutils_target_release_printf_end[];
WEAK_BIN _binary_lib_coreutils_target_release_pathchk_start[];
WEAK_BIN _binary_lib_coreutils_target_release_pathchk_end[];
WEAK_BIN _binary_lib_coreutils_target_release_tty_start[];
WEAK_BIN _binary_lib_coreutils_target_release_tty_end[];
WEAK_BIN _binary_lib_coreutils_target_release_whoami_start[];
WEAK_BIN _binary_lib_coreutils_target_release_whoami_end[];
WEAK_BIN _binary_lib_coreutils_target_release_id_start[];
WEAK_BIN _binary_lib_coreutils_target_release_id_end[];
WEAK_BIN _binary_lib_coreutils_target_release_groups_start[];
WEAK_BIN _binary_lib_coreutils_target_release_groups_end[];
WEAK_BIN _binary_lib_coreutils_target_release_logname_start[];
WEAK_BIN _binary_lib_coreutils_target_release_logname_end[];
WEAK_BIN _binary_lib_coreutils_target_release_users_start[];
WEAK_BIN _binary_lib_coreutils_target_release_users_end[];
WEAK_BIN _binary_lib_coreutils_target_release_who_start[];
WEAK_BIN _binary_lib_coreutils_target_release_who_end[];
WEAK_BIN _binary_lib_coreutils_target_release_w_start[];
WEAK_BIN _binary_lib_coreutils_target_release_w_end[];
WEAK_BIN _binary_lib_coreutils_target_release_uptime_start[];
WEAK_BIN _binary_lib_coreutils_target_release_uptime_end[];
WEAK_BIN _binary_lib_coreutils_target_release_hostname_start[];
WEAK_BIN _binary_lib_coreutils_target_release_hostname_end[];
WEAK_BIN _binary_lib_coreutils_target_release_uname_start[];
WEAK_BIN _binary_lib_coreutils_target_release_uname_end[];
WEAK_BIN _binary_lib_coreutils_target_release_arch_start[];
WEAK_BIN _binary_lib_coreutils_target_release_arch_end[];
WEAK_BIN _binary_lib_coreutils_target_release_nproc_start[];
WEAK_BIN _binary_lib_coreutils_target_release_nproc_end[];
WEAK_BIN _binary_lib_coreutils_target_release_sync_start[];
WEAK_BIN _binary_lib_coreutils_target_release_sync_end[];
WEAK_BIN _binary_lib_coreutils_target_release_fsync_start[];
WEAK_BIN _binary_lib_coreutils_target_release_fsync_end[];
WEAK_BIN _binary_lib_coreutils_target_release_fdatasync_start[];
WEAK_BIN _binary_lib_coreutils_target_release_fdatasync_end[];
WEAK_BIN _binary_build_hello_user_elf_start[];
WEAK_BIN _binary_build_hello_user_elf_end[];

struct embedded_bin bins[] = {
    {"ls", _binary_lib_coreutils_target_release_ls_start, _binary_lib_coreutils_target_release_ls_end},
    {"cat", _binary_lib_coreutils_target_release_cat_start, _binary_lib_coreutils_target_release_cat_end},
    {"echo", _binary_lib_coreutils_target_release_echo_start, _binary_lib_coreutils_target_release_echo_end},
    {"mkdir", _binary_lib_coreutils_target_release_mkdir_start, _binary_lib_coreutils_target_release_mkdir_end},
    {"rm", _binary_lib_coreutils_target_release_rm_start, _binary_lib_coreutils_target_release_rm_end},
    {"cp", _binary_lib_coreutils_target_release_cp_start, _binary_lib_coreutils_target_release_cp_end},
    {"mv", _binary_lib_coreutils_target_release_mv_start, _binary_lib_coreutils_target_release_mv_end},
    {"pwd", _binary_lib_coreutils_target_release_pwd_start, _binary_lib_coreutils_target_release_pwd_end},
    {"touch", _binary_lib_coreutils_target_release_touch_start, _binary_lib_coreutils_target_release_touch_end},
    {"chmod", _binary_lib_coreutils_target_release_chmod_start, _binary_lib_coreutils_target_release_chmod_end},
    {"chown", _binary_lib_coreutils_target_release_chown_start, _binary_lib_coreutils_target_release_chown_end},
    {"ln", _binary_lib_coreutils_target_release_ln_start, _binary_lib_coreutils_target_release_ln_end},
    {"df", _binary_lib_coreutils_target_release_df_start, _binary_lib_coreutils_target_release_df_end},
    {"du", _binary_lib_coreutils_target_release_du_start, _binary_lib_coreutils_target_release_du_end},
    {"head", _binary_lib_coreutils_target_release_head_start, _binary_lib_coreutils_target_release_head_end},
    {"tail", _binary_lib_coreutils_target_release_tail_start, _binary_lib_coreutils_target_release_tail_end},
    {"grep", _binary_lib_coreutils_target_release_grep_start, _binary_lib_coreutils_target_release_grep_end},
    {"sort", _binary_lib_coreutils_target_release_sort_start, _binary_lib_coreutils_target_release_sort_end},
    {"uniq", _binary_lib_coreutils_target_release_uniq_start, _binary_lib_coreutils_target_release_uniq_end},
    {"wc", _binary_lib_coreutils_target_release_wc_start, _binary_lib_coreutils_target_release_wc_end},
    {"cut", _binary_lib_coreutils_target_release_cut_start, _binary_lib_coreutils_target_release_cut_end},
    {"paste", _binary_lib_coreutils_target_release_paste_start, _binary_lib_coreutils_target_release_paste_end},
    {"tr", _binary_lib_coreutils_target_release_tr_start, _binary_lib_coreutils_target_release_tr_end},
    {"sed", _binary_lib_coreutils_target_release_sed_start, _binary_lib_coreutils_target_release_sed_end},
    {"awk", _binary_lib_coreutils_target_release_awk_start, _binary_lib_coreutils_target_release_awk_end},
    {"find", _binary_lib_coreutils_target_release_find_start, _binary_lib_coreutils_target_release_find_end},
    {"xargs", _binary_lib_coreutils_target_release_xargs_start, _binary_lib_coreutils_target_release_xargs_end},
    {"tar", _binary_lib_coreutils_target_release_tar_start, _binary_lib_coreutils_target_release_tar_end},
    {"gzip", _binary_lib_coreutils_target_release_gzip_start, _binary_lib_coreutils_target_release_gzip_end},
    {"gunzip", _binary_lib_coreutils_target_release_gunzip_start, _binary_lib_coreutils_target_release_gunzip_end},
    {"bzcat", _binary_lib_coreutils_target_release_bzcat_start, _binary_lib_coreutils_target_release_bzcat_end},
    {"bzip2", _binary_lib_coreutils_target_release_bzip2_start, _binary_lib_coreutils_target_release_bzip2_end},
    {"xz", _binary_lib_coreutils_target_release_xz_start, _binary_lib_coreutils_target_release_xz_end},
    {"unxz", _binary_lib_coreutils_target_release_unxz_start, _binary_lib_coreutils_target_release_unxz_end},
    {"shred", _binary_lib_coreutils_target_release_shred_start, _binary_lib_coreutils_target_release_shred_end},
    {"dd", _binary_lib_coreutils_target_release_dd_start, _binary_lib_coreutils_target_release_dd_end},
    {"od", _binary_lib_coreutils_target_release_od_start, _binary_lib_coreutils_target_release_od_end},
    {"hexdump", _binary_lib_coreutils_target_release_hexdump_start, _binary_lib_coreutils_target_release_hexdump_end},
    {"strings", _binary_lib_coreutils_target_release_strings_start, _binary_lib_coreutils_target_release_strings_end},
    {"file", _binary_lib_coreutils_target_release_file_start, _binary_lib_coreutils_target_release_file_end},
    {"stat", _binary_lib_coreutils_target_release_stat_start, _binary_lib_coreutils_target_release_stat_end},
    {"readlink", _binary_lib_coreutils_target_release_readlink_start, _binary_lib_coreutils_target_release_readlink_end},
    {"basename", _binary_lib_coreutils_target_release_basename_start, _binary_lib_coreutils_target_release_basename_end},
    {"dirname", _binary_lib_coreutils_target_release_dirname_start, _binary_lib_coreutils_target_release_dirname_end},
    {"realpath", _binary_lib_coreutils_target_release_realpath_start, _binary_lib_coreutils_target_release_realpath_end},
    {"mktemp", _binary_lib_coreutils_target_release_mktemp_start, _binary_lib_coreutils_target_release_mktemp_end},
    {"tempfile", _binary_lib_coreutils_target_release_tempfile_start, _binary_lib_coreutils_target_release_tempfile_end},
    {"seq", _binary_lib_coreutils_target_release_seq_start, _binary_lib_coreutils_target_release_seq_end},
    {"factor", _binary_lib_coreutils_target_release_factor_start, _binary_lib_coreutils_target_release_factor_end},
    {"numfmt", _binary_lib_coreutils_target_release_numfmt_start, _binary_lib_coreutils_target_release_numfmt_end},
    {"shuf", _binary_lib_coreutils_target_release_shuf_start, _binary_lib_coreutils_target_release_shuf_end},
    {"tac", _binary_lib_coreutils_target_release_tac_start, _binary_lib_coreutils_target_release_tac_end},
    {"nl", _binary_lib_coreutils_target_release_nl_start, _binary_lib_coreutils_target_release_nl_end},
    {"pr", _binary_lib_coreutils_target_release_pr_start, _binary_lib_coreutils_target_release_pr_end},
    {"fmt", _binary_lib_coreutils_target_release_fmt_start, _binary_lib_coreutils_target_release_fmt_end},
    {"fold", _binary_lib_coreutils_target_release_fold_start, _binary_lib_coreutils_target_release_fold_end},
    {"expand", _binary_lib_coreutils_target_release_expand_start, _binary_lib_coreutils_target_release_expand_end},
    {"unexpand", _binary_lib_coreutils_target_release_unexpand_start, _binary_lib_coreutils_target_release_unexpand_end},
    {"yes", _binary_lib_coreutils_target_release_yes_start, _binary_lib_coreutils_target_release_yes_end},
    {"false", _binary_lib_coreutils_target_release_false_start, _binary_lib_coreutils_target_release_false_end},
    {"true", _binary_lib_coreutils_target_release_true_start, _binary_lib_coreutils_target_release_true_end},
    {"test", _binary_lib_coreutils_target_release_test_start, _binary_lib_coreutils_target_release_test_end},
    {"expr", _binary_lib_coreutils_target_release_expr_start, _binary_lib_coreutils_target_release_expr_end},
    {"date", _binary_lib_coreutils_target_release_date_start, _binary_lib_coreutils_target_release_date_end},
    {"sleep", _binary_lib_coreutils_target_release_sleep_start, _binary_lib_coreutils_target_release_sleep_end},
    {"timeout", _binary_lib_coreutils_target_release_timeout_start, _binary_lib_coreutils_target_release_timeout_end},
    {"nice", _binary_lib_coreutils_target_release_nice_start, _binary_lib_coreutils_target_release_nice_end},
    {"nohup", _binary_lib_coreutils_target_release_nohup_start, _binary_lib_coreutils_target_release_nohup_end},
    {"time", _binary_lib_coreutils_target_release_time_start, _binary_lib_coreutils_target_release_time_end},
    {"printenv", _binary_lib_coreutils_target_release_printenv_start, _binary_lib_coreutils_target_release_printenv_end},
    {"printf", _binary_lib_coreutils_target_release_printf_start, _binary_lib_coreutils_target_release_printf_end},
    {"pathchk", _binary_lib_coreutils_target_release_pathchk_start, _binary_lib_coreutils_target_release_pathchk_end},
    {"tty", _binary_lib_coreutils_target_release_tty_start, _binary_lib_coreutils_target_release_tty_end},
    {"whoami", _binary_lib_coreutils_target_release_whoami_start, _binary_lib_coreutils_target_release_whoami_end},
    {"id", _binary_lib_coreutils_target_release_id_start, _binary_lib_coreutils_target_release_id_end},
    {"groups", _binary_lib_coreutils_target_release_groups_start, _binary_lib_coreutils_target_release_groups_end},
    {"logname", _binary_lib_coreutils_target_release_logname_start, _binary_lib_coreutils_target_release_logname_end},
    {"users", _binary_lib_coreutils_target_release_users_start, _binary_lib_coreutils_target_release_users_end},
    {"who", _binary_lib_coreutils_target_release_who_start, _binary_lib_coreutils_target_release_who_end},
    {"w", _binary_lib_coreutils_target_release_w_start, _binary_lib_coreutils_target_release_w_end},
    {"uptime", _binary_lib_coreutils_target_release_uptime_start, _binary_lib_coreutils_target_release_uptime_end},
    {"hostname", _binary_lib_coreutils_target_release_hostname_start, _binary_lib_coreutils_target_release_hostname_end},
    {"uname", _binary_lib_coreutils_target_release_uname_start, _binary_lib_coreutils_target_release_uname_end},
    {"arch", _binary_lib_coreutils_target_release_arch_start, _binary_lib_coreutils_target_release_arch_end},
    {"nproc", _binary_lib_coreutils_target_release_nproc_start, _binary_lib_coreutils_target_release_nproc_end},
    {"sync", _binary_lib_coreutils_target_release_sync_start, _binary_lib_coreutils_target_release_sync_end},
    {"fsync", _binary_lib_coreutils_target_release_fsync_start, _binary_lib_coreutils_target_release_fsync_end},
    {"fdatasync", _binary_lib_coreutils_target_release_fdatasync_start, _binary_lib_coreutils_target_release_fdatasync_end},
    {NULL, NULL, NULL}
};
#include "../drivers/keyboard.h"

void kernel_main64(void)
{
    serial_init();
    kset_color(7, 0);
    kclear();
    kprintf("[kernel64] Bootstage (64-bit)\n");
    idt_init();
    irq_init();

    extern void irq_kbd_install(void);
    irq_kbd_install();
    extern void irq_timer_install(void);
    irq_timer_install();
    extern void keyboard_irq_handler(void);

    if (serial_present())
    {
        kprint_enable_serial();
        kprintf("[conf] Serial console enabled\n");
    }
    kprintf("[fs] fs_init() starting...\n");
    fs_init();
    kprintf("[fs] fs_init() done\n");
    fs_mkdir(fs_root(), "home");
    
    /* Initialize /proc filesystem */
    extern void procfs_init(void);
    procfs_init();
    
    tty_init();
    tty_register_fs();
    kprintf("[tty] initialized /dev/tty0\n");
    // Bind initial process stdio fds 0/1/2 to /dev/tty0
    node_t *tty0 = fs_lookup(fs_root(), "/dev/tty0");
    if (tty0) {
        process_t *p = proc_current();
        if (p) {
            for (int fd=0; fd<3; fd++) {
                p->fds[fd].used = 1;
                p->fds[fd].node = tty0;
                p->fds[fd].ofs = 0;
                p->fds[fd].flags = 0;
            }
            kprintf("[tty] stdio attached to /dev/tty0\n");
        }
    }
    node_t *bin_dir = fs_mkdir(fs_root(), "bin");
    for (int i = 0; bins[i].name; i++) {
        if (bins[i].start) {
            size_t size = bins[i].end - bins[i].start;
            node_t *file = fs_create_file(bin_dir, bins[i].name);
            fs_write(file, bins[i].start, size, 0);
            kprintf("[fs] embedded: /bin/%s size=%u\n", bins[i].name, (unsigned)size);
        }
    }
    if (_binary_build_hello_user_elf_start) {
        size_t hsize = _binary_build_hello_user_elf_end - _binary_build_hello_user_elf_start;
        node_t *hello = fs_create_file(bin_dir, "hello");
        if (hello) {
            fs_write(hello, (char*)_binary_build_hello_user_elf_start, hsize, 0);
            kprintf("[fs] userprog: /bin/hello size=%u\n", (unsigned)hsize);
        }
    }
    env_init();
    kprintf("[env] initialized PATH=%s\n", env_get("PATH"));
    kprintf("[shell] Ready. Type 'help' for commands (64-bit)\n\n");
    idt_enable();
    pmm_init();
    vm_init();
    proc_init();
    vm_set_kernel_cr3(vm_get_cr3());
    shell_run();
}
