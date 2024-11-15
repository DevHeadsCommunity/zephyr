/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <fcntl.h>
#include <ff.h>
#include <zephyr/fs/fs.h>
#include <zephyr/posix/unistd.h>
#include <zephyr/ztest.h>

const char test_str[] = "Hello World!";

#define FATFS_MNTP	"/RAM:"
#define TEST_FILE FATFS_MNTP"/testfile.txt"

int file = -1;
static FATFS fat_fs;

static struct fs_mount_t fatfs_mnt = {
  .type = FS_FATFS,
	.mnt_point = FATFS_MNTP,
	.fs_data = &fat_fs,
};
  
static void *test_mount(void) {
  int res;

  res = fs_mount(&fatfs_mnt);
  if (res < 0) {
    TC_ERROR("Error mounting fs [%d]\n", res);
    __ASSERT_NO_MSG(res == 0);
  }
  return NULL;
}

void test_unmount(void *unused)
{
  int res;
  
  ARG_UNUSED(unused);
  res = fs_unmount(&fatfs_mnt);
  if (res < 0) {
    TC_ERROR("Error unmounting fs [%d]\n", res);
    /* FIXME: restructure tests as per #46897 */
    __ASSERT_NO_MSG(res == 0);
  }
}

static int file_open(void)
{
	int res;

	res = open(TEST_FILE, O_CREAT | O_RDWR, 0660);
	if (res < 0) {
		TC_ERROR("Failed opening file: %d, errno=%d\n", res, errno);
		/* FIXME: restructure tests as per #46897 */
		__ASSERT_NO_MSG(res >= 0);
	}

	file = res;

	return TC_PASS;
}

int file_write(void)
{
	ssize_t brw;
	off_t res;

	res = lseek(file, 0, SEEK_SET);
	if (res != 0) {
		TC_PRINT("lseek failed [%d]\n", (int)res);
		close(file);
		file = -1;
		return TC_FAIL;
	}

	brw = write(file, (char *)test_str, strlen(test_str));
	if (brw < 0) {
		TC_PRINT("Failed writing to file [%d]\n", (int)brw);
		close(file);
		file = -1;
		return TC_FAIL;
	}

	if (brw < strlen(test_str)) {
		TC_PRINT("Unable to complete write. Volume full.\n");
		TC_PRINT("Number of bytes written: [%d]\n", (int)brw);
		close(file);
		file = -1;
		return TC_FAIL;
	}

	return res;
}

static int file_close(void)
{
	int res = 0;

	if (file >= 0) {
		res = close(file);
		if (res < 0) {
			TC_ERROR("Failed closing file: %d, errno=%d\n", res, errno);
			/* FIXME: restructure tests as per #46897 */
			__ASSERT_NO_MSG(res == 0);
		}

		file = -1;
	}

	return res;
}

static int test_file_fsync(void)
{
	int res = 0;

	if (file < 0) {
		return res;
	}

	res = fsync(file);
	if (res < 0) {
		TC_ERROR("Failed to sync file: %d, errno = %d\n", res, errno);
		res = TC_FAIL;
	}

	close(file);
	file = -1;
	return res;
}


static int test_file_fdatasync(void)
{
	int res = 0;

	if (file < 0) {
		return res;
	}

	res = fdatasync(file);
	if (res < 0) {
		TC_ERROR("Failed to sync file: %d, errno = %d\n", res, errno);
		res = TC_FAIL;
	}

	close(file);
	file = -1;
	return res;
}

/**
 * @brief Test for POSIX fsync API
 *
 * @details Test sync the file through POSIX fsync API.
 */
ZTEST(xsi_realtime, test_fs_sync)
{
	/* FIXME: restructure tests as per #46897 */	
	zassert_true(file_write() == TC_PASS);
	zassert_true(test_file_fsync() == TC_PASS);
}

/**
 * @brief Test for POSIX fdatasync API
 *
 * @details Test sync the file through POSIX fdatasync API.
 */
ZTEST(xsi_realtime, test_fs_datasync)
{
	/* FIXME: restructure tests as per #46897 */	
	zassert_true(file_write() == TC_PASS);
	zassert_true(test_file_fdatasync() == TC_PASS);
}

void before(void *arg) {
  ARG_UNUSED(arg);

  test_mount();
  
  if (file_open() != TC_PASS) {
    ztest_test_skip();
  }
}

void after(void *arg) {
  ARG_UNUSED(arg);

  file_close();
  unlink(TEST_FILE);
  test_unmount(NULL);
}
