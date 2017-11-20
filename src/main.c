#include "mgos.h"
#include "mgos_gpio.h"
#include "mgos_i2c.h"

#include "cap1106.h"

/* CAP1106 PIN definition */
#define CAP_CLK_PIN 18
#define CAP_SDA_PIN 5
#define CAP_ALERT_PIN 17
/* LEDs definition */
#define LED_E1_PIN 25
#define LED_E2_PIN 33
#define LED_E3_PIN 32
/*  */
#define CAP_ELECTRODE_NO 3

static IRAM void s_touch_alert_hdl(int pin, void *arg);
static void s_cap1106_init(void);
static void s_led_init(void);
static void s_led_set(int val);
static int s_cap1106_read(int *latched, int *current);
static bool s_cap1106_reset_state(void);
static void s_touch_read_tmr(void *args);

static struct mgos_i2c *i2c_ptr = NULL;
static mgos_timer_id s_touch_tmr;
static int s_prev_touch = -1;

enum mgos_app_init_result mgos_app_init(void) {
	LOG(LL_INFO, ("START APP, heap %u", mgos_get_free_heap_size()));
	s_led_init();

	s_cap1106_init();

	// s_touch_tmr = mgos_set_timer(1000, MGOS_TIMER_REPEAT, s_touch_read_tmr, NULL);

	return MGOS_APP_INIT_SUCCESS;
}

static void s_touch_alert_hdl(int pin, void *arg)
{
	s_touch_tmr = mgos_set_timer(10, 0, s_touch_read_tmr, NULL);
}

static void s_touch_read_tmr(void *args)
{
	int latched = 0, current = 0;

	int reg = s_cap1106_read(&latched, &current);
	LOG(LL_INFO,
		("CAP read %x - %x", latched, reg));
	if (s_prev_touch != reg) {
		s_led_set(reg);
		s_prev_touch = reg;
	}
}

static void s_cap1106_init(void)
{
	int reg;
	bool res;

	// mgos_sys_config_get_i2c();
	// i2c_ptr = mgos_i2c_get_global();
	if (!i2c_ptr) {
		LOG(LL_DEBUG, ("i2c global does not exist"));
		struct mgos_config_i2c i2c_cfg;
		memcpy(&i2c_cfg, mgos_sys_config_get_i2c(), sizeof(struct mgos_config_i2c));
		i2c_cfg.enable = 1;
		LOG(LL_DEBUG,
			("i2c en %d, freq %d, debug %d, sda %d, scl %d, no %d",
				i2c_cfg.enable, i2c_cfg.freq, i2c_cfg.debug,
				i2c_cfg.sda_gpio, i2c_cfg.scl_gpio, i2c_cfg.unit_no));

		i2c_ptr = mgos_i2c_create(&i2c_cfg);
		if (!i2c_ptr) {
			LOG(LL_ERROR, ("I2C master create failed"));
			return;
		}
	}
	LOG(LL_DEBUG, ("I2C master created %p", i2c_ptr));

	// Read some info
	reg = mgos_i2c_read_reg_b(i2c_ptr, CAP1106_SLAVE_ADDR, CAP1106_REG_REVISION);
	if (reg < 0) {
		LOG(LL_ERROR, ("I2C read revision failed, err %d", reg));
	} else {
		LOG(LL_INFO, ("Revision: 0x%x", reg));
	}
	reg = mgos_i2c_read_reg_b(i2c_ptr, CAP1106_SLAVE_ADDR, CAP1106_REG_REVISION);
	if (reg < 0) {
		LOG(LL_ERROR, ("I2C read revision failed, err %d", reg));
	} else {
		LOG(LL_INFO, ("Revision: 0x%x", reg));
	}

	reg = mgos_i2c_read_reg_b(i2c_ptr, CAP1106_SLAVE_ADDR, CAP1106_REG_PRODUCT_ID);
	if (reg < 0) {
		LOG(LL_ERROR, ("I2C read product id failed, err %d", reg));
	} else {
		LOG(LL_INFO, ("Product ID: 0x%x", reg));
	}

	reg = mgos_i2c_read_reg_b(i2c_ptr, CAP1106_SLAVE_ADDR, CAP1106_REG_MANUFACTURER_ID);
	if (reg < 0) {
		LOG(LL_ERROR, ("I2C read manufacturer id failed, err %d", reg));
	} else {
		LOG(LL_INFO, ("Manufacturer ID: 0x%x", reg));
	}

	reg = mgos_i2c_read_reg_b(i2c_ptr, CAP1106_SLAVE_ADDR, CAP1106_REG_SENSOR_ENABLE);
	if (reg < 0) {
		LOG(LL_ERROR,
			("SENSOR ENABLE failed %d", reg));
		return;
	}
	LOG(LL_DEBUG,
		("SENSOR_ENABLE = 0x%x", reg));
	reg &= 0x07;
	// enable CS1-4
	res = mgos_i2c_write_reg_b(i2c_ptr, CAP1106_SLAVE_ADDR, CAP1106_REG_SENSOR_ENABLE, reg);
	if (!res) {
		LOG(LL_ERROR,
			("write SENSOR ENABLE to 0x%x", reg));
		return;
	}
	LOG(LL_DEBUG,
		("write SENSOR_ENABLE = 0x%x", reg));

	reg = mgos_i2c_read_reg_b(i2c_ptr, CAP1106_SLAVE_ADDR, CAP1106_REG_STANDBY_CHANNEL);
	if (reg < 0) {
		LOG(LL_ERROR,
			("STANDBY_CHANNEL failed %d", reg));
		return;
	}
	LOG(LL_DEBUG,
		("STANDBY_CHANNEL = 0x%x", reg));
	reg &= 0x07;
	// enable CS1-4 in standby mode
	res = mgos_i2c_write_reg_b(i2c_ptr, CAP1106_SLAVE_ADDR, CAP1106_REG_STANDBY_CHANNEL, reg);
	if (!res) {
		LOG(LL_ERROR,
			("write STANDBY_CHANNEL to 0x%x", reg));
		return;
	}
	LOG(LL_DEBUG,
		("write STANDBY_CHANNEL = 0x%x", reg));

	// set sensitivity in standby mode
	reg = 0x00; // 0x01: 64x, 0x00: 128x, default 0x02: 32x
	res = mgos_i2c_write_reg_b(i2c_ptr, CAP1106_SLAVE_ADDR, CAP1106_REG_STANDBY_SENSITIVITY, reg);
	if (!res) {
		LOG(LL_ERROR,
			("write STANDBY_SENSITIVITY to 0x%x", reg));
		return;
	}
	LOG(LL_DEBUG,
		("STANDBY_SENSITIVITY = 0x%x", reg));

	// set thresh hold in standby mode, default 0x40: 64,
	reg = 0x08; // thresh hold 32
	res = mgos_i2c_write_reg_b(i2c_ptr, CAP1106_SLAVE_ADDR, CAP1106_REG_STANDBY_THRESH, reg);
	if (!res) {
		LOG(LL_ERROR,
			("write STANDBY_THRESH to 0x%x", reg));
		return;
	}
	LOG(LL_DEBUG,
		("STANDBY_THRESH = 0x%x", reg));

	reg = mgos_i2c_read_reg_b(i2c_ptr, CAP1106_SLAVE_ADDR, CAP1106_REG_CONFIG);
	if (reg < 0) {
		LOG(LL_ERROR,
			("CONFIG failed %d", reg));
		return;
	}
	LOG(LL_DEBUG,
		("CONFIG = 0x%x", reg));
	reg |= 1 << 5;
	// enable CS1-4 in standby mode
	res = mgos_i2c_write_reg_b(i2c_ptr, CAP1106_SLAVE_ADDR, CAP1106_REG_CONFIG, reg);
	if (!res) {
		LOG(LL_ERROR,
			("write CONFIG to 0x%x", reg));
		return;
	}
	LOG(LL_DEBUG,
		("write CONFIG = 0x%x", reg));

	// set repeat 525ms(1110), duration 7800ms(1100)
	reg = 0xCE;
	res = mgos_i2c_write_reg_b(i2c_ptr, CAP1106_SLAVE_ADDR, CAP1106_REG_SENSOR_CONFIG, reg);
	if (!res) {
		LOG(LL_ERROR,
			("write SENSOR_CONFIG to 0x%x", reg));
		return;
	}
	LOG(LL_DEBUG,
		("SENSOR_CONFIG = 0x%x", reg));

	// set m press = 560ms
	reg = 0x0F;
	res = mgos_i2c_write_reg_b(i2c_ptr, CAP1106_SLAVE_ADDR, CAP1106_REG_SENSOR_CONFIG2, reg);
	if (!res) {
		LOG(LL_ERROR,
			("write SENSOR_CONFIG2 to 0x%x", reg));
		return;
	}
	LOG(LL_DEBUG,
		("SENSOR_CONFIG2 = 0x%x", reg));

	// Init ALERT GPIO pin
	res = mgos_gpio_set_mode(CAP_ALERT_PIN, MGOS_GPIO_MODE_INPUT);
	res &= mgos_gpio_set_int_handler(CAP_ALERT_PIN, MGOS_GPIO_INT_EDGE_NEG, s_touch_alert_hdl, NULL);
	res &= mgos_gpio_enable_int(CAP_ALERT_PIN);
	if (!res) {
		LOG(LL_ERROR,
			("Setup ALERT PIN %d error", CAP_ALERT_PIN));
		return;
	}

	LOG(LL_INFO, ("CAP1106 initialize DONE"));
}

static void s_led_init(void)
{
	int ret;
	LOG(LL_DEBUG, ("LED init"));

	ret = mgos_gpio_set_mode(LED_E1_PIN, MGOS_GPIO_MODE_OUTPUT);
	ret &= mgos_gpio_set_mode(LED_E2_PIN, MGOS_GPIO_MODE_OUTPUT);
	ret &= mgos_gpio_set_mode(LED_E3_PIN, MGOS_GPIO_MODE_OUTPUT);

	if (!ret) {
		LOG(LL_ERROR, ("GPIO set mode failed"));
	}

	s_led_set(0x0F);
}

static int s_cap1106_read(int *latched, int *current)
{
	int reg;
	bool res;

	reg = mgos_i2c_read_reg_b(i2c_ptr, CAP1106_SLAVE_ADDR, CAP1106_REG_SENSOR_INPUT);
	if (reg < 0) {
		LOG(LL_ERROR,
			("read SENSOR_INPUT failed %d", reg));
		return -1;
	}
	if (latched) {
		*latched = reg;
	}

	res = s_cap1106_reset_state();
	if (!res) {
		LOG(LL_ERROR,
			("cap1106 reset state failed"));
		return -2;
	}

	reg = mgos_i2c_read_reg_b(i2c_ptr, CAP1106_SLAVE_ADDR, CAP1106_REG_SENSOR_INPUT);
	if (reg < 0) {
		LOG(LL_ERROR,
			("read SENSOR_INPUT failed %d", reg));
		return -3;
	}
	if (current) {
		*current = reg;
	}

	return reg;
}


static bool s_cap1106_reset_state(void)
{
	int reg;
	bool res = false;

	reg = mgos_i2c_read_reg_b(i2c_ptr, CAP1106_SLAVE_ADDR, CAP1106_REG_MAIN_CONTROL);
	if (reg < 0) {
		LOG(LL_ERROR,
			("read MAIN_CONTROL failed %d", reg));
		return false;
	}
	reg &= 0xFE;  // Clear the INT bit (bit 0)
	res = mgos_i2c_write_reg_b(i2c_ptr, CAP1106_SLAVE_ADDR, CAP1106_REG_MAIN_CONTROL, reg);
	if (!res) {
		LOG(LL_ERROR,
			("write MAIN_CONTROL to 0x%x", reg));
		return false;
	}
	return true;
}

static void s_led_set(int val)
{
	int v = ~val;
	mgos_gpio_write(LED_E1_PIN, v & 0x01);
	mgos_gpio_write(LED_E2_PIN, v & 0x02);
	mgos_gpio_write(LED_E3_PIN, v & 0x04);
}
