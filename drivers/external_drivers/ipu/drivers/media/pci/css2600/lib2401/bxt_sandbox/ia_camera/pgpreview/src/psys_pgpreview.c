/*
 * Support for Intel Camera Imaging ISP subsystem.
 *
 * Copyright (c) 2010 - 2014 Intel Corporation. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 */

#include "ia_css_pgpreview.h"
#include "assert_support.h"
#include "gdc_device.h"
#include "sh_css_defs.h"
#include "vf/vf_1.0/ia_css_vf.host.h"

#include "isp/modes/interface/isp_const.h"
#include "ia_css_pg_param_internal.h"

#include "ia_css_program_group_data.h"
#include "ia_css_psys_program_group_manifest.h"
#include "ia_css_psys_program_manifest.h"
#include "ia_css_psys_terminal_manifest.h"
#include "ia_css_program_group_param.h"
#include "ia_camera_utils_cached_param_configs.h" /* for ia_css_s3a_config_ext */
#include "vied_nci_psys_system_global.h"
#include "ia_css_psys_manifest_types.h"

/* TODO: This needs to come from manifest compiler */
#define PSYS_PREVIEW_VAR2_N_TERMINALS		6
#define PSYS_PREVIEW_VAR2_N_PROGRAMS		1
#define PSYS_PREVIEW_VAR2_PROGRAM_GROUP_ID	SH_CSS_BINARY_ID_PREVIEW_DZ_ISP2
#define PSYS_PREVIEW_VAR2_SECTION_COUNT		2  /* 1: ia_css_pg_param strucutre
						      2: ia_css_s3a_config parameter configuration
						      */
const	ia_css_terminal_type_t ia_camera_preview_terminal_types[PSYS_PREVIEW_VAR2_N_TERMINALS] = {
						IA_CSS_TERMINAL_TYPE_DATA_IN,
						IA_CSS_TERMINAL_TYPE_DATA_OUT,
						IA_CSS_TERMINAL_TYPE_PARAM_CACHED,
						IA_CSS_TERMINAL_TYPE_DATA_OUT, /* 3a Statistics*/
						IA_CSS_TERMINAL_TYPE_DATA_OUT,  /* 3a statistics*/
						IA_CSS_TERMINAL_TYPE_DATA_OUT  /* 3a histogram*/
						};

const ia_css_frame_format_type_t ia_camera_preview_format_types[PSYS_PREVIEW_VAR2_N_TERMINALS] = {
						IA_CSS_DATA_CUSTOM_NO_DESCRIPTOR,  /*set by client.*/
						IA_CSS_DATA_CUSTOM_NO_DESCRIPTOR,  /*set by client.*/
						IA_CSS_DATA_GENERIC_PARAMETER,
						IA_CSS_DATA_S3A_STATISTICS_HI,
						IA_CSS_DATA_S3A_STATISTICS_LO,
						IA_CSS_DATA_S3A_HISTOGRAM
						};

/* TODO: fix this after exposing kernel id's */
static const ia_css_kernel_bitmap_t preview_kernel_bitmap = 1;

static const uint8_t
ia_camera_preview_program_dependency[PSYS_PREVIEW_VAR2_N_PROGRAMS] = {0};

static const uint8_t
ia_camera_preview_terminal_dependency[PSYS_PREVIEW_VAR2_N_PROGRAMS] =
	{PSYS_PREVIEW_VAR2_N_TERMINALS - 1};

static uint16_t psyspoc_preview_min_in_dimensions[IA_CSS_N_DATA_DIMENSION] =
	{0, 0};
static uint16_t psyspoc_preview_max_in_dimensions[IA_CSS_N_DATA_DIMENSION] =
	{4736, 3450};
static uint16_t psyspoc_preview_min_out_dimensions[IA_CSS_N_DATA_DIMENSION] =
	{2, 2};
static uint16_t psyspoc_preview_max_out_dimensions[IA_CSS_N_DATA_DIMENSION] =
	{3840, 3840};

static int32_t ia_camera_preview_var2_initialize_sections(
		ia_css_program_group_manifest_t *prg_group_manifest,
		uint32_t index);
size_t
ia_camera_sizeof_preview_var2_program_group_manifest(void)
{
	return ia_css_sizeof_program_group_manifest(PSYS_PREVIEW_VAR2_N_PROGRAMS,
							PSYS_PREVIEW_VAR2_N_TERMINALS,
							ia_camera_preview_program_dependency,
							ia_camera_preview_terminal_dependency,
							ia_camera_preview_terminal_types,
							PSYS_PREVIEW_VAR2_SECTION_COUNT);
}

ia_css_program_group_manifest_t *
ia_camera_preview_var2_program_group_manifest_get(void *blob)
{
	ia_css_program_group_manifest_t *prg_group_manifest;
	ia_css_program_manifest_t *prg_manifest;
	ia_css_data_terminal_manifest_t *data_manifest;
	ia_css_frame_format_bitmap_t data_format_bitmap;
	uint32_t program_count;
	uint32_t terminal_count;
	int ret;

	assert(blob != NULL);

	prg_group_manifest = (ia_css_program_group_manifest_t *)blob;
	memset(prg_group_manifest, 0,
		ia_camera_sizeof_preview_var2_program_group_manifest());
	program_count = PSYS_PREVIEW_VAR2_N_PROGRAMS;
	terminal_count = PSYS_PREVIEW_VAR2_N_TERMINALS;
	ia_css_program_group_manifest_init(
		blob,
		program_count,
		terminal_count,
		ia_camera_preview_program_dependency,
		ia_camera_preview_terminal_dependency,
		ia_camera_preview_terminal_types,
		PSYS_PREVIEW_VAR2_SECTION_COUNT);

	ret = ia_css_program_group_manifest_set_program_group_ID(prg_group_manifest,
		PSYS_PREVIEW_VAR2_PROGRAM_GROUP_ID);
	assert(ret == 0);

	/* setting alignment so that the validity check passes. Not sure what is the
	 * correct value. Fix this */
	ret = ia_css_program_group_manifest_set_alignment(prg_group_manifest, 1);
	assert(ret == 0);
	/* setting kernel bitmap so that the validity check passes. Not sure what
	 * is the correct value. Fix this */
	ret = ia_css_program_group_manifest_set_kernel_bitmap(prg_group_manifest,
		preview_kernel_bitmap);
	assert(ret == 0);

	/* Program manifest */
	prg_manifest = ia_css_program_group_manifest_get_program_manifest(
		prg_group_manifest, 0);
	assert(prg_manifest != NULL);

	ret = ia_css_program_manifest_set_program_ID(prg_manifest,
		PSYS_PREVIEW_VAR2_PROGRAM_GROUP_ID);
	assert(ret == 0);
	/* satisfy validity checks */
	ret = ia_css_program_manifest_set_kernel_bitmap(prg_manifest,
		preview_kernel_bitmap);
	assert(ret == 0);
	ret = ia_css_program_manifest_set_terminal_dependency(prg_manifest, 0, 0);
	assert(ret == 0);
	ret = ia_css_program_manifest_set_terminal_dependency(prg_manifest, 1, 1);
	assert(ret == 0);
	ret = ia_css_program_manifest_set_terminal_dependency(prg_manifest, 3, 2);
	assert(ret == 0);
	ret = ia_css_program_manifest_set_terminal_dependency(prg_manifest, 4, 3);
	assert(ret == 0);
	ret = ia_css_program_manifest_set_terminal_dependency(prg_manifest, 5, 4);
	assert(ret == 0);

	/* Populate Terminal manifest */
	data_manifest = ia_css_program_group_manifest_get_data_terminal_manifest(
		prg_group_manifest, 0);
	assert(data_manifest != NULL);
	data_format_bitmap = ia_css_frame_format_bitmap_clear();
	data_format_bitmap |= ia_css_frame_format_bit_mask(
		IA_CSS_DATA_FORMAT_RAW);
	ia_css_data_terminal_manifest_set_frame_format_bitmap(data_manifest,
		data_format_bitmap);
	ia_css_data_terminal_manifest_set_min_size(data_manifest,
		psyspoc_preview_min_in_dimensions);
	ia_css_data_terminal_manifest_set_max_size(data_manifest,
		psyspoc_preview_max_in_dimensions);
	ia_css_data_terminal_manifest_set_kernel_bitmap(data_manifest,
		preview_kernel_bitmap);

	/* TODO: preview_var_2 can output NV12. Add support for this if required. */
	data_manifest = ia_css_program_group_manifest_get_data_terminal_manifest(
		prg_group_manifest, 1);
	assert(data_manifest != NULL);
	data_format_bitmap = ia_css_frame_format_bitmap_clear();
	data_format_bitmap |= ia_css_frame_format_bit_mask(
		IA_CSS_DATA_FORMAT_YUV420_LINE);
	ia_css_data_terminal_manifest_set_frame_format_bitmap(data_manifest,
		data_format_bitmap);
	ia_css_data_terminal_manifest_set_min_size(data_manifest,
		psyspoc_preview_min_out_dimensions);
	ia_css_data_terminal_manifest_set_max_size(data_manifest,
		psyspoc_preview_max_out_dimensions);
	ia_css_data_terminal_manifest_set_kernel_bitmap(data_manifest,
		preview_kernel_bitmap);

	/* Populate 3a stats manifest */
	/* TODO: figure out how to fill 3a min/max sizes */
	data_manifest = ia_css_program_group_manifest_get_data_terminal_manifest(
		prg_group_manifest, 3);
	assert(data_manifest != NULL);
	data_format_bitmap = ia_css_frame_format_bitmap_clear();
	data_format_bitmap |= ia_css_frame_format_bit_mask(
		IA_CSS_DATA_S3A_STATISTICS_HI);
	ia_css_data_terminal_manifest_set_frame_format_bitmap(data_manifest,
		data_format_bitmap);
	ia_css_data_terminal_manifest_set_kernel_bitmap(data_manifest,
		preview_kernel_bitmap);

	data_manifest = ia_css_program_group_manifest_get_data_terminal_manifest(
		prg_group_manifest, 4);
	assert(data_manifest != NULL);
	data_format_bitmap = ia_css_frame_format_bitmap_clear();
	data_format_bitmap |= ia_css_frame_format_bit_mask(
		IA_CSS_DATA_S3A_STATISTICS_LO);
	ia_css_data_terminal_manifest_set_frame_format_bitmap(data_manifest,
		data_format_bitmap);
	ia_css_data_terminal_manifest_set_kernel_bitmap(data_manifest,
		preview_kernel_bitmap);

	data_manifest = ia_css_program_group_manifest_get_data_terminal_manifest(
		prg_group_manifest, 5);
	assert(data_manifest != NULL);
	data_format_bitmap = ia_css_frame_format_bitmap_clear();
	data_format_bitmap |= ia_css_frame_format_bit_mask(
		IA_CSS_DATA_S3A_HISTOGRAM);
	ia_css_data_terminal_manifest_set_frame_format_bitmap(data_manifest,
		data_format_bitmap);
	ia_css_data_terminal_manifest_set_kernel_bitmap(data_manifest,
		preview_kernel_bitmap);

	/* 	HARD CODED to terminal index 2,
		because here we should have all
		binary specific info available
	*/
	assert(ia_camera_preview_terminal_types[2] == IA_CSS_TERMINAL_TYPE_PARAM_CACHED);
	ia_camera_preview_var2_initialize_sections( prg_group_manifest, 2);

	return prg_group_manifest;
}

static int32_t ia_camera_preview_var2_initialize_sections(
		ia_css_program_group_manifest_t *prg_group_manifest,
		uint32_t index)
{
	ia_css_parameter_manifest_t *poc_param, *s3a_config_param;
	ia_css_param_terminal_manifest_t *param_terminal;
	int section_count;

	assert(prg_group_manifest != NULL);
	if (prg_group_manifest == NULL) {
		return -1;
	}

	/* Get cached param terminal.*/
	param_terminal = ia_css_program_group_manifest_get_param_terminal_manifest(
				prg_group_manifest,
				index);

	assert(param_terminal != NULL);
	if (param_terminal == NULL) {
		return -1;
	}

	section_count = ia_css_param_terminal_manifest_get_section_count(
				param_terminal);
	assert(section_count == PSYS_PREVIEW_VAR2_SECTION_COUNT);
	if (section_count != PSYS_PREVIEW_VAR2_SECTION_COUNT)
		return -1;

	/* populate Sections . */
	/* Section: 0 struct ia_css_pg_param */
	poc_param = ia_css_param_terminal_manifest_get_parameter_manifest(
						param_terminal,
						0);
	assert(poc_param != NULL);
	if (poc_param == NULL) {
		return -1;
	}
	poc_param->kernel_id = (vied_nci_resource_id_t)IA_CAMERA_PSYS_PARAMS_KERNEL_ID_POC;
	poc_param->mem_type_id = VIED_NCI_GMEM_TYPE_ID;  /* ?? Is this ok ? This won't go down to ISP at all.*/
	poc_param->mem_size = sizeof(ia_css_pg_param_t);
	poc_param->mem_offset = 0;

	/* Section: 1 struct ia_css_3a_config*/
	s3a_config_param = ia_css_param_terminal_manifest_get_parameter_manifest(
						param_terminal,
						1);

	assert(s3a_config_param != NULL);
	if (s3a_config_param == NULL) {
		return -1;
	}
	s3a_config_param->kernel_id = (vied_nci_resource_id_t)IA_CAMERA_PSYS_PARAMS_KERNEL_ID_S3A;
	s3a_config_param->mem_type_id = VIED_NCI_DMEM_TYPE_ID;
	s3a_config_param->mem_size = sizeof(struct ia_css_3a_config_ext);
	s3a_config_param->mem_offset = sizeof(struct ia_css_pg_param);
	return 0;
}

size_t
ia_camera_sizeof_preview_var2_program_group_param(void)
{
	return ia_css_sizeof_program_group_param(PSYS_PREVIEW_VAR2_N_PROGRAMS,
		PSYS_PREVIEW_VAR2_N_TERMINALS, 1);
}

ia_css_program_group_param_t *
ia_camera_preview_var2_program_group_param_get(void *blob)
{
	ia_css_program_group_param_t *prg_group_param;
	uint16_t fragment_count = 1;

	assert(blob != NULL);

	prg_group_param = (ia_css_program_group_param_t *)blob;
	memset(prg_group_param, 0, ia_camera_sizeof_preview_var2_program_group_param());
	ia_css_program_group_param_init(blob,
		PSYS_PREVIEW_VAR2_N_PROGRAMS, PSYS_PREVIEW_VAR2_N_TERMINALS,
		fragment_count, ia_camera_preview_format_types);

	/* TODO: user needs to set this. But he does not know about kernels yet. */
	ia_css_program_group_param_set_kernel_enable_bitmap(prg_group_param,
		preview_kernel_bitmap);

	return prg_group_param;
}

size_t
ia_camera_sizeof_preview_var2_cached_param(void)
{
	return sizeof(ia_css_pg_param_t);
}

/*TODO: modify this to accept pg_id*/
ia_css_pg_param_t *
ia_camera_pg_preview_var2_cached_param_get(
	void *blob,
	ia_css_frame_descriptor_t *in_desc,
	ia_css_frame_descriptor_t *out_desc)
{
	ia_css_pg_param_t *param = NULL;

	if(in_desc == NULL || out_desc == NULL) {
		return NULL;
	}

	assert(blob != NULL);
	param = (ia_css_pg_param_t *)blob;
	memset(blob, 0, ia_camera_sizeof_preview_var2_cached_param());

	/* replace with relevant callbacks */
	if(param != NULL) {
		param->isp_pipe_version = 2;
		param->required_bds_factor = SH_CSS_BDS_FACTOR_1_00;
		param->dvs_frame_delay = IA_CSS_FRAME_DELAY_1;

		/*TODO: UDS parameters are redundant for preview*/
		param->uds.curr_dx = 0;
		param->uds.curr_dy = 0;
		param->uds.xc = 0;
		param->uds.yc = 0;

		param->isp_deci_log_factor =
			(uint8_t) ia_css_psyspoc_util_calc_deci_log_factor(in_desc);

		param->copy_output = true;   /* TODO: is this needed,
						already set in sh_css_binary_args_reset() */
		param->left_cropping = 12;
		param->top_cropping = 12;
		param->fixed_s3a_deci_log = 0;   /* from binary info, this is specific to binary*/
	}

	return param;
}
