#include "drv.h"
#include <xboot.h>

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
// This need to set to f1c100s or f1c200s
#include <f1c200s/reg-debe.h>
#include <dma/dmapool.h>
#include <framebuffer/framebuffer.h>
#include "lvgl.h"
struct fb_pdata_t
{
	virtual_addr_t virtdefe;
	virtual_addr_t virtdebe;
	virtual_addr_t virttcon;

	char * clkdefe;
	char * clkdebe;
	char * clktcon;
	int rstdefe;
	int rstdebe;
	int rsttcon;
	int width;
	int height;
	int pwidth;
	int pheight;
	int bits_per_pixel;
	int bytes_per_pixel;
	int pixlen;
	int index;
	void * vram[2];
	struct region_list_t * nrl, * orl;

	struct {
		int pixel_clock_hz;
		int h_front_porch;
		int h_back_porch;
		int h_sync_len;
		int v_front_porch;
		int v_back_porch;
		int v_sync_len;
		int h_sync_active;
		int v_sync_active;
		int den_active;
		int clk_active;
	} timing;

	struct led_t * backlight;
	int brightness;
};
struct device_t * pos, * n;
static lv_disp_draw_buf_t disp_buf;
static lv_color_t buf_1[LV_HOR_RES_MAX * LV_VER_RES_MAX];
static lv_color_t buf_2[LV_HOR_RES_MAX * LV_VER_RES_MAX];
struct framebuffer_t* fb;
struct timer_t timer;
static inline void debe_set_address(struct fb_pdata_t * pdat, void * vram)
{
	// This need to set to f1c100s or f1c200s
	struct f1c200s_debe_reg_t * debe = (struct f1c200s_debe_reg_t *)(pdat->virtdebe);

	write32((virtual_addr_t)&debe->layer0_addr_low32b, (u32_t)vram << 3);
	write32((virtual_addr_t)&debe->layer0_addr_high4b, (u32_t)vram >> 29);
}
static void disp_flush(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p)
{
    lv_coord_t hres = disp_drv->rotated == 0 ? disp_drv->hor_res : disp_drv->ver_res;
	uint16_t x = area->x2 - area->x1 + 1;
    struct fb_pdata_t * pdat = (struct fb_pdata_t *)fb->priv;
	uint16_t size_of_row = x*sizeof(uint32_t);
    pdat->index = 0;
    for(int i=area->y1; i<area->y2+1; i++) {
        memcpy(&((uint32_t*)pdat->vram[0])[i*hres + area->x1], color_p, size_of_row);
        color_p += x;
    }
    dma_cache_sync(pdat->vram[0], pdat->width * pdat->height, DMA_TO_DEVICE);
    debe_set_address(pdat, pdat->vram[0]);
    lv_disp_flush_ready(disp_drv);
}
static int lvgui_timer_function(struct timer_t * timer, void * data)
{
    timer_forward(timer, ms_to_ktime(1));
    lv_tick_inc(1);    
    return 1;
}
static void timer_init_drv() {
	/***********************
     * Tick interface
     ***********************/
    /* Initialize a Timer for 1 ms period and
     * in its interrupt call
     * lv_tick_inc(1); */
    timer_init(&timer, lvgui_timer_function, NULL);
    timer_start(&timer, ms_to_ktime(5));
}
void do_init_lvgui(void)
{
	if(!list_empty_careful(&__device_head[DEVICE_TYPE_FRAMEBUFFER]))
	{
		list_for_each_entry_safe(pos, n, &__device_head[DEVICE_TYPE_FRAMEBUFFER], head)
		{
			if((fb = (struct framebuffer_t *)(pos->priv)))
			{
				break;
			}
		}
		static lv_disp_drv_t disp_drv;
		lv_init();
		lv_disp_draw_buf_init(&disp_buf, buf_1, buf_2, LV_HOR_RES_MAX * LV_VER_RES_MAX);
		lv_disp_drv_init(&disp_drv);
		disp_drv.hor_res=LV_HOR_RES_MAX;
		disp_drv.ver_res=LV_VER_RES_MAX;
		disp_drv.draw_buf=&disp_buf;
		disp_drv.flush_cb=disp_flush;
		lv_disp_drv_register(&disp_drv);
	}
}

void hal_init(void)
{   
	timer_init_drv();
    do_init_lvgui();
}