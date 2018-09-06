
/*
 * ov5645 Camera Driver
 *
 * Copyright (C) 2011 Actions Semiconductor Co.,LTD
 * Wang Xin <wangxin@actions-semi.com>
 *
 * Based on ov227x driver
 *
 * Copyright (C) 2008 Renesas Solutions Corp.
 * Kuninori Morimoto <morimoto.kuninori@renesas.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * fixed by swpark@nexell.co.kr for compatibility with general v4l2 layer (not using soc camera interface)
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/videodev2.h>
#include <linux/v4l2-subdev.h>
#include <media/v4l2-chip-ident.h>
#include <media/v4l2-device.h>
#include <media/v4l2-subdev.h>
#include <media/v4l2-ctrls.h>
#include <linux/delay.h>
#include "ov5645.h"
#include "./nexell/capture/nxp-capture.h"  //ace 20150721

#define MODULE_NAME "OV5645"

#if 1
#define OV5645_DBG(fmt, arg...) printk(KERN_INFO "ov5645: " fmt, ## arg)
#else
#define OV5645_DBG(fmt, arg...)
#endif
                                                          
#if 1
#define OV5645_ERR(fmt, arg...) printk(KERN_ERR "ov5645: " fmt, ## arg)
#else
#define OV5645_ERR(fmt, arg...)
#endif

#define PID                 0x02 /* Product ID Number  *///caichsh
#define OV5645              0x53
#define OUTTO_SENSO_CLOCK   24000000
#define NUM_CTRLS           11
#define V4L2_IDENT_OV5645   64188

/* private ctrls */
#define V4L2_CID_SCENE_EXPOSURE         (V4L2_CTRL_CLASS_CAMERA | 0x1001)
#define V4L2_CID_PRIVATE_PREV_CAPT      (V4L2_CTRL_CLASS_CAMERA | 0x1002)

#if 0
enum {
    V4L2_WHITE_BALANCE_INCANDESCENT = 0,
    V4L2_WHITE_BALANCE_FLUORESCENT,
    V4L2_WHITE_BALANCE_DAYLIGHT,
    V4L2_WHITE_BALANCE_CLOUDY_DAYLIGHT,
    V4L2_WHITE_BALANCE_TUNGSTEN
};
#else
enum {
    V4L2_WHITE_BALANCE_INCANDESCENT = 0,
    /*V4L2_WHITE_BALANCE_FLUORESCENT,*/
    V4L2_WHITE_BALANCE_DAYLIGHT,
    V4L2_WHITE_BALANCE_CLOUDY_DAYLIGHT,
    /*V4L2_WHITE_BALANCE_TUNGSTEN*/
};
#endif

static void ov5640_auto_focus(struct work_struct *work);
static DECLARE_DELAYED_WORK(focus_work, ov5640_auto_focus); 

static int ov5645_video_probe(struct i2c_client *client);
/****************************************************************************************
 * predefined reg values
 */
//#define ARRAY_END { 0xffff, 0xff }
//#define DELAYMARKER { 0xfffe, 0xff }


static struct regval_list ov5645_fmt_yuv422_yuyv[] =
{
    ARRAY_END,
};

static struct regval_list ov5645_fmt_yuv422_yvyu[] =
{
    ARRAY_END,
};

static struct regval_list ov5645_fmt_yuv422_vyuy[] =
{
    ARRAY_END,
};

static struct regval_list ov5645_fmt_yuv422_uyvy[] =
{
    ARRAY_END,
};


/*
 *AWB
 */
static const struct regval_list ov5645_awb_regs_enable[] =
{
    ARRAY_END,
};

static const struct regval_list ov5645_awb_regs_diable[] =
{
    ARRAY_END,
};

static struct regval_list ov5645_wb_cloud_regs[] =
{
    ARRAY_END,
};

static struct regval_list ov5645_wb_daylight_regs[] =
{
    ARRAY_END,
};

static struct regval_list ov5645_wb_incandescence_regs[] =
{
    ARRAY_END,
};

static struct regval_list ov5645_wb_fluorescent_regs[] =
{
    ARRAY_END,
};

static struct regval_list ov5645_wb_tungsten_regs[] =
{
    ARRAY_END,
};

static struct regval_list ov5645_colorfx_none_regs[] =
{
    ARRAY_END,
};

static struct regval_list ov5645_colorfx_bw_regs[] =
{
    ARRAY_END,
};

static struct regval_list ov5645_colorfx_sepia_regs[] =
{
    ARRAY_END,
};

static struct regval_list ov5645_colorfx_negative_regs[] =
{
    ARRAY_END,
};

static struct regval_list ov5645_whitebance_auto[] __attribute__((unused)) =
{
	ARRAY_END,
};

static struct regval_list ov5645_whitebance_cloudy[] __attribute__((unused)) =
{
	ARRAY_END,
};

static  struct regval_list ov5645_whitebance_sunny[] __attribute__((unused)) =
{
	ARRAY_END,
};

static  struct regval_list ov5645_whitebance_fluorescent[] __attribute__((unused)) =
{
	ARRAY_END,

};
static  struct regval_list ov5645_whitebance_incandescent[] __attribute__((unused)) =
{
	ARRAY_END,
};


static  struct regval_list ov5645_effect_normal[] __attribute__((unused)) =
{
    ARRAY_END,
};

static  struct regval_list ov5645_effect_white_black[] __attribute__((unused)) =
{
  ARRAY_END,
};

/* Effect */
static  struct regval_list ov5645_effect_negative[] __attribute__((unused)) =
{
	ARRAY_END,
};
/*žŽ¹ÅÐ§¹û*/
static  struct regval_list ov5645_effect_antique[] __attribute__((unused)) =
{
	ARRAY_END,
};

/* Scene */
static  struct regval_list ov5645_scene_auto[] __attribute__((unused)) =
{
	ARRAY_END,
};

static  struct regval_list ov5645_scene_night[] __attribute__((unused)) =
{
	ARRAY_END,
};


/****************************************************************************************
 * structures
 */
struct ov5645_win_size {
    char                        *name;
    __u32                       width;
    __u32                       height;
    __u32                       exposure_line_width;
    __u32                       capture_maximum_shutter;
    const struct regval_list    *win_regs;
    const struct regval_list    *lsc_regs;
    unsigned int                *frame_rate_array;
};

typedef struct {
    unsigned int max_shutter;
    unsigned int shutter;
    unsigned int gain;
    unsigned int dummy_line;
    unsigned int dummy_pixel;
    unsigned int extra_line;
} exposure_param_t;

enum prev_capt {
    PREVIEW_MODE = 0,
    CAPTURE_MODE
};

struct ov5645_priv {
    struct v4l2_subdev                  subdev;
    struct media_pad                    pad;
    struct v4l2_ctrl_handler            hdl;
    const struct ov5645_color_format    *cfmt;
    const struct ov5645_win_size        *win;
    int                                 model;
    bool                                initialized;
bool autofocused;	//hdc 20151123

    /**
     * ctrls
    */
    /* standard */
    struct v4l2_ctrl *brightness;
    struct v4l2_ctrl *contrast;
    struct v4l2_ctrl *auto_white_balance;
    struct v4l2_ctrl *exposure;
    struct v4l2_ctrl *gain;
    struct v4l2_ctrl *hflip;
    struct v4l2_ctrl *white_balance_temperature;
    /* menu */
    struct v4l2_ctrl *colorfx;
    struct v4l2_ctrl *exposure_auto;
    /* custom */
    struct v4l2_ctrl *scene_exposure;
    struct v4l2_ctrl *prev_capt;

    struct v4l2_rect rect; /* Sensor window */
    struct v4l2_fract timeperframe;
    enum prev_capt prev_capt_mode;
    exposure_param_t preview_exposure_param;
    exposure_param_t capture_exposure_param;
};

struct ov5645_color_format {
    enum v4l2_mbus_pixelcode code;
    enum v4l2_colorspace colorspace;
};

/****************************************************************************************
 * tables
 */
static const struct ov5645_color_format ov5645_cfmts[] = {
    {
        .code		= V4L2_MBUS_FMT_YUYV8_2X8,
        .colorspace	= V4L2_COLORSPACE_JPEG,
    },
    {
        .code		= V4L2_MBUS_FMT_UYVY8_2X8,
        .colorspace	= V4L2_COLORSPACE_JPEG,
    },
    {
        .code		= V4L2_MBUS_FMT_YVYU8_2X8,
        .colorspace	= V4L2_COLORSPACE_JPEG,
    },
    {
        .code		= V4L2_MBUS_FMT_VYUY8_2X8,
        .colorspace	= V4L2_COLORSPACE_JPEG,
    },
};

/*
 * window size list
 */
#define VGA_WIDTH           640
#define VGA_HEIGHT          480
#define UXGA_WIDTH          1600
#define UXGA_HEIGHT         1200
#define SVGA_WIDTH          1280
#define SVGA_HEIGHT         960
#define OV5645_MAX_WIDTH    UXGA_WIDTH
#define OV5645_MAX_HEIGHT   UXGA_HEIGHT
#define AHEAD_LINE_NUM		15    //10ÐÐ = 50ŽÎÑ­»·(OV5645)
#define DROP_NUM_CAPTURE			0
#define DROP_NUM_PREVIEW			0

static unsigned int frame_rate_svga[] = {20,};
static unsigned int frame_rate_uxga[] = {20,30,};

/* 2593x1944 */
static const struct ov5645_win_size OV5645_win_2592x1944 = {
    .name     = "2592x1944",
    .width    = 2592,
    .height   = 1944,
    .win_regs = OV5645_res_2592x1944,
    .frame_rate_array = frame_rate_svga,
};

/* 1920x1080 */
static const struct ov5645_win_size OV5645_win_1920x1080 = {
    .name     = "1920x1080",
    .width    = 1920,
    .height   = 1080,
    .win_regs = OV5645_res_1920x1080,
    .frame_rate_array = frame_rate_svga,
};

/* 1280x960 */
static const struct ov5645_win_size OV5645_win_1280x960 = {
    .name     = "SVGA",
    .width    = SVGA_WIDTH,
    .height   = SVGA_HEIGHT,
    .win_regs = OV5645_res_1280x960,
    .frame_rate_array = frame_rate_svga,
};

/* 1280x720 */
static const struct ov5645_win_size OV5645_win_1280x720 = {
    .name     = "1280x720",
    .width    = 1280,
    .height   = 720,
    .win_regs = OV5645_res_1280x720,
    .frame_rate_array = frame_rate_svga,
};

/* 1600X1200 */
static const struct ov5645_win_size OV5645_win_1600x1200 = {
    .name     = "UXGA",
    .width    = UXGA_WIDTH,
    .height   = UXGA_HEIGHT,
    .win_regs = OV5645_res_1600x1200,
    .frame_rate_array = frame_rate_uxga,
};

/* 640x480 */
static const struct ov5645_win_size OV5645_win_640x480 = {
    .name     = "VGA",
    .width    = 640,
    .height   = 480,
    .win_regs = OV5645_res_640x480,
    .frame_rate_array = frame_rate_uxga,
};

static const struct ov5645_win_size *ov5645_win[] = {
    &OV5645_win_2592x1944,
    &OV5645_win_1920x1080,
    &OV5645_win_1280x960,
    &OV5645_win_1280x720,
    &OV5645_win_1600x1200,
    &OV5645_win_640x480,
};

/****************************************************************************************
 * general functions
 */
static inline struct ov5645_priv *to_priv(struct v4l2_subdev *subdev)
{
    return container_of(subdev, struct ov5645_priv, subdev);
}

static inline struct v4l2_subdev *ctrl_to_sd(struct v4l2_ctrl *ctrl)
{
    return &container_of(ctrl->handler, struct ov5645_priv, hdl)->subdev;
}

static int reg_read_16(struct i2c_client *client, u16 reg, u8 *val)
{
	int ret;
	/* We have 16-bit i2c addresses - care for endianess */
	unsigned char data[2] = { reg >> 8, reg & 0xff };

	ret = i2c_master_send(client, data, 2);
	if (ret < 2) {
		dev_err(&client->dev, "%s: i2c read send error,addr= %x , reg: %x\n",
			__func__, client->addr,reg);
		return ret < 0 ? ret : -EIO;
	}

	ret = i2c_master_recv(client, val, 1);
	if (ret < 1) {
		dev_err(&client->dev, "%s: i2c read recv error, reg: %x\n",
				__func__, reg);
		return ret < 0 ? ret : -EIO;
	}
	return 0;
}

static int reg_write_16(struct i2c_client *client, u16 reg, u8 val)
{
	int ret;
	unsigned char data[3] = { reg >> 8, reg & 0xff, val };

	ret = i2c_master_send(client, data, 3);
	if (ret < 3) {
		dev_err(&client->dev, "%s: i2c write error, reg: %x\n",	__func__, reg);
		return ret < 0 ? ret : -EIO;
	}
	return 0;
}
static int reg_write_array(struct i2c_client *client,
				struct regval_list  *vals)
{
	while(vals->reg != ARRAY_END_ADDR){
		int ret = reg_write_16(client, vals->reg, vals->value);
		if (ret < 0)
			return ret;
		#if 0 // for test write sensor reg
			//mdelay(50);
			msleep(10);
			uint8_t value = -1;;

			ret = reg_read_16(client, vals->reg, &value);
			if(vals->value != (value & 0xff))
				printk("[addr]%x, [R] reg[0x:%x] [W]:value:[0x%x]  [R]value[:0x%x]\n",client->addr, vals->reg,vals->value,value);
		#endif
		if(vals->delayms> 0)
			msleep(vals->delayms);
		vals++;
	}
	return 0;
}

int ov5645_read(struct v4l2_subdev *sd, unsigned short reg,
		unsigned char *value)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	unsigned char buf[2] = {reg >> 8, reg & 0xff};
	struct i2c_msg msg[2] = {
		[0] = {
			.addr	= client->addr,
			.flags	= 0,
			.len	= 2,
			.buf	= buf,
		},
		[1] = {
			.addr	= client->addr,
			.flags	= I2C_M_RD,
			.len	= 1,
			.buf	= value,
		}
	};
	int ret;

	ret = i2c_transfer(client->adapter, msg, 2);
	if (ret > 0)
		ret = 0;

	return ret;
}

static int ov5645_write(struct v4l2_subdev *sd, unsigned short reg,
		unsigned char value)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	unsigned char buf[3] = {reg >> 8, reg & 0xff, value};
	struct i2c_msg msg = {
		.addr	= client->addr,
		.flags	= 0,
		.len	= 3,
		.buf	= buf,
	};
	int ret;

	ret = i2c_transfer(client->adapter, &msg, 1);
	if (ret > 0)
		ret = 0;

	return ret;
}

static const struct ov5645_win_size *ov5645_select_win(u32 width, u32 height)
{
	const struct ov5645_win_size *win;
    int i;
    OV5645_DBG("%s\n",__func__);

    for (i = 0; i < ARRAY_SIZE(ov5645_win); i++) {
        win = ov5645_win[i];
        if (width == win->width && height == win->height)
            return win;
    }

    printk(KERN_ERR "%s: unsupported width, height (%dx%d)\n", __func__, width, height);
    return NULL;
}

static int ov5645_set_mbusformat(struct i2c_client *client, const struct ov5645_color_format *cfmt)
{

    enum v4l2_mbus_pixelcode code;
    int ret = -1;

    OV5645_DBG("%s\n",__func__);

    code = cfmt->code;
    switch (code) {
        case V4L2_MBUS_FMT_YUYV8_2X8:
            ret  = reg_write_array(client, ov5645_fmt_yuv422_yuyv);
            break;
        case V4L2_MBUS_FMT_UYVY8_2X8:
            ret  = reg_write_array(client, ov5645_fmt_yuv422_uyvy);
            break;
        case V4L2_MBUS_FMT_YVYU8_2X8:
            ret  = reg_write_array(client, ov5645_fmt_yuv422_yvyu);
            break;
        case V4L2_MBUS_FMT_VYUY8_2X8:
            ret  = reg_write_array(client, ov5645_fmt_yuv422_vyuy);
            break;
        default:
            printk(KERN_ERR "mbus code error in %s() line %d\n",__FUNCTION__, __LINE__);
    }
    return ret;
}

static int ov5645_set_params(struct v4l2_subdev *sd, u32 *width, u32 *height, enum v4l2_mbus_pixelcode code)
{
    struct ov5645_priv *priv = to_priv(sd);
    const struct ov5645_win_size *old_win, *new_win;
    int i;
    OV5645_DBG("%s\n",__func__);

    /*
     * select format
     */
    priv->cfmt = NULL;
    for (i = 0; i < ARRAY_SIZE(ov5645_cfmts); i++) {
        if (code == ov5645_cfmts[i].code) {
            priv->cfmt = ov5645_cfmts + i;
            break;
        }
    }
    if (!priv->cfmt) {
        printk(KERN_ERR "Unsupported sensor format.\n");
        return -EINVAL;
    }

    /*
     * select win
     */
    old_win = priv->win;
    new_win = ov5645_select_win(*width, *height);
    if (!new_win) {
        printk(KERN_ERR "Unsupported win size\n");
        return -EINVAL;
    }
    priv->win = new_win;

    priv->rect.left = 0;
    priv->rect.top = 0;
    priv->rect.width = priv->win->width;
    priv->rect.height = priv->win->height;

    *width = priv->win->width;
    *height = priv->win->height;

    return 0;
}

/****************************************************************************************
 * control functions
 */
static int ov5645_set_brightness(struct v4l2_subdev *sd, struct v4l2_ctrl *ctrl)
{
    struct i2c_client *client = v4l2_get_subdevdata(sd);
    //struct ov5645_priv *priv = to_priv(sd);
    int value = ctrl->val;
    int ret = 0;
    OV5645_DBG("%s: val %d\n", __func__, ctrl->val);

    switch(value){
		case -3:
			ret=reg_write_array(client, OV5645_reg_exposure_n3);
			break;
		case -2:
			ret=reg_write_array(client, OV5645_reg_exposure_n2);
			break;
		case -1:
			ret=reg_write_array(client, OV5645_reg_exposure_n1);
			break;
		case 0:
			ret=reg_write_array(client, OV5645_reg_exposure_0);
			break;
		case 1:
			ret=reg_write_array(client, OV5645_reg_exposure_1);
			break;
		case 2:
			ret=reg_write_array(client, OV5645_reg_exposure_2);
			break;
		case 3:
			ret=reg_write_array(client, OV5645_reg_exposure_3);
			break;
		default:
			//OV5645_DBG("%s:default \n",__func__);
			ret = -EINVAL	;
	}

    return 0;
}

static int ov5645_set_contrast(struct v4l2_subdev *sd, struct v4l2_ctrl *ctrl)
{
    /* TODO */
    int contrast = ctrl->val;
    OV5645_DBG("%s: val %d\n", __func__, contrast);

    return 0;
}

static int ov5645_set_auto_white_balance(struct v4l2_subdev *sd, struct v4l2_ctrl *ctrl)
{
    struct i2c_client *client = v4l2_get_subdevdata(sd);
    /* struct ov5645_priv *priv = to_priv(sd); */
    int auto_white_balance = ctrl->val;
    int ret;
    u8 awb;

    OV5645_DBG("%s: val %d\n", __func__, auto_white_balance);
    if (auto_white_balance < 0 || auto_white_balance > 1) {
        dev_err(&client->dev, "set auto_white_balance over range, auto_white_balance = %d\n", auto_white_balance);
        return -ERANGE;
    }

    reg_read_16(client, 0x3406, &awb);
    switch(auto_white_balance) {
        case 0:
            OV5645_ERR("===awb disable===\n");
            ret = reg_write_16(client, 0x3406, awb&0xfe);
            break;
        case 1:
            OV5645_ERR("===awb enable===\n");
            ret = reg_write_16(client, 0x3406, awb|0x01);
            break;
    }

    return 0;
}

/* TODO : exposure */
static int ov5645_set_exposure(struct v4l2_subdev *sd, struct v4l2_ctrl *ctrl)
{
    OV5645_DBG("%s: val %d\n", __func__, ctrl->val);
    return 0;
}

/* TODO */
static int ov5645_set_gain(struct v4l2_subdev *sd, struct v4l2_ctrl *ctrl)
{
    /* struct i2c_client *client = v4l2_get_subdevdata(sd); */
    /* struct ov5645_priv *priv = to_priv(sd); */
    OV5645_DBG("%s: val %d\n", __func__, ctrl->val);
    return 0;
}

/* TODO */
static int ov5645_set_hflip(struct v4l2_subdev *sd, struct v4l2_ctrl *ctrl)
{
    /* struct i2c_client *client = v4l2_get_subdevdata(sd); */
    /* struct ov5645_priv *priv = to_priv(sd); */
    OV5645_DBG("%s: val %d\n", __func__, ctrl->val);
    return 0;
}

static int ov5645_set_white_balance_temperature(struct v4l2_subdev *sd, struct v4l2_ctrl *ctrl)
{
    struct i2c_client *client = v4l2_get_subdevdata(sd);
    /* struct ov5645_priv *priv = to_priv(sd); */
    int white_balance_temperature = ctrl->val;
    int ret;

    OV5645_DBG("%s: val %d\n", __func__, ctrl->val);

    switch(white_balance_temperature) {
        case V4L2_WHITE_BALANCE_FLUORESCENT:
            ret = reg_write_array(client, OV5645_reg_wb_fluorscent);
            break;
        case V4L2_WHITE_BALANCE_SUNNY:
            ret = reg_write_array(client, OV5645_reg_wb_sunny);
            break;
        case V4L2_WHITE_BALANCE_CLOUDY_DAYLIGHT:
            ret = reg_write_array(client, OV5645_reg_wb_cloudy);
            break;
        case V4L2_WHITE_BALANCE_TUNGSTEN:
            ret = reg_write_array(client, OV5645_reg_wb_tungsten);
            break;
        default:
            dev_err(&client->dev, "set white_balance_temperature over range, white_balance_temperature = %d\n", white_balance_temperature);
            return -ERANGE;
    }

    return 0;
}

static int ov5645_set_colorfx(struct v4l2_subdev *sd, struct v4l2_ctrl *ctrl)
{
    struct i2c_client *client = v4l2_get_subdevdata(sd);
    /* struct ov5645_priv *priv = to_priv(sd); */
    int colorfx = ctrl->val;
    int ret;

    OV5645_DBG("%s: val %d\n", __func__, ctrl->val);

    switch (colorfx) {
        case V4L2_COLORFX_NONE: /* normal */
            ret = reg_write_array(client, ov5645_colorfx_none_regs);
            break;
        case V4L2_COLORFX_BW: /* black and white */
            ret = reg_write_array(client, ov5645_colorfx_bw_regs);
            break;
        case V4L2_COLORFX_SEPIA: /* antique ,žŽ¹Å*/
            ret = reg_write_array(client, ov5645_colorfx_sepia_regs);
            break;
        case V4L2_COLORFX_NEGATIVE: /* negative£¬žºÆ¬ */
            ret = reg_write_array(client, ov5645_colorfx_negative_regs);
            break;
        default:
            dev_err(&client->dev, "set colorfx over range, colorfx = %d\n", colorfx);
            return -ERANGE;
    }

    return 0;
}

static int ov5645_set_exposure_auto(struct v4l2_subdev *sd, struct v4l2_ctrl *ctrl)
{
    struct i2c_client *client = v4l2_get_subdevdata(sd);
    int exposure_auto = ctrl->val;

    /* unsigned int reg_0xec; */
    /* int ret; */

	OV5645_DBG("%s:set auto focus,value=%d \n",__func__,exposure_auto);

	if(exposure_auto == 0){               //ace 20150729 add test for auot focus
		OV5645_DBG("set auto focus cancel \n");
		//state->focus_mode = -1;
		//reg_write_array(client, OV5645_focus_cancel);
reg_write_array(client, OV5645_focus_constant);	//not work;
	}else{
		OV5645_DBG("start to single focus \n");
		//state->focus_mode = FOCUS_MODE_AUTO;
		//reg_write_array(client, OV5645_focus_single);

//reg_write_array(client, OV5645_focus_constant);

#if 0
//if(!priv->autofocused)
{
	priv->autofocused = true;
	//reg_write_array(client, OV5645_focus_constant);
	printk("hdc auto focus constant\n");
}
#endif 

#if 0
		while(1)
		{
			ov5645_read(sd,0x3029,&temval);
			if(temval=0x10)
				break;
			mdelay(10);
		}
		mdelay(10);
#endif
	}

    if (exposure_auto < 0 || exposure_auto > 1) {
        dev_err(&client->dev, "set exposure_auto over range, exposure_auto = %d\n", exposure_auto);
        return -ERANGE;
    }

    return 0;
}

/* TODO */
static int ov5645_set_scene_exposure(struct v4l2_subdev *sd, struct v4l2_ctrl *ctrl)
{
    OV5645_DBG("%s: val %d\n", __func__, ctrl->val);
    return 0;
}

static int ov5645_set_prev_capt_mode(struct v4l2_subdev *sd, struct v4l2_ctrl *ctrl)
{
    struct i2c_client *client = v4l2_get_subdevdata(sd);
    struct ov5645_priv *priv = to_priv(sd);
    OV5645_DBG("%s: val %d\n", __func__, ctrl->val);

    switch(ctrl->val) {
        case PREVIEW_MODE:
            priv->prev_capt_mode = ctrl->val;
            break;
        case CAPTURE_MODE:
            priv->prev_capt_mode = ctrl->val;
            break;
        default:
            dev_err(&client->dev, "set_prev_capt_mode over range, prev_capt_mode = %d\n", ctrl->val);
            return -ERANGE;
    }

    return 0;
}

static int ov5645_s_ctrl(struct v4l2_ctrl *ctrl)
{
    struct v4l2_subdev *sd = ctrl_to_sd(ctrl);
    struct i2c_client *client = v4l2_get_subdevdata(sd);
    int ret = 0;
    OV5645_DBG("%s.............%x...........\n",__func__,ctrl->id);

    switch (ctrl->id) {
        case V4L2_CID_BRIGHTNESS:
            ov5645_set_brightness(sd, ctrl);
            break;

        case V4L2_CID_CONTRAST:
            ov5645_set_contrast(sd, ctrl);
            break;

        case V4L2_CID_AUTO_WHITE_BALANCE:
            ov5645_set_auto_white_balance(sd, ctrl);
            break;

        case V4L2_CID_EXPOSURE:
            ov5645_set_exposure(sd, ctrl);
            break;

        case V4L2_CID_GAIN:
            ov5645_set_gain(sd, ctrl);
            break;

        case V4L2_CID_HFLIP:
            ov5645_set_hflip(sd, ctrl);
            break;

        case V4L2_CID_WHITE_BALANCE_TEMPERATURE:
            ov5645_set_white_balance_temperature(sd, ctrl);
            break;

        case V4L2_CID_COLORFX:
            ov5645_set_colorfx(sd, ctrl);
            break;

        case V4L2_CID_EXPOSURE_AUTO:
            ov5645_set_exposure_auto(sd, ctrl);
            break;

        case V4L2_CID_SCENE_EXPOSURE:
            ov5645_set_scene_exposure(sd, ctrl);
            break;

        case V4L2_CID_PRIVATE_PREV_CAPT:
            ov5645_set_prev_capt_mode(sd, ctrl);
            break;

        default:
            dev_err(&client->dev, "%s: invalid control id %d\n", __func__, ctrl->id);
            return -EINVAL;
    }

    return ret;
}

static const struct v4l2_ctrl_ops ov5645_ctrl_ops = {
    .s_ctrl = ov5645_s_ctrl,
};

static const struct v4l2_ctrl_config ov5645_custom_ctrls[] = {
    {
        .ops    = &ov5645_ctrl_ops,
        .id     = V4L2_CID_SCENE_EXPOSURE,
        .type   = V4L2_CTRL_TYPE_INTEGER,
        .name   = "SceneExposure",
        .min    = 0,
        .max    = 1,
        .def    = 0,
        .step   = 1,
    }, {
        .ops    = &ov5645_ctrl_ops,
        .id     = V4L2_CID_PRIVATE_PREV_CAPT,
        .type   = V4L2_CTRL_TYPE_INTEGER,
        .name   = "PrevCapt",
        .min    = 0,
        .max    = 1,
        .def    = 0,
        .step   = 1,
    }
};

static int ov5645_initialize_ctrls(struct ov5645_priv *priv)
{
    OV5645_DBG("%s\n",__func__);

    v4l2_ctrl_handler_init(&priv->hdl, NUM_CTRLS);

    /* standard ctrls */
    priv->brightness = v4l2_ctrl_new_std(&priv->hdl, &ov5645_ctrl_ops,
            V4L2_CID_BRIGHTNESS, 0, 6, 1, 0);
    if (!priv->brightness) {
        printk(KERN_ERR "%s: failed to create brightness ctrl\n", __func__);
        return -ENOENT;
    }

    priv->contrast = v4l2_ctrl_new_std(&priv->hdl, &ov5645_ctrl_ops,
            V4L2_CID_CONTRAST, -6, 6, 1, 0);
    if (!priv->contrast) {
        printk(KERN_ERR "%s: failed to create contrast ctrl\n", __func__);
        return -ENOENT;
    }

    priv->auto_white_balance = v4l2_ctrl_new_std(&priv->hdl, &ov5645_ctrl_ops,
            V4L2_CID_AUTO_WHITE_BALANCE, 0, 1, 1, 1);
    if (!priv->auto_white_balance) {
        printk(KERN_ERR "%s: failed to create auto_white_balance ctrl\n", __func__);
        return -ENOENT;
    }

#if 0
    priv->exposure = v4l2_ctrl_new_std(&priv->hdl, &ov5645_ctrl_ops,
            V4L2_CID_EXPOSURE, 0, 0xFFFF, 1, 500);
    if (!priv->exposure) {
        printk(KERN_ERR "%s: failed to create exposure ctrl\n", __func__);
        return -ENOENT;
    }
#endif

    priv->gain = v4l2_ctrl_new_std(&priv->hdl, &ov5645_ctrl_ops,
            V4L2_CID_GAIN, 0, 0xFF, 1, 128);
    if (!priv->gain) {
        printk(KERN_ERR "%s: failed to create gain ctrl\n", __func__);
        return -ENOENT;
    }

    priv->hflip = v4l2_ctrl_new_std(&priv->hdl, &ov5645_ctrl_ops,
            V4L2_CID_HFLIP, 0, 1, 1, 0);
    if (!priv->hflip) {
        printk(KERN_ERR "%s: failed to create hflip ctrl\n", __func__);
        return -ENOENT;
    }

    priv->white_balance_temperature = v4l2_ctrl_new_std(&priv->hdl, &ov5645_ctrl_ops,
            V4L2_CID_WHITE_BALANCE_TEMPERATURE, 0, 3, 1, 1);
    if (!priv->white_balance_temperature) {
        printk(KERN_ERR "%s: failed to create white_balance_temperature ctrl\n", __func__);
        return -ENOENT;
    }

    /* standard menus */
    priv->colorfx = v4l2_ctrl_new_std_menu(&priv->hdl, &ov5645_ctrl_ops,
            V4L2_CID_COLORFX, 3, 0, 0);
    if (!priv->colorfx) {
        printk(KERN_ERR "%s: failed to create colorfx ctrl\n", __func__);
        return -ENOENT;
    }

    priv->exposure_auto = v4l2_ctrl_new_std_menu(&priv->hdl, &ov5645_ctrl_ops,
            V4L2_CID_EXPOSURE_AUTO, 1, 0, 1);
    if (!priv->exposure_auto) {
        printk(KERN_ERR "%s: failed to create exposure_auto ctrl\n", __func__);
        return -ENOENT;
    }

    /* custom ctrls */
    priv->scene_exposure = v4l2_ctrl_new_custom(&priv->hdl, &ov5645_custom_ctrls[0], NULL);
    if (!priv->scene_exposure) {
        printk(KERN_ERR "%s: failed to create scene_exposure ctrl\n", __func__);
        return -ENOENT;
    }

    priv->prev_capt = v4l2_ctrl_new_custom(&priv->hdl, &ov5645_custom_ctrls[1], NULL);
    if (!priv->prev_capt) {
        printk(KERN_ERR "%s: failed to create prev_capt ctrl\n", __func__);
        return -ENOENT;
    }

    priv->subdev.ctrl_handler = &priv->hdl;
    if (priv->hdl.error) {
        printk(KERN_ERR "%s: ctrl handler error(%d)\n", __func__, priv->hdl.error);
        v4l2_ctrl_handler_free(&priv->hdl);
        return -EINVAL;
    }

    return 0;
}

static int ov5645_save_exposure_param(struct v4l2_subdev *sd)
{
    //struct i2c_client *client = v4l2_get_subdevdata(sd);
    //struct ov5645_priv *priv = to_priv(sd);
    int ret = 0;
  /*  unsigned int reg_0x03 = 0x20;
    unsigned int reg_0x80;
    unsigned int reg_0x81;
    unsigned int reg_0x82;

    i2c_smbus_write_byte_data(client, 0x03, reg_0x03); //page 20
    reg_0x80 = i2c_smbus_read_byte_data(client, 0x80);
    reg_0x81 = i2c_smbus_read_byte_data(client, 0x81);
    reg_0x82 = i2c_smbus_read_byte_data(client, 0x82);

    priv->preview_exposure_param.shutter = (reg_0x80 << 16)|(reg_0x81 << 8)|reg_0x82;
    priv->capture_exposure_param.shutter = (priv->preview_exposure_param.shutter)/2;
*/
    return ret;
}

static int ov5645_set_exposure_param(struct v4l2_subdev *sd) __attribute__((unused));
static int ov5645_set_exposure_param(struct v4l2_subdev *sd)
{
    //struct i2c_client *client = v4l2_get_subdevdata(sd);
    //struct ov5645_priv *priv = to_priv(sd);
    //int ret;
 /*   unsigned int reg_0x03 = 0x20;
    unsigned int reg_0x83;
    unsigned int reg_0x84;
    unsigned int reg_0x85;

    if(priv->capture_exposure_param.shutter < 1)
        priv->capture_exposure_param.shutter = 1;

    reg_0x83 = (priv->capture_exposure_param.shutter)>>16;
    reg_0x84 = ((priv->capture_exposure_param.shutter)>>8) & 0x000000FF;
    reg_0x85 = (priv->capture_exposure_param.shutter) & 0x000000FF;

    ret = i2c_smbus_write_byte_data(client, 0x03, reg_0x03); //page 20
    ret |= i2c_smbus_write_byte_data(client, 0x83, reg_0x83);
    ret |= i2c_smbus_write_byte_data(client, 0x84, reg_0x84);
    ret |= i2c_smbus_write_byte_data(client, 0x85, reg_0x85);
*/
    return 0;
}

/****************************************************************************************
 * v4l2 subdev ops
 */

/**
 * core ops
 */
static int ov5645_g_chip_ident(struct v4l2_subdev *sd, struct v4l2_dbg_chip_ident *id)
{

    struct ov5645_priv *priv = to_priv(sd);
    OV5645_DBG("%s\n",__func__);

    id->ident    = priv->model;
    id->revision = 0;
    return 0;
}

static int ov5645_s_power(struct v4l2_subdev *sd, int on)
{
    /* used when suspending */
    OV5645_DBG("%s: on %d\n", __func__, on);
    if (!on) {
        struct ov5645_priv *priv = to_priv(sd);
        priv->initialized = false;
	    priv->autofocused = false;	//hdc 20151123
    } 

    return 0;
}

static const struct v4l2_subdev_core_ops ov5645_subdev_core_ops = {
    .g_chip_ident	= ov5645_g_chip_ident,
    .s_power        = ov5645_s_power,
    .s_ctrl         = v4l2_subdev_s_ctrl,
};

static int reg_wait_stat(struct i2c_client *client, u16 reg, u8 mask, u8 stat)
{
    int retry = 20;
    u8 val = 0;
    int ret;

    do {
        ret = reg_read_16(client, reg, &val);
        if (ret < 0)
            return ret;
        OV5645_DBG("%s status %x\n", __func__, val);
        if ((val & mask) == stat)
            break;

        msleep(50);

    } while (--retry);

    if (!retry)
        return -ETIMEDOUT;

    return 0;
}

static int reg_txdata(struct i2c_client *client, u8 *data, int length)
{
    struct i2c_msg msg[] = {
        {
            .addr   = client->addr,
            .flags  = 0,
            .len    = length,
            .buf    = data,
        },
    };

    if (i2c_transfer(client->adapter, msg, 1) < 0) {
        dev_err(&client->dev, "%s: i2c transfer data (%d bytes) error\n",
                __func__, length);
        return -EIO;
    }

    return 0;
}


static struct i2c_client *gclient;
static void ov5640_auto_focus(struct work_struct *work)
{
    reg_write_array(gclient, OV5645_auto_focus_init_s1);

    msleep(20);

    reg_txdata(gclient, OV5645_auto_focus_init_s2, ARRAY_SIZE(OV5645_auto_focus_init_s2));
    msleep(100);

    reg_write_array(gclient, OV5645_auto_focus_init_s3);

    reg_wait_stat(gclient, 0x3029, 0xff, 0x70);

    reg_write_array(gclient, OV5645_focus_constant);
}




static int ov5645_init(struct v4l2_subdev *sd)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
    gclient = client;
	OV5645_DBG("enter %s\n",__func__);

	reg_write_array(client, ov5645_reset_regs);
	usleep_range(5000, 5500);  // must
    
    reg_write_array(client, OV5645_reg_init_2lane);
    
    msleep(20);
    
    return 0;
}


/**
 * video ops
 */
static int ov5645_s_stream(struct v4l2_subdev *sd, int enable)
{
    struct i2c_client *client = v4l2_get_subdevdata(sd);
    struct ov5645_priv *priv = to_priv(sd);
    int ret = 0;
	
    OV5645_DBG("%s: enable %d, initialized %d\n", __func__, enable, priv->initialized);
    
    if (enable) {
        if (!priv->win || !priv->cfmt) {
            dev_err(&client->dev, "norm or win select error\n");
            return -EPERM;
        }
        /* write init regs */
        if (!priv->initialized) {

            ov5645_video_probe(client);

            ret = ov5645_init(sd);
            if (ret < 0) {
                printk(KERN_ERR "%s: failed to reg_write_array init regs\n", __func__);
                return -EIO;
            }
            priv->initialized = true;
            OV5645_DBG("%s reg_write_array init regs\n", __func__);
        } 
        reg_write_array(client, OV5645_reg_stop_stream);
        ret = reg_write_array(client, (struct regval_list *)priv->win->win_regs);
        if (ret < 0) {
            printk(KERN_ERR "%s: failed to reg_write_array win regs\n", __func__);
            return -EIO;
        }
        OV5645_DBG("%s: reg_write_array win regs\n", __func__);

        ret = ov5645_set_mbusformat(client, priv->cfmt);
        if (ret < 0) {
            printk(KERN_ERR "%s: failed to ov5645_set_mbusformat()\n", __func__);
            return -EIO;
        }

        reg_write_array(client, OV5645_reg_start_stream);
    
	schedule_delayed_work(&focus_work, msecs_to_jiffies(3000));
    } else {
        //reg_write_array(client, OV5645_reg_stop_stream);
	    OV5645_DBG("%s: OV5645_reg_stop_stream\n", __func__);
    }

    return ret;
}

static int ov5645_enum_framesizes(struct v4l2_subdev *sd, struct v4l2_frmsizeenum *fsize)
{
    struct i2c_client *client = v4l2_get_subdevdata(sd);

    if (fsize->index >= ARRAY_SIZE(ov5645_win)) {
        dev_err(&client->dev, "index(%d) is over range %d\n", fsize->index, ARRAY_SIZE(ov5645_win));
        return -EINVAL;
    }

    switch (fsize->pixel_format) {
        case V4L2_PIX_FMT_YUV420:
        case V4L2_PIX_FMT_YUV422P:
        case V4L2_PIX_FMT_NV12:
        case V4L2_PIX_FMT_YUYV:
            fsize->type = V4L2_FRMSIZE_TYPE_DISCRETE;
            fsize->discrete.width = ov5645_win[fsize->index]->width;
            fsize->discrete.height = ov5645_win[fsize->index]->height;
            break;
        default:
            dev_err(&client->dev, "pixel_format(%d) is Unsupported\n", fsize->pixel_format);
            return -EINVAL;
    }

    dev_info(&client->dev, "type %d, width %d, height %d\n", V4L2_FRMSIZE_TYPE_DISCRETE, fsize->discrete.width, fsize->discrete.height);
    return 0;
}

static int ov5645_enum_mbus_fmt(struct v4l2_subdev *sd, unsigned int index,
        enum v4l2_mbus_pixelcode *code)
{
    if (index >= ARRAY_SIZE(ov5645_cfmts))
        return -EINVAL;
    OV5645_DBG("%s\n",__func__);

    *code = ov5645_cfmts[index].code;
    return 0;
}

static int ov5645_g_mbus_fmt(struct v4l2_subdev *sd, struct v4l2_mbus_framefmt *mf)
{
    struct i2c_client *client = v4l2_get_subdevdata(sd);
    struct ov5645_priv *priv = to_priv(sd);
    if (!priv->win || !priv->cfmt) {
        u32 width = SVGA_WIDTH;
        u32 height = SVGA_HEIGHT;
        int ret = ov5645_set_params(sd, &width, &height, V4L2_MBUS_FMT_UYVY8_2X8);
        if (ret < 0) {
            dev_info(&client->dev, "%s, %d\n", __func__, __LINE__);
            return ret;
        }
    }
    OV5645_DBG("%s\n",__func__);

    mf->width   = priv->win->width;
    mf->height  = priv->win->height;
    mf->code    = priv->cfmt->code;
    mf->colorspace  = priv->cfmt->colorspace;
    mf->field   = V4L2_FIELD_NONE;
    dev_info(&client->dev, "%s, %d\n", __func__, __LINE__);
    return 0;
}

static int ov5645_try_mbus_fmt(struct v4l2_subdev *sd,
        struct v4l2_mbus_framefmt *mf)
{
    /* struct i2c_client *client = v4l2_get_subdevdata(sd); */
    struct ov5645_priv *priv = to_priv(sd);
    const struct ov5645_win_size *win;
    int i;
    OV5645_DBG("%s\n",__func__);

    /*
     * select suitable win
     */
    win = ov5645_select_win(mf->width, mf->height);
    if (!win)
        return -EINVAL;

    mf->width   = win->width;
    mf->height  = win->height;
    mf->field   = V4L2_FIELD_NONE;


    for (i = 0; i < ARRAY_SIZE(ov5645_cfmts); i++)
        if (mf->code == ov5645_cfmts[i].code)
            break;

    if (i == ARRAY_SIZE(ov5645_cfmts)) {
        /* Unsupported format requested. Propose either */
        if (priv->cfmt) {
            /* the current one or */
            mf->colorspace = priv->cfmt->colorspace;
            mf->code = priv->cfmt->code;
        } else {
            /* the default one */
            mf->colorspace = ov5645_cfmts[0].colorspace;
            mf->code = ov5645_cfmts[0].code;
        }
    } else {
        /* Also return the colorspace */
        mf->colorspace	= ov5645_cfmts[i].colorspace;
    }

    return 0;
}

static int ov5645_s_mbus_fmt(struct v4l2_subdev *sd, struct v4l2_mbus_framefmt *mf)
{
    /* struct i2c_client *client = v4l2_get_subdevdata(sd); */
    struct ov5645_priv *priv = to_priv(sd);
    int ret = -1;
    OV5645_DBG("%s\n",__func__);

    ret = ov5645_set_params(sd, &mf->width, &mf->height, mf->code);
    if (!ret)
        mf->colorspace = priv->cfmt->colorspace;

    return ret;
}

static const struct v4l2_subdev_video_ops ov5645_subdev_video_ops = {
    .s_stream               = ov5645_s_stream,
    .enum_framesizes        = ov5645_enum_framesizes,
    .enum_mbus_fmt          = ov5645_enum_mbus_fmt,
    .g_mbus_fmt             = ov5645_g_mbus_fmt,
    .try_mbus_fmt           = ov5645_try_mbus_fmt,
    .s_mbus_fmt             = ov5645_s_mbus_fmt,
};

/**
 * pad ops
 */
static int ov5645_s_fmt(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh,
        struct v4l2_subdev_format *fmt)
{
    struct v4l2_mbus_framefmt *mf = &fmt->format;
    OV5645_DBG("%s: %dx%d\n", __func__, mf->width, mf->height);
    return ov5645_s_mbus_fmt(sd, mf);
}

static const struct v4l2_subdev_pad_ops ov5645_subdev_pad_ops = {
    .set_fmt    = ov5645_s_fmt,
};

/**
 * subdev ops
 */
static const struct v4l2_subdev_ops ov5645_subdev_ops = {
    .core   = &ov5645_subdev_core_ops,
    .video  = &ov5645_subdev_video_ops,
    .pad    = &ov5645_subdev_pad_ops,
};

/**
 * media_entity_operations
 */
static int ov5645_link_setup(struct media_entity *entity,
        const struct media_pad *local,
        const struct media_pad *remote, u32 flags)
{
    OV5645_DBG("%s\n", __func__);  // printk oc5645_link_setup
	
    printk("This is turf speaking\n");
    return 0;
}

static const struct media_entity_operations ov5645_media_ops = {
    .link_setup = ov5645_link_setup,
};

/****************************************************************************************
 * initialize
 */
static void ov5645_priv_init(struct ov5645_priv * priv)
{
    priv->model = V4L2_IDENT_OV5645;
    priv->prev_capt_mode = PREVIEW_MODE;
    priv->timeperframe.denominator = 12;//30;
    priv->timeperframe.numerator = 1;
    priv->win = &OV5645_win_1280x960;
}

static int ov5645_video_probe(struct i2c_client *client)
{
	int ret;
	u8 id_high, id_low;
	u16 id;

	/* Read sensor Model ID */
	ret = reg_read_16(client, 0x300a, &id_high);
	if (ret < 0)
    {
        OV5645_DBG("ov5645 read high ID error!\n");
		return ret;
    }

	id = id_high << 8;

	ret = reg_read_16(client, 0x300b, &id_low);
	if (ret < 0)
    {
        OV5645_DBG("ov5645 read low ID error!\n");
		return ret;
    }

	id |= id_low;

	OV5645_DBG("OV5645 Chip ID 0x%04x detected\n", id);

	if (id != 0x5645)
		return -ENODEV;

	return 0;
}

static int ov5645_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    struct ov5645_priv *priv;
    struct v4l2_subdev *sd;
    int ret;
    priv = kzalloc(sizeof(struct ov5645_priv), GFP_KERNEL);
    if (!priv)
        return -ENOMEM;

    OV5645_DBG("%s\n",__func__);
    ov5645_priv_init(priv);

    sd = &priv->subdev;
    strcpy(sd->name, MODULE_NAME);
	
    //ov5645_video_probe(client);

    /* register subdev */
    v4l2_i2c_subdev_init(sd, client, &ov5645_subdev_ops);

    sd->flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
    priv->pad.flags = MEDIA_PAD_FL_SOURCE;
    sd->entity.type = MEDIA_ENT_T_V4L2_SUBDEV_SENSOR;
    sd->entity.ops  = &ov5645_media_ops;
    if (media_entity_init(&sd->entity, 1, &priv->pad, 0)) {
        dev_err(&client->dev, "%s: failed to media_entity_init()\n", __func__);
        kfree(priv);
        return -ENOENT;
    }

    ret = ov5645_initialize_ctrls(priv);
    if (ret < 0) {
        printk(KERN_ERR "%s: failed to initialize controls\n", __func__);
        kfree(priv);
        return ret;
    }

    return 0;
}

static int ov5645_remove(struct i2c_client *client)
{
    struct v4l2_subdev *sd = i2c_get_clientdata(client);
    v4l2_device_unregister_subdev(sd);
    v4l2_ctrl_handler_free(sd->ctrl_handler);
    media_entity_cleanup(&sd->entity);
    kfree(to_priv(sd));
    return 0;
}

static const struct i2c_device_id ov5645_id[] = {
    { MODULE_NAME, 0 },
    { }
};

MODULE_DEVICE_TABLE(i2c, ov5645_id);

static struct i2c_driver ov5645_i2c_driver = {
    .driver = {
        .name = MODULE_NAME,
    },
    .probe    = ov5645_probe,
    .remove   = ov5645_remove,
    .id_table = ov5645_id,
};

module_i2c_driver(ov5645_i2c_driver);

MODULE_DESCRIPTION("SoC Camera driver for ov5645");
MODULE_AUTHOR("caichsh(caichsh@artekmicro.com)");
MODULE_LICENSE("GPL v2");
