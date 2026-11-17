// Original Sheets47 code, MIT license.
#pragma once
extern "C" {
#include <dmcp.h>
}

// Compile the real platform adapters against declared SDK functions in tests.
// Device builds retain the SDK's fixed firmware service-table addresses.
#ifdef SHEETS47_DMCP5_TEST
#undef f_open
#undef f_close
#undef f_read
#undef f_write
#undef sys_disk_ok
#undef sys_disk_write_enable
#undef sys_is_disk_write_enable
#undef check_create_dir
#undef handle_menu
#undef wait_for_key_release
#undef sdb
extern "C" sys_sdb_t sheets47_test_sdb;
#define sdb sheets47_test_sdb
#endif
