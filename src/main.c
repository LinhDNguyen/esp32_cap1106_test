#include "mgos.h"
#include "mgos_gpio.h"
#include "mgos_i2c.h"

#include "cap1106.h"

/* CAP1106 PIN definition */
#define CAP_CLK_PIN 32
#define CAP_SDA_PIN 26
#define CAP_ALERT_PIN 35
/* LEDs definition */
#define LED_E1_PIN 19
#define LED_E2_PIN 18
#define LED_E3_PIN 5
#define LED_E4_PIN 17
/*  */
#define CAP_ELECTRODE_NO 4

static IRAM void s_touch_alert_hdl(int pin, void *arg);
static void s_cap1106_init(void);
static void s_led_init(void);

static struct mgos_i2c *i2c_ptr = NULL;

enum mgos_app_init_result mgos_app_init(void) {
	LOG(LL_INFO, ("START APP, heap %u", mgos_get_free_heap_size()));
	s_led_init();

	// s_cap1106_init();

	return MGOS_APP_INIT_SUCCESS;
}

static void s_touch_alert_hdl(int pin, void *arg)
{

}

static void s_cap1106_init(void)
{
	struct mgos_config_i2c i2c_cfg = {
		.enable = 1,
		.freq = 200000,
		.debug = 1,
		.sda_gpio = CAP_SDA_PIN,
		.scl_gpio = CAP_CLK_PIN,
		.unit_no = 0,
	};

	int reg;

	// mgos_sys_config_get_i2c();
	i2c_ptr = mgos_i2c_get_global();
	if (!i2c_ptr) {
		LOG(LL_DEBUG, ("i2c global does not exist"));
		i2c_ptr = mgos_i2c_create(&i2c_cfg);
		if (!i2c_ptr) {
			LOG(LL_ERROR, ("I2C master create failed"));
			return;
		}
	}
	LOG(LL_DEBUG, ("I2C master created %p", i2c_ptr));

	// Read some info
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

	reg = mgos_i2c_read_reg_b(i2c_ptr, CAP1106_SLAVE_ADDR, CAP1106_REG_REVISION);
	if (reg < 0) {
		LOG(LL_ERROR, ("I2C read revision failed, err %d", reg));
	} else {
		LOG(LL_INFO, ("Revision: 0x%x", reg));
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
	ret &= mgos_gpio_set_mode(LED_E4_PIN, MGOS_GPIO_MODE_OUTPUT);

	if (!ret) {
		LOG(LL_ERROR, ("GPIO set mode failed"));
	}

	mgos_gpio_write(LED_E1_PIN, false);
	mgos_gpio_write(LED_E2_PIN, false);
	mgos_gpio_write(LED_E3_PIN, false);
	mgos_gpio_write(LED_E4_PIN, false);
}
