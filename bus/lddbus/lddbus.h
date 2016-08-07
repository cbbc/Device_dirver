/*
 * Definitions for the virtual LDD bus.
 *
 * $Id: lddbus.h,v 1.4 2004/08/20 18:49:44 corbet Exp $
 */

//extern struct device ldd_bus;
extern struct bus_type ldd_bus_type;


/*
 * The LDD driver type.
 */

/*
 * A device type for things "plugged" into the LDD bus.
 */

struct ldd_device {
	char *name;
	//struct ldd_driver *driver;
	struct device dev;
};

#define to_ldd_device(x) container_of((x), struct ldd_device, dev)

struct ldd_driver {
	char *version;
	struct module *module;
	int (*probe)(struct ldd_device *);
	int (*remove)(struct ldd_device *);
	void (*shutdown)(struct ldd_device *);
	int (*suspend)(struct ldd_device *, pm_message_t state);
	int (*resume)(struct ldd_device *);	
	struct device_driver driver;
	struct driver_attribute version_attr;
};

#define to_ldd_driver(drv) container_of(drv, struct ldd_driver, driver)

static int lddbus_kill(struct ldd_device *dev);

int register_ldd_device(struct ldd_device *);
void unregister_ldd_device(struct ldd_device *);
int register_ldd_driver(struct ldd_driver *);
void unregister_ldd_driver(struct ldd_driver *);

