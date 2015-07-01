#include "testitem.h"

#define SPRD_GSENSOR_DEV					"/dev/gma303"
//#define	GSENSOR_IOCTL_BASE 77
//#define GSENSOR_IOCTL_GET_XYZ           _IOW(GSENSOR_IOCTL_BASE, 22, int)
//#define GSENSOR_IOCTL_SET_ENABLE      	_IOW(GSENSOR_IOCTL_BASE, 2, int)
//#define LIS3DH_ACC_IOCTL_GET_CHIP_ID    _IOR(GSENSOR_IOCTL_BASE, 255, char[32])


#define GMA_IOCTL  0x99
#define GMA_IOCTL_READ_ACCEL_XYZ		_IOR(GMA_IOCTL, 0x09, int[3])	//read input report data
#define GMA_IOCTL_CALIBRATION			_IOWR(GMA_IOCTL, 0x05, int[3])	//ACC sensor Calibration
#define GMA_IOCTL_GET_OPEN_STATUS		_IO(GMA_IOCTL, 0x0B)	//get sensor open status
#define GMA_IOCTL_RESET					_IO(GMA_IOCTL, 0x04)			
#define GMA_IOCTL_GET_CLOSE_STATUS		_IO(GMA_IOCTL, 0x0C)	//get sensor close status
#define GMA_IOCTL_GET_LAYOUT			_IOR(GMA_IOCTL, 0x0E, int)						//get sensor layout(for sensor fusion)

#define SPRD_GSENSOR_OFFSET					60
#define SPRD_GSENSOR_1G						1024

int x_pass, y_pass, z_pass;
static int thread_run;
static char device_info[32] = {0};

static int gsensor_open(void)
{
    int fd;
    int enable = 1;
	int layout;

    fd = open(SPRD_GSENSOR_DEV, O_RDWR);

	if (fd<0)
			{
			LOGD("Set G-sensor enable error: %s", strerror(errno));
			close(fd);
			return -1;
		}


    LOGD("Open %s fd:%d", SPRD_GSENSOR_DEV, fd);


		
	ioctl(fd, GMA_IOCTL_RESET);

	
	ioctl(fd, GMA_IOCTL_GET_OPEN_STATUS);

	usleep(1000*50);

	ioctl(fd, GMA_IOCTL_GET_LAYOUT, &layout);
	LOGD("==========layout is %d==============", layout);

	if (layout < 0 ) layout = 2;
	else layout = 1;

	ioctl(fd, GMA_IOCTL_CALIBRATION,&layout);
	
	LOGD("@@@ ioctl id done!@@@@\n");
	//LOGD("@@@ flag = %d @@@@\n",);
	#if 0
    if (ioctl(fd, GMA_IOCTL_CALIBRATION, &enable))
		{
		LOGD("@@@ ######## @@@@\n");
        LOGD("Set G-sensor enable error: %s", strerror(errno));
        close(fd);
        return -1;
    }
	#endif
    return fd;
}

int gsensor_close(int fd)
{
    int enable = 0;
	if(fd > 0) {
		if (ioctl(fd, GMA_IOCTL_GET_CLOSE_STATUS)) {
			LOGD("Set G-sensor disable error: %s", strerror(errno));
		}
		close(fd);
	}
    return 0;
}

int gsensor_get_devinfo(int fd, char * devinfo)
{
    if (fd < 0 || devinfo == NULL)
        return -1;

    if (ioctl(fd, GMA_IOCTL_GET_OPEN_STATUS, devinfo)) {
		LOGD("[%s]: Get device info error. %s]", __func__, strerror(errno));
    }
	return 0;
}

int gsensor_get_data(int fd, int *data)
{
    int tmp[3];
    int ret;

    ret = ioctl(fd, GMA_IOCTL_READ_ACCEL_XYZ, tmp);
    data[0] = tmp[0];
    data[1] = tmp[1];
    data[2] = tmp[2];
    return ret;
}

int gsensor_check(int data)
{
	int ret = -1;
	int start_1 = SPRD_GSENSOR_1G-SPRD_GSENSOR_OFFSET;
	int end_1 = SPRD_GSENSOR_1G+SPRD_GSENSOR_OFFSET;
	int start_2 = -SPRD_GSENSOR_1G-SPRD_GSENSOR_OFFSET;
	int end_2 = -SPRD_GSENSOR_1G+SPRD_GSENSOR_OFFSET;

	if( ((start_1<data)&&(data<end_1))||
		((start_2<data)&&(data<end_2)) ){
		ret = 0;
	}

	return ret;
}


static void *gsensor_thread(void *param)
{
	int fd;
	int counter;
	int cur_row = 2;
	int x_row, y_row, z_row;
	int col = 5;
	int data[3];
	char str[10];

	
	x_pass = y_pass = z_pass = 0;

	//enable gsensor
	fd = gsensor_open();
	if(fd < 0) {
		ui_set_color(CL_RED);
		cur_row = ui_show_text(cur_row, 0, TEXT_GS_OPEN_FAIL);	
	}
	gsensor_get_devinfo(fd, device_info);

	ui_set_color(CL_WHITE);
	cur_row = ui_show_text(cur_row, 0, TEXT_GS_DEV_INFO);
	cur_row = ui_show_text(cur_row, 0, device_info);
	cur_row = ui_show_text(cur_row+1, 0, TEXT_GS_OPER1);
	cur_row = ui_show_text(cur_row, 0, TEXT_GS_OPER2);
	ui_set_color(CL_GREEN);
	x_row = cur_row;
	cur_row = ui_show_text(cur_row, 0, TEXT_GS_X);
	y_row = cur_row;
	cur_row = ui_show_text(cur_row, 0, TEXT_GS_Y);
	z_row = cur_row;
	cur_row = ui_show_text(cur_row, 0, TEXT_GS_Z);

	gr_flip();
	while(!(x_pass&y_pass&z_pass)) {
		//get data
		gsensor_get_data(fd, data);

		ui_set_color(CL_GREEN);
		if(x_pass == 0 && gsensor_check(data[0]) == 0) {
			x_pass = 1;	
			LOGD("[%s] x_pass", __func__);
			sprintf(str,"%d",data[0]);
			ui_show_text(x_row, col, str);

			ui_show_text(x_row, col+7, TEXT_GS_PASS);
			gr_flip();
		}

		if(y_pass == 0 && gsensor_check(data[1]) == 0) {
			y_pass = 1;	
			LOGD("[%s] y_pass", __func__);
			sprintf(str,"%d",data[1]);
			ui_show_text(y_row, col, str);

			ui_show_text(y_row, col+7, TEXT_GS_PASS);
			gr_flip();
		}

		if(z_pass == 0 && gsensor_check(data[2]) == 0) {
			z_pass = 1;	
			LOGD("[%s] z_pass", __func__);
			sprintf(str,"%d",data[2]);
			ui_show_text(z_row, col, str);

			ui_show_text(z_row, col+7, TEXT_GS_PASS);
			gr_flip();
		}
		usleep(200*1000);
	}

	gsensor_close(fd);
	return NULL;
}

int test_gsensor_start(void)
{
	pthread_t thread;

	ui_fill_locked();
	ui_show_title(MENU_TEST_GSENSOR);
 
	thread_run = 1;
	pthread_create(&thread, NULL, (void*)gsensor_thread, NULL);
//	ui_handle_button(LEFT_BTN_NAME, NULL, RIGHT_BTN_NAME);
	//thread_run = 0;
	pthread_join(thread, NULL); /* wait "handle key" thread exit. */
	return 2;
}

